"""Generate the ReaPack index with immutable source commits and release URLs.

Commit the installer sources first, then pass that commit's full SHA. Existing
package versions are retained, allowing ReaPack users to roll back.
"""
from pathlib import Path
import argparse
import datetime
import re
import urllib.parse
import xml.etree.ElementTree as ET

ROOT = Path(__file__).resolve().parents[1]
REPO = 'andreynovikov86-cmd/SideCurve'


def generate(commit, output):
    if not re.fullmatch('[a-f0-9]{40}', commit):
        raise ValueError('Pass a full source commit SHA')
    module = (ROOT / 'distribution/SideCurve/installer.lua').read_text()
    version = re.search(r"M.version = '([^']+)'", module)[1]
    package_name = 'SideCurve - Install or Update VST3.lua'
    if output.exists():
        index = ET.parse(output).getroot()
        assert index.tag == 'index' and index.get('name') == 'SideCurve by Andrey Novikov'
        category = index.find('category')
        package = category.find('reapack')
        assert package.get('name') == package_name
        if package.find(f"version[@name='{version}']") is not None:
            raise ValueError('This installer version is already indexed; bump M.version and action headers')
    else:
        index = ET.Element('index', version='1', name='SideCurve by Andrey Novikov')
        category = ET.SubElement(index, 'category', name='SideCurve')
        package = ET.SubElement(category, 'reapack', name=package_name, type='script',
                                desc='SideCurve - VST3 and JSFX installers')
        metadata = ET.SubElement(package, 'metadata')
        ET.SubElement(metadata, 'description').text = r'{\rtf1\ansi SideCurve installers for macOS and Windows. After installing or updating this package, run the VST3 or JSFX installer action in REAPER. Synchronization alone does not replace the plugin.}'
    release = ET.SubElement(package, 'version', name=version, author='Andrey Novikov',
                            time=datetime.datetime.now(datetime.timezone.utc).strftime('%Y-%m-%dT%H:%M:%SZ'))
    base = f'https://raw.githubusercontent.com/{REPO}/{commit}/'
    for name, main in [(package_name, True), ('SideCurve - Install or Update JSFX.lua', True), ('installer.lua', False)]:
        contents = (ROOT / 'distribution/SideCurve' / name).read_text()
        if main: assert f'-- @version {version}' in contents
        attributes = {'file': name}
        if main: attributes['main'] = 'main'
        ET.SubElement(release, 'source', **attributes).text = base + urllib.parse.quote('distribution/SideCurve/' + name)
    ET.SubElement(release, 'source', file='files/SideCurve.jsfx').text = base + 'JSFX/Effects/SideKick/SideKick.jsfx'
    assets = re.findall(r"(mac|win) = \{ file = '([^']+)', version = '([^']+)'", module)
    assert len(assets) == 2
    for platform, filename, native_version in assets:
        ET.SubElement(release, 'source', file='files/' + filename,
                      platform='darwin' if platform == 'mac' else 'win64').text = (
            f'https://github.com/{REPO}/releases/download/v{native_version}/{filename}')
    index.set('commit', commit)
    ET.indent(index, space='  ')
    output.write_bytes(ET.tostring(index, encoding='utf-8', xml_declaration=True) + b'\n')


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--commit', required=True)
    parser.add_argument('--output', type=Path, default=ROOT / 'index.xml')
    args = parser.parse_args()
    generate(args.commit, args.output)
