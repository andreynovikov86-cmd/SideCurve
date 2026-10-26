"""Package the complete plug-in bundle and notices, preserving file modes."""
from pathlib import Path
import sys, zipfile
bundle=Path(sys.argv[1]);archive=Path(sys.argv[2]);root=Path(__file__).resolve().parents[1]
assert bundle.is_dir(),bundle
archive.parent.mkdir(parents=True,exist_ok=True)
with zipfile.ZipFile(archive,'w',zipfile.ZIP_DEFLATED) as z:
 for path in sorted(bundle.rglob('*')):
  if path.is_file():z.write(path,Path(bundle.name)/path.relative_to(bundle))
 for name in ('README.md','LICENSE','THIRD_PARTY_NOTICES.md','docs/INSTALL.md'):
  z.write(root/name,name)
 for path in sorted((root/'licenses').glob('*')):z.write(path,Path('licenses')/path.name)
with zipfile.ZipFile(archive) as z:assert z.testzip() is None
print(archive)
