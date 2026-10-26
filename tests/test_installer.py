"""Exercise the real Lua installer against temporary files, never a user VST3 folder."""
from pathlib import Path
import argparse
import hashlib
import os
import shutil
import subprocess
import sys
import tempfile
import urllib.request
import zipfile

ROOT = Path(__file__).resolve().parents[1]
ASSETS = {
    'mac': ('SideCurve-0.5.5-macOS-universal.zip', '68fdb40efa12d1654959b24f68d117fa091d484640935385b5456cb6daaab1a6', 'Contents/MacOS/SideCurve'),
    'win': ('SideCurve-0.5.5-Windows-x64.zip', '803d6644cbfefba9c39397d1d59c147d14db334f30490b204c1e6922120fa3be', 'Contents/x86_64-win/SideCurve.vst3'),
}


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--lua', required=True)
    parser.add_argument('--archives', type=Path)
    args = parser.parse_args()
    platform = 'win' if sys.platform == 'win32' else 'mac'
    if sys.platform not in ('darwin', 'win32'):
        parser.error('Run filesystem integration tests on macOS or Windows')
    name, checksum, binary = ASSETS[platform]
    with tempfile.TemporaryDirectory(prefix='SideCurve installer tests ') as temp:
        base = Path(temp)
        archive = base / name
        if args.archives:
            shutil.copy2(args.archives / name, archive)
        else:
            urllib.request.urlretrieve('https://github.com/andreynovikov86-cmd/SideCurve/releases/download/v0.5.5/' + name, archive)
        assert hashlib.sha256(archive.read_bytes()).hexdigest() == checksum
        with zipfile.ZipFile(archive) as package:
            expected_binary = package.read('SideCurve.vst3/' + binary)
        helper = base / 'host_mock.py'
        helper.write_text('''from pathlib import Path
import subprocess,sys
action,path=sys.argv[1:];p=Path(path)
if action=='mkdir': p.mkdir(parents=True,exist_ok=True)
elif action=='execute':
 command=p.read_text(encoding='utf-8')
 if sys.platform=='win32': args=command
 else:
  # WDL LineParser: a quote at token start selects a literal delimiter;
  # backslash escapes and adjacent quoted-token concatenation are not special.
  args=[];i=0
  while i<len(command):
   if command[i] in ' \\t': i+=1;continue
   if command[i] in chr(34)+chr(39)+chr(96):
    delimiter=command[i];i+=1;end=command.index(delimiter,i)
   else:
    end=i
    while end<len(command) and command[end] not in ' \\t': end+=1
   args.append(command[i:end]);i=end+1
 result=subprocess.run(args,capture_output=True)
 sys.stdout.buffer.write(str(result.returncode).encode()+b'\\n'+result.stdout+result.stderr)
elif p.is_dir():
 for entry in sorted(p.iterdir()):
  if (action=='files' and entry.is_file()) or (action=='directories' and entry.is_dir()): print(entry.name)
''')
        scenarios = ['fresh', 'update', 'missing', 'corrupt', 'malformed', 'extraction-failure',
                     'publish-failure', 'rollback-failure', 'in-use', 'legacy', 'unsupported',
                     'jsfx-fresh', 'jsfx-update', 'jsfx-incomplete']
        for scenario in scenarios:
            case = base / scenario
            # The stock Windows Lua runtime follows the system code page;
            # REAPER has its own UTF-8 filesystem wrappers. Exercise spaces on
            # both platforms and Unicode/apostrophe quoting on macOS.
            user = case / ('User with spaces' if platform == 'win' else 'Музыка with spaces \' " $ & quotes')
            resource = user / 'REAPER resource'
            payload = case / 'ReaPack files'
            (payload / 'files').mkdir(parents=True)
            resource.mkdir(parents=True)
            parent = user / ('Programs/Common/VST3' if platform == 'win' else 'Library/Audio/Plug-Ins/VST3')
            target = parent / 'SideCurve.vst3'
            old = target / 'old-marker'
            if scenario not in ('fresh', 'jsfx-fresh'):
                target.mkdir(parents=True)
                old.write_bytes(b'previous version must survive failures')
            if scenario == 'legacy':
                (parent / 'SideKick.vst3').mkdir()
            source_archive = payload / 'files' / name
            digest = ''
            if scenario != 'missing':
                shutil.copy2(archive, source_archive)
            if scenario == 'corrupt':
                source_archive.write_bytes(b'not the published release')
            if scenario == 'malformed':
                with zipfile.ZipFile(source_archive, 'w') as z:
                    z.writestr('unrelated.txt', 'wrong package')
                digest = hashlib.sha256(source_archive.read_bytes()).hexdigest()
            jsfx = payload / 'files/SideCurve.jsfx'
            jsfx.write_bytes((ROOT / 'JSFX/Effects/SideKick/SideKick.jsfx').read_bytes())
            effect = resource / 'Effects/SideKick/SideKick.jsfx'
            if scenario == 'jsfx-update':
                effect.parent.mkdir(parents=True)
                effect.write_text('previous effect')
                (effect.parent / 'unrelated.txt').write_text('leave me alone')
            if scenario == 'jsfx-incomplete':
                jsfx.write_text('not a SideCurve package')
            subprocess.run([args.lua, str(ROOT / 'tests/installer_mock.lua'), str(ROOT), str(payload),
                            str(user), str(resource), 'Win64' if platform == 'win' else 'OSX64',
                            scenario, str(helper), sys.executable, digest], check=True)
            if scenario in ('fresh', 'update'):
                assert (target / binary).read_bytes() == expected_binary
                assert not old.exists()
                if platform == 'mac':
                    assert os.access(target / binary, os.X_OK)
                    subprocess.run(['codesign', '--verify', '--deep', '--strict', str(target)], check=True)
            elif scenario == 'rollback-failure':
                backups = list(parent.glob('.SideCurve-install-*/previous/old-marker'))
                assert len(backups) == 1 and backups[0].read_bytes() == b'previous version must survive failures'
            elif not scenario.startswith('jsfx'):
                assert old.read_bytes() == b'previous version must survive failures'
            if scenario != 'rollback-failure':
                assert not list(parent.glob('.SideCurve-install-*')), scenario
            if scenario in ('jsfx-fresh', 'jsfx-update'):
                source = effect.read_text()
                escaped = str(resource).replace('\\', '/').replace('"', '\\"')
                assert f'#resource_root = "{escaped}";' in source
                assert 'desc:SideCurve' in source
                assert (ROOT / 'JSFX/Effects/SideKick/SideKick.jsfx').read_text().count('#resource_root = "";') == 1
                if scenario == 'jsfx-update': assert (effect.parent / 'unrelated.txt').read_text() == 'leave me alone'
            elif scenario == 'jsfx-incomplete':
                assert not effect.exists()
        print(f'PASS: {len(scenarios)} installer filesystem scenarios on {platform}')


if __name__ == '__main__':
    main()
