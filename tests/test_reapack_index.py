"""Check published package routing and immutable, append-only index generation."""
from pathlib import Path, PurePosixPath
import importlib.util
import re
import shutil
import tempfile
import urllib.parse
import xml.etree.ElementTree as ET

ROOT = Path(__file__).resolve().parents[1]
spec = importlib.util.spec_from_file_location('reapack_index', ROOT / 'tools/reapack_index.py')
generator = importlib.util.module_from_spec(spec)
spec.loader.exec_module(generator)


def validate(path, source_root):
    index = ET.parse(path).getroot()
    assert index.tag == 'index' and index.get('version') == '1'
    assert index.get('name') == 'SideCurve by Andrey Novikov'
    package = index.find('category/reapack')
    assert package.get('type') == 'script'
    versions = package.findall('version')
    assert versions and len({v.get('name') for v in versions}) == len(versions)
    release = versions[-1]
    sources = release.findall('source')
    assert len(sources) == 6
    actions = {s.get('file') for s in sources if s.get('main') == 'main'}
    assert actions == {'SideCurve - Install or Update VST3.lua', 'SideCurve - Install or Update JSFX.lua'}
    assert len({s.get('file') for s in sources}) == len(sources)
    for source in sources:
        name = PurePosixPath(source.get('file'))
        assert not name.is_absolute() and '..' not in name.parts
        url = urllib.parse.urlsplit(source.text)
        assert url.scheme == 'https'
        if source.get('platform'):
            assert source.get('platform') in ('darwin', 'win64')
            assert url.netloc == 'github.com' and '/releases/download/v' in url.path
            assert source.text.endswith('/' + name.name)
        else:
            assert url.netloc == 'raw.githubusercontent.com'
            match = re.fullmatch(r'/andreynovikov86-cmd/SideCurve/([a-f0-9]{40})/(.+)', url.path)
            assert match and match[1] == index.get('commit')
            local = source_root / urllib.parse.unquote(match[2])
            assert local.is_file(), local
            if source.get('main'):
                assert '-- @version ' + release.get('name') in local.read_text()
    for platform in ('darwin', 'win64'):
        selected = [s for s in sources if s.get('platform') in (None, platform)]
        assert len(selected) == 5 and len([s for s in selected if s.get('platform')]) == 1
    return index


def main():
    with tempfile.TemporaryDirectory() as temp:
        fixture = Path(temp)
        shutil.copytree(ROOT / 'distribution', fixture / 'distribution')
        shutil.copytree(ROOT / 'JSFX/Effects', fixture / 'JSFX/Effects')
        generator.ROOT = fixture
        output = fixture / 'index.xml'
        try:
            generator.generate('main', output)
            raise AssertionError('Mutable ref accepted')
        except ValueError:
            pass
        generator.generate('a' * 40, output)
        first = validate(output, fixture)
        original = first.find('category/reapack/version')
        original.tail = None
        first_version = ET.tostring(original)
        try:
            generator.generate('b' * 40, output)
            raise AssertionError('Reused package version accepted')
        except ValueError:
            pass
        for script in (fixture / 'distribution/SideCurve').glob('*.lua'):
            script.write_text(script.read_text().replace('1.0.0', '1.0.1'))
        generator.generate('b' * 40, output)
        second = validate(output, fixture)
        assert len(second.findall('category/reapack/version')) == 2
        retained = second.find('category/reapack/version')
        retained.tail = None
        assert ET.tostring(retained) == first_version
    if (ROOT / 'index.xml').exists():
        validate(ROOT / 'index.xml', ROOT)
    print('PASS: ReaPack action routing, platform selection, immutable sources and version history')


if __name__ == '__main__':
    main()
