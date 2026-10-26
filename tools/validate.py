from pathlib import Path
import subprocess,sys
root=Path(sys.argv[1]);exe='.exe' if sys.platform=='win32' else ''
validator=root/'bin'/'Release'/('validator'+exe)
bundle=root/'VST3'/'Release'/'SideCurve.vst3'
subprocess.run([str(validator.resolve()),str(bundle.resolve())],check=True)
