"""Compare the C++ audio engine against the versioned JSFX/EEL2 reference."""
from pathlib import Path
import re, subprocess, sys, tempfile
source=(Path(__file__).resolve().parents[1]/'reference/SideCurve.jsfx').read_text()
sections=dict(re.findall(r'^@(\w+)[^\n]*\n(.*?)(?=^@|\Z)',source,re.M|re.S))
defaults={int(k):float(v) for k,v in re.findall(r'^slider(\d+):([-\d.]+)<',source,re.M)}
ids=[1,2,4,5,6,7,8,9,10,11,12,13,14,16,19,20,21,22]
cases=[({},0),({1:1},0),({1:1},1),({1:1,10:1},1),({1:1},2),({1:1},3),
       ({1:1,9:1},1),({1:1,6:1000,7:16000},3),({1:1,4:42,22:.31},1),
       ({1:1,11:0,21:.8},1),({1:1,11:.07,21:.86,10:1},1),
       ({13:.05,19:.9,16:.75},0),({4:0},0)]
cases += [({2:n},0) for n in range(8)]
for rate in (44100,48000,96000):
 for changes,signal in cases:
  values=defaults|changes;count=int(rate*.9)
  kick=['0','((testtime>=.1 && testtime<.15)||(testtime>=.35 && testtime<.4)) ? .8*sin(2*$pi*65*testtime) : 0',
        '.8*sin(2*$pi*65*testtime)','.8*sin(2*$pi*8000*testtime)'][signal]
  code='testsliders=10000;function slider(n)(testsliders[n]);\n'
  code+=f'srate={rate};tempo=120;play_state=1;ts_denom=4;\n'
  code+='\n'.join(f'slider{k}={v};testsliders[{k}]={v};' for k,v in values.items())
  code+=sections['init']+sections['slider']+'ui_visible=1;testn=0;\n'
  code+=f'loop({count},testtime=testn/srate;testn%128==0?(beat_position=testtime*2;play_position=testtime;'+sections['block']+');'
  code+=f'spl0=.25;spl1=.125;spl2={kick};spl3=-spl2;'+sections['sample']
  code+='testn%128==0?printf("%d,%.12f,%.12f,%d,%.12f\\n",testn,spl0,phase,hits,spl1);testn+=1;);'
  with tempfile.TemporaryDirectory() as d:
   path=Path(d)/'reference.eel';path.write_text(code)
   ref=subprocess.run([sys.argv[2],str(path)],text=True,capture_output=True,check=True).stdout
  args=[sys.argv[1],str(rate),'.9',str(signal)]
  for i,js in enumerate(ids):args += [str(i),str(values[js])]
  actual=subprocess.run(args,text=True,capture_output=True,check=True).stdout
  def rows(text):return [[float(v) for v in line.split(',')] for line in text.splitlines() if re.match(r'^\d+,',line)]
  a,b=rows(ref),rows(actual)
  assert a and len(a)==len(b),(rate,changes,signal,len(a),len(b))
  delta=max(abs(x-y) for r,s in zip(a,b) for x,y in zip(r,s))
  assert delta<1e-7,(rate,changes,signal,delta)
print(f'PASS {len(cases)*3} C++ / JSFX conformance cases (44.1, 48 and 96 kHz)')
