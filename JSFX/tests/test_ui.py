"""Run the actual native @gfx code in EEL2 with a small host/drawing adapter.

The adapter skips drawing, but preserves sliders, pointer events and time.
Use alongside native REAPER visual inspection, not as a rendering test.
"""
from pathlib import Path
import re
import json
import subprocess
import sys
import tempfile

source = (Path(__file__).resolve().parents[1] / 'Effects/SideKick/SideKick.jsfx').read_text()
sections = dict(re.findall(r'^@(\w+)[^\n]*\n(.*?)(?=^@|\Z)', source, re.M | re.S))
values = {int(k): float(v) for k, v in re.findall(r'^slider(\d+):([-\d.]+)<', source, re.M)}
# In REAPER slider(n) and sliderN address the same slot.
def bind(code):
    code = re.sub(r'slider\((\w+)\)=', r'testsliders[\1]=', code)
    code = re.sub(r'gfx_drawstr\(([^(),]+)\)', r'gfx_drawstr(\1,0,0,0)', code)
    code = re.sub(r'slider_automate\(([^(),]+)\)', r'slider_automate(\1,0)', code)
    return re.sub(r'\bslider(\d+)\b', lambda m: f'testsliders[{m[1]}]', code)

stubs = '''
testsliders=10000;factory_expected=11000;
function file_open(path)(
 !files_enabled ? -1 : match("*/reaper-defpresets.ini",path) ? (map_index=0;1;) : (preset_index=0;2;);
);
function file_avail(h)(h==1 ? map_count-map_index : preset_count-preset_index;);
function file_string(h str)(strcpy(str,h==1 ? map_lines[map_index] : preset_lines[preset_index]);h==1 ? map_index+=1 : preset_index+=1;);
function file_close(h)(0;);
function slider(n)(testsliders[n]);
function sliderchange(mask)(notifications+=1;);
function slider_automate(mask endtouch)(notifications+=1;);
function set_host_numchan(n)(hostchannels=n;);
function get_host_numchan()(hostchannels;);
function gfx_triangle(x y xx yy xxx yyy xxxx yyyy)(0;);
function gfx_set(r g b a)(0;);
function gfx_rect(x y w h)(0;);
function gfx_circle(x y r fill aa)(0;);
function gfx_line(x y xx yy aa)(0;);
function gfx_roundrect(x y w h r aa)(0;);
function gfx_setfont(i face size flags)(last_font_flags=flags;);
function gfx_drawstr(txt flags right bottom)(0;);
function gfx_arc(x y r a b aa)(0;);
function gfx_setimgdim(i w h)(0;);
function gfx_blit(i scale angle sx sy sw sh dx dy dw dh)(0;);
function gfx_getchar()(0;);
function gfx_showmenu(txt)(menuchoice;);
function time_precise()(testclock;);
function require(ok text)(!ok ? (printf("FAIL %s\\n",text);failures+=1;) : printf("PASS %s\\n",text););
'''
code = stubs + '\n'.join(f'testsliders[{k}]={v};factory_expected[{k}]={v};' for k,v in values.items())
code += '\nsrate=48000;tempo=120;ts_num=4;ts_denom=4;play_state=1;gfx_w=820;gfx_h=660;testclock=10;\n'
code += bind(sections['init']) + bind(sections['slider']) + bind(sections['block'])
helpers, body = sections['gfx'].split('// gfx_idle', 1)
code += bind(helpers) + '\nfunction frame()(\n' + bind('// gfx_idle' + body) + '\n);\n'

user_values = dict(values) | {1: 1, 2: 5, 4: 37, 5: -42, 6: 400, 7: 8000, 8: 67, 11: .04, 21: .91}
def preset_hex(params):
    text = ' '.join(str(params[i]) for i in range(1, 26)) + ' ' + '- '*39 + '"No preset"'
    raw = text.encode()
    return raw.hex().upper() + f'{sum(raw)&255:02X}'
map_fixture = ['[defaultpresets]', 'js-SideKick/SideKick.jsfx=No preset']
preset_fixture = ['[General]', 'NbPresets=2', '[Preset0]', 'Data='+preset_hex(values),
                  'Name=Another preset', '[Preset1]', 'Name=No preset', 'Data='+preset_hex(user_values)]
code += '\nmap_lines=15000;preset_lines=16000;user_expected=17000;\n'
code += f'map_count={len(map_fixture)};preset_count={len(preset_fixture)};\n'
for array, lines in [('map_lines',map_fixture),('preset_lines',preset_fixture)]:
    for i,line in enumerate(lines):
        code += f'{array}[{i}]={json.dumps(line+chr(13)+chr(10))};\n'
code += '\n'.join(f'user_expected[{k}]={v};' for k,v in user_values.items())
code += '''
frame();
require(hostchannels==4,"native window sets four track channels");
require(ui_visible==1,"native UI is visible");
mouse_x=644;mouse_y=28;mouse_cap=1;frame();
require(slider(10)==1,"INVERT button works");
require(abs(ux[1]-(factory_expected[11]+factory_expected[13]*(factory_expected[21]-factory_expected[11])))<.00001 && uy[0]==1 && uy[4]==0,"INVERT keeps time and reflects levels vertically");
mouse_cap=0;frame();testclock+=.5;
mouse_cap=1;frame();mouse_cap=0;frame();
require(slider(10)==0,"second INVERT returns original curve");
put(13,.23);put(14,.12);put(19,.63);put(20,.88);put(12,.04);put(22,.96);put(16,.27);read_points();
first=ui_curve(uwrap+.37*uspan);put(10,1);read_points();
require(abs(1-first-ui_curve(uwrap+.37*uspan))<.0000001,"custom curve inverts vertically at the same time");
put(10,0);read_points();
move_point(2,.5,.7);read_points();
require(abs(uy[2]-.7)<.000001,"curvature handle follows pointer vertically");
require(slider(13)==.23 && slider(19)==.63,"curvature handle does not move anchors");
put(10,1);read_points();move_point(1,.44,.81);read_points();
require(abs(ux[1]-.44)<.000001 && abs(uy[1]-.81)<.000001,"editing an inverted anchor updates correct source point");
move_point(1,.4217,.81);read_points();
require(abs(ux[1]-.4217)<.000001,"anchor moves freely without beat snapping");
put(3,0);put(10,0);read_points();
put(12,.17);put(14,.2);put(13,.2);read_points();
mouse_x=pl+ux[1]*pw;mouse_y=pt+ph*(1-uy[1]);
mouse_cap=1;testclock=20;frame();mouse_cap=0;frame();
mouse_cap=1;testclock=20.15;frame();mouse_cap=0;frame();
require(abs(ux[1]-.25)<.000001 && abs(slider(14)-.2)<.000001,"double click snaps anchor to a time-grid line without resetting its level");
require(slider(12)==.17,"time snapping preserves other anchors");
put(13,.8);put(19,.9);snap_point(3);read_points();
require(slider(19)>=slider(13)+.0019,"time snapping cannot cross neighbour");
put(13,.18);put(19,.5);put(10,1);read_points();keep_y=uy[1];snap_point(1);read_points();
require(abs(ux[1]/grid_step-floor(ux[1]/grid_step+.5))<.000001 && uy[1]==keep_y,"time snapping respects vertical inversion");
put(1,1);put(10,0);put(3,0);mouse_cap=0;frame();
mouse_x=fx+hpv*fw;mouse_y=rail;mouse_cap=1;frame();
mouse_x=fx+fw*.1;frame();mouse_cap=0;frame();
require(slider(6)>10 && slider(6)<slider(7),"HP handle edits high-pass edge");
mouse_x=fx+lpv*fw;mouse_y=rail;mouse_cap=1;frame();
mouse_x=fx+fw*.6;frame();mouse_cap=0;frame();
require(slider(7)>500 && slider(7)>slider(6),"LP handle edits low-pass edge");
mouse_x=hx;mouse_y=hy;mouse_cap=1;frame();mouse_cap=0;frame();
require(slider(9)==1,"headphone button enables sidechain audition");
gfx_ext_flags=2;frame();
require(slider(9)==0 && ui_visible==0,"closing native UI turns audition off");
gfx_ext_flags=0;frame();
mouse_x=192;mouse_y=by+25;mouse_cap=1;menuchoice=1;frame();mouse_cap=0;frame();
require(slider(2)==0,"length menu includes 2/1");
put(10,0);put(13,.18);put(14,0);put(19,.5);put(20,1);put(12,0);put(22,1);put(16,.15);read_points();
eps=.0000000001;
require(abs(ui_curve(uwrap+.18*uspan+eps)-ui_curve(uwrap+.18*uspan))/eps<.001,"transition starts with a smooth tangent");
require(abs(ui_curve(uwrap+.5*uspan)-ui_curve(uwrap+.5*uspan-eps))/eps<.001,"transition ends with a smooth tangent");
require(abs(ui_curve(ux[2])-uy[2])<.0000001,"smooth curve still follows its curvature handle");
put(1,0);gfx_w=1640;gfx_h=1320;mouse_cap=0;frame();
mouse_x=1288;mouse_y=56;mouse_cap=1;frame();mouse_cap=0;frame();
require(slider(10)==1,"scaled interface retains pointer hit testing");
require(abs(ui_curve(0)-ui_curve(1))<.000000001,"period begins from the previous cycle end level");
put(10,0);read_points();
require(ui_curve(uwrap*.5)>ui_curve(uwrap) && ui_curve(uwrap*.5)<ui_curve(0),"boundary join continues into the start of the period");
require(abs(ui_curve(uwrap)-ui_curve(uwrap-eps))/eps<.001,"start fade reaches the first point with a smooth tangent");
put(1,1);read_points();
require(abs(ui_curve(ux[4])-slider(22))<.000001,"sidechain graph reaches the held level at its last anchor");
put(6,100);put(7,1000);mouse_cap=0;frame();
oldhp=slider(6);oldlp=slider(7);
mouse_x=(fx+(hpv+lpv)*fw*.5)*us;mouse_y=rail*us;mouse_cap=1;frame();
mouse_x+=25*us;frame();mouse_cap=0;frame();
require(slider(6)>oldhp && slider(7)>oldlp,"dragging the filter band moves both edges");
require(abs(slider(7)/slider(6)-oldlp/oldhp)<.0000001,"filter band drag preserves bandwidth");
mouse_x=(fx+(hpv+lpv)*fw*.5)*us;mouse_y=rail*us;mouse_cap=1;frame();
mouse_x+=10000*us;frame();mouse_cap=0;frame();
require(slider(7)<=20000.00001 && abs(slider(7)/slider(6)-10)<.000001,"filter band stops at upper boundary without narrowing");
mouse_x=(fx+(hpv+lpv)*fw*.5)*us;mouse_y=rail*us;mouse_cap=1;frame();
mouse_x-=10000*us;frame();mouse_cap=0;frame();
require(slider(6)>=9.99999 && abs(slider(7)/slider(6)-10)<.000001,"filter band stops at lower boundary without narrowing");
put(1,0);put(10,0);put(11,.02);put(21,1);put(13,.18);put(19,.5);read_points();
keep_a=ux[1];keep_b=ux[3];
move_point(0,.06,.1);read_points();
require(abs(ux[0]-.06)<.000001 && abs(uy[0]-.1)<.000001,"first anchor moves freely in time and level");
require(abs(ux[1]-keep_a)<.000001 && abs(ux[3]-keep_b)<.000001,"moving first anchor preserves inner anchor times");
move_point(4,.88,.9);read_points();
require(abs(ux[4]-.88)<.000001 && abs(uy[4]-.9)<.000001,"last anchor moves freely in time and level");
require(abs(ux[1]-keep_a)<.000001 && abs(ux[3]-keep_b)<.000001,"moving last anchor preserves inner anchor times");
require(abs(ui_curve(0)-ui_curve(1))<.00000001,"outer anchors create one continuous join across the period");
require(abs(ui_curve(.06)-.1)<.000001 && abs(ui_curve(.88)-.9)<.000001,"periodic join passes through both outer anchors");
first=ui_curve(.13);put(10,1);read_points();
require(abs(ui_curve(.13)+first-1)<.000001,"INVERT reflects custom outer-anchor join vertically");
put(1,0);mouse_cap=0;frame();
oldhp=slider(6);oldlp=slider(7);oldth=slider(5);oldguard=slider(8);
mouse_x=(fx+hpv*fw)*us;mouse_y=rail*us;mouse_cap=1;frame();mouse_x+=25*us;frame();mouse_cap=0;frame();
require(slider(6)==oldhp && slider(7)==oldlp,"GRID disables filter dragging");
mouse_x=hx*us;mouse_y=hy*us;mouse_cap=1;frame();mouse_cap=0;frame();
require(slider(9)==0,"GRID disables headphone audition");
mouse_x=(fx+fw*.3)*us;mouse_y=(by+93+36)*us;mouse_cap=1;frame();mouse_cap=0;frame();
mouse_x=(right_x+90)*us;mouse_y=(by+93+36)*us;mouse_cap=1;frame();mouse_cap=0;frame();
require(slider(5)==oldth && slider(8)==oldguard,"GRID disables detector settings");

// Gesture region is hover-only and drags both inner anchors in time.
gfx_w=820;gfx_h=660;put(1,0);put(10,0);put(11,.02);put(21,.97);put(13,.2);put(19,.6);put(14,.1);put(20,.9);put(16,.3);
mouse_x=0;mouse_y=0;mouse_cap=0;frame();require(!plot_hover,"region is hidden outside the graph");
mouse_x=pl+pw*.4;mouse_y=pt+ph*.2;frame();require(plot_hover,"region appears only over the transition");
mouse_x=pl+pw*.8;frame();require(!plot_hover,"region stays hidden over the rest of the graph");
mouse_x=pl+pw*.4;frame();
keep_width=slider(19)-slider(13);keep_a=slider(13);keep_b=slider(19);
mouse_cap=1;frame();require(drag==27,"empty transition region captures a group drag");
mouse_x+=pw*.08;frame();mouse_cap=0;frame();
require(slider(13)>keep_a && slider(19)>keep_b,"region drag moves both main anchors");
require(abs(slider(19)-slider(13)-keep_width)<.000001 && slider(14)==.1 && slider(20)==.9 && slider(16)==.3,"region drag preserves width levels and bend");
mouse_x=pl+(ux[1]+ux[3])*.5*pw;mouse_y=pt+ph*.2;mouse_cap=1;frame();mouse_x+=10000;frame();mouse_cap=0;frame();
require(slider(19)<=.99900001 && abs(slider(19)-slider(13)-keep_width)<.000001,"group drag clamps without narrowing");
put(13,.2);put(19,.6);mouse_cap=0;frame();testclock+=1;
mouse_x=pl+ux[1]*pw;mouse_y=pt+ph*(1-uy[1]);mouse_cap=1;frame();
require(drag==31,"point dragging takes priority over shaded region");
mouse_cap=0;frame();
// Middle double-click snaps only points 2 and 4, with an off-grid middle allowed.
put(11,0);put(21,1);put(13,.13);put(19,.26);put(14,.15);put(20,.8);put(16,.3);
mouse_cap=0;frame();testclock+=1;
mouse_x=pl+ux[2]*pw;mouse_y=pt+ph*(1-uy[2]);mouse_cap=1;frame();mouse_cap=0;frame();
testclock+=.15;mouse_cap=1;frame();mouse_cap=0;frame();
require(abs(ux[1]-.125)<.000001 && abs(ux[3]-.25)<.000001,"middle double-click snaps points 2 and 4 to their nearest time lines");
require(abs(ux[2]-.1875)<.000001,"middle handle is not snapped independently");
require(slider(14)==.15 && slider(20)==.8 && slider(16)==.3,"middle double-click preserves levels and bend");
// Drag the actual line away from the handles, lifting each outer pair together.
put(11,.02);put(21,.98);put(13,.3);put(19,.65);put(12,.2);put(14,.3);put(20,.7);put(22,.8);
mouse_cap=0;frame();section_x=(ux[0]+ux[1])*.5;
mouse_x=pl+pw*section_x;mouse_y=pt+ph*(1-ui_curve(section_x));testclock+=1;
mouse_cap=1;frame();require(drag==28,"head curve segment captures vertical group drag");
mouse_y-=ph*.1;frame();mouse_cap=0;frame();
require(abs(slider(12)-.3)<.000001 && abs(slider(14)-.4)<.000001,"head segment raises points 1 and 2 together");
require(slider(20)==.7 && slider(22)==.8,"head drag preserves tail points");
section_x=(ux[3]+ux[4])*.5;mouse_x=pl+pw*section_x;mouse_y=pt+ph*(1-ui_curve(section_x));
mouse_cap=1;frame();require(drag==29,"tail curve segment captures vertical group drag");
mouse_y+=ph*.2;frame();mouse_cap=0;frame();
require(abs(slider(20)-.5)<.000001 && abs(slider(22)-.6)<.000001,"tail segment lowers points 4 and 5 together");
put(10,1);mouse_cap=0;frame();section_x=(ux[3]+ux[4])*.5;
mouse_x=pl+pw*section_x;mouse_y=pt+ph*(1-ui_curve(section_x));mouse_cap=1;frame();mouse_y-=10000;frame();mouse_cap=0;frame();
require(abs(slider(20))<.000001 && abs(slider(22)-.1)<.000001,"inverted tail drag clamps both endpoints without changing their difference");
put(10,0);mouse_cap=0;frame();testclock+=1;
mouse_x=pl+ux[0]*pw;mouse_y=pt+ph*(1-uy[0]);mouse_cap=1;frame();
require(drag==30,"individual endpoint has priority over segment dragging");mouse_cap=0;frame();
// The readout is momentary, including a press without motion.
put(4,65);mouse_x=kx;mouse_y=ky;testclock+=1;mouse_cap=1;frame();
require(mix_readout && slider(4)==65,"pressing MIX shows current percent without changing it");
mouse_y-=20;frame();require(mix_readout && abs(slider(4)-73.8)<.000001,"MIX drag is twelve percent slower and keeps its readout visible");
mouse_cap=0;frame();require(!mix_readout,"releasing MIX hides its percent");
require(abs(knob_mix-slider(4)*.01)<.0000001,"cached MIX arc follows the actual value");
require(last_font_flags==98,"plugin labels use the bold font flag");
require(ui_accent==0xAF85FF,"one saturated purple is shared by active controls");
keep_mix=slider(4);keep_length=slider(2);keep_hp=slider(6);keep_threshold=slider(5);
mouse_x=(uw-70)*us;mouse_y=30*us;mouse_cap=1;frame();mouse_cap=0;frame();
require(slider(10)==0 && abs(slider(11)-factory_expected[11])<.000001 && abs(slider(21)-factory_expected[21])<.000001,"RESET restores factory outer anchors and direction");
require(slider(12)==0 && slider(14)==0 && slider(20)==1 && slider(22)==1 && abs(slider(16)-.15)<.000001,"RESET restores factory curve levels and bend");
i=1;factory_ok=1;loop(25,abs(slider(i)-factory_expected[i])>.0000001 ? factory_ok=0;i+=1;);
require(factory_ok,"RESET exactly matches every declared REAPER factory parameter");

#resource_root="/fake-reaper";files_enabled=1;
require(user_default(),"RESET finds the mapped user default named No preset");
i=1;user_ok=1;loop(25,abs(slider(i)-user_expected[i])>.0000001 ? user_ok=0;i+=1;);
require(user_ok,"user default loads every saved parameter instead of the first preset or factory values");
put(4,12);reset_shape();require(slider(4)==37 && slider(2)==5,"RESET button restores the current user default");
preset_lines[7]="Data=INVALID";keep=slider(4);
require(!user_default() && slider(4)==keep,"malformed default cannot partially overwrite live parameters");
map_lines[1]="js-SideKick/SideKick.jsfx=Missing preset";
require(!user_default(),"missing mapped preset is detected");
reset_shape();require(slider(4)==100 && slider(2)==3,"RESET falls back to factory values when user default is unavailable");
printf("DONE\\n");
'''
with tempfile.TemporaryDirectory() as tmp:
    path=Path(tmp)/'ui.eel'
    path.write_text(code)
    result=subprocess.run([sys.argv[1],str(path)],text=True,capture_output=True,timeout=30)
print(result.stdout, end='')
assert result.returncode==0 and 'DONE' in result.stdout and 'FAIL' not in result.stdout, result.stderr+result.stdout
