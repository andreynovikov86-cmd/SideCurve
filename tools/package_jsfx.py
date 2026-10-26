"""Package the standalone JSFX source and one-time installer."""
from pathlib import Path
import sys, zipfile
root=Path(sys.argv[1]); archive=Path(sys.argv[2]); archive.parent.mkdir(parents=True,exist_ok=True)
with zipfile.ZipFile(archive,'w',zipfile.ZIP_DEFLATED) as z:
 for p in sorted(root.rglob('*')):
  if p.is_file() and not any(x in {'.git','__pycache__','.DS_Store'} for x in p.relative_to(root).parts):
   z.write(p,Path('SideCurve-JSFX')/p.relative_to(root))
with zipfile.ZipFile(archive) as z: assert z.testzip() is None
print(archive)
