"""Execute the actual DSP sections with Cockos WDL's EEL2 VM (not a Python DSP copy).

Usage: python3 tests/test_engine.py /path/to/WDL/WDL/eel2/loose_eel
Build the official WDL runner: make -C WDL/WDL/eel2 NO_GFX=1 loose_eel
The harness supplies JSFX host variables, defaults and slider(index) lookup.
"""
from pathlib import Path
import re
import math
import subprocess
import sys
import tempfile

ENGINE = Path(__file__).resolve().parents[1] / "Effects/SideKick/SideKick.jsfx"
runner = sys.argv[1]
source = ENGINE.read_text()
sections = dict(re.findall(r"^@(\w+)[^\n]*\n(.*?)(?=^@|\Z)", source, re.M | re.S))
defaults = {int(k): float(v) for k, v in re.findall(r"^slider(\d+):([-\d.]+)<", source, re.M)}


def run(params=None, kick="0", seconds=2, rate=48000, bpm=120, start=0, events="", stride=128, visible=1):
    params = dict(defaults, **{}) | (params or {})
    values = "\n".join(f"slider{k}={v}; testsliders[{k}]={v};" for k, v in params.items())
    code = f"""
testsliders=10000;
function slider(n)(testsliders[n]);
srate={rate}; tempo={bpm}; play_state=1; ts_num=4; ts_denom=4;
{values}
{sections['init']}
{sections['slider']}
ui_visible={visible};
testn=0;
loop({int(seconds * rate)},
  testtime=testn/srate;
  {events}
  testn%128==0 ? (
    beat_position={start}+testtime*tempo/60; play_position=testtime;
    {sections['block']}
  );
  spl0=0.25; spl1=0.125;
  spl2={kick}; spl3=-spl2;
  {sections['sample']}
  testn%{stride}==0 ? printf("%.0f,%.10f,%.10f,%.0f,%.10f,%.10f\\n",testn,spl0,phase,hits,detector,spl1);
  testn+=1;
);
printf("END,%.0f,%.10f\\n",hits,phase);
"""
    with tempfile.TemporaryDirectory() as tmp:
        path = Path(tmp) / "test.eel"
        path.write_text(code)
        result = subprocess.run([runner, str(path)], text=True, capture_output=True, timeout=30)
    assert result.returncode == 0, result.stderr + result.stdout
    rows = []
    end = None
    for line in result.stdout.splitlines():
        if line.startswith("END,"):
            end = [float(x) for x in line.split(",")[1:]]
        elif re.match(r"^\d+,", line):
            rows.append([float(x) for x in line.split(",")])
    assert end is not None and rows, result.stdout + result.stderr
    return rows, end


def at(rows, t, rate=48000):
    return min(rows, key=lambda r: abs(r[0] / rate - t))


checks = 0


def check(value, label):
    global checks
    assert value, label
    checks += 1
    print("PASS", label)


rows, end = run({1: 1})
check(end[0] == 0 and all(abs(r[1] - .25) < 1e-7 for r in rows), "sidechain silence: no hits, unity output")
rows, _ = run({4: 0})
check(all(abs(r[1] - .25) < 1e-7 for r in rows), "depth 0 is transparent")
rows, _ = run()
check(at(rows, .02)[1] < .005 and at(rows, .3)[1] > .249, "grid: early duck then unity")
check(all(abs(r[5] * 2 - r[1]) < 1e-8 for r in rows), "stereo gain linking")
rows, _ = run(start=.5)
check(at(rows, .1)[1] > .18, "grid starts at the host musical phase, not phase zero")
for index in range(8):
    rows, _ = run({2: index}, seconds=.12)
    row = at(rows, .1)
    expected = ((row[0] + 1) / 48000 * 2 / (8 / 2 ** index)) % 1
    actual = row[2]
    check(min(abs(actual - expected), 1-abs(actual - expected)) < 1e-7, f"duration {index}: host quarter-note scaling")
rows, _ = run({13: 0, 19: 1, 16: .5}, seconds=.5)
check(abs(at(rows, .2475)[1] - .125) < .003, "centered curvature handle passes through transition midpoint")
rows, _ = run({13: 0, 19: 1, 16: .8}, seconds=.5)
check(at(rows, .2475)[1] > .195, "upper curvature handle bows whole transition upward")
rows, _ = run({13: 0, 19: 1, 16: .2}, seconds=.5)
check(at(rows, .2475)[1] < .055, "lower curvature handle bows whole transition downward")
rows, _ = run({13: 0, 19: 1, 14: 1, 20: 0, 16: .2}, seconds=.5)
check(at(rows, .2475)[1] > .195, "falling curve uses the same geometric bend behavior")
rows, _ = run({4: 50})
check(min(r[1] for r in rows)>.1249, "half depth never attenuates more than 50 percent")
rows, _ = run(events="testn>=14400 ? play_state=0;")
check(at(rows,.45)[1]>.249, "grid stop returns to unity")
rows, _ = run(start=.8, seconds=.16, stride=1)
window=[r[1] for r in rows if .095<=r[0]/48000<=.11]
check(max(abs(a-b) for a,b in zip(window,window[1:]))<.003, "last-to-first grid seam has a bounded, smooth gain transition")
rows, _ = run({1:1}, kick="testtime>.1 && testtime<.14 ? .8*sin(2*$pi*65*testtime) : 0", seconds=.2, stride=1)
check(max(abs(a[1]-b[1]) for a,b in zip(rows,rows[1:]))<.003, "sidechain trigger starts from actual gain without an instantaneous jump")
kick = "(testtime>=.1 && testtime<.15 || testtime>=.35 && testtime<.4) ? .8*sin(2*$pi*65*testtime) : 0"
rows, end = run({1: 1}, kick=kick)
check(end[0] == 2, "two kicks retrigger an unfinished quarter-note shape (opposite-polarity L/R)")
check(at(rows, .41)[1] < .02, "second hit audibly restarts attenuation")
check(at(rows, 1.2)[1] > .249, "early one-shot holds its final unity level without looping")
rows, end = run({1: 1}, kick=".8*sin(2*$pi*65*testtime)")
check(end[0] == 1, "continuous low tone does not retrigger every waveform cycle")
rows, end = run({1: 1}, kick=".8*sin(2*$pi*8000*testtime)")
check(end[0] == 0, "detector low pass rejects high-frequency signal")
rows, _ = run({1: 1, 9: 1}, kick=kick)
check(max(abs(r[1] + r[5]) for r in rows[20:]) < .001, "audition outputs filtered stereo detector, including polarity")
for rate in (44100, 96000):
    rows, end = run({1: 1}, kick=kick, rate=rate)
    check(end[0] == 2, f"retrigger at {rate} Hz")
rows, _ = run({1:1,9:1}, kick=kick, visible=0)
check(all(abs(r[1] - 2*r[5])<1e-8 for r in rows), "closed UI cannot audition sidechain")
rows, _ = run({10:1}, seconds=.5)
check(at(rows,.02)[1]>.249 and at(rows,.46)[1]<.002, "INVERT moves attenuation from beginning to end")
rows, _ = run({1:1,10:1}, kick=kick)
check(at(rows,1.2)[1]<.00001, "inverted one-shot holds its final muted level until the next kick")
rows, _ = run({1:1,10:1,4:50}, kick=kick)
check(abs(at(rows,1.2)[1]-.125)<.00001, "half-mix one-shot holds the final blended level")
rows, _ = run({1:1,22:.37}, kick=kick)
check(abs(at(rows,1.2)[1]-.25*.37)<.00001, "custom final point is held indefinitely after one-shot")
separated_kicks = "(testtime>=.1 && testtime<.15 || testtime>=1.1 && testtime<1.15) ? .8*sin(2*$pi*65*testtime) : 0"
rows, _ = run({1:1,10:1}, kick=separated_kicks)
check(at(rows,.9)[1]<.00001 and at(rows,1.17)[1]>.24, "new kick reopens a one-shot after its held muted state")
for invert in (0,1):
    rows, _ = run({1:1,10:invert,11:.02,21:.98}, kick=kick, seconds=.2, stride=1)
    trigger_index = next(i for i,r in enumerate(rows) if r[3]>0)
    early = rows[trigger_index+48][1]
    settled = rows[trigger_index+1440][1]
    check(early<.05 and settled>.24 if invert else early>.2 and settled<.002,
          f"outer anchor controls gradual start fade from actual gain, inversion {invert}")

for first_time in (.02,0):
    rows, _ = run({1:1,11:first_time,21:.9}, kick=kick, seconds=.2, stride=1)
    trigger_index = next(i for i,r in enumerate(rows) if r[3]>0)
    midway = rows[trigger_index+960][1]
    settled = rows[trigger_index+3600][1]
    check(midway>.15 and settled<.004,
          f"sidechain replays the full tail-plus-head join, first anchor {first_time}")

# Inspect the real per-phase buffers, not a separate waveform implementation.
values = "\n".join(f"slider{k}={v};" for k,v in (defaults | {1:1}).items())
wave_code=f"""
srate=48000;tempo=120;play_state=1;
{values}
{sections['init']}
{sections['slider']}
testn=0;
loop(48000,
 testtime=testn/srate;
 testn%128==0 ? (beat_position=testtime*2;play_position=testtime;{sections['block']});
 spl0=.25*sin(2*$pi*220*testtime);spl1=spl0*.5;
 spl2=testtime>.1 && testtime<.15 ? .8*sin(2*$pi*65*testtime) : 0;spl3=-spl2;
 {sections['sample']}
 testn+=1;
);
i=0;occupied=0;scoccupied=0;
loop(256,wave[i]>.00001 ? occupied+=1;scwave[i]>.00001 ? scoccupied+=1;i+=1;);
printf("BINS %.0f %.0f\\n",occupied,scoccupied);
"""
with tempfile.TemporaryDirectory() as tmp:
    path=Path(tmp)/'wave.eel';path.write_text(wave_code)
    result=subprocess.run([runner,str(path)],text=True,capture_output=True,timeout=30)
assert result.returncode==0,result.stderr
bins=re.search(r'BINS (\d+) (\d+)',result.stdout)
check(bins is not None and int(bins[1])>100 and int(bins[2])>10,
      "sidechain audio and filtered trigger occupy many waveform bins")

def tone_rms(freq, hp, lp):
    rows, _ = run({1:1,9:1,6:hp,7:lp}, kick=f".8*sin(2*$pi*{freq}*testtime)", seconds=1.5, stride=1)
    settled = [r[1] for r in rows if r[0]/48000 >= 1.0]
    return math.sqrt(sum(x*x for x in settled)/len(settled))

hp_near = tone_rms(100,200,20000)
hp_far = tone_rms(50,200,20000)
check(46<20*math.log10(hp_near/hp_far)<50, "HP stopband rolls off at 48 dB per octave")
lp_near = tone_rms(400,10,200)
lp_far = tone_rms(800,10,200)
check(46<20*math.log10(lp_near/lp_far)<50, "LP stopband rolls off at 48 dB per octave")
print(f"PASS {checks} EEL2 audio checks")
