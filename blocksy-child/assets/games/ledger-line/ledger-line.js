(function(){'use strict';
/* ============================================================
   LEDGER LINE LEGEND — Game Engine v7
   ============================================================ */

const CFG={
GRAVITY:.28,JUMP:-6.8,DJUMP:-5.6,DJUMP_HIGH:-7.8,SPEED:2.4,MAX_FALL:6.5,
GLIDE_GRAV:.10,FRICTION:.80,AIR_FRIC:.96,
PLAYER_W:38,PLAYER_H:56,LIVES:3,MAX_LIVES:8,INVINCE:1500,CAM_LERP:.08,
STAFF_SP:20,STAFF_GAP:60,
NOTE_HEAD_RX:22,NOTE_HEAD_RY:14,NOTE_HITBOX_W:56,NOTE_HITBOX_H:32,
NOTE_ATTRACT:.28,
ACC_EXTRA_W:22,GAP_MIN:100,GAP_MAX:180,GAP_MIN_EASY:90,GAP_MAX_EASY:160,
PTS_NOTE:100,PTS_COLLECT:50,PTS_REST:200,COMBO_MULT:.5,
ENEMY_CH:.07,COLLECT_CH:.28,GUESS_CHANCE:.45,LIVE_NOTE_CH:.12,
REST_BONUS_CH:.03,REST_PLATFORM_CH:.06,BEAM_PAIR_CH:.18,ACC_CHANCE:.22,
CHORD_GROUP_CH:.10,CCLEF_ENEMY_CH:.30,
DISAPPEAR_CH:.18,DISAPPEAR_TIME_MIN:4500,DISAPPEAR_TIME_MAX:10000,
STAFF_GLOW_INTERVAL:[6000,10000],STAFF_GLOW_DUR:[3000,5000],
FLY_DURATION:5000,HINT_DURATION:10000,
COLORS:{
BG:'#08080E',
STAFF:'rgba(255,255,255,.45)',STAFF_M:'rgba(255,255,255,.65)',
STAFF_GLOW:'rgba(255,223,100,.95)',STAFF_GLOW_DIM:'rgba(255,223,100,.35)',
LEDGER:'#D0B870',LEDGER_OK:'#E8D092',LEDGER_BAD:'#E6735C',
GOLD:'#D7BF81',GOLD_GL:'rgba(215,191,129,.4)',GOLD_BRIGHT:'#F0E0A0',
PLAYER:'#E8D8B8',PLAYER_HAIR:'#0F0A18',PLAYER_JACKET:'#1A2A4A',
PLAYER_SKIN:'#E8C090',PLAYER_ARMOR:'#2A3A5A',
ENEMY:'#E6735C',COIN:'#FFD700',TXT:'#FFF',TXT_DIM:'#6A6A7A',
MID_C:'rgba(80,200,120,.28)',BONUS:'#50C878',REST:'#5BBCFF',
BAR_GOLD:'rgba(255,215,0,.9)',BAR_GOLD_GL:'rgba(255,215,0,.5)',
VINE:'#FFD700',VINE_GLOW:'rgba(255,210,0,.8)',
REST_PLATFORM:'rgba(80,180,255,.85)'},
NOTES:['C','D','E','F','G','A','B'],
NOTE_POS:{C:0,D:1,E:2,F:3,G:4,A:5,B:6},
KEYS:{KeyA:'C',KeyS:'D',KeyD:'E',KeyF:'F',KeyG:'G',KeyH:'A',KeyJ:'B',
KeyW:'C#',KeyE:'D#',KeyT:'F#',KeyY:'G#',KeyU:'A#'},
REALMS:[
{id:1,name:'The Middle Bridge',desc:'Treble/Bass via Middle C',
noteRange:['B3','C4','D4','E4','F4','G4','A4'],color:'#50C878',unlocked:true},
{id:2,name:'The Attic',desc:'High ledger lines + Middle C zone',
noteRange:['B3','C4','D4','E4','F4','G4','A4','B4','C5','D5','E5','F5','G5','A5','B5','C6','D6','E6','F6'],color:'#5B8DEE',unlocked:true},
{id:3,name:'The Basement',desc:'Low ledger lines + Middle C zone',
noteRange:['E2','F2','G2','A2','B2','C3','D3','E3','F3','G3','A3','B3','C4','D4','E4','F4','G4'],color:'#9B7BDB',unlocked:true},
{id:4,name:'The Stratosphere',desc:'Extreme high + Attic + Middle',
noteRange:['C4','D4','E4','F4','G4','A4','B4','C5','D5','E5','F5','G5','A5','B5','C6','D6','E6','F6','G6','A6','B6','C7','D7','E7','F7'],color:'#E6735C',unlocked:true},
{id:5,name:'The Abyss',desc:'Extreme low + Basement + Middle',
noteRange:['A0','B0','C1','D1','E1','F1','G1','A1','B1','C2','D2','E2','F2','G2','A2','B2','C3','D3','E3','F3','G3','A3','B3','C4'],color:'#BEA86E',unlocked:true}],
MELODIES:{
0:[
{name:'Ode to Joy',notes:['E4','E4','F4','G4','G4','F4','E4','D4','C4','C4','D4','E4','E4','D4','D4']},
{name:'Mary Had a Little Lamb',notes:['E4','D4','C4','D4','E4','E4','E4','D4','D4','D4','E4','G4','G4']},
{name:'Twinkle Twinkle',notes:['C4','C4','G4','G4','A4','A4','G4','F4','F4','E4','E4','D4','D4','C4']}],
1:[
{name:'Attic Ascent',notes:['C4','E4','G4','B4','D5','F5','A5','C6','E6','F6','D6','B5','G5','E5','C5','A4','F4','D4','C4']},
{name:'High Climb',notes:['B3','D4','F4','A4','C5','E5','G5','B5','D6','E6','C6','A5','F5','D5','B4','G4','E4','C4']},
{name:'Skyward Journey',notes:['C4','D4','F4','A4','C5','E5','A5','C6','D6','F6','E6','C6','A5','E5','C5','A4','F4','C4']}],
2:[
{name:'Basement Descent',notes:['G4','E4','C4','A3','F3','D3','B2','G2','E2','F2','A2','C3','E3','G3','B3','D4','F4','G4']},
{name:'Deep Dive',notes:['C4','B3','G3','E3','C3','A2','F2','E2','G2','B2','D3','F3','A3','C4','E4']},
{name:'Cave Explorer',notes:['F4','D4','B3','G3','E3','C3','A2','F2','G2','B2','D3','F3','A3','C4','E4','F4']}],
3:[
{name:'Stratosphere Ascent',notes:['C4','G4','D5','A5','E6','B6','C7','D7','E7','F7','D7','B6','G6','E6','C6','A5','F5','C5','G4','C4']},
{name:'Heaven Run',notes:['E4','A4','D5','G5','C6','F6','A6','C7','E7','F7','D7','A6','F6','C6','G5','D5','A4','E4']}],
4:[
{name:'Abyss Descent',notes:['C4','A3','F3','D3','B2','G2','E2','C2','A1','F1','D1','B0','A0','C1','E1','G1','B1','D2','F2','A2','C3','E3','G3','C4']},
{name:'Deep Ocean',notes:['G3','E3','C3','A2','F2','D2','B1','G1','E1','C1','A0','B0','D1','F1','A1','C2','E2','G2','B2','D3','G3']}]},
SAVE_KEY:'ll_save_v7',
latin:(function(){
// Detect notation system: latin (Do,Ré,Mi) for French/Italian/Spanish/Portuguese-speaking regions
if(window.pmNotation && typeof window.pmNotation.system==='string')return window.pmNotation.system==='latin';
// Fallback: check browser language
const lang=(navigator.language||navigator.userLanguage||'en').toLowerCase();
const latinLangs=['fr','it','es','pt','ro','ca','gl','oc'];
return latinLangs.some(l=>lang.startsWith(l));
})()}; // geo-detected: false=international (A,B,C), true=latin (Do,Ré,Mi)

const Mus={
noteToY(n,o){
// Base Y from staff position; positive sp = below middle C (bass), negative = above (treble)
const sp=((4-o)*7-CFG.NOTE_POS[n]);
// Apply visual gap: bass notes (sp>0) shift down by STAFF_GAP to match bass staff rendering
return sp*CFG.STAFF_SP+(sp>0?CFG.STAFF_GAP:0);},
yToNote(y){
// Reverse: remove gap if in bass territory, then compute note
const inBass=y>CFG.STAFF_SP*0.5;
const adjustedY=inBass?y-CFG.STAFF_GAP:y;
const p=-Math.round(adjustedY/CFG.STAFF_SP),o=4-Math.floor(p/7);
const idx=(((-p)%7)+7)%7;return{note:CFG.NOTES[idx],octave:o};},
randNote(range){const s=range[Math.floor(Math.random()*range.length)];
return{note:s.replace(/[0-9]/g,''),octave:parseInt(s.replace(/[^0-9]/g,''))};},
staffPosFromY(y){
const inBass=y>CFG.STAFF_SP*0.5;
return -Math.round((inBass?y-CFG.STAFF_GAP:y)/CFG.STAFF_SP);},
noteStep(n,steps){const i=(CFG.NOTES.indexOf(n)+steps+70)%7;return CFG.NOTES[i];},
// Build a staircase melody: ascend through range then descend, with small random skips
// This creates coherent musical phrases that traverse the full range like a real score
staircaseMelody(range){
const parsed=range.map(s=>({note:s.replace(/[0-9]/g,''),octave:parseInt(s.replace(/[^0-9]/g,''))}));
const notes=[];
// Ascending phase: walk up with occasional small skips (1-3 steps)
let i=0;
while(i<parsed.length){
notes.push(parsed[i]);
const step=Math.random()<.3?2:Math.random()<.15?3:1;
i+=step;}
// Descending phase: walk back down
i=parsed.length-2;
while(i>=0){
notes.push(parsed[i]);
const step=Math.random()<.3?2:Math.random()<.15?3:1;
i-=step;}
// Add a small random wiggle section in the middle of the range for variety
const mid=Math.floor(parsed.length/2);
const wiggleLen=4+Math.floor(Math.random()*4);
for(let w=0;w<wiggleLen;w++){
const off=Math.floor(Math.random()*5)-2;
const idx=Math.max(0,Math.min(parsed.length-1,mid+off));
notes.push(parsed[idx]);}
return notes;},
melodyForRealm(realm){
const rl=CFG.REALMS[realm];
if(!rl)return Mus.staircaseMelody(CFG.REALMS[0].noteRange);
// Basement/Abyss: reverse range so staircase descends from middle C zone
if(realm===2||realm===4){
const rev=[...rl.noteRange].reverse();
return Mus.staircaseMelody(rev);}
return Mus.staircaseMelody(rl.noteRange);},
generatePhrase(range,len){
const notes=[];let prev=null;
for(let i=0;i<len;i++){let nd,att=0;
do{nd=Mus.randNote(range);att++;}
while(prev&&nd.note===prev.note&&nd.octave===prev.octave&&att<10);
notes.push(nd);prev=nd;}return notes;},
// Return display name for a note respecting international/latin notation setting
noteLabel(note,accidental,latin){
const acc=accidental==='sharp'?'#':accidental==='flat'?'b':'';
if(latin){const L={C:'Do',D:'Ré',E:'Mi',F:'Fa',G:'Sol',A:'La',B:'Si'};return(L[note]||note)+acc;}
return note+acc;}
};

function rrect(ctx,x,y,w,h,r){
r=Math.min(r,w/2,h/2);
ctx.beginPath();ctx.moveTo(x+r,y);ctx.lineTo(x+w-r,y);ctx.arcTo(x+w,y,x+w,y+r,r);
ctx.lineTo(x+w,y+h-r);ctx.arcTo(x+w,y+h,x+w-r,y+h,r);
ctx.lineTo(x+r,y+h);ctx.arcTo(x,y+h,x,y+h-r,r);
ctx.lineTo(x,y+r);ctx.arcTo(x,y,x+r,y,r);ctx.closePath();}

const NoteRenderer={
drawNote(ctx,x,y,staffPos,opts){
opts=opts||{};const col=opts.color||CFG.COLORS.GOLD;
const pulse=opts.pulse||0;const sz=opts.size||1;
const nType=opts.noteType||'quarter';
const rx=CFG.NOTE_HEAD_RX*sz,ry=CFG.NOTE_HEAD_RY*sz;
const isHollow=nType==='half'||nType==='whole';
ctx.save();
if(opts.glow){ctx.shadowColor=col;ctx.shadowBlur=5+pulse*10;}
ctx.translate(x,y);ctx.rotate(-0.2);
ctx.beginPath();ctx.ellipse(0,0,rx,ry,0,0,Math.PI*2);
if(isHollow){
ctx.strokeStyle=col;ctx.lineWidth=2.8;ctx.stroke();
ctx.fillStyle='rgba(8,8,16,.6)';ctx.fill();
}else{
ctx.fillStyle=col;ctx.fill();
ctx.strokeStyle='rgba(0,0,0,.3)';ctx.lineWidth=1.5;ctx.stroke();}
if(!isHollow){
ctx.beginPath();ctx.ellipse(-rx*.2,-ry*.3,rx*.45,ry*.4,0,0,Math.PI*2);
ctx.fillStyle='rgba(255,255,255,.12)';ctx.fill();}
ctx.rotate(0.2);ctx.translate(-x,-y);
if(!opts.noStem&&nType!=='whole'){
const stemUp=staffPos>=0;
const stemLen=Math.max(50,CFG.STAFF_SP*1.6)*sz;
const stemX=stemUp?x+rx*.88:x-rx*.88;
const stemY2=stemUp?y-stemLen:y+stemLen;
ctx.beginPath();ctx.moveTo(stemX,y);ctx.lineTo(stemX,stemY2);
ctx.strokeStyle=col;ctx.lineWidth=2.5;ctx.lineCap='round';ctx.stroke();
// Eighth note flag
if(nType==='eighth'){
const flagDir=stemUp?1:-1;
const fX=stemX,fY=stemY2;
ctx.beginPath();
ctx.moveTo(fX,fY);
ctx.bezierCurveTo(fX+12*sz,fY+flagDir*8*sz,fX+10*sz,fY+flagDir*20*sz,fX+2*sz,fY+flagDir*28*sz);
ctx.strokeStyle=col;ctx.lineWidth=2.5;ctx.stroke();}}
ctx.shadowBlur=0;ctx.restore();},

drawAccidental(ctx,x,y,type,col){
col=col||CFG.COLORS.GOLD;ctx.save();
ctx.font='bold 20px serif';ctx.textAlign='center';ctx.textBaseline='middle';
ctx.fillStyle=col;ctx.shadowColor=col;ctx.shadowBlur=5;
ctx.fillText(type==='sharp'?'\u266F':'\u266D',x-33,y+3);
ctx.shadowBlur=0;ctx.restore();},

// sx,sy = screen coords; staffPos; noteColor; noteName (e.g. 'C','D'...)
drawLedgerLines(ctx,sx,sy,staffPos,noteColor,noteName){
const S=CFG.STAFF_SP;
// Middle C ledger line: narrower, precisely centered on note
const hwC=CFG.NOTE_HEAD_RX*1.28;
// Above/below staff ledger lines: slightly wider for readability
const hw=CFG.NOTE_HEAD_RX*1.55;
const col=noteColor||CFG.COLORS.LEDGER;
ctx.save();ctx.lineCap='round';ctx.lineWidth=2.5;ctx.globalAlpha=.92;
// Helper: draw a ledger line at staffPos lp (screen coords from note sy)
const drawL=(lp,c,halfW)=>{
const ly=sy+(staffPos-lp)*S;
ctx.strokeStyle=c||CFG.COLORS.LEDGER;
ctx.beginPath();ctx.moveTo(sx-(halfW||hw),ly);ctx.lineTo(sx+(halfW||hw),ly);ctx.stroke();};
// ── Middle C: ONLY the C4 note itself (staffPos===0, noteName==='C')
// D4 and B3 do NOT get this bar
if(staffPos===0&&noteName==='C'){drawL(0,col,hwC);}
// ── Above treble staff: ledger lines at even positions ≥ 12 (match note color)
if(staffPos>=12){
const topLp=staffPos%2===0?staffPos:staffPos-1;
for(let p=12;p<=topLp;p+=2)drawL(p,col,hw);}
// ── Below bass staff: ledger lines at even positions ≤ -12 (match note color)
if(staffPos<=-12){
const botLp=staffPos%2===0?staffPos:staffPos+1;
for(let p=-12;p>=botLp;p-=2)drawL(p,col,hw);}
ctx.restore();}
};

const RestRenderer={
// Quarter rest: zig-zag symbol, centered on the staff line y
drawQuarterRest(ctx,x,y,col,sz){sz=sz||1;col=col||CFG.COLORS.REST;
ctx.save();ctx.translate(x,y);ctx.scale(sz,sz);
ctx.strokeStyle=col;ctx.lineWidth=3;ctx.lineCap='round';ctx.lineJoin='round';
ctx.shadowColor=col;ctx.shadowBlur=4;
ctx.beginPath();
ctx.moveTo(4,-20);ctx.lineTo(8,-12);ctx.lineTo(-4,-6);
ctx.lineTo(8,2);ctx.lineTo(-4,8);
ctx.bezierCurveTo(-8,12,-6,18,0,20);
ctx.stroke();ctx.shadowBlur=0;ctx.restore();},
// Half rest: sits ON the staff line (flat bottom on line, rectangle rises ABOVE)
// y = staff line position → bottom edge of rect at y
drawHalfRest(ctx,x,y,col,sz){sz=sz||1;col=col||CFG.COLORS.REST;
ctx.save();ctx.translate(x,y);ctx.scale(sz,sz);
ctx.fillStyle=col;ctx.strokeStyle=col;
ctx.shadowColor=col;ctx.shadowBlur=4;
// rect: top at -9, bottom at 0 (sits on line y=0)
ctx.fillRect(-13,-9,26,9);
ctx.lineWidth=2.5;ctx.beginPath();ctx.moveTo(-17,0);ctx.lineTo(17,0);ctx.stroke();
ctx.shadowBlur=0;ctx.restore();},
// Whole rest: HANGS from the staff line (flat top on line, rectangle hangs DOWN)
// y = staff line position → top edge of rect at y
drawWholeRest(ctx,x,y,col,sz){sz=sz||1;col=col||CFG.COLORS.REST;
ctx.save();ctx.translate(x,y);ctx.scale(sz,sz);
ctx.fillStyle=col;ctx.strokeStyle=col;
ctx.shadowColor=col;ctx.shadowBlur=4;
// rect: top at 0, bottom at 9 (hangs from line y=0)
ctx.fillRect(-13,0,26,9);
ctx.lineWidth=2.5;ctx.beginPath();ctx.moveTo(-17,0);ctx.lineTo(17,0);ctx.stroke();
ctx.shadowBlur=0;ctx.restore();}
};

class Particle{
constructor(x,y,c,vx,vy,life,sz){this.x=x;this.y=y;this.c=c;
this.vx=vx||(Math.random()-.5)*4;this.vy=vy||(Math.random()-.5)*4-2;
this.life=life||40;this.ml=this.life;this.sz=sz||3;}
update(){this.x+=this.vx;this.y+=this.vy;this.vy+=.06;this.life--;return this.life>0;}
draw(ctx,cx,cy){const a=this.life/this.ml;ctx.globalAlpha=a;ctx.fillStyle=this.c;
ctx.beginPath();ctx.arc(this.x-cx,this.y-cy,this.sz*a,0,Math.PI*2);ctx.fill();ctx.globalAlpha=1;}}

class Particles{
constructor(){this.p=[];}
emit(x,y,c,n,o){o=o||{};for(let i=0;i<n;i++)this.p.push(new Particle(x,y,c,
o.vx?o.vx():(Math.random()-.5)*(o.sp||4),o.vy?o.vy():(Math.random()-.5)*(o.sp||4)-2,o.life||40,o.sz||3));}
update(){this.p=this.p.filter(p=>p.update());}
draw(ctx,cx,cy){this.p.forEach(p=>p.draw(ctx,cx,cy));}}


// ── NoteBlock ──
class NoteBlock{
constructor(x,y,nd,requireGuess,showLabel){
this.x=x;this.y=y;this.w=CFG.NOTE_HITBOX_W;this.h=CFG.NOTE_HITBOX_H;
this.note=nd.note;this.oct=nd.octave;
this.requireGuess=requireGuess;this.showLabel=!!showLabel;this.state='idle';
this.identified=!requireGuess;this.crumT=0;this.glow=0;this.pulse=0;
this.staffPos=Mus.staffPosFromY(this.y);
// Easy mode: sharps only (simpler reading); hard/pianist: sharps and flats
this.accidental=Math.random()<CFG.ACC_CHANCE?(this.showLabel?'sharp':(Math.random()<.5?'sharp':'flat')):null;
// Prevent enharmonic duplicates: Cb=B, Fb=E, E#=F, B#=C
if((this.note==='C'||this.note==='F')&&this.accidental==='flat')this.accidental=null;
if((this.note==='E'||this.note==='B')&&this.accidental==='sharp')this.accidental=null;
// Visual note type (cosmetic only — does not affect gameplay)
const rnd=Math.random();
this.noteType=rnd<0.45?'quarter':rnd<0.7?'eighth':rnd<0.9?'half':'whole';
// Disappearing note (post-correct fade)
this.disappear=Math.random()<CFG.DISAPPEAR_CH;
this.disappearTimer=this.disappear?(CFG.DISAPPEAR_TIME_MIN+Math.random()*(CFG.DISAPPEAR_TIME_MAX-CFG.DISAPPEAR_TIME_MIN)):0;
this.alpha=1;
// Live note: randomly appears/disappears BEFORE being answered — unpredictable
this.blinkCycle=Math.random()<(showLabel?0.09:0.30); // 9% easy, 30% hard
this.blinkState='visible'; // 'visible'|'hidden'
this.blinkTimer=2000+Math.random()*5000; // initial delay before first blink
this.blinkVisT=3000+Math.random()*1000; // 3-4s visible
this.blinkHidT=3000+Math.random()*1000;} // 3-4s hidden — slow, random rhythm
update(dt){
this.pulse+=dt*.003;
if(this.state==='correct')this.glow=Math.min(1,this.glow+dt*.003);
if(this.state==='crumbling')this.crumT+=dt;
// Post-correct disappear fade
if(this.disappear&&this.disappearTimer>0&&this.state==='correct'){
this.disappearTimer-=dt;
if(this.disappearTimer<=0)this.alpha=Math.max(0,this.alpha-dt*.001);
else if(this.disappearTimer<2000)this.alpha=.4+Math.sin(Date.now()*.02)*.3;}
// Live note: blink in and out before player answers
if(this.blinkCycle&&this.state!=='correct'&&this.state!=='wrong'&&this.state!=='crumbling'&&this.state!=='challenged'){
this.blinkTimer-=dt;
if(this.blinkTimer<=0){
if(this.blinkState==='visible'){this.blinkState='hidden';this.blinkTimer=this.blinkHidT;}
else{this.blinkState='visible';this.blinkTimer=this.blinkVisT;}}
// Smooth transitions over ~500ms
if(this.blinkState==='hidden'){this.alpha=Math.max(0,this.alpha-dt*.002);}
else{this.alpha=Math.min(1,this.alpha+dt*.002);}}}

isSolid(){return this.alpha>.25&&this.state!=='crumbling';}
rect(){
const ew=this.accidental?CFG.ACC_EXTRA_W:0;
return{x:this.x-this.w/2-ew,y:this.y-this.h/2,w:this.w+ew,h:this.h};}
draw(ctx,cx,cy){
if(this.alpha<=0)return; // fully invisible — skip everything including stems/ledger lines
const sx=this.x-cx,sy=this.y-cy;
if(sx<-200||sx>ctx.canvas.width*2.5+200||sy<-300||sy>ctx.canvas.height*2.5+300)return;
// Fade notes that slide under the sticky clef area (left ~110px of screen)
const clefFade=sx<110?Math.max(0,(sx-65)/45):1;
if(clefFade<=0)return;
ctx.save();ctx.globalAlpha=this.alpha*clefFade;
if(this.state==='crumbling'){const sh=Math.sin(this.crumT*.08)*5;
ctx.translate(sh,0);ctx.globalAlpha*=Math.max(0,1-this.crumT/800);}
let col=CFG.COLORS.GOLD;
if(this.state==='correct')col='#E8C84A'; // warm amber-gold when correctly played
else if(this.state==='challenged')col='#9B59F5'; // soft violet when player must play it
else if(this.state==='wrong')col=CFG.COLORS.LEDGER_BAD;
// Blink slowly when walkable (correct/idle identified)
const p=Math.sin(this.pulse)*.5+.5;
if(this.identified&&this.state!=='wrong'){
ctx.shadowColor=col;ctx.shadowBlur=3+p*8;}
NoteRenderer.drawNote(ctx,sx,sy,this.staffPos,{color:col,glow:true,pulse:p,noteType:this.noteType});
NoteRenderer.drawLedgerLines(ctx,sx,sy,this.staffPos,col,this.note);
if(this.accidental)NoteRenderer.drawAccidental(ctx,sx,sy,this.accidental,col);
// Easy mode: show note name label below note with background pill for readability
if(this.showLabel&&this.state!=='crumbling'){
const lbl=Mus.noteLabel(this.note,this.accidental,CFG.latin);
const ly=sy+CFG.NOTE_HEAD_RX+18;
ctx.font='bold 13px Montserrat,sans-serif';ctx.textAlign='center';ctx.textBaseline='middle';
const tw=ctx.measureText(lbl).width;
// Background pill
ctx.fillStyle='rgba(5,5,15,.75)';
rrect(ctx,sx-tw/2-5,ly-9,tw+10,18,6);ctx.fill();
ctx.strokeStyle='rgba(180,220,255,.3)';ctx.lineWidth=1;
rrect(ctx,sx-tw/2-5,ly-9,tw+10,18,6);ctx.stroke();
// Text
ctx.fillStyle='rgba(200,230,255,'+(0.85+p*.15)+')';
ctx.shadowColor='rgba(180,220,255,.5)';ctx.shadowBlur=4;
ctx.fillText(lbl,sx,ly);
ctx.shadowBlur=0;}
ctx.restore();}}

// ── RestPlatform (blue rest on staff line — safe landing, no note challenge) ──
class RestPlatform{
constructor(x,staffLinePos){
this.x=x;this.staffLinePos=staffLinePos;
this.y=-staffLinePos*CFG.STAFF_SP+(staffLinePos<0?CFG.STAFF_GAP:0);
this.w=60;this.h=16;
this.state='idle'; // idle | used
this.pulse=Math.random()*Math.PI*2;
this.type=['quarter','half','whole'][Math.floor(Math.random()*3)];}
update(dt){this.pulse+=dt*.004;}
rect(){return{x:this.x-this.w/2,y:this.y-this.h/2,w:this.w,h:this.h};}
draw(ctx,cx,cy){
const sx=this.x-cx,sy=this.y-cy;
if(sx<-150||sx>ctx.canvas.width*2.5+150)return;
const p=Math.sin(this.pulse)*.5+.5;
ctx.save();
ctx.shadowColor=CFG.COLORS.REST_PLATFORM;ctx.shadowBlur=10+p*14;
ctx.globalAlpha=.8+p*.18;
// Staff line segment under the rest (like a real staff line, golden-blue)
ctx.strokeStyle=`rgba(100,200,255,${.5+p*.3})`;ctx.lineWidth=2;
ctx.beginPath();ctx.moveTo(sx-36,sy);ctx.lineTo(sx+36,sy);ctx.stroke();
// Draw the rest symbol anchored to the staff line
if(this.type==='quarter')RestRenderer.drawQuarterRest(ctx,sx,sy,CFG.COLORS.REST_PLATFORM,1.1);
else if(this.type==='half')RestRenderer.drawHalfRest(ctx,sx,sy,CFG.COLORS.REST_PLATFORM,1.1);
else RestRenderer.drawWholeRest(ctx,sx,sy,CFG.COLORS.REST_PLATFORM,1.1);
ctx.shadowBlur=0;ctx.restore();}}

// ── BeamPair (eighth note pair — BOTH notes have stems, beam connects their tips) ──
class BeamPair{
constructor(x,nd1,realm,showLabel){
this.nd1=nd1;this.showLabel=!!showLabel;
const steps=1+Math.floor(Math.random()*2);
const n2name=Mus.noteStep(nd1.note,steps);
const oct2=nd1.octave+(nd1.note==='B'&&steps>0?1:0);
this.nd2={note:n2name,octave:oct2};
this.x1=x;this.x2=x+58;
this.y1=Mus.noteToY(nd1.note,nd1.octave);
this.y2=Mus.noteToY(n2name,oct2);
// Easy mode: sharps only; hard: sharps and flats
this.acc1=Math.random()<CFG.ACC_CHANCE?(this.showLabel?'sharp':(Math.random()<.5?'sharp':'flat')):null;
this.acc2=Math.random()<CFG.ACC_CHANCE?(this.showLabel?'sharp':(Math.random()<.5?'sharp':'flat')):null;
// Prevent enharmonic duplicates: Cb=B, Fb=E, E#=F, B#=C
if((this.nd1.note==='E'||this.nd1.note==='B')&&this.acc1==='sharp')this.acc1=null;
if((this.nd1.note==='C'||this.nd1.note==='F')&&this.acc1==='flat')this.acc1=null;
if((n2name==='E'||n2name==='B')&&this.acc2==='sharp')this.acc2=null;
if((n2name==='C'||n2name==='F')&&this.acc2==='flat')this.acc2=null;
this.sp1=Mus.staffPosFromY(this.y1);
this.sp2=Mus.staffPosFromY(this.y2);
// Both notes get stems; stem direction based on average staff position
const avgSp=(this.sp1+this.sp2)/2;
this.stemUp=avgSp>=0;
const stemLen=Math.max(52,CFG.STAFF_SP*1.7);
const sxOff=this.stemUp?CFG.NOTE_HEAD_RX*.88:-CFG.NOTE_HEAD_RX*.88;
// Stem X and tip Y for each note (constant-length stems from note heads)
this.stem1X=this.x1+sxOff;
this.stem2X=this.x2+sxOff;
this.stem1TipY=this.stemUp?this.y1-stemLen:this.y1+stemLen;
this.stem2TipY=this.stemUp?this.y2-stemLen:this.y2+stemLen;
// Beam slope
this.slope=(this.stem2TipY-this.stem1TipY)/(this.stem2X-this.stem1X||1);
this.state='idle';this.pulse=Math.random()*Math.PI*2;this.crumT=0;
// Per-note state for individual challenge/play
this.state1='idle';this.identified1=false;
this.state2='idle';this.identified2=false;
// Stable chalNote wrappers so player.lp identity checks work
const self=this;
this.beamNote1={x:this.x1,y:this.y1,note:this.nd1.note,oct:this.nd1.octave,accidental:this.acc1,
  requireGuess:true,crumT:0,
  get state(){return self.state1;},set state(v){self.state1=v;},
  get identified(){return self.identified1;},set identified(v){self.identified1=v;}};
this.beamNote2={x:this.x2,y:this.y2,note:this.nd2.note,oct:this.nd2.octave,accidental:this.acc2,
  requireGuess:true,crumT:0,
  get state(){return self.state2;},set state(v){self.state2=v;},
  get identified(){return self.identified2;},set identified(v){self.identified2=v;}};}

// Note head hit areas — solid landing zones (NOT the beam bar)
noteRect1(){return{x:this.x1-CFG.NOTE_HITBOX_W/2,y:this.y1-CFG.NOTE_HITBOX_H/2,w:CFG.NOTE_HITBOX_W,h:CFG.NOTE_HITBOX_H};}
noteRect2(){return{x:this.x2-CFG.NOTE_HITBOX_W/2,y:this.y2-CFG.NOTE_HITBOX_H/2,w:CFG.NOTE_HITBOX_W,h:CFG.NOTE_HITBOX_H};}
isSolid1(){return this.state!=='crumbling'||this.crumT<600;}
isSolid2(){return this.state!=='crumbling'||this.crumT<600;}
update(dt){this.pulse+=dt*.003;if(this.state==='crumbling')this.crumT+=dt;}
draw(ctx,cx,cy){
const sx1=this.x1-cx,sy1=this.y1-cy;
const sx2=this.x2-cx,sy2=this.y2-cy;
if(sx2<-200||sx1>ctx.canvas.width*2.5+200)return;
ctx.save();
if(this.state==='crumbling'){const sh=Math.sin(this.crumT*.08)*4;ctx.translate(sh,0);ctx.globalAlpha=Math.max(0,1-this.crumT/800);}
const p=Math.sin(this.pulse)*.5+.5;
// Per-note colors based on individual state
const _nc=s=>s==='correct'?'#E8C84A':s==='challenged'?'#9B59F5':s==='wrong'?CFG.COLORS.LEDGER_BAD:CFG.COLORS.GOLD;
const col1=_nc(this.state1),col2=_nc(this.state2);
// Draw note heads individually with their own color
NoteRenderer.drawNote(ctx,sx1,sy1,this.sp1,{color:col1,glow:true,pulse:p,noStem:true});
NoteRenderer.drawNote(ctx,sx2,sy2,this.sp2,{color:col2,glow:true,pulse:p,noStem:true});
if(this.acc1)NoteRenderer.drawAccidental(ctx,sx1,sy1,this.acc1,col1);
if(this.acc2)NoteRenderer.drawAccidental(ctx,sx2,sy2,this.acc2,col2);
// Ledger lines match note color
NoteRenderer.drawLedgerLines(ctx,sx1,sy1,this.sp1,col1,this.nd1.note);
NoteRenderer.drawLedgerLines(ctx,sx2,sy2,this.sp2,col2,this.nd2.note);
// === Draw BOTH stems in blended color ===
const sbx1=this.stem1X-cx,sby1=this.stem1TipY-cy;
const sbx2=this.stem2X-cx,sby2=this.stem2TipY-cy;
ctx.lineWidth=2.5;ctx.lineCap='round';
// Stem 1
ctx.strokeStyle=col1;ctx.shadowColor=col1;ctx.shadowBlur=3;
ctx.beginPath();ctx.moveTo(sbx1,sy1);ctx.lineTo(sbx1,sby1);ctx.stroke();
// Stem 2
ctx.strokeStyle=col2;ctx.shadowColor=col2;
ctx.beginPath();ctx.moveTo(sbx2,sy2);ctx.lineTo(sbx2,sby2);ctx.stroke();
// === Beam bar — decorative only, NO glow, NOT solid (player passes through) ===
ctx.beginPath();ctx.moveTo(sbx1,sby1);ctx.lineTo(sbx2,sby2);
ctx.lineWidth=7;ctx.strokeStyle='rgba(215,191,129,.6)';ctx.lineCap='butt';ctx.shadowBlur=0;ctx.stroke();
// Note name labels below note heads (always show in easy/label mode, regardless of state)
if(this.showLabel){
ctx.save();ctx.font='bold 12px Montserrat,sans-serif';ctx.textAlign='center';ctx.textBaseline='middle';
ctx.shadowColor='rgba(180,220,255,.6)';ctx.shadowBlur=6;
const lc='rgba(180,220,255,'+(0.72+p*.2)+')';
if(this.state1!=='crumbling'){ctx.fillStyle=lc;ctx.fillText(Mus.noteLabel(this.nd1.note,this.acc1,CFG.latin),sx1,sy1+CFG.NOTE_HEAD_RX+16);}
if(this.state2!=='crumbling'){ctx.fillStyle=lc;ctx.fillText(Mus.noteLabel(this.nd2.note,this.acc2,CFG.latin),sx2,sy2+CFG.NOTE_HEAD_RX+16);}
ctx.shadowBlur=0;ctx.restore();}
ctx.restore();}}


// ── Enemy — orbits the note platform in a circle ──
class Enemy{
constructor(x,y,mode){
this.mode=mode||'patrol';
this.alive=true;this.phase=Math.random()*Math.PI*2;
this.w=24;this.h=24;
this.noteX=x;this.noteY=y; // store note position for virus detection
if(this.mode==='wander'){
// Wander near the note position but in a wider orbit
this.orbitR=60+Math.random()*40;
this.orbitSpd=0.0012+Math.random()*.0006;
}else{
// Orbits the note in a tight circle (right to left = counterclockwise)
this.orbitR=30+Math.random()*10;
this.orbitSpd=0.0018+Math.random()*.001;}
this.x=this.noteX+Math.cos(this.phase)*this.orbitR;
this.y=this.noteY+Math.sin(this.phase)*this.orbitR;}
update(dt){if(!this.alive)return;
// Orbit counterclockwise (right to left)
this.phase-=this.orbitSpd*dt;
this.x=this.noteX+Math.cos(this.phase)*this.orbitR;
this.y=this.noteY+Math.sin(this.phase)*this.orbitR;}
rect(){return{x:this.x-this.w/2,y:this.y-this.h/2,w:this.w,h:this.h};}
draw(ctx,cx,cy){if(!this.alive)return;const r=this.rect();
const sx=r.x-cx,sy=r.y-cy;
if(sx<-120||sx>ctx.canvas.width*2.5+120)return;
const pulse=Math.sin(this.phase*2)*.5+.5;
const cx2=sx+this.w/2,cy2=sy+this.h/2;
ctx.save();
ctx.shadowColor='#FF2200';ctx.shadowBlur=14+pulse*14;
// Outer pulsing aura
ctx.fillStyle=`rgba(230,40,20,${.12+pulse*.1})`;
ctx.beginPath();ctx.arc(cx2,cy2,18,0,Math.PI*2);ctx.fill();
// 6 spikes rotating outward
ctx.strokeStyle=`rgba(255,90,40,.8)`;ctx.lineWidth=2.2;
for(let i=0;i<6;i++){const a=i/6*Math.PI*2+this.phase*.4;
const ir=11,or=18;
ctx.beginPath();ctx.moveTo(cx2+Math.cos(a)*ir,cy2+Math.sin(a)*ir);
ctx.lineTo(cx2+Math.cos(a)*or,cy2+Math.sin(a)*or);ctx.stroke();}
// Body
const grad=ctx.createRadialGradient(cx2-2,cy2-2,2,cx2,cy2,11);
grad.addColorStop(0,'#FF5530');grad.addColorStop(.5,'#CC2010');grad.addColorStop(1,'#880000');
ctx.fillStyle=grad;ctx.beginPath();ctx.arc(cx2,cy2,11,0,Math.PI*2);ctx.fill();
// Dark inner
ctx.fillStyle='rgba(50,0,0,.6)';ctx.beginPath();ctx.arc(cx2,cy2,6,0,Math.PI*2);ctx.fill();
// Evil eyes
const eyeY=cy2-1;
ctx.fillStyle='#FFE000';
ctx.beginPath();ctx.ellipse(cx2-3.5,eyeY,2.8,2.2,0,0,Math.PI*2);ctx.fill();
ctx.beginPath();ctx.ellipse(cx2+3.5,eyeY,2.8,2.2,0,0,Math.PI*2);ctx.fill();
ctx.fillStyle='#600000';
ctx.beginPath();ctx.ellipse(cx2-3,eyeY+.3,1.4,1.6,.2,0,Math.PI*2);ctx.fill();
ctx.beginPath();ctx.ellipse(cx2+4,eyeY+.3,1.4,1.6,-.2,0,Math.PI*2);ctx.fill();
// Angry brows
ctx.strokeStyle='#FF2200';ctx.lineWidth=1.8;ctx.lineCap='round';
ctx.beginPath();ctx.moveTo(cx2-5.5,eyeY-3.5);ctx.lineTo(cx2-1,eyeY-2);ctx.stroke();
ctx.beginPath();ctx.moveTo(cx2+5.5,eyeY-3.5);ctx.lineTo(cx2+1,eyeY-2);ctx.stroke();
// Symbol (# or b) centered
ctx.fillStyle=`rgba(255,160,120,${.7+pulse*.3})`;
ctx.font=`bold 8px serif`;ctx.textAlign='center';ctx.textBaseline='middle';
ctx.fillText(this.phase%2<1?'\u266F':'\u266D',cx2,cy2+4);
ctx.shadowBlur=0;ctx.restore();}}

// ── Collectibles ──
class Collectible{
constructor(x,y,type){this.x=x;this.y=y-32;this.w=20;this.h=20;
this.type=type||'note';this.collected=false;this.bp=Math.random()*Math.PI*2;}
update(dt){if(this.collected)return;this.bp+=dt*.003;}
rect(){return{x:this.x-this.w/2,y:this.y+Math.sin(this.bp)*5-this.h/2,w:this.w,h:this.h};}
draw(ctx,cx,cy){if(this.collected)return;const r=this.rect(),sx=r.x-cx,sy=r.y-cy;
if(sx<-80||sx>ctx.canvas.width*2.5+80)return;
ctx.save();const p=Math.sin(this.bp)*.5+.5;
const configs={
note:{c:CFG.COLORS.COIN,l:'\u266A',bg:CFG.COLORS.COIN+'22'},
life:{c:CFG.COLORS.BONUS,l:'\u2764',bg:CFG.COLORS.BONUS+'22'},
fly:{c:'#88EEFF',l:'\u2708',bg:'rgba(100,220,255,.15)'},
hint:{c:'#C088FF',l:'\u266B',bg:'rgba(180,100,255,.15)'}};
const cc=configs[this.type]||configs.note;
ctx.shadowColor=cc.c;ctx.shadowBlur=10+p*8;
ctx.beginPath();ctx.arc(sx+10,sy+10,15,0,Math.PI*2);ctx.fillStyle=cc.bg;ctx.fill();
ctx.font='18px serif';ctx.textAlign='center';ctx.textBaseline='middle';ctx.fillStyle=cc.c;
ctx.fillText(cc.l,sx+10,sy+10);ctx.shadowBlur=0;ctx.restore();}}

// ── RestBonus ──
class RestBonus{
constructor(x,y,type){this.x=x;this.y=y;this.w=30;this.h=40;this.type=type;
this.collected=false;this.pulse=Math.random()*Math.PI*2;
this.points=type==='whole'?300:type==='half'?200:150;this.givesLife=type==='whole';}
update(dt){if(this.collected)return;this.pulse+=dt*.004;}
rect(){return{x:this.x-this.w/2,y:this.y-this.h/2,w:this.w,h:this.h};}
draw(ctx,cx,cy){if(this.collected)return;const sx=this.x-cx,sy=this.y-cy;
if(sx<-100||sx>ctx.canvas.width*2.5+100)return;
ctx.save();const p=Math.sin(this.pulse)*.5+.5,bob=Math.sin(this.pulse*1.2)*3;
ctx.shadowColor=CFG.COLORS.REST;ctx.shadowBlur=8+p*10;ctx.globalAlpha=.8+p*.2;
if(this.type==='quarter')RestRenderer.drawQuarterRest(ctx,sx,sy+bob,CFG.COLORS.REST,1.2);
else if(this.type==='half')RestRenderer.drawHalfRest(ctx,sx,sy+bob,CFG.COLORS.REST,1.2);
else RestRenderer.drawWholeRest(ctx,sx,sy+bob,CFG.COLORS.REST,1.2);
if(this.givesLife){ctx.font='10px serif';ctx.textAlign='center';
ctx.fillStyle='#FF8888';ctx.globalAlpha=.6+p*.4;ctx.fillText('\u2665',sx,sy+bob-28);}
ctx.shadowBlur=0;ctx.restore();}}

// ── VoidRescueBar — horizontal platform oscillating far below staff as emergency catch ──
class VoidRescueBar{
constructor(x,realm){
// HORIZONTAL bar (wide, short) — player can land on top of it
this.x=x;this.w=110+Math.random()*60;this.h=14;
// Per-realm: bars stay FAR below the staff and note zones — never overlap notes
// They oscillate in a zone well below the lowest staff line (-10*SP for bass = -320)
// Bridge: notes around 0, bars orbit below 600-1000
// Attic: notes go up to -600, bars orbit below 500-900
// Basement: notes go down to +600, bars orbit below 900-1400
// Strato: notes go up to -800, bars orbit below 400-900
// Abyss: notes go down to +1200, bars orbit below 1400-2000
const rCfg=[
{b:24,a:200,r:80},   // Bridge:   much lower, bigger sweep
{b:22,a:180,r:70},   // Attic:    below treble staff, deeper
{b:30,a:240,r:80},   // Basement: much lower below bass notes
{b:20,a:180,r:70},   // Strato:   below high notes, deeper
{b:34,a:280,r:90}    // Abyss:    far below deep notes
];
const rc=rCfg[realm]||rCfg[0];
// On mobile (small STAFF_SP), bring bars closer and lower
const mobScale=CFG.STAFF_SP<16?0.7:CFG.STAFF_SP<18?0.85:1;
this.baseY=rc.b*CFG.STAFF_SP*(mobScale<1?1.15:1);
this.phase=Math.random()*Math.PI*2;
this.amp=(rc.a+Math.random()*rc.r)*mobScale;
this.speed=0.0008+Math.random()*.0008;
this.pulse=0;}
update(dt){this.phase+=dt*this.speed;this.pulse+=dt*.003;}
currentY(){return this.baseY-Math.abs(Math.sin(this.phase))*this.amp;}
rect(){return{x:this.x-this.w/2,y:this.currentY(),w:this.w,h:this.h};}
draw(ctx,cx,cy){
const r=this.rect(),sx=r.x-cx,sy=r.y-cy;
if(sx+r.w<-60||sx>ctx.canvas.width*2.5+60||sy<-60||sy>ctx.canvas.height*2.5+60)return;
const glow=Math.sin(this.pulse)*.5+.5;
ctx.save();
ctx.shadowColor='rgba(215,191,129,.55)';ctx.shadowBlur=8+glow*12;
const grad=ctx.createLinearGradient(sx,sy,sx,sy+r.h);
grad.addColorStop(0,'rgba(240,224,160,.95)');
grad.addColorStop(.5,CFG.COLORS.GOLD);
grad.addColorStop(1,'rgba(180,150,90,.85)');
ctx.fillStyle=grad;rrect(ctx,sx,sy,r.w,r.h,4);ctx.fill();
// Highlight line on top
ctx.fillStyle='rgba(255,255,255,.32)';ctx.fillRect(sx+4,sy+1,r.w-8,2);
ctx.shadowBlur=0;ctx.restore();}}

// ── GlowingStaffLine ──
class GlowingStaffLine{
constructor(){this.active=false;this.lineY=0;this.timer=0;this.blinkTimer=0;
this.state='off';this.cooldown=this._ri();
// Lines per staff group
this.trebleLines=[2,4,6,8,10];
this.bassLines=[-2,-4,-6,-8,-10];
this.allLines=[2,4,6,8,10,-2,-4,-6,-8,-10];}
_ri(){return CFG.STAFF_GLOW_INTERVAL[0]+Math.random()*(CFG.STAFF_GLOW_INTERVAL[1]-CFG.STAFF_GLOW_INTERVAL[0]);}
// realm: 0=both staves, 1/3=treble only, 2/4=bass only
update(dt,realm){
if(this.state==='off'){this.cooldown-=dt;if(this.cooldown<=0){
let pool;
if(realm===1||realm===3)pool=this.trebleLines;
else if(realm===2||realm===4)pool=this.bassLines;
else pool=this.allLines;
const lp=pool[Math.floor(Math.random()*pool.length)];
this.lineY=-lp*CFG.STAFF_SP+(lp<0?CFG.STAFF_GAP:0);
this.timer=CFG.STAFF_GLOW_DUR[0]+Math.random()*(CFG.STAFF_GLOW_DUR[1]-CFG.STAFF_GLOW_DUR[0]);
this.state='glowing';this.active=true;}}
else if(this.state==='glowing'){this.timer-=dt;if(this.timer<=0){this.state='blinking';this.blinkTimer=2000;}}
else if(this.state==='blinking'){this.blinkTimer-=dt;if(this.blinkTimer<=0){this.state='off';this.active=false;this.cooldown=this._ri();}}}
canLandOn(){return this.active&&(this.state==='glowing'||this.state==='blinking');}
draw(ctx,cx,cy,W){
if(this.state==='off')return;const sy=this.lineY-cy;
if(sy<-20||sy>ctx.canvas.height+20)return;
ctx.save();let alpha=this.state==='blinking'?(Math.sin(this.blinkTimer*.008)>.0?.8:.2):1;
ctx.globalAlpha=alpha;ctx.shadowColor=CFG.COLORS.STAFF_GLOW;ctx.shadowBlur=15;
ctx.strokeStyle=CFG.COLORS.STAFF_GLOW;ctx.lineWidth=3;
ctx.beginPath();ctx.moveTo(0,sy);ctx.lineTo(W,sy);ctx.stroke();
ctx.shadowBlur=0;ctx.restore();}}


// ── Player ──
class Player{
constructor(x,y){this.x=x;this.y=y;this.w=CFG.PLAYER_W;this.h=CFG.PLAYER_H;
this.vx=0;this.vy=0;this.onG=false;this.dj=true;this.tj=true;this.fr=true;
this.state='idle';this.af=0;this.at=0;this.inv=false;this.invT=0;
this.sqX=1;this.sqY=1;this.lp=null;this.capePhase=0;
this.gliding=false;this.flyTime=0;
this.slowFall=false;this.slowFallT=0;
this.trailPts=[];
// Coyote time: allow jumping shortly after leaving a platform
this.coyoteT=0;
// Jump buffer: remember jump input to execute on next landing
this.jumpBuf=0;
// Barrier proximity state
this._nearBarrier=false;this._nearBarrierDist=999;this._lockedToBarrier=false;this._levitatePhase=0;}

jump(){
if(this.flyTime>0||this._lockedToBarrier)return false;
if(this.onG||this.coyoteT>0){this.vy=CFG.JUMP;this.onG=false;this.coyoteT=0;this.dj=true;this.tj=true;
this.sqX=1.3;this.sqY=.72;this.state='jump';this.gliding=false;this.jumpBuf=0;return true;}
if(this.dj){this.vy=CFG.DJUMP_HIGH;
this.dj=false;this.sqX=.8;this.sqY=1.28;this.gliding=false;this.jumpBuf=0;return true;}
if(this.tj){this.vy=CFG.DJUMP*.85;this.tj=false;this.sqX=.75;this.sqY=1.32;this.jumpBuf=0;return true;}
// Buffer the jump attempt for 100ms
this.jumpBuf=100;
return false;}

update(dt,k){
// Fly mode
if(this.flyTime>0){
this.flyTime-=dt;
let mx=0;if(k.left)mx=-CFG.SPEED*3.5;if(k.right)mx=CFG.SPEED*3.5;
if(k.up)this.vy-=.7;else if(k.down)this.vy+=.7;
else{this.vy*=.85;}
this.vx=mx;this.vx*=CFG.FRICTION;
this.x+=this.vx;this.y+=this.vy;
this.state=Math.abs(this.vx)>.5?'run':'idle';
this.at+=dt;if(this.at>150){this.af=(this.af+1)%8;this.at=0;}
if(this.inv){this.invT-=dt;if(this.invT<=0)this.inv=false;}
return;}

// Slow fall
const effGrav=this.slowFall?CFG.GRAVITY*.25:CFG.GRAVITY;
const effMaxFall=this.slowFall?4:CFG.MAX_FALL;
if(this.slowFall){this.slowFallT-=dt;if(this.slowFallT<=0)this.slowFall=false;}

// Coyote time: count down when airborne
if(!this.onG&&this.coyoteT>0)this.coyoteT-=dt;
// Jump buffer: execute buffered jump on landing
if(this.jumpBuf>0){this.jumpBuf-=dt;if(this.onG&&this.jumpBuf>0){this.jump();}}

// Gliding (hold jump after double jump)
this.gliding=!this.onG&&this.dj===false&&k.up&&this.vy>-2;

let mx=0;if(k.left){mx=-CFG.SPEED;this.fr=false;}if(k.right){mx=CFG.SPEED;this.fr=true;}
if(this.onG){this.vx=mx;this.vx*=CFG.FRICTION;}else{this.vx+=mx*.32;this.vx*=CFG.AIR_FRIC;}

const grav=this.gliding?CFG.GLIDE_GRAV:effGrav;
this.vy+=grav;
if(this.vy>effMaxFall)this.vy=effMaxFall;
this.x+=this.vx;this.y+=this.vy;

// Levitation when locked to electric barrier
if(this._lockedToBarrier){
this.vx=0;this.vy=0;
this._levitatePhase=(this._levitatePhase||0)+dt*.004;
this.y+=Math.sin(this._levitatePhase)*0.5; // gentle bob
this.state='jump'; // arms-up pose
}else if(!this.onG)this.state=this.vy<0?'jump':(this.gliding?'glide':'fall');
else this.state=Math.abs(this.vx)>.5?'run':'idle';
this.at+=dt;if(this.at>(this.state==='run'?110:200)){this.af=(this.af+1)%8;this.at=0;}
this.sqX+=(1-this.sqX)*.15;this.sqY+=(1-this.sqY)*.15;
this.capePhase+=dt*.005;
if(Math.abs(this.vx)>2||Math.abs(this.vy)>2){
this.trailPts.unshift({x:this.x+this.w/2,y:this.y+this.h/2,a:1});}
if(this.trailPts.length>6)this.trailPts.pop();
this.trailPts.forEach(p=>{p.a-=.14;});
this.trailPts=this.trailPts.filter(p=>p.a>0);
if(this.inv){this.invT-=dt;if(this.invT<=0)this.inv=false;}}

startFly(){this.flyTime=CFG.FLY_DURATION;this.vy=0;}

rect(){return{x:this.x,y:this.y,w:this.w,h:this.h};}

draw(ctx,cx,cy){
const sx=this.x-cx,sy=this.y-cy;ctx.save();
const blinkOff=this.inv&&Math.floor(Date.now()/80)%2===0;
ctx.globalAlpha=blinkOff?.3:1;
this.trailPts.forEach((p,i)=>{
const ga=ctx.globalAlpha;ctx.globalAlpha=p.a*.4;
const tc=this.flyTime>0?'#88EEFF':CFG.COLORS.GOLD;
ctx.fillStyle=tc;const ts=4*(1-i/this.trailPts.length);
ctx.beginPath();ctx.arc(p.x-cx,p.y-cy,ts,0,Math.PI*2);ctx.fill();
ctx.globalAlpha=ga;});
const ccx=sx+this.w/2,ccy=sy+this.h;
ctx.translate(ccx,ccy);ctx.scale(this.sqX*(this.fr?1:-1),this.sqY);ctx.translate(-this.w/2,-this.h);
// Fly aura — big sparkly energy field with animated rings
if(this.flyTime>0){
const ft=Date.now()*.003;
ctx.shadowColor='#88EEFF';ctx.shadowBlur=25+Math.sin(ft*3)*12;
// Outer energy ring
ctx.strokeStyle=`rgba(100,220,255,${.2+Math.sin(ft*2)*.1})`;ctx.lineWidth=3;
ctx.beginPath();ctx.ellipse(this.w/2,this.h/2,this.w*1.3+Math.sin(ft*4)*4,this.h*0.9,0,0,Math.PI*2);ctx.stroke();
// Inner glow
ctx.fillStyle='rgba(100,220,255,.12)';
ctx.beginPath();ctx.ellipse(this.w/2,this.h/2,this.w,this.h*.8,0,0,Math.PI*2);ctx.fill();
// Sparkle particles around player
for(let i=0;i<6;i++){
const a=i/6*Math.PI*2+ft*1.5;
const r=this.w*1.1+Math.sin(ft*3+i)*6;
ctx.fillStyle=`rgba(140,238,255,${.5+Math.sin(ft*4+i)*.3})`;
ctx.beginPath();ctx.arc(this.w/2+Math.cos(a)*r,this.h/2+Math.sin(a)*r*.6,2,0,Math.PI*2);ctx.fill();}
// Wing shapes (translucent)
ctx.fillStyle='rgba(100,220,255,.08)';
ctx.beginPath();ctx.moveTo(this.w/2-5,this.h*0.3);
ctx.quadraticCurveTo(this.w/2-this.w*2,this.h*0.15+Math.sin(ft*5)*8,this.w/2-10,this.h*0.6);ctx.fill();
ctx.beginPath();ctx.moveTo(this.w/2+5,this.h*0.3);
ctx.quadraticCurveTo(this.w/2+this.w*2,this.h*0.15+Math.sin(ft*5+1)*8,this.w/2+10,this.h*0.6);ctx.fill();
ctx.shadowBlur=0;}
// Electric barrier aura — blue crackling energy when near/locked to a barrier
if(this._nearBarrier){
const et=Date.now()*.004;
const intensity=this._lockedToBarrier?1:Math.max(0,1-this._nearBarrierDist/120);
const baseAlpha=.15+intensity*.35;
// Outer crackling electric field
ctx.shadowColor=`rgba(60,120,255,${.4*intensity})`;ctx.shadowBlur=18+intensity*22+Math.sin(et*5)*8;
ctx.strokeStyle=`rgba(100,160,255,${baseAlpha+Math.sin(et*3)*.1})`;ctx.lineWidth=2;
ctx.beginPath();ctx.ellipse(this.w/2,this.h/2,this.w*1.4+Math.sin(et*3)*5,this.h*0.95+Math.cos(et*2.5)*3,0,0,Math.PI*2);ctx.stroke();
// Inner electric glow
ctx.fillStyle=`rgba(60,120,255,${.08+intensity*.12})`;
ctx.beginPath();ctx.ellipse(this.w/2,this.h/2,this.w*1.1,this.h*.75,0,0,Math.PI*2);ctx.fill();
// Electric arc sparks around player
for(let i=0;i<8;i++){
const a=i/8*Math.PI*2+et*2;
const r=this.w*1.2+Math.sin(et*4+i*1.7)*8;
const sparkA=(.3+intensity*.4)*(.5+Math.random()*.5);
ctx.fillStyle=`rgba(140,180,255,${sparkA})`;
ctx.beginPath();ctx.arc(this.w/2+Math.cos(a)*r,this.h/2+Math.sin(a)*r*.55,1.5+Math.random()*1.5,0,Math.PI*2);ctx.fill();}
// Mini lightning bolts radiating outward when locked
if(this._lockedToBarrier){
ctx.strokeStyle=`rgba(160,200,255,${.3+Math.sin(et*6)*.2})`;ctx.lineWidth=1.2;
for(let i=0;i<5;i++){
const a=i/5*Math.PI*2+et*1.5;
const r1=this.w*.6,r2=this.w*1.5+Math.sin(et*3+i)*6;
const mx=(r1+r2)/2,jitter=Math.sin(et*7+i*3)*8;
ctx.beginPath();
ctx.moveTo(this.w/2+Math.cos(a)*r1,this.h/2+Math.sin(a)*r1*.5);
ctx.lineTo(this.w/2+Math.cos(a+.1)*mx+jitter,this.h/2+Math.sin(a+.1)*mx*.5);
ctx.lineTo(this.w/2+Math.cos(a)*r2,this.h/2+Math.sin(a)*r2*.5);
ctx.stroke();}
// Levitation bob — shift player up/down slightly
this._levitatePhase=(this._levitatePhase||0)+.03;
}
ctx.shadowBlur=0;}
// Glide aura — cape-like wind effect with trailing ribbons
if(this.gliding){
const gt=Date.now()*.002;
ctx.fillStyle='rgba(215,191,129,.06)';
ctx.beginPath();ctx.ellipse(this.w/2,this.h/2,this.w*1.8,this.h*.4,0,0,Math.PI*2);ctx.fill();
// Wind ribbons trailing behind
ctx.strokeStyle='rgba(215,191,129,.15)';ctx.lineWidth=2;
for(let i=0;i<3;i++){
ctx.beginPath();ctx.moveTo(this.w/2,this.h*0.3+i*10);
ctx.quadraticCurveTo(this.w/2-20-i*12,this.h*0.3+i*10+Math.sin(gt*3+i)*8,
this.w/2-40-i*15,this.h*0.4+i*10+Math.sin(gt*4+i)*6);ctx.stroke();}
// Subtle golden sparkles
for(let i=0;i<4;i++){
const a=i/4*Math.PI+gt*1.2;
ctx.fillStyle=`rgba(215,191,129,${.3+Math.sin(gt*3+i)*.2})`;
ctx.beginPath();ctx.arc(this.w/2+Math.cos(a)*this.w*1.3,this.h/2+Math.sin(a)*this.h*0.3,1.5,0,Math.PI*2);ctx.fill();}}
this._draw(ctx,0,0);ctx.restore();}

_draw(ctx,x,y){
const w=this.w,h=this.h,cx=x+w/2;
const t=Date.now()*.001;
const anim=this.state==='run'?Math.sin(this.af*.8):0;
const jOff=this.state==='jump'?-5:this.state==='fall'?4:this.state==='glide'?2:0;
const bounce=this.state==='idle'?Math.sin(t*3)*1.5:0;
const lean=this.state==='run'?(this.fr?3:-3):0;
const headR=w*0.38; // big cartoon head
const bodyTop=y+headR*2+2;
const legLen=h*0.32;

// ── Shadow on ground
ctx.fillStyle='rgba(0,0,0,.18)';
ctx.beginPath();ctx.ellipse(cx,y+h+1,w*0.4,3,0,0,Math.PI*2);ctx.fill();

// ── Musical cape (flows behind)
const capeW=anim*6+(this.state==='jump'?7:this.state==='fall'?-5:this.state==='glide'?10:0)+Math.sin(this.capePhase)*3;
const cg=ctx.createLinearGradient(cx,bodyTop-4,cx+12+capeW,y+h-6);
cg.addColorStop(0,'rgba(100,60,180,.85)');cg.addColorStop(.5,'rgba(60,30,140,.7)');cg.addColorStop(1,'rgba(40,15,100,.2)');
ctx.fillStyle=cg;
ctx.beginPath();ctx.moveTo(cx+3,bodyTop-2);
ctx.quadraticCurveTo(cx+18+capeW,bodyTop+10,cx+15+capeW*1.3,y+h-6);
ctx.lineTo(cx+5,y+h-6);ctx.closePath();ctx.fill();
// Cape music notes decoration
ctx.fillStyle='rgba(215,191,129,.3)';ctx.font='8px serif';
ctx.fillText('\u266A',cx+8+capeW*0.5,bodyTop+8);
ctx.fillText('\u266B',cx+12+capeW*0.3,bodyTop+16);

// ── Legs (bouncy cartoon legs with sneakers)
const ls=anim*6;
ctx.lineCap='round';
// Left leg
ctx.strokeStyle='#2A2A3A';ctx.lineWidth=5;
ctx.beginPath();ctx.moveTo(cx-5,bodyTop+12);ctx.lineTo(cx-6+ls,y+h-4+jOff);ctx.stroke();
// Right leg
ctx.beginPath();ctx.moveTo(cx+5,bodyTop+12);ctx.lineTo(cx+6-ls,y+h-4+jOff);ctx.stroke();
// Sneakers (gold!)
ctx.fillStyle='#D7BF81';
ctx.beginPath();ctx.ellipse(cx-6+ls,y+h-1+jOff,7,4,.15,0,Math.PI*2);ctx.fill();
ctx.beginPath();ctx.ellipse(cx+6-ls,y+h-1+jOff,7,4,-.15,0,Math.PI*2);ctx.fill();
// Sneaker detail stripe
ctx.strokeStyle='#FFF';ctx.lineWidth=1;
ctx.beginPath();ctx.moveTo(cx-10+ls,y+h-1+jOff);ctx.lineTo(cx-3+ls,y+h-2+jOff);ctx.stroke();
ctx.beginPath();ctx.moveTo(cx+10-ls,y+h-1+jOff);ctx.lineTo(cx+3-ls,y+h-2+jOff);ctx.stroke();

// ── Body (round, puffy jacket)
const bodyG=ctx.createLinearGradient(cx-9,bodyTop-4,cx+9,bodyTop+16);
bodyG.addColorStop(0,'#3A5A9A');bodyG.addColorStop(.5,'#2A4580');bodyG.addColorStop(1,'#1E3060');
ctx.fillStyle=bodyG;
ctx.beginPath();
ctx.moveTo(cx-9,bodyTop);ctx.quadraticCurveTo(cx-11,bodyTop+8,cx-8,bodyTop+16);
ctx.lineTo(cx+8,bodyTop+16);ctx.quadraticCurveTo(cx+11,bodyTop+8,cx+9,bodyTop);
ctx.closePath();ctx.fill();
// Jacket gold buttons
ctx.fillStyle='#D7BF81';
ctx.beginPath();ctx.arc(cx,bodyTop+4,2,0,Math.PI*2);ctx.fill();
ctx.beginPath();ctx.arc(cx,bodyTop+10,2,0,Math.PI*2);ctx.fill();
// Gold collar
ctx.strokeStyle='#D7BF81';ctx.lineWidth=2;
ctx.beginPath();ctx.moveTo(cx-7,bodyTop+1);ctx.quadraticCurveTo(cx,bodyTop-2,cx+7,bodyTop+1);ctx.stroke();

// ── Arms (animated, holding a tiny baton in right hand)
const as=anim*9;
ctx.strokeStyle='#3A5A9A';ctx.lineWidth=5;ctx.lineCap='round';
// Left arm
ctx.beginPath();ctx.moveTo(cx-9,bodyTop+3);ctx.lineTo(cx-15,bodyTop+14+as+jOff);ctx.stroke();
// Left hand
ctx.fillStyle='#F0C888';
ctx.beginPath();ctx.arc(cx-15,bodyTop+14+as+jOff,3.5,0,Math.PI*2);ctx.fill();
// Right arm
ctx.beginPath();ctx.strokeStyle='#3A5A9A';
ctx.moveTo(cx+9,bodyTop+3);ctx.lineTo(cx+15,bodyTop+12-as+jOff);ctx.stroke();
// Right hand
ctx.fillStyle='#F0C888';
ctx.beginPath();ctx.arc(cx+15,bodyTop+12-as+jOff,3.5,0,Math.PI*2);ctx.fill();
// Baton (conductor stick!)
ctx.strokeStyle='#F5E6C8';ctx.lineWidth=1.8;
ctx.shadowColor='#FFD700';ctx.shadowBlur=6;
ctx.beginPath();ctx.moveTo(cx+15,bodyTop+12-as+jOff);
ctx.lineTo(cx+26,bodyTop+2-as+jOff);ctx.stroke();
ctx.fillStyle='#FFD700';
ctx.beginPath();ctx.arc(cx+26,bodyTop+2-as+jOff,2,0,Math.PI*2);ctx.fill();
ctx.shadowBlur=0;

// ── Head (big, round, cartoon)
const headY=y+headR+bounce;
// Head circle
const hg=ctx.createRadialGradient(cx-2,headY-2,0,cx,headY,headR);
hg.addColorStop(0,'#FFD8AA');hg.addColorStop(.7,'#F0C080');hg.addColorStop(1,'#D4A060');
ctx.fillStyle=hg;
ctx.beginPath();ctx.arc(cx,headY,headR,0,Math.PI*2);ctx.fill();
// Cheek blush
ctx.fillStyle='rgba(255,120,120,.25)';
ctx.beginPath();ctx.ellipse(cx-headR*0.55,headY+headR*0.2,headR*0.25,headR*0.15,0,0,Math.PI*2);ctx.fill();
ctx.beginPath();ctx.ellipse(cx+headR*0.55,headY+headR*0.2,headR*0.25,headR*0.15,0,0,Math.PI*2);ctx.fill();

// ── Eyes (big, expressive cartoon eyes)
const blink=Math.random()>.988?0:1;
const eyeW=headR*0.28,eyeH=headR*0.35*blink;
const eyeY=headY-headR*0.05;
// White sclera
ctx.fillStyle='#FFF';
ctx.beginPath();ctx.ellipse(cx-headR*0.28,eyeY,eyeW+1,eyeH+1,0,0,Math.PI*2);ctx.fill();
ctx.beginPath();ctx.ellipse(cx+headR*0.28,eyeY,eyeW+1,eyeH+1,0,0,Math.PI*2);ctx.fill();
// Iris (big, dark)
ctx.fillStyle='#1A1A2E';
ctx.beginPath();ctx.ellipse(cx-headR*0.25,eyeY+1,eyeW*.65,eyeH*.7,0,0,Math.PI*2);ctx.fill();
ctx.beginPath();ctx.ellipse(cx+headR*0.25,eyeY+1,eyeW*.65,eyeH*.7,0,0,Math.PI*2);ctx.fill();
// Sparkle in eyes
ctx.fillStyle='rgba(255,255,255,.85)';
ctx.beginPath();ctx.arc(cx-headR*0.3,eyeY-1,2,0,Math.PI*2);ctx.fill();
ctx.beginPath();ctx.arc(cx+headR*0.2,eyeY-1,2,0,Math.PI*2);ctx.fill();
// Tiny lower sparkle
ctx.beginPath();ctx.arc(cx-headR*0.2,eyeY+2,1,0,Math.PI*2);ctx.fill();
ctx.beginPath();ctx.arc(cx+headR*0.3,eyeY+2,1,0,Math.PI*2);ctx.fill();

// ── Eyebrows (expressive)
ctx.strokeStyle='#5A3A1A';ctx.lineWidth=2;ctx.lineCap='round';
const ebLift=this.state==='jump'?-3:this.state==='fall'?2:0;
ctx.beginPath();ctx.moveTo(cx-headR*0.5,eyeY-eyeH-2+ebLift);
ctx.quadraticCurveTo(cx-headR*0.28,eyeY-eyeH-5+ebLift,cx-headR*0.08,eyeY-eyeH-1+ebLift);ctx.stroke();
ctx.beginPath();ctx.moveTo(cx+headR*0.08,eyeY-eyeH-1+ebLift);
ctx.quadraticCurveTo(cx+headR*0.28,eyeY-eyeH-5+ebLift,cx+headR*0.5,eyeY-eyeH-2+ebLift);ctx.stroke();

// ── Mouth (happy smile, wider when jumping)
const mouthW=this.state==='jump'?headR*0.4:this.state==='fall'?headR*0.15:headR*0.3;
const mouthY=headY+headR*0.4;
ctx.strokeStyle='#6B3A1A';ctx.lineWidth=1.8;
ctx.beginPath();ctx.arc(cx,mouthY-headR*0.1,mouthW,0.2,Math.PI-0.2);ctx.stroke();
// Open mouth when jumping/falling (excited!)
if(this.state==='jump'||this.state==='fall'){
ctx.fillStyle='#8B2020';
ctx.beginPath();ctx.ellipse(cx,mouthY+2,mouthW*0.6,headR*0.12,0,0,Math.PI*2);ctx.fill();}

// ── Beret (music-themed hat!)
ctx.fillStyle='#6B2FA0';
ctx.beginPath();
ctx.ellipse(cx+2,headY-headR*0.85,headR*0.75,headR*0.25,0.1,0,Math.PI*2);ctx.fill();
ctx.fillStyle='#7B3FB8';
ctx.beginPath();
ctx.arc(cx+1,headY-headR*0.75,headR*0.5,Math.PI,2*Math.PI);ctx.fill();
// Beret pompom
ctx.fillStyle='#D7BF81';
ctx.beginPath();ctx.arc(cx+headR*0.3,headY-headR*1.05,3,0,Math.PI*2);ctx.fill();
// Music note on beret
ctx.fillStyle='#D7BF81';ctx.font=`${Math.round(headR*0.45)}px serif`;ctx.textAlign='center';ctx.textBaseline='middle';
ctx.fillText('\u266A',cx-1,headY-headR*0.8);

// ── Hair peeking out from beret
ctx.fillStyle='#3A1A0A';
ctx.beginPath();
ctx.moveTo(cx-headR*0.7,headY-headR*0.4);
ctx.quadraticCurveTo(cx-headR*0.9,headY-headR*0.1,cx-headR*0.75,headY+headR*0.1);ctx.fill();
ctx.beginPath();
ctx.moveTo(cx+headR*0.65,headY-headR*0.45);
ctx.quadraticCurveTo(cx+headR*0.85,headY-headR*0.1,cx+headR*0.7,headY+headR*0.15);ctx.fill();}}


// ── Audio — Grand Piano via Tone.js Sampler + SFX ──
class Audio{
constructor(){this.ok=false;this.muted=false;this.piano=null;this.noiseOk=null;this.noiseBad=null;this._samplerReady=false;}
async init(){if(this.ok)return;try{
await Tone.start();
// Reverb + compressor chain for rich piano sound
const comp=new Tone.Compressor(-22,5).toDestination();
const rev=new Tone.Reverb({decay:2.2,wet:.22,preDelay:.01});
await rev.ready;rev.connect(comp);
// Grand piano sampler using Tone.js built-in Salamander samples
const baseUrl='https://tonejs.github.io/audio/salamander/';
this.piano=new Tone.Sampler({
urls:{A0:'A0.mp3',C1:'C1.mp3',A1:'A1.mp3',C2:'C2.mp3',A2:'A2.mp3',
C3:'C3.mp3',A3:'A3.mp3',C4:'C4.mp3',A4:'A4.mp3',C5:'C5.mp3',
A5:'A5.mp3',C6:'C6.mp3',A6:'A6.mp3',C7:'C7.mp3'},
baseUrl:baseUrl,
release:1.5,
volume:-4,
onload:()=>{this._samplerReady=true;}
});
this.piano.connect(rev);
// Fallback synth while samples load
this._fallbackPiano=new Tone.PolySynth(Tone.FMSynth,{
maxPolyphony:6,
options:{
harmonicity:2,modulationIndex:0.8,
oscillator:{type:'sine'},
envelope:{attack:.003,decay:.6,sustain:.04,release:.7},
modulation:{type:'triangle'},
modulationEnvelope:{attack:.002,decay:.3,sustain:0,release:.4},
volume:-22}});
this._fallbackPiano.connect(rev);
// Non-melodic SFX
this.noiseOk=new Tone.NoiseSynth({
noise:{type:'white'},
envelope:{attack:.001,decay:.05,sustain:0,release:.04},
volume:-32}).toDestination();
this.noiseBad=new Tone.NoiseSynth({
noise:{type:'pink'},
envelope:{attack:.001,decay:.10,sustain:0,release:.06},
volume:-28}).toDestination();
this.ok=true;}catch(e){console.warn('Audio init',e);}}

// Build a harmonically rich chord from a root note, following chord progression logic
_buildChord(noteName,octave){
const PC={C:0,D:2,E:4,F:5,G:7,A:9,B:11};
const NAMES=['C','C#','D','D#','E','F','F#','G','G#','A','A#','B'];
const root=PC[noteName.replace(/[#b]/g,'')]||0;
const acc=noteName.includes('#')?1:noteName.includes('b')?-1:0;
const rootPc=(root+acc+12)%12;
// Harmonic progression: choose chord quality based on root and progression history
const chordTypes=[
[0,4,7],[0,3,7],[0,4,7,11],[0,3,7,10], // maj, min, maj7, m7
[0,4,7,9],[0,5,7],[0,2,7],[0,4,7,14],   // 6, sus4, sus2, add9
[0,3,7,14],[0,4,7,11,14],[0,3,6]];       // madd9, maj9, dim
// Harmonic memory: prefer chords that follow circle-of-fifths progression
const lastRoot=this._lastChordRoot||0;
const fifthDist=Math.abs(rootPc-lastRoot);
const tension=fifthDist===5||fifthDist===7?0:fifthDist===0?2:1;
const idx=((rootPc+tension+(this._chordIdx||0))%chordTypes.length);
this._chordIdx=(this._chordIdx||0)+1;this._lastChordRoot=rootPc;
const intervals=chordTypes[idx];
// Spread voicing: occasionally invert chords for smoother voice leading
const inversion=this._chordIdx%3===0?1:0;
const notes=[];
for(let i=0;i<intervals.length;i++){
let iv=intervals[(i+inversion)%intervals.length];
if(i<inversion)iv+=12; // raise inverted notes by octave
const pc=(rootPc+iv)%12;
let o=octave+Math.floor((rootPc+iv)/12);
if(o>6)o=6;if(o<1)o=1;
notes.push({n:NAMES[pc]+o,vel:i===0?0.65:0.3-i*0.04,delay:i*0.015});}
return notes;}

// Play a rich piano chord with pedal sustain
play(n,o){if(!this.ok||this.muted)return;
try{
const oct=parseInt(o)||4;
const dur='2n'; // long sustain like pedal held
const chord=this._buildChord(n,oct);
const synth=this._samplerReady&&this.piano?this.piano:this._fallbackPiano;
if(!synth)return;
const t=Tone.now();
for(const cn of chord){
const vel=Math.min(0.85,Math.max(0.2,cn.vel-Math.abs(oct-4)*0.06));
synth.triggerAttackRelease(cn.n,dur,t+cn.delay,vel);}
}catch(e){console.warn('Audio play',e);}}

// SFX: non-melodic noise bursts — do not interfere with piano note identification
playOk(){if(!this.ok||this.muted)return;
try{const t=Tone.now();
this.noiseOk.triggerAttackRelease('32n',t);
this.noiseOk.triggerAttackRelease('32n',t+.08);}catch(e){}}

playBad(){if(!this.ok||this.muted)return;
try{this.noiseBad.triggerAttackRelease('16n');}catch(e){}}

playJmp(){if(!this.ok||this.muted)return;
try{this.noiseOk.triggerAttackRelease('64n');}catch(e){}}

playCoin(){if(!this.ok||this.muted)return;
try{const t=Tone.now();
this.noiseOk.triggerAttackRelease('32n',t);
this.noiseOk.triggerAttackRelease('32n',t+.055);}catch(e){}}

// playRest intentionally does nothing — rests are silence
playRest(){}

playVine(){if(!this.ok||this.muted)return;
try{this.noiseOk.triggerAttackRelease('16n');}catch(e){}}

playFly(){if(!this.ok||this.muted)return;
try{this.noiseOk.triggerAttackRelease('8n');}catch(e){}}

// Game over: gentle descending minor chord — melancholic, quiet
playEnd(){if(!this.ok||this.muted)return;try{
const end=new Tone.PolySynth(Tone.Synth,{
options:{oscillator:{type:'sine'},envelope:{attack:.08,decay:.4,sustain:.05,release:.8},volume:-26}});
const rev2=new Tone.Reverb({decay:1.2,wet:.3});
rev2.toDestination();end.connect(rev2);
const t=Tone.now();
end.triggerAttackRelease(['E4','G4','B4'],'8n',t,0.25);
end.triggerAttackRelease(['D4','F4','A4'],'4n',t+0.3,0.2);
setTimeout(()=>{try{end.dispose();rev2.dispose();}catch(e){}},3000);}catch(e){}}

toggle(){this.muted=!this.muted;return this.muted;}}

// ── BGMusic — three-voice generative music, realm-adaptive, speeds up with distance ──
class BGMusic{
constructor(){this.running=false;this.mel=null;this.pad=null;this.bass=null;
this.melIdx=0;this.padIdx=0;this.bassIdx=0;
this.melTO=null;this.padTO=null;this.bassTO=null;
this.muted=false;this.realm=0;this._rev=null;
this._melInterval=420;
// Arpeggio voice: flowing, with chromatic tension and realm-specific register
this.melSeqs={
0:['C4','Eb4','G4','Bb4','C5','Bb4','Ab4','G4','F4','Ab4','C4','D4','F4','Ab4','Bb4','G4','Eb4','D4','C4','Eb4','F4','G4','Bb4','Ab4'],
1:['C4','E4','G4','B4','D5','G5','B5','D6','E6','D6','B5','G5','E5','C5','A4','F4','D4','E4','G4','B4','C5','E5','G5','A5','B5','G5','E5','C5'],
2:['C4','Bb3','Ab3','G3','F3','Eb3','D3','C3','Bb2','A2','G2','F2','E2','G2','Bb2','C3','Eb3','F3','G3','Ab3','Bb3','C4','D4','Eb4'],
3:['E4','G4','B4','D5','G5','B5','E6','G6','B6','G6','E6','B5','G5','D5','B4','G4','E4','A4','C5','E5','A5','C6','A5','E5'],
4:['C4','A3','F3','D3','Bb2','G2','Eb2','C2','A1','C2','Eb2','G2','Bb2','D3','F3','A3','C4','Eb4','C4','A3','F3','D3','Bb2']};
// Pad voice: longer sustained harmonics — creates emotional atmosphere
this.padSeqs={
0:['Eb3','G3','Bb3','F3','Ab3','C4','D3','Bb3','Eb3','Ab3'],
1:['E3','G3','B3','D4','G4','B4','E4','G4','D4','B3'],
2:['Eb3','C3','Ab2','F2','Eb2','Ab2','C3','Eb3','G3','Eb3'],
3:['E4','B4','G5','D5','B5','G5','E5','B4','G4','E4'],
4:['C3','Ab2','Eb2','C2','Ab1','C2','Eb2','Ab2','C3','Eb3']};
// Bass voice: deep pedal tones that shift with the harmony
this.bassSeqs={
0:['C2','C2','F2','F2','Ab2','Ab2','Bb2','Bb2','G2','G2','Eb2','Eb2'],
1:['C3','C3','G3','G3','E3','E3','D3','D3','A3','A3','E3','E3'],
2:['C2','C2','Ab1','Ab1','Eb2','Eb2','Bb1','Bb1','F2','F2','C2','C2'],
3:['C3','E3','G3','E3','B3','G3','D3','A3','E3','C3'],
4:['C1','C1','Eb1','Eb1','Ab1','Ab1','Bb1','Bb1','G1','G1']};}

async start(realm){
this.stop();this.realm=realm||0;
if(this.muted)return;
try{if(typeof Tone==='undefined')return;await Tone.start();
// Rich reverb + subtle delay for spacious atmospheric sound
this._rev=new Tone.Reverb({decay:6,wet:.55,preDelay:.04});await this._rev.ready;
const delay=new Tone.FeedbackDelay({delayTime:'8n.',feedback:.15,wet:.12});
delay.connect(this._rev);this._rev.toDestination();
const comp=new Tone.Compressor(-20,4);comp.connect(delay);
// Melody voice: triangle wave for sparkly quality
this.mel=new Tone.Synth({oscillator:{type:'triangle'},
envelope:{attack:.15,decay:.8,sustain:.2,release:2.5},volume:-38}).connect(comp);
// Pad voice: soft sine with slow attack for ambient wash
this.pad=new Tone.Synth({oscillator:{type:'sine'},
envelope:{attack:1.2,decay:2.5,sustain:.3,release:5},volume:-48}).connect(this._rev);
// Bass voice: deep sine, gentle throb
this.bass=new Tone.Synth({oscillator:{type:'sine'},
envelope:{attack:.4,decay:1.5,sustain:.4,release:3},volume:-44}).connect(this._rev);
this.running=true;this.melIdx=0;this.padIdx=0;this.bassIdx=0;
this._melTick();this._padTick();this._bassTick();}catch(e){}}

_melTick(){if(!this.running||this.muted)return;
const seq=this.melSeqs[this.realm]||this.melSeqs[0];
const note=seq[this.melIdx%seq.length];
// Occasional silence for breathing room (rest)
const isRest=this.melIdx%8===7||this.melIdx%13===12;
// Vary note duration for musicality
const durations=['8n','8n','16n','8n','4n','8n','8n','16n'];
const dur=durations[this.melIdx%durations.length];
if(!isRest){try{if(this.mel)this.mel.triggerAttackRelease(note,dur);}catch(e){}}
this.melIdx++;
// Dynamic rhythm with swing and syncopation
const patterns=[1,.85,.75,1,1.15,.9,.7,1.1,1,.8,1.2,.95,.85,1.3,.75,1];
const rhythmMult=patterns[this.melIdx%patterns.length];
this.melTO=setTimeout(()=>this._melTick(),Math.round(this._melInterval*rhythmMult));}

_padTick(){if(!this.running||this.muted)return;
const seq=this.padSeqs[this.realm]||this.padSeqs[0];
const note=seq[this.padIdx%seq.length];
try{if(this.pad)this.pad.triggerAttackRelease(note,'2n');}catch(e){}
this.padIdx++;
// Pad: slow, steady, with occasional longer hold
const hold=this.padIdx%5===0?3.5:2.2;
this.padTO=setTimeout(()=>this._padTick(),Math.round(this._melInterval*hold));}

_bassTick(){if(!this.running||this.muted)return;
const seq=this.bassSeqs[this.realm]||this.bassSeqs[0];
const note=seq[this.bassIdx%seq.length];
try{if(this.bass)this.bass.triggerAttackRelease(note,'2n');}catch(e){}
this.bassIdx++;
// Bass: slow heartbeat rhythm
this.bassTO=setTimeout(()=>this._bassTick(),Math.round(this._melInterval*3.2));}

setTempo(dist,combo){
// Music accelerates smoothly as player progresses — max speed at 600m
const t=Math.min(1,dist/600);
const comboBoost=combo?Math.min(0.15,combo*0.02):0;
this._melInterval=Math.round((420-t*220)*(1-comboBoost));
// Dynamic volume: louder melody at high combo
if(this.mel){try{this.mel.volume.value=-38+Math.min(6,combo||0)*1;}catch(e){}}}

stop(){this.running=false;
if(this.melTO)clearTimeout(this.melTO);if(this.padTO)clearTimeout(this.padTO);
if(this.bassTO)clearTimeout(this.bassTO);
try{if(this.mel)this.mel.dispose();}catch(e){}
try{if(this.pad)this.pad.dispose();}catch(e){}
try{if(this.bass)this.bass.dispose();}catch(e){}
try{if(this._rev)this._rev.dispose();}catch(e){}
this.mel=null;this.pad=null;this.bass=null;this._rev=null;}

toggle(){this.muted=!this.muted;if(this.muted)this.stop();else this.start(this.realm);}
setRealm(r){if(r!==this.realm&&this.running){this.stop();setTimeout(()=>this.start(r),100);}}}

// ── VineRescue ──
class VineRescue{
constructor(){this.reset();}
reset(){this.state='off';this.eligible=[];this.targetNote=null;
this.startPos={x:0,y:0};this.animPos={x:0,y:0};this.swingT=0;this.totalT=680;}
check(player,noteBlocks){
if(this.state==='swinging'||this.state==='landing')return;
// Trigger vine rescue earlier (vy>2.5) for easier rescue, wider search radius
if(player.vy>2.5&&!player.onG&&player.flyTime<=0&&!player.gliding){
const py=player.y+player.h/2,px=player.x+player.w/2;
this.eligible=noteBlocks.filter(nb=>{
if(nb.state==='crumbling'||nb.state==='wrong'||!nb.isSolid())return false;
// Notes ABOVE the player — wider range for easier rescue
const dy=py-nb.y,dx=Math.abs(nb.x-px);
return dy>20&&dy<600&&dx<450;});
// Sort by distance — closest first
this.eligible.sort((a,b)=>{
const da=Math.hypot(a.x-px,a.y-py),db=Math.hypot(b.x-px,b.y-py);return da-db;});
this.state=this.eligible.length>0?'available':'off';
}else{this.state='off';this.eligible=[];}}
tryAnswer(noteName,player){
if(this.state!=='available'||this.eligible.length===0)return false;
const PC={C:0,D:2,E:4,F:5,G:7,A:9,B:11};
const toPc=s=>{const base=s.replace(/[#b]/g,'');const acc=s.includes('#')?1:s.includes('b')?-1:0;return((PC[base]||0)+acc+12)%12;};
const inPc=toPc(noteName);
// First try exact pitch-class match
let match=this.eligible.find(nb=>{
const nbPc=toPc(nb.note+(nb.accidental==='sharp'?'#':nb.accidental==='flat'?'b':''));
return nbPc===inPc;});
// If no exact match, grab the CLOSEST eligible note (any key rescues)
if(!match)match=this.eligible[0];
if(match){this.targetNote=match;
this.startPos={x:player.x+player.w/2,y:player.y+player.h/2};
this.swingT=0;this.state='swinging';return true;}
return false;}
update(dt,player){
if(this.state!=='swinging')return;
this.swingT+=dt;const raw=Math.min(1,this.swingT/this.totalT);
const t=raw<.5?2*raw*raw:1-Math.pow(-2*raw+2,2)/2;
const tx=this.targetNote.x,ty=this.targetNote.y;
const midX=(this.startPos.x+tx)/2,ctlY=Math.min(this.startPos.y,ty)-90;
const px=(1-t)*(1-t)*this.startPos.x+2*(1-t)*t*midX+t*t*tx;
const py=(1-t)*(1-t)*this.startPos.y+2*(1-t)*t*ctlY+t*t*ty;
this.animPos={x:px,y:py};
player.x=px-player.w/2;player.y=py-player.h/2;player.vx=0;player.vy=0;
if(raw>=1){
// Land player ON TOP of the note hitbox (not at center)
const noteTopY=this.targetNote.y-CFG.NOTE_HITBOX_H/2;
player.x=tx-player.w/2;player.y=noteTopY-player.h;
player.vy=0;player.vx=0;player.onG=true;player.dj=true;player.tj=true;
this.state='landing';setTimeout(()=>{this.state='off';},300);}}
draw(ctx,cx,cy,player,showNoteNames){
if(this.state==='off'||this.state==='landing')return;const t2=Date.now()*.003;
if(this.state==='available'){ctx.save();
this.eligible.forEach(nb=>{const sx=nb.x-cx,sy=nb.y-cy;
const pulse=Math.sin(t2*2+nb.x*.01)*.5+.5;
const alpha=.4+pulse*.25;
// Glowing ring
ctx.shadowColor='rgba(255,215,0,.7)';ctx.shadowBlur=18+pulse*12;
ctx.beginPath();ctx.arc(sx,sy,28+pulse*4,0,Math.PI*2);
ctx.strokeStyle=`rgba(255,215,0,${alpha})`;ctx.lineWidth=2.5;ctx.stroke();
ctx.shadowBlur=0;
// Inner fill pulse
ctx.fillStyle=`rgba(255,215,0,${.06+pulse*.08})`;
ctx.beginPath();ctx.arc(sx,sy,24,0,Math.PI*2);ctx.fill();
if(showNoteNames){
ctx.font='bold 13px Montserrat,sans-serif';ctx.textAlign='center';ctx.textBaseline='middle';
ctx.fillStyle=`rgba(255,215,0,${alpha+.2})`;ctx.shadowColor='#FFD700';ctx.shadowBlur=10;
ctx.fillText(Mus.noteLabel(nb.note,nb.accidental,CFG.latin),sx,sy+CFG.NOTE_HEAD_RX+18);ctx.shadowBlur=0;}});ctx.restore();}
if(this.state==='swinging'){
const tx=this.targetNote.x-cx,ty=this.targetNote.y-cy-CFG.NOTE_HITBOX_H/2;
const midX=(this.startPos.x-cx+tx)/2,ctlY=Math.min(this.startPos.y-cy,ty)-110;
const progress=Math.min(1,this.swingT/this.totalT);
const steps=30,sx0=this.startPos.x-cx,sy0=this.startPos.y-cy;
ctx.save();ctx.lineCap='round';
// Vine outer glow
ctx.strokeStyle='rgba(255,215,0,.18)';ctx.lineWidth=10;ctx.shadowBlur=0;
ctx.beginPath();ctx.moveTo(sx0,sy0);
for(let i=1;i<=Math.ceil(steps*progress);i++){const ti=i/steps;
const bx=(1-ti)*(1-ti)*sx0+2*(1-ti)*ti*midX+ti*ti*tx;
const by=(1-ti)*(1-ti)*sy0+2*(1-ti)*ti*ctlY+ti*ti*ty;ctx.lineTo(bx,by);}
ctx.stroke();
// Vine core
ctx.shadowColor=CFG.COLORS.VINE_GLOW;ctx.shadowBlur=16;
ctx.strokeStyle=CFG.COLORS.VINE;ctx.lineWidth=4;
ctx.beginPath();ctx.moveTo(sx0,sy0);
for(let i=1;i<=Math.ceil(steps*progress);i++){const ti=i/steps;
const bx=(1-ti)*(1-ti)*sx0+2*(1-ti)*ti*midX+ti*ti*tx;
const by=(1-ti)*(1-ti)*sy0+2*(1-ti)*ti*ctlY+ti*ti*ty;ctx.lineTo(bx,by);}
ctx.stroke();
// Animated nodes along vine
for(let i=1;i<5;i++){const ti=i/5*progress;
const bx=(1-ti)*(1-ti)*sx0+2*(1-ti)*ti*midX+ti*ti*tx;
const by=(1-ti)*(1-ti)*sy0+2*(1-ti)*ti*ctlY+ti*ti*ty;
ctx.fillStyle='rgba(255,240,100,.8)';ctx.beginPath();ctx.arc(bx,by,3,0,Math.PI*2);ctx.fill();}
ctx.shadowBlur=0;ctx.restore();}}}

// ── MIDI ──
class MIDI{
constructor(){this.on=false;this.dev='';this.cb=null;}
async init(){if(!navigator.requestMIDIAccess)return;try{
const a=await navigator.requestMIDIAccess({sysex:false});
a.inputs.forEach(i=>{i.onmidimessage=m=>this._msg(m);this.on=true;this.dev=i.name||'MIDI';});
a.onstatechange=e=>{if(e.port.type==='input'&&e.port.state==='connected'){
e.port.onmidimessage=m=>this._msg(m);this.on=true;this.dev=e.port.name||'MIDI';}};}catch(e){}}
_msg(m){const[s,n,v]=m.data;if((s&0xF0)===0x90&&v>0){
const nn=['C','C#','D','D#','E','F','F#','G','G#','A','A#','B'];
const note=nn[n%12],oct=Math.floor(n/12)-1;
if(this.cb)this.cb({note:note.replace('#',''),octave:oct,full:note});}}}


// ── Chord Barrier — giant electric wall with chord notes displayed ON the staff ──
// Player must play each note (keyboard/piano/MIDI) one by one to dissolve the barrier.
// No modal — all happens in-game.
class ChordBarrier{
constructor(x,realm){
this.x=x;this.w=80;this.pulse=Math.random()*Math.PI*2;
this.dissolveT=0;this.state='active'; // active | dissolving | gone
// Barrier extends massively above and below — impossible to bypass even flying
this.topY=-120*CFG.STAFF_SP;
this.botY=120*CFG.STAFF_SP;
// Generate 2-4 chord notes within the realm's register
const allNotes=['C','C#','D','D#','E','F','F#','G','G#','A','A#','B'];
const roots=['C','D','E','F','G','A','B'];
const root=roots[Math.floor(Math.random()*roots.length)];
const chordShapes=[[0,4,7],[0,3,7],[0,4,7,11],[0,3,7,10]]; // maj, min, maj7, m7
const shape=chordShapes[Math.floor(Math.random()*chordShapes.length)];
const rootPc=allNotes.indexOf(root);
// Pick a base octave appropriate for the realm
const baseOct=[4,4,3,5,2][realm]||4;
this.chordNotes=shape.map(interval=>{
const pc=(rootPc+interval)%12;
const noteName=allNotes[pc];
const oct=baseOct+(pc<rootPc?1:0);
return{note:noteName.replace('#',''),accidental:noteName.includes('#')?'sharp':null,
octave:oct,y:Mus.noteToY(noteName.replace('#',''),oct),validated:false,pc:pc};});
// Track which notes the player has validated
this.validatedCount=0;}
update(dt){
this.pulse+=dt*.004;
if(this.state==='dissolving'){this.dissolveT+=dt;if(this.dissolveT>1000)this.state='gone';}}
rect(){return{x:this.x-this.w/2,y:this.topY,w:this.w,h:this.botY-this.topY};}
isBlocking(){return this.state==='active';}
dissolve(){this.state='dissolving';this.dissolveT=0;this._lockTime=null;}
// Try to validate a note input (returns true if it matched an unvalidated chord note)
tryNote(noteName){
const PC={C:0,D:2,E:4,F:5,G:7,A:9,B:11};
const toPc=s=>{const base=s.replace(/[#b]/g,'');const acc=s.includes('#')?1:s.includes('b')?-1:0;return((PC[base]||0)+acc+12)%12;};
const inputPc=toPc(noteName);
for(const cn of this.chordNotes){
if(!cn.validated&&cn.pc===inputPc){cn.validated=true;this.validatedCount++;
if(this.validatedCount>=this.chordNotes.length)this.dissolve();
return true;}}
return false;}
allValidated(){return this.validatedCount>=this.chordNotes.length;}
draw(ctx,cx,cy){
if(this.state==='gone')return;
const sx=this.x-cx;
if(sx<-250||sx>ctx.canvas.width*2.5+250)return;
const topSy=this.topY-cy,botSy=this.botY-cy;
const p=Math.sin(this.pulse)*.5+.5;
ctx.save();
if(this.state==='dissolving'){ctx.globalAlpha=Math.max(0,1-this.dissolveT/1000);}
// Electric barrier body — tall translucent blue wall
ctx.shadowColor='rgba(60,100,255,.7)';ctx.shadowBlur=20+p*20;
const grad=ctx.createLinearGradient(sx-this.w/2,0,sx+this.w/2,0);
grad.addColorStop(0,'rgba(20,40,160,.02)');
grad.addColorStop(.2,'rgba(50,80,255,.18)');
grad.addColorStop(.5,'rgba(80,130,255,.28)');
grad.addColorStop(.8,'rgba(50,80,255,.18)');
grad.addColorStop(1,'rgba(20,40,160,.02)');
ctx.fillStyle=grad;
ctx.fillRect(sx-this.w/2,topSy,this.w,botSy-topSy);
// Animated lightning arcs across the barrier
ctx.strokeStyle=`rgba(140,180,255,${.35+p*.25})`;ctx.lineWidth=1.5;
for(let i=0;i<12;i++){
const ay=topSy+((botSy-topSy)/12)*i;
const arcW=15+Math.sin(this.pulse*2.5+i*1.7)*12;
ctx.beginPath();ctx.moveTo(sx-arcW,ay);
ctx.quadraticCurveTo(sx,ay+14*Math.sin(this.pulse*3+i*2.3),sx+arcW,ay);ctx.stroke();}
// Side lightning edges
ctx.strokeStyle=`rgba(160,200,255,${.5+p*.3})`;ctx.lineWidth=2.5;
for(let side=-1;side<=1;side+=2){
ctx.beginPath();ctx.moveTo(sx+side*(this.w/2),topSy);
for(let y=topSy;y<botSy;y+=25){
const off=Math.sin(y*.06+this.pulse*2.5+side)*14;
ctx.lineTo(sx+side*(this.w/2)+off,y);}ctx.stroke();}
// Pre-compute X offsets for adjacent chord notes (seconds)
const csorted=[...this.chordNotes].sort((a,b)=>{
const spa=Mus.staffPosFromY(a.y),spb=Mus.staffPosFromY(b.y);return spa-spb;});
const cnOffsets={};
for(let ci=0;ci<csorted.length;ci++){
const ck=this.chordNotes.indexOf(csorted[ci]);cnOffsets[ck]=0;
if(ci>0){const spCur=Mus.staffPosFromY(csorted[ci].y),spPrev=Mus.staffPosFromY(csorted[ci-1].y);
if(Math.abs(spCur-spPrev)===1){const pk=this.chordNotes.indexOf(csorted[ci-1]);
cnOffsets[ck]=cnOffsets[pk]===0?(CFG.NOTE_HEAD_RX*1.8):0;}}}
// Draw chord notes ON the staff inside the barrier
for(let ci2=0;ci2<this.chordNotes.length;ci2++){
const cn=this.chordNotes[ci2];
const ny=cn.y-cy;
const cnox=cnOffsets[ci2]||0;
if(ny<topSy-30||ny>botSy+30)continue;
// Blink unvalidated notes after 6s of being locked
const shouldBlink=!cn.validated&&this._lockTime&&(Date.now()-this._lockTime)>6000;
const blinkOn=shouldBlink?Math.sin(Date.now()*.008)>0:true;
const col=cn.validated?'rgba(80,200,120,.9)':(shouldBlink&&blinkOn?'rgba(255,200,60,.95)':'rgba(180,220,255,.9)');
const glowCol=cn.validated?'rgba(80,200,120,.6)':(shouldBlink&&blinkOn?'rgba(255,180,40,.8)':'rgba(140,180,255,.6)');
// Note head
ctx.save();ctx.shadowColor=glowCol;ctx.shadowBlur=cn.validated?18:(shouldBlink&&blinkOn?22:12);
ctx.translate(sx+cnox,ny);ctx.rotate(-0.2);
ctx.beginPath();ctx.ellipse(0,0,CFG.NOTE_HEAD_RX*.9,CFG.NOTE_HEAD_RY*.9,0,0,Math.PI*2);
ctx.fillStyle=col;ctx.fill();
ctx.strokeStyle='rgba(0,0,0,.4)';ctx.lineWidth=1.5;ctx.stroke();
ctx.rotate(0.2);ctx.translate(-sx-cnox,-ny);
// Validated checkmark
if(cn.validated){ctx.font='bold 14px sans-serif';ctx.textAlign='center';ctx.textBaseline='middle';
ctx.fillStyle='#50C878';ctx.fillText('\u2713',sx+cnox,ny);}
// Note name label on the SIDE to avoid overlap in stacked chords
ctx.font='bold 11px Montserrat,sans-serif';ctx.textBaseline='middle';
ctx.fillStyle=cn.validated?'rgba(80,200,120,.8)':'rgba(220,240,255,.8)';
const label=Mus.noteLabel(cn.note,cn.accidental,CFG.latin);
const cSide=(ci2%2===0)?1:-1;
ctx.textAlign=cSide>0?'left':'right';
ctx.fillText(label,sx+cnox+cSide*(CFG.NOTE_HEAD_RX+10),ny);
// Ledger lines for this note
const sp=Mus.staffPosFromY(cn.y);
NoteRenderer.drawLedgerLines(ctx,sx,ny,sp,col,cn.note);
ctx.shadowBlur=0;ctx.restore();}
// "PLAY THE NOTES" indicator at barrier center
if(this.state==='active'){
const midY=(Math.max(topSy,0)+Math.min(botSy,ctx.canvas.height))/2;
const hintUrgent=this._lockTime&&(Date.now()-this._lockTime)>6000;
const hintBlink=hintUrgent?Math.sin(Date.now()*.006)>0:false;
ctx.font=hintUrgent?'bold 16px Montserrat,sans-serif':'bold 14px Montserrat,sans-serif';
ctx.textAlign='center';ctx.textBaseline='middle';
if(hintUrgent){
const hA=hintBlink?.95:.4;
ctx.fillStyle=`rgba(255,200,60,${hA})`;
ctx.shadowColor='rgba(255,180,40,.9)';ctx.shadowBlur=16+Math.sin(Date.now()*.01)*6;
}else{
ctx.fillStyle=`rgba(180,220,255,${.6+p*.3})`;
ctx.shadowColor='rgba(100,160,255,.8)';ctx.shadowBlur=10;}
ctx.fillText('\u26A1 PLAY '+this.validatedCount+'/'+this.chordNotes.length,sx,midY-20);
ctx.font='500 10px Montserrat,sans-serif';
ctx.fillStyle=hintUrgent?`rgba(255,220,100,${hintBlink?.7:.3})`:'rgba(200,230,255,.5)';
ctx.fillText(hintUrgent?'Play the notes!':'Use keys or piano',sx,midY-4);
ctx.shadowBlur=0;}
ctx.restore();}}

// ── ChordGroup — vertical chord (2-4 stemless notes) that player must identify all notes ──
class ChordGroup{
constructor(x,nd,realm,showLabel){
this.x=x;this.showLabel=showLabel;this.state='idle';this.pulse=Math.random()*Math.PI*2;
// Build a real chord rooted on the given note
const CHORDS=[
{name:'maj',intervals:[0,4,7]},{name:'min',intervals:[0,3,7]},
{name:'maj7',intervals:[0,4,7,11]},{name:'min7',intervals:[0,3,7,10]},
{name:'sus4',intervals:[0,5,7]},{name:'dim',intervals:[0,3,6]}];
const allNotes=['C','C#','D','D#','E','F','F#','G','G#','A','A#','B'];
const rootPc=allNotes.indexOf(nd.note+(nd.accidental||''));
const root=rootPc>=0?rootPc:allNotes.indexOf(nd.note);
const chord=CHORDS[Math.floor(Math.random()*CHORDS.length)];
this.chordName=nd.note+chord.name;
this.notes=chord.intervals.map(iv=>{
const pc=(root+iv)%12;const n=allNotes[pc];
const oct=nd.octave+(root+iv>=12?1:0);
const base=n.replace('#','');const acc=n.includes('#')?'sharp':null;
return{note:base,accidental:acc,octave:oct,
y:Mus.noteToY(base,oct),staffPos:Mus.staffPosFromY(Mus.noteToY(base,oct)),
validated:false,pc:pc};});
this.validatedCount=0;this.crumT=0;
// Hitbox covers the full vertical span of the chord
const ys=this.notes.map(n=>n.y);
this.minY=Math.min(...ys);this.maxY=Math.max(...ys);
this.w=CFG.NOTE_HITBOX_W;this.h=Math.max(CFG.NOTE_HITBOX_H,(this.maxY-this.minY)+CFG.NOTE_HITBOX_H);}
update(dt){this.pulse+=dt*.003;if(this.state==='crumbling')this.crumT+=dt;}
isSolid(){return this.state!=='crumbling'||this.crumT<600;}
rect(){return{x:this.x-this.w/2,y:this.minY-CFG.NOTE_HITBOX_H/2,w:this.w,h:this.h};}
// Try validating a note input
tryNote(noteName){
const PC={C:0,D:2,E:4,F:5,G:7,A:9,B:11};
const toPc=s=>{const base=s.replace(/[#b]/g,'');const acc=s.includes('#')?1:s.includes('b')?-1:0;return((PC[base]||0)+acc+12)%12;};
const inputPc=toPc(noteName);
for(const cn of this.notes){
if(!cn.validated&&cn.pc===inputPc){cn.validated=true;this.validatedCount++;
if(this.validatedCount>=this.notes.length){this.state='correct';}
return true;}}
return false;}
allValidated(){return this.validatedCount>=this.notes.length;}
draw(ctx,cx,cy){
const sx=this.x-cx;
if(sx<-200||sx>ctx.canvas.width*2.5+200)return;
ctx.save();
if(this.state==='crumbling'){const sh=Math.sin(this.crumT*.08)*5;
ctx.translate(sh,0);ctx.globalAlpha=Math.max(0,1-this.crumT/800);}
const p=Math.sin(this.pulse)*.5+.5;
// Pre-compute X offsets for adjacent notes (seconds: staffPos difference of 1)
const sorted=[...this.notes].sort((a,b)=>a.staffPos-b.staffPos);
const noteOffsets={};
for(let i=0;i<sorted.length;i++){
const key=this.notes.indexOf(sorted[i]);
noteOffsets[key]=0;
if(i>0&&Math.abs(sorted[i].staffPos-sorted[i-1].staffPos)===1){
const prevKey=this.notes.indexOf(sorted[i-1]);
noteOffsets[key]=noteOffsets[prevKey]===0?(CFG.NOTE_HEAD_RX*1.8):0;}}
// Draw each note head (stemless) and ledger lines
for(let ni=0;ni<this.notes.length;ni++){
const cn=this.notes[ni];
const ny=cn.y-cy;
if(ny<-60||ny>ctx.canvas.height+60)continue;
const col=cn.validated?'#50C878':(this.state==='challenged'?'#9B59F5':CFG.COLORS.GOLD);
const nox=noteOffsets[ni]||0; // X offset for adjacent seconds
// Stemless note head
NoteRenderer.drawNote(ctx,sx+nox,ny,cn.staffPos,{color:col,glow:true,pulse:p,noStem:true});
NoteRenderer.drawLedgerLines(ctx,sx+nox,ny,cn.staffPos,col,cn.note);
if(cn.accidental)NoteRenderer.drawAccidental(ctx,sx+nox,ny,cn.accidental,col);
// Validated checkmark
if(cn.validated){ctx.font='bold 12px sans-serif';ctx.textAlign='center';ctx.textBaseline='middle';
ctx.fillStyle='#50C878';ctx.fillText('\u2713',sx+nox,ny);}
// Note name label on the SIDE (not below) to avoid overlap in stacked chords
if(this.showLabel&&!cn.validated){
ctx.font='bold 10px Montserrat,sans-serif';ctx.textBaseline='middle';
ctx.fillStyle='rgba(180,220,255,.7)';
// Alternate left/right for each note in chord to avoid overlap
const labelSide=(ni%2===0)?1:-1; // even=right, odd=left
ctx.textAlign=labelSide>0?'left':'right';
const labelX=sx+nox+labelSide*(CFG.NOTE_HEAD_RX+8);
ctx.fillText(Mus.noteLabel(cn.note,cn.accidental,CFG.latin),labelX,ny);}}
ctx.restore();}}

// ── CClefEnemy — C clef symbol (𝄡) traverses BOTH staves with random movement — 2 lives damage ──
class CClefEnemy{
constructor(x,y){
this.x=x;this.y=y;
this.w=34;this.h=CFG.PLAYER_H;
this.alive=true;
this.damage=2; // costs 2 lives!
// Random horizontal movement (left/right)
this.vx=-(1+Math.random()*2)*(Math.random()<0.3?-1:1);
// Random vertical oscillation across both staves
this.lateralAmp=80+Math.random()*120; // much bigger range for both staves
this.lateralSpeed=0.001+Math.random()*0.003;
this.phase=Math.random()*Math.PI*2;
this.baseY=y;
// Random direction change timer
this.dirTimer=2000+Math.random()*3000;
this.pulse=Math.random()*Math.PI*2;}
update(dt){if(!this.alive)return;
this.x+=this.vx;
this.phase+=this.lateralSpeed*dt;
this.y=this.baseY+Math.sin(this.phase)*this.lateralAmp;
this.pulse+=dt*.004;
// Randomly change horizontal direction
this.dirTimer-=dt;
if(this.dirTimer<=0){
this.vx=-this.vx*(0.8+Math.random()*0.4);
this.dirTimer=2000+Math.random()*3000;}}
rect(){return{x:this.x-this.w/2,y:this.y-this.h/2,w:this.w,h:this.h};}
draw(ctx,cx,cy){if(!this.alive)return;
const sx=this.x-cx,sy=this.y-cy;
if(sx<-120||sx>ctx.canvas.width*2.5+120)return;
const p=Math.sin(this.pulse)*.5+.5;
ctx.save();
// Danger aura
ctx.shadowColor='rgba(200,50,50,.7)';ctx.shadowBlur=12+p*10;
ctx.fillStyle=`rgba(200,50,50,${.08+p*.06})`;
ctx.beginPath();ctx.arc(sx,sy,this.h/2+4,0,Math.PI*2);ctx.fill();
// Draw the C clef symbol
const fontSize=Math.max(28,Math.round(this.h*1.2));
ctx.font=`${fontSize}px "Times New Roman",Georgia,serif`;
ctx.textAlign='center';ctx.textBaseline='middle';
// Red-tinted clef with glow
ctx.fillStyle=`rgba(220,80,60,${.85+p*.15})`;
ctx.shadowColor='rgba(255,60,30,.8)';ctx.shadowBlur=15+p*10;
ctx.fillText('\u{1D121}',sx,sy);
// Second pass for inner detail
ctx.fillStyle=`rgba(255,120,90,${.3+p*.2})`;
ctx.shadowBlur=0;
ctx.fillText('\u{1D121}',sx,sy);
// Danger sparks
ctx.strokeStyle=`rgba(255,80,40,${.4+p*.3})`;ctx.lineWidth=1.5;
for(let i=0;i<4;i++){
const a=i/4*Math.PI*2+this.pulse*.8;
const ir=this.h/2-2,or=this.h/2+6+p*4;
ctx.beginPath();ctx.moveTo(sx+Math.cos(a)*ir,sy+Math.sin(a)*ir);
ctx.lineTo(sx+Math.cos(a)*or,sy+Math.sin(a)*or);ctx.stroke();}
ctx.shadowBlur=0;ctx.restore();}}

// ── GoldenClefReward — rare golden treble/bass/alto clef placed very high on moving bars ──
class GoldenClefReward{
constructor(x,realm){
this.x=x;this.collected=false;
// Place extremely high (treble range) or extremely low (bass range)
const isHigh=realm<=1||realm===3;
const basePos=isHigh?(-45-Math.random()*25):(45+Math.random()*25);
this.baseY=basePos*CFG.STAFF_SP+(basePos>0?CFG.STAFF_GAP:0);
// Moving bar that carries the clef
this.barW=70;this.barH=10;
this.barAmplitude=80+Math.random()*60;
this.barSpeed=0.001+Math.random()*0.001;
this.barPhase=Math.random()*Math.PI*2;
// Clef type: treble, bass, or alto
const types=['\u{1D11E}','\u{1D122}','\u{1D121}'];
const names=['Treble','Bass','Alto'];
const idx=Math.floor(Math.random()*3);
this.glyph=types[idx];this.name=names[idx];
this.pulse=Math.random()*Math.PI*2;
this.sparkles=[];
for(let i=0;i<8;i++)this.sparkles.push({a:Math.random()*Math.PI*2,r:12+Math.random()*16,s:0.5+Math.random()*1.5,p:Math.random()*Math.PI*2});}
update(dt){if(this.collected)return;this.pulse+=dt*0.004;this.barPhase+=this.barSpeed*dt;}
getY(){return this.baseY+Math.sin(this.barPhase)*this.barAmplitude;}
rect(){const y=this.getY();return{x:this.x-this.barW/2,y:y-20,w:this.barW,h:30};}
draw(ctx,cx,cy){if(this.collected)return;
const sx=this.x-cx,sy=this.getY()-cy;
if(sx<-100||sx>ctx.canvas.width*2.5+100||sy<-200||sy>ctx.canvas.height*2.5+200)return;
const p=Math.sin(this.pulse)*.5+.5;const t=Date.now()*.002;
ctx.save();
// Moving bar (golden, glowing)
ctx.shadowColor='rgba(255,215,0,.6)';ctx.shadowBlur=12+p*8;
ctx.fillStyle=`rgba(215,191,129,${.6+p*.2})`;
rrect(ctx,sx-this.barW/2,sy+5,this.barW,this.barH,4);ctx.fill();
ctx.strokeStyle='rgba(255,215,0,.8)';ctx.lineWidth=1.5;
rrect(ctx,sx-this.barW/2,sy+5,this.barW,this.barH,4);ctx.stroke();
// Sparkles orbiting
for(const sp of this.sparkles){
const sa=sp.a+t*sp.s;
const spx=sx+Math.cos(sa)*sp.r;const spy=sy+Math.sin(sa)*sp.r*0.6;
ctx.fillStyle=`rgba(255,225,100,${.4+Math.sin(t*3+sp.p)*.3})`;
ctx.beginPath();ctx.arc(spx,spy,1.5+p,0,Math.PI*2);ctx.fill();}
// Golden clef glyph
ctx.shadowColor='rgba(255,215,0,.9)';ctx.shadowBlur=20+p*15;
ctx.fillStyle=`rgba(255,225,80,${.9+p*.1})`;
ctx.font='bold 36px "Times New Roman",serif';
ctx.textAlign='center';ctx.textBaseline='middle';
ctx.fillText(this.glyph,sx,sy-6);
ctx.shadowBlur=0;ctx.restore();}}

// ── FallingNote — red notes falling from sky, cost 1 life ──
class FallingNote{
constructor(x,topY,screenW){
// Spread across full screen width, with slight bias toward player
const spread=screenW||800;
if(Math.random()<0.35){
// 35% chance: near the given x (player area)
this.x=x-60+Math.random()*120;
}else{
// 65% chance: random across full visible area
this.x=x-spread/2+Math.random()*spread;}
this.y=topY-80-Math.random()*200;
this.w=22;this.h=22;
this.vy=1.2+Math.random()*1.8;
this.vx=(Math.random()-.5)*0.4;
this.alive=true;
this.pulse=Math.random()*Math.PI*2;
this.trail=[];}
update(dt){if(!this.alive)return;
this.y+=this.vy;this.x+=this.vx;this.pulse+=dt*.004;
// Trail
if(Math.random()<0.3)this.trail.push({x:this.x,y:this.y,a:1});
this.trail=this.trail.filter(t=>{t.a-=0.03;t.y-=0.3;return t.a>0;});
if(this.y>5000)this.alive=false;}
rect(){return{x:this.x-this.w/2,y:this.y-this.h/2,w:this.w,h:this.h};}
draw(ctx,cx,cy){if(!this.alive)return;
const sx=this.x-cx,sy=this.y-cy;
if(sx<-80||sx>ctx.canvas.width*2.5+80||sy<-80||sy>ctx.canvas.height*2.5+80)return;
const p=Math.sin(this.pulse)*.5+.5;
ctx.save();
// Trail particles
this.trail.forEach(t=>{
ctx.fillStyle=`rgba(255,50,30,${t.a*0.3})`;
ctx.beginPath();ctx.arc(t.x-cx,t.y-cy,2+t.a*2,0,Math.PI*2);ctx.fill();});
ctx.translate(sx,sy);
// Danger glow
ctx.shadowColor='rgba(255,30,10,.6)';ctx.shadowBlur=10+p*10;
// Note head — proper ellipse, tilted
ctx.beginPath();ctx.ellipse(0,0,9,6.5,-0.25,0,Math.PI*2);
ctx.fillStyle=`rgba(200,30,20,${.9+p*.1})`;ctx.fill();
ctx.strokeStyle='rgba(100,10,5,.6)';ctx.lineWidth=1.2;ctx.stroke();
// Single stem going UP from right side of note head
ctx.strokeStyle='rgba(200,30,20,.85)';ctx.lineWidth=2;
ctx.beginPath();ctx.moveTo(8,-2);ctx.lineTo(8,-30);ctx.stroke();
// Flag at top of stem
ctx.beginPath();ctx.moveTo(8,-30);
ctx.quadraticCurveTo(14,-22,10,-14);ctx.strokeStyle='rgba(200,30,20,.7)';ctx.lineWidth=1.5;ctx.stroke();
ctx.shadowBlur=0;ctx.restore();}}

// ── SpeedZone — accelerated scroll area with visual indicators ──
class SpeedZone{
constructor(x,realm){
this.x=x;this.w=400+Math.random()*300;
this.mult=1.8+Math.random()*0.8; // speed multiplier 1.8x-2.6x
this.topY=-60*CFG.STAFF_SP;this.botY=60*CFG.STAFF_SP;
this.pulse=Math.random()*Math.PI*2;this.active=true;}
contains(px){return px>=this.x&&px<=this.x+this.w;}
update(dt){this.pulse+=dt*0.005;}
draw(ctx,cx,cy){if(!this.active)return;
const sx=this.x-cx,sw=this.w;
if(sx>ctx.canvas.width*2.5||sx+sw<-100)return;
const p=Math.sin(this.pulse)*.3+.7;const t=Date.now()*0.002;
ctx.save();
// Speed zone vertical bars — golden translucent columns
ctx.fillStyle=`rgba(215,191,129,${0.03+p*0.02})`;
ctx.fillRect(sx,0,sw,ctx.canvas.height*2.5);
// Entry/exit markers
ctx.strokeStyle=`rgba(215,191,129,${0.3+p*0.2})`;ctx.lineWidth=2;
ctx.setLineDash([8,8]);
ctx.beginPath();ctx.moveTo(sx,0);ctx.lineTo(sx,ctx.canvas.height*2.5);ctx.stroke();
ctx.beginPath();ctx.moveTo(sx+sw,0);ctx.lineTo(sx+sw,ctx.canvas.height*2.5);ctx.stroke();
ctx.setLineDash([]);
// Speed arrows flowing right
ctx.fillStyle=`rgba(215,191,129,${0.15+p*0.1})`;
const arrowH=ctx.canvas.height;
for(let i=0;i<6;i++){
const ax=sx+((t*80+i*sw/6)%sw);
const ay=arrowH*0.3+i*arrowH*0.08;
ctx.font='bold 18px sans-serif';ctx.textAlign='center';
ctx.fillText('\u00BB',ax,ay);}
// Label at top
ctx.font='bold 10px Montserrat,sans-serif';ctx.textAlign='center';
ctx.fillStyle=`rgba(215,191,129,${0.5+p*0.3})`;
ctx.fillText('SPEED \u00D7'+this.mult.toFixed(1),sx+sw/2,20);
ctx.restore();}}

// ── GravityFlipZone — inverts gravity within its bounds ──
class GravityFlipZone{
constructor(x,realm){
this.x=x;this.w=350+Math.random()*250;
this.topY=-60*CFG.STAFF_SP;this.botY=60*CFG.STAFF_SP;
this.pulse=Math.random()*Math.PI*2;this.active=true;}
contains(px){return px>=this.x&&px<=this.x+this.w;}
update(dt){this.pulse+=dt*0.004;}
draw(ctx,cx,cy){if(!this.active)return;
const sx=this.x-cx,sw=this.w;
if(sx>ctx.canvas.width*2.5||sx+sw<-100)return;
const p=Math.sin(this.pulse)*.4+.6;const t=Date.now()*0.001;
ctx.save();
// Zone background — purple tint
ctx.fillStyle=`rgba(140,80,220,${0.04+p*0.02})`;
ctx.fillRect(sx,0,sw,ctx.canvas.height*2.5);
// Clean border lines
ctx.strokeStyle=`rgba(160,100,240,${0.35+p*0.15})`;ctx.lineWidth=2.5;
ctx.beginPath();ctx.moveTo(sx,0);ctx.lineTo(sx,ctx.canvas.height*2.5);ctx.stroke();
ctx.beginPath();ctx.moveTo(sx+sw,0);ctx.lineTo(sx+sw,ctx.canvas.height*2.5);ctx.stroke();
// Gravity arrows (pointing up to show flipped)
ctx.fillStyle=`rgba(180,120,255,${0.2+p*0.15})`;
ctx.font='bold 20px sans-serif';ctx.textAlign='center';
for(let i=0;i<5;i++){
const ax=sx+sw*0.15+i*sw*0.18;
const ay=ctx.canvas.height*0.5+Math.sin(t*2+i)*30;
ctx.fillText('\u2191',ax,ay);}
// Label
ctx.font='bold 10px Montserrat,sans-serif';ctx.textAlign='center';
ctx.fillStyle=`rgba(180,120,255,${0.6+p*0.3})`;
ctx.fillText('\u21C5 GRAVITY FLIP',sx+sw/2,16);
// Corner markers — clean squares
const cs=6;
ctx.fillStyle=`rgba(160,100,240,${0.5+p*0.3})`;
ctx.fillRect(sx-1,8,cs,cs);ctx.fillRect(sx+sw-cs+1,8,cs,cs);
ctx.fillRect(sx-1,ctx.canvas.height*0.5,cs,cs);ctx.fillRect(sx+sw-cs+1,ctx.canvas.height*0.5,cs,cs);
ctx.restore();}}

// ── RhythmSection — play notes on beat for bonus ──
class RhythmSection{
constructor(x,realm){
this.x=x;this.w=500;this.active=true;this.entered=false;
this.bpm=90+Math.floor(Math.random()*40); // 90-130 BPM
this.beatInterval=60000/this.bpm;
this.beats=[];this.beatIdx=0;this.lastBeatTime=0;
this.score=0;this.totalBeats=8;this.hitCount=0;
this.pulse=0;this.beatPulse=0;
// Generate target notes for beats
const range=CFG.REALMS[realm]?.noteRange||CFG.REALMS[0].noteRange;
for(let i=0;i<this.totalBeats;i++){
const s=range[Math.floor(Math.random()*range.length)];
this.beats.push({note:s.replace(/[0-9]/g,''),octave:parseInt(s.replace(/[^0-9]/g,'')),
hit:false,time:0,active:false});}
this.state='waiting';} // waiting, active, done
contains(px){return px>=this.x&&px<=this.x+this.w;}
update(dt){
this.pulse+=dt*0.003;
if(this.state==='active'){
this.beatPulse=Math.max(0,this.beatPulse-dt*0.004);
const now=Date.now();
if(now-this.lastBeatTime>=this.beatInterval&&this.beatIdx<this.totalBeats){
this.lastBeatTime=now;
this.beats[this.beatIdx].active=true;
this.beats[this.beatIdx].time=now;
this.beatIdx++;this.beatPulse=1;}
if(this.beatIdx>=this.totalBeats&&now-this.lastBeatTime>this.beatInterval*2){
this.state='done';}}}
tryHit(noteName,toPc){
if(this.state!=='active')return false;
const PC={C:0,D:2,E:4,F:5,G:7,A:9,B:11};
const inputPc=toPc(noteName);
const now=Date.now();
// Find the most recent unmatched active beat
for(let i=this.beats.length-1;i>=0;i--){
const b=this.beats[i];
if(b.active&&!b.hit&&now-b.time<this.beatInterval*1.2){
const bPc=((PC[b.note]||0)+12)%12;
if(bPc===inputPc){b.hit=true;this.hitCount++;return true;}}}
return false;}
draw(ctx,cx,cy){if(!this.active)return;
const sx=this.x-cx,sw=this.w;
if(sx>ctx.canvas.width*2.5||sx+sw<-100)return;
const p=Math.sin(this.pulse)*.3+.7;
ctx.save();
// Zone background — warm gold tint
ctx.fillStyle=`rgba(215,170,60,${0.03+p*0.02})`;
ctx.fillRect(sx,0,sw,ctx.canvas.height*2.5);
// Border
ctx.strokeStyle=`rgba(215,191,129,${0.4+p*0.2})`;ctx.lineWidth=2;
ctx.setLineDash([4,4]);
ctx.beginPath();ctx.moveTo(sx,0);ctx.lineTo(sx,ctx.canvas.height*2.5);ctx.stroke();
ctx.beginPath();ctx.moveTo(sx+sw,0);ctx.lineTo(sx+sw,ctx.canvas.height*2.5);ctx.stroke();
ctx.setLineDash([]);
// Label + BPM
ctx.font='bold 10px Montserrat,sans-serif';ctx.textAlign='center';
ctx.fillStyle=`rgba(215,191,129,${0.6+p*0.3})`;
ctx.fillText('\u266A RHYTHM '+this.bpm+' BPM',sx+sw/2,16);
// Beat indicators at top
if(this.state==='active'){
const beatW=sw/this.totalBeats;
for(let i=0;i<this.totalBeats;i++){
const bx=sx+i*beatW;
const b=this.beats[i];
if(b.hit){ctx.fillStyle='rgba(100,230,100,.5)';}
else if(b.active){ctx.fillStyle=`rgba(255,215,0,${0.3+this.beatPulse*0.5})`;}
else{ctx.fillStyle='rgba(255,255,255,.1)';}
ctx.fillRect(bx+2,28,beatW-4,6);}}
if(this.state==='done'){
const pct=Math.round(this.hitCount/this.totalBeats*100);
ctx.font='bold 14px Montserrat,sans-serif';
ctx.fillStyle=pct>=75?'rgba(100,230,100,.8)':'rgba(255,100,60,.8)';
ctx.fillText(pct+'% RHYTHM!',sx+sw/2,40);}
ctx.restore();}}

// ── NoteTrail — visual trail of music notes behind the player ──
class NoteTrail{
constructor(){this._particles=[];this._timer=0;
this._notes=['\u266A','\u266B','\u2669','\u266C'];}
update(dt,player,combo){
this._timer+=dt;
// Emit more particles at higher combos
const interval=combo>=5?80:combo>=3?120:200;
if(this._timer>=interval&&player){
this._timer=0;
const n=this._notes[Math.floor(Math.random()*this._notes.length)];
this._particles.push({
x:player.x+player.w/2+(Math.random()-.5)*10,
y:player.y+player.h*0.6+(Math.random()-.5)*8,
vx:-0.5-Math.random()*0.8,vy:-0.3-Math.random()*0.6,
a:0.6+Math.random()*0.3,
sz:8+Math.random()*6,
rot:(Math.random()-.5)*0.3,
n:n,life:1});}
// Update particles
this._particles=this._particles.filter(p=>{
p.x+=p.vx;p.y+=p.vy;p.life-=dt*0.0015;p.a=Math.max(0,p.life*0.6);
return p.life>0;});}
draw(ctx,cx,cy){
if(this._particles.length===0)return;
ctx.save();
for(const p of this._particles){
const sx=p.x-cx,sy=p.y-cy;
if(sx<-30||sx>ctx.canvas.width*2.5||sy<-30||sy>ctx.canvas.height*2.5)continue;
ctx.globalAlpha=p.a;
ctx.fillStyle='#D7BF81';
ctx.font=`${Math.round(p.sz)}px serif`;
ctx.textAlign='center';ctx.textBaseline='middle';
ctx.save();ctx.translate(sx,sy);ctx.rotate(p.rot);
ctx.fillText(p.n,0,0);
ctx.restore();}
ctx.globalAlpha=1;ctx.restore();}}

// ── World Generator ──
class World{
constructor(realm,difficulty){
this.realm=realm;this.difficulty=difficulty;
this.noteBlocks=[];this.beamPairs=[];this.restPlatforms=[];
this.enemies=[];this.collectibles=[];this.restBonuses=[];
this.voidBars=[];this.chordBarriers=[];this.chordGroups=[];this.cclefEnemies=[];this.fallingNotes=[];
this.goldenClefs=[];
this.speedZones=[];this.gravFlipZones=[];this.rhythmSections=[];
this.genTo=0;this.melodyNotes=Mus.melodyForRealm(realm);this.melodyIdx=0;
this._fallingNoteTimer=3000+Math.random()*2000;
this._lastBarrierX=0;this._barrierInterval=2500+Math.random()*1500;
// Extra staff state (independent barrier tracking per extra staff)
this._extraLastBarrierX={};this._extraBarrierInterval={};
this._extraMelodyNotes={};this._extraMelodyIdx={};}

chunk(sx){
const rl=CFG.REALMS[this.realm];
const ex=sx+2400;
// Generate main staff content at offset 0
this._chunkStaff(sx,ex,0,rl.noteRange);
// Generate FULL cloned content on ONE extra staff for Stratosphere/Abyss
// Stratosphere (realm 3): extra staff BELOW (player goes up on central, extra below)
// Abyss (realm 4): extra staff ABOVE (player goes down on central, extra above)
if(this.realm===3||this.realm===4){
const extraRange=['B3','C4','D4','E4','F4','G4','A4','B4','C5','D5','E5'];
const off=this.realm===4?-150*CFG.STAFF_SP:150*CFG.STAFF_SP+CFG.STAFF_GAP;
this._chunkStaff(sx,ex,off,extraRange,true);}
this.genTo=ex;}

// Generate full game content for one staff layer at yOff
_chunkStaff(sx,ex,yOff,noteRange,isExtra){
const easy=this.difficulty==='easy';
const gMin=easy?CFG.GAP_MIN_EASY:CFG.GAP_MIN;
const gMax=easy?CFG.GAP_MAX_EASY:CFG.GAP_MAX;
let x=sx;
// Melody tracking per staff layer
const offKey=String(yOff);
if(yOff!==0){
if(!this._extraMelodyNotes[offKey]){
this._extraMelodyNotes[offKey]=Mus.staircaseMelody(noteRange);
this._extraMelodyIdx[offKey]=0;}
if(!this._extraLastBarrierX[offKey]){
this._extraLastBarrierX[offKey]=0;
this._extraBarrierInterval[offKey]=3000+Math.random()*2000;}}
const getMelody=()=>{
if(yOff===0){
const nd=this.melodyNotes[this.melodyIdx%this.melodyNotes.length];
this.melodyIdx++;
if(this.melodyIdx>=this.melodyNotes.length){this.melodyNotes=Mus.melodyForRealm(this.realm);this.melodyIdx=0;}
return nd;}
const notes=this._extraMelodyNotes[offKey];
let idx=this._extraMelodyIdx[offKey];
const nd=notes[idx%notes.length];
idx++;
if(idx>=notes.length){this._extraMelodyNotes[offKey]=Mus.staircaseMelody(noteRange);idx=0;}
this._extraMelodyIdx[offKey]=idx;
return nd;};
const getLastBarrierX=()=>yOff===0?this._lastBarrierX:this._extraLastBarrierX[offKey];
const setLastBarrierX=(v)=>{if(yOff===0)this._lastBarrierX=v;else this._extraLastBarrierX[offKey]=v;};
const getBarrierInterval=()=>yOff===0?this._barrierInterval:this._extraBarrierInterval[offKey];
const setBarrierInterval=(v)=>{if(yOff===0)this._barrierInterval=v;else this._extraBarrierInterval[offKey]=v;};
// Pattern variety
const patterns=['normal','normal','tight-cluster','wide-gaps','staircase'];
let pattern=patterns[Math.floor(Math.random()*patterns.length)];
let patternCount=0;const patternLen=4+Math.floor(Math.random()*6);
// Pre-calculate barrier zone
const willBarrier=ex-getLastBarrierX()>=getBarrierInterval();
const barrierZoneX=willBarrier?ex+200:null;
const barrierZoneW=80;
// Slightly wider gaps on extra staves for difficulty
const gapMult=isExtra?1.15:1;
while(x<ex){
let gap;
if(pattern==='tight-cluster'){gap=gMin*0.6+Math.random()*gMin*0.4;}
else if(pattern==='wide-gaps'){gap=gMax*0.8+Math.random()*gMax*0.6;}
else if(pattern==='staircase'){gap=gMin+Math.random()*(gMax-gMin)*0.5;}
else{gap=gMin+Math.random()*(gMax-gMin);}
gap*=gapMult;
x+=gap;
if(x>=ex)break;
if(barrierZoneX!==null&&x>barrierZoneX-350&&x<barrierZoneX+barrierZoneW+350)continue;
// Add natural empty spaces (no notes) every so often for breathing room
if(Math.random()<0.08){x+=gMax*1.5;continue;}
patternCount++;
if(patternCount>=patternLen){pattern=patterns[Math.floor(Math.random()*patterns.length)];patternCount=0;}
const nd=getMelody();
// Rest platform
if(Math.random()<CFG.REST_PLATFORM_CH){
let sl;
if(this.realm===1||this.realm===3)sl=10;
else if(this.realm===2||this.realm===4)sl=-10;
else{const opts=[2,4,6,8,10];sl=opts[Math.floor(Math.random()*opts.length)];}
const rp=new RestPlatform(x,sl);
rp.y+=yOff; // offset Y for extra staff
this.restPlatforms.push(rp);continue;}
// Beam pair
if(Math.random()<CFG.BEAM_PAIR_CH){
const bpCloseN=this.noteBlocks.some(n=>Math.abs(n.x-x)<80);
const bpCloseB=this.beamPairs.some(b=>Math.abs(b.x2-x)<100);
const bpCloseC=this.chordGroups.some(g=>Math.abs(g.x-x)<100);
if(bpCloseN||bpCloseB||bpCloseC){x+=gMin;continue;}
const nd2=getMelody();
const bp=new BeamPair(x,nd,this.realm,easy);
if(yOff!==0){bp.y1+=yOff;bp.y2+=yOff;bp.stem1TipY+=yOff;bp.stem2TipY+=yOff;
bp.beamNote1.y+=yOff;bp.beamNote2.y+=yOff;bp._yOff=yOff;}
this.beamPairs.push(bp);
x+=90;continue;}
// Note block
const nbY=Mus.noteToY(nd.note,nd.octave)+yOff;
const tooCloseNote=this.noteBlocks.some(n=>Math.abs(n.x-x)<80&&Math.abs(n.y-nbY)<40);
const tooCloseBeam=this.beamPairs.some(b=>Math.abs(b.x1-x)<90||Math.abs(b.x2-x)<90);
const tooCloseChord=this.chordGroups.some(g=>Math.abs(g.x-x)<100);
if(tooCloseNote||tooCloseBeam||tooCloseChord)continue;
const nb=new NoteBlock(x,nbY,nd,true,easy);
if(easy)nb.disappear=false;
if(yOff!==0){nb._extraStaff=true;nb._staffOffset=yOff;nb.disappear=false;
nb.staffPos=Mus.staffPosFromY(nb.y-yOff);} // staffPos relative to extra staff, not central
this.noteBlocks.push(nb);
if(Math.random()<CFG.ENEMY_CH){
this.enemies.push(new Enemy(x,Mus.noteToY(nd.note,nd.octave)+yOff,'patrol'));}
if(Math.random()<CFG.COLLECT_CH){
const ctype=Math.random()<.05?'life':Math.random()<.08?'fly':Math.random()<.1?'hint':'note';
this.collectibles.push(new Collectible(x,Mus.noteToY(nd.note,nd.octave)+yOff,ctype));}
// Chord groups
if(Math.random()<CFG.CHORD_GROUP_CH){
const cgX=x+gap*0.6;
const cgCloseN=this.noteBlocks.some(n=>Math.abs(n.x-cgX)<90);
const cgCloseB=this.beamPairs.some(b=>Math.abs(b.x2-cgX)<100);
const cgCloseC=this.chordGroups.some(g=>Math.abs(g.x-cgX)<120);
if(cgCloseN||cgCloseB||cgCloseC)continue;
const cg=new ChordGroup(cgX,nd,this.realm,easy);
if(yOff!==0){cg.notes.forEach(n=>{n.y+=yOff;n.staffPos=Mus.staffPosFromY(n.y-yOff);});
cg.minY=Math.min(...cg.notes.map(n=>n.y));cg.maxY=Math.max(...cg.notes.map(n=>n.y));
cg.h=Math.max(CFG.NOTE_HITBOX_H,(cg.maxY-cg.minY)+CFG.NOTE_HITBOX_H);cg._yOff=yOff;}
this.chordGroups.push(cg);}}
// CClef enemy
if(Math.random()<CFG.CCLEF_ENEMY_CH){
const cNd=getMelody();
const cY=Mus.noteToY(cNd.note,cNd.octave)+yOff;
this.cclefEnemies.push(new CClefEnemy(ex+400,cY));}
// Rest bonuses
if(Math.random()<CFG.REST_BONUS_CH){
const rx=sx+100+Math.random()*600;
const positions=[2,4,6,8,10,-2,-4,-6,-8,-10];
const pos=positions[Math.floor(Math.random()*positions.length)];
const rt=['quarter','half','whole'][Math.floor(Math.random()*3)];
const rbY=-pos*CFG.STAFF_SP+(pos<0?CFG.STAFF_GAP:0)+yOff;
this.restBonuses.push(new RestBonus(rx,rbY,rt));}
// Wandering enemies
if(Math.random()<.06){
const wNd=getMelody();
const wY=Mus.noteToY(wNd.note,wNd.octave)+yOff;
this.enemies.push(new Enemy(sx+200+Math.random()*500,wY,'wander'));}
// Chord barriers
let barrierX=null;
if(ex-getLastBarrierX()>=getBarrierInterval()){
setLastBarrierX(ex);
setBarrierInterval(2500+Math.random()*2000);
barrierX=ex+200;
const cb=new ChordBarrier(barrierX,this.realm);
if(yOff!==0){cb.chordNotes.forEach(cn=>{cn.y+=yOff;});
cb.topY=yOff-30*CFG.STAFF_SP;cb.botY=yOff+30*CFG.STAFF_SP;cb._yOff=yOff;}
this.chordBarriers.push(cb);}
// Post-generate: remove ANY normal notes/beams that overlap with barriers on this staff layer
if(barrierX!==null){
const bxMin=barrierX-80/2-60,bxMax=barrierX+80/2+60;
this.noteBlocks=this.noteBlocks.filter(nb=>{
if(nb.x>bxMin&&nb.x<bxMax){
// Check Y range: only remove notes on the same staff layer
if(yOff===0&&!nb._extraStaff)return false;
if(yOff!==0&&nb._extraStaff&&nb._staffOffset===yOff)return false;}
return true;});
this.beamPairs=this.beamPairs.filter(bp=>{
const midX=(bp.x1+bp.x2)/2;
if(midX>bxMin&&midX<bxMax){
if(yOff===0&&!bp._yOff)return false;
if(yOff!==0&&bp._yOff===yOff)return false;}
return true;});
this.chordGroups=this.chordGroups.filter(cg=>{
if(cg.x>bxMin&&cg.x<bxMax){
if(yOff===0&&!cg._yOff)return false;
if(yOff!==0&&cg._yOff===yOff)return false;}
return true;});}
// Void rescue bars
const voidCount=1+Math.floor(Math.random()*2);
const spacing=Math.floor((ex-sx)/(voidCount+1));
for(let i=0;i<voidCount;i++){
const vx=sx+spacing*(i+1)+Math.round((Math.random()-.5)*spacing*0.3);
const vb=new VoidRescueBar(vx,this.realm);
if(yOff!==0){vb.baseY+=yOff;} // shift void bar to extra staff region
this.voidBars.push(vb);}
if(barrierX!==null){
const vb2=new VoidRescueBar(barrierX-200,this.realm);
if(yOff!==0){vb2.baseY+=yOff;}
this.voidBars.push(vb2);}
// Golden clef reward
if(Math.random()<(isExtra?0.05:0.03)){
const gcX=sx+400+Math.random()*1600;
const gc=new GoldenClefReward(gcX,this.realm);
if(yOff!==0){gc.baseY+=yOff;gc._yOff=yOff;}
this.goldenClefs.push(gc);
// Staircase notes leading to golden clef
const isUp=gc.baseY<yOff;const steps=5+Math.floor(Math.random()*3);
for(let i=0;i<steps;i++){
const stepX=gcX-steps*60+i*60;
const stepY=isUp?(yOff-10*CFG.STAFF_SP-i*(Math.abs(gc.baseY-(yOff-10*CFG.STAFF_SP))/(steps)))
:(yOff+10*CFG.STAFF_SP+CFG.STAFF_GAP+i*(Math.abs(gc.baseY-(yOff+10*CFG.STAFF_SP+CFG.STAFF_GAP))/(steps)));
const snd=Mus.randNote(noteRange);
const snb=new NoteBlock(stepX,stepY,snd,true,easy);
snb.disappear=false;snb._stairNote=true;
if(yOff!==0){snb._extraStaff=true;snb._staffOffset=yOff;snb.staffPos=Mus.staffPosFromY(snb.y-yOff);}
this.noteBlocks.push(snb);
if(i%2===0){const ct=i===steps-1?'fly':'note';
this.collectibles.push(new Collectible(stepX,stepY-25,ct));}}}
// Zones — only on main staff (too complex for extra staves, keeps gameplay focused)
if(yOff===0){
const occupied=[];
if(barrierZoneX!==null)occupied.push([barrierZoneX-150,barrierZoneX+barrierZoneW+150]);
this.speedZones.filter(z=>z.x+z.w>sx&&z.x<ex).forEach(z=>occupied.push([z.x-50,z.x+z.w+50]));
this.gravFlipZones.filter(z=>z.x+z.w>sx&&z.x<ex).forEach(z=>occupied.push([z.x-50,z.x+z.w+50]));
this.rhythmSections.filter(r=>r.x+r.w>sx&&r.x<ex).forEach(r=>occupied.push([r.x-50,r.x+r.w+50]));
const zoneFits=(zx,zw)=>!occupied.some(([a,b])=>zx<b&&zx+zw>a);
const zoneRoll=Math.random();
if(zoneRoll<0.05){
const szX=sx+200+Math.random()*1600;const sz=new SpeedZone(szX,this.realm);
if(zoneFits(szX,sz.w))this.speedZones.push(sz);
}else if(zoneRoll<0.09){
const gfX=sx+300+Math.random()*1400;const gf=new GravityFlipZone(gfX,this.realm);
if(zoneFits(gfX,gf.w))this.gravFlipZones.push(gf);
}else if(zoneRoll<0.13){
const rsX=sx+400+Math.random()*1300;const rs=new RhythmSection(rsX,this.realm);
if(zoneFits(rsX,rs.w))this.rhythmSections.push(rs);}}}

ensure(cx){while(this.genTo<cx+8000)this.chunk(this.genTo);}
clean(cx){const c=cx-3000;
this.noteBlocks=this.noteBlocks.filter(n=>n.x+40>c);
this.beamPairs=this.beamPairs.filter(b=>b.x2+40>c);
this.restPlatforms=this.restPlatforms.filter(r=>r.x+40>c);
this.enemies=this.enemies.filter(e=>e.x>c);
this.collectibles=this.collectibles.filter(c2=>c2.x>c);
this.restBonuses=this.restBonuses.filter(r=>r.x+20>c&&!r.collected);
this.voidBars=this.voidBars.filter(b=>b.x+b.w>c);
this.chordBarriers=this.chordBarriers.filter(b=>b.x+b.w>c&&b.state!=='gone');
this.chordGroups=this.chordGroups.filter(g=>g.x+40>c&&g.state!=='crumbling');
this.cclefEnemies=this.cclefEnemies.filter(e=>e.alive&&e.x>c-200);
this.fallingNotes=this.fallingNotes.filter(f=>f.alive);
this.goldenClefs=this.goldenClefs.filter(gc=>!gc.collected&&gc.x>c-500);
this.speedZones=this.speedZones.filter(z=>z.x+z.w>c);
this.gravFlipZones=this.gravFlipZones.filter(z=>z.x+z.w>c);
this.rhythmSections=this.rhythmSections.filter(r=>r.x+r.w>c&&r.state!=='done');}
// Spawn falling notes near the player
spawnFallingNote(playerX,camY,screenW){
this._fallingNoteTimer-=16;
if(this._fallingNoteTimer<=0){
this._fallingNoteTimer=2500+Math.random()*4000;
this.fallingNotes.push(new FallingNote(playerX,camY,screenW||800));}}}

// ── LevelKey — floating golden clef on a platform, validates level completion ──
class LevelKey{
constructor(x,realm){
this.x=x;this.realm=realm;this.collected=false;
// Place platform at realm center Y so player encounters it naturally during play
// realm 0: middle bridge (C4 line, Y=0)
// realm 1: attic (A5 zone, Y=-12*SP)
// realm 2: basement (E2 zone, Y=12*SP)
// realm 3: stratosphere (C7 zone, Y=-21*SP)
// realm 4: abyss (A0 zone, Y=21*SP)
const realmY=[0,-12,12,-21,21];
this.y=(realmY[realm]||0)*CFG.STAFF_SP;
this.platW=90;this.platH=12;this.pulse=0;this.phase=0;
// Treble clef for treble realms, bass clef for bass realms
const isBass=realm===2||realm===4;
this.clefChar=isBass?'\u{1D122}':'\u{1D11E}';}
update(dt){if(this.collected)return;this.pulse+=dt*.004;this.phase+=dt*.0015;}
platY(){return this.y+Math.sin(this.phase)*8;}
platRect(){return{x:this.x-this.platW/2,y:this.platY()+20,w:this.platW,h:this.platH};}
rect(){const py=this.platY();return{x:this.x-30,y:py-60,w:60,h:80};}
draw(ctx,cx,cy){
if(this.collected)return;
const sx=this.x-cx,py=this.platY();const sy=py-cy;
if(sx<-120||sx>ctx.canvas.width+120)return;
const p=Math.sin(this.pulse)*.5+.5;
const bob=Math.sin(this.phase*1.5)*6;
const ryC=sy-28+bob; // center of the clef/orbit zone
ctx.save();
// ── Platform — dark indigo, cyan highlight ──
const pr=this.platRect(),psx=pr.x-cx,psy=pr.y-cy;
ctx.shadowColor='rgba(100,80,255,.7)';ctx.shadowBlur=18+p*14;
const pg=ctx.createLinearGradient(psx,psy,psx,psy+pr.h);
pg.addColorStop(0,'rgba(160,120,255,.97)');pg.addColorStop(.5,'rgba(70,45,200,.95)');pg.addColorStop(1,'rgba(30,15,110,.88)');
ctx.fillStyle=pg;rrect(ctx,psx,psy,pr.w,pr.h,5);ctx.fill();
ctx.fillStyle='rgba(180,160,255,.55)';ctx.fillRect(psx+5,psy+1,pr.w-10,2);
// ── Concentric energy rings ──
for(let ri=0;ri<3;ri++){
const rr=20+ri*15+p*(ri+1)*4;
const ra=(.18-ri*.04)+p*.15;
const hue=ri===0?'200,150,255':ri===1?'100,200,255':'200,240,255';
ctx.beginPath();ctx.arc(sx,ryC,rr,0,Math.PI*2);
ctx.strokeStyle=`rgba(${hue},${ra.toFixed(2)})`;ctx.lineWidth=ri===0?2:1.2;
ctx.shadowColor=`rgba(${hue},.5)`;ctx.shadowBlur=ri===0?10:6;ctx.stroke();}
// ── Inner orbit: 12 small sparkles (fast, purple/cyan) ──
for(let i=0;i<12;i++){
const a=i/12*Math.PI*2+this.phase*2.4;
const r=24+Math.sin(this.phase*3+i)*3;
const spx=sx+Math.cos(a)*r,spy=ryC+Math.sin(a)*r*.42;
const sa=.3+Math.sin(this.phase*4+i*1.3)*.35;
const hue=i%3===0?'190,140,255':i%3===1?'100,220,255':'240,180,255';
ctx.fillStyle=`rgba(${hue},${sa.toFixed(2)})`;
ctx.shadowColor=`rgba(${hue},.7)`;ctx.shadowBlur=7;
ctx.beginPath();ctx.arc(spx,spy,1.6,0,Math.PI*2);ctx.fill();}
// ── Outer orbit: 8 larger sparkles (slow, counter-rotating, indigo/cyan) ──
for(let i=0;i<8;i++){
const a=i/8*Math.PI*2-this.phase*0.85+Math.PI*.1;
const r=48+Math.sin(this.phase*2+i*.9)*7;
const spx=sx+Math.cos(a)*r,spy=ryC+Math.sin(a)*r*.38;
const sa=.38+Math.sin(this.phase*2.2+i)*.38;
const hue=i%2===0?'140,100,255':'60,200,255';
ctx.fillStyle=`rgba(${hue},${sa.toFixed(2)})`;
ctx.shadowColor=`rgba(${hue},.8)`;ctx.shadowBlur=12;
ctx.beginPath();ctx.arc(spx,spy,2.6,0,Math.PI*2);ctx.fill();}
// ── Clef — silver-white with cyan glow ──
ctx.textAlign='center';ctx.textBaseline='middle';
ctx.font=`${Math.round((36+p*5)*CFG.STAFF_SP/20)}px "Times New Roman",Georgia,serif`;
ctx.shadowColor='rgba(140,200,255,.95)';ctx.shadowBlur=30+p*24;
ctx.fillStyle=`rgba(230,240,255,${.88+p*.12})`;
ctx.fillText(this.clefChar,sx,ryC);
ctx.shadowBlur=0;ctx.restore();}}

// ── Background ──
class BG{
constructor(){
// Layered star field: far (faint/tiny/slow) → near (bright/large/fast)
this._sl=[];
const lo=[
{n:120,r1:.15,r2:.6,a1:.08,a2:.20,p:.05, tw:.0012}, // far — dust (reduced)
{n:80,r1:.5, r2:1.2,a1:.18,a2:.38,p:.14, tw:.0018}, // mid (reduced)
{n:40, r1:1.0,r2:1.8,a1:.32,a2:.58,p:.30, tw:.0024}, // near (reduced)
{n:15, r1:1.8,r2:3.0,a1:.55,a2:.85,p:.55, tw:.0032}, // foreground — bright (reduced)
];
for(const l of lo){for(let i=0;i<l.n;i++){this._sl.push({
nx:Math.random(),ny:Math.random(),
r:l.r1+Math.random()*(l.r2-l.r1),
a:l.a1+Math.random()*(l.a2-l.a1),
p:l.p,ph:Math.random()*Math.PI*2,tws:l.tw*(0.6+Math.random()*.8),
bc:Math.random()<.13});}}}
draw(ctx,cx,cy,W,H,realm){
this._realm=realm||0;
const g=ctx.createLinearGradient(0,0,0,H);
g.addColorStop(0,'#060610');g.addColorStop(.45,'#0A0A16');g.addColorStop(1,'#060610');
ctx.fillStyle=g;ctx.fillRect(0,0,W,H);
// Staff lines — WHITE, with gap between treble and bass
const trebleLines=[2,4,6,8,10];
const bassLines=[-2,-4,-6,-8,-10];
ctx.lineCap='butt';
// Treble staff lines (no gap offset)
trebleLines.forEach(p=>{const sy=-p*CFG.STAFF_SP-cy;
if(sy>-40&&sy<H+40){
ctx.strokeStyle='rgba(255,255,255,.15)';ctx.lineWidth=3;
ctx.beginPath();ctx.moveTo(0,sy);ctx.lineTo(W,sy);ctx.stroke();
ctx.strokeStyle=CFG.COLORS.STAFF_M;ctx.lineWidth=1.5;
ctx.beginPath();ctx.moveTo(0,sy);ctx.lineTo(W,sy);ctx.stroke();}});
// Bass staff lines (shifted down by STAFF_GAP)
bassLines.forEach(p=>{const sy=-p*CFG.STAFF_SP+CFG.STAFF_GAP-cy;
if(sy>-40&&sy<H+40){
ctx.strokeStyle='rgba(255,255,255,.15)';ctx.lineWidth=3;
ctx.beginPath();ctx.moveTo(0,sy);ctx.lineTo(W,sy);ctx.stroke();
ctx.strokeStyle=CFG.COLORS.STAFF_M;ctx.lineWidth=1.5;
ctx.beginPath();ctx.moveTo(0,sy);ctx.lineTo(W,sy);ctx.stroke();}});
// Staff bracket lines
// Treble bracket
{const ty=-10*CFG.STAFF_SP-cy,by=-2*CFG.STAFF_SP-cy;
if(by>-40&&ty<H+40){ctx.strokeStyle='rgba(255,255,255,.45)';ctx.lineWidth=2.5;
ctx.beginPath();ctx.moveTo(10,ty);ctx.lineTo(10,by);ctx.stroke();}}
// Bass bracket
{const ty=2*CFG.STAFF_SP+CFG.STAFF_GAP-cy,by=10*CFG.STAFF_SP+CFG.STAFF_GAP-cy;
if(by>-40&&ty<H+40){ctx.strokeStyle='rgba(255,255,255,.45)';ctx.lineWidth=2.5;
ctx.beginPath();ctx.moveTo(10,ty);ctx.lineTo(10,by);ctx.stroke();}}
// ── Single extra staff for Stratosphere (below) & Abyss (above) ──
if(this._realm===3||this._realm===4){
const off=this._realm===4?-150*CFG.STAFF_SP:150*CFG.STAFF_SP+CFG.STAFF_GAP;
const alpha=0.35;
trebleLines.forEach(p=>{const sy=-p*CFG.STAFF_SP+off-cy;
if(sy>-40&&sy<H+40){
ctx.strokeStyle=`rgba(215,191,129,${alpha})`;ctx.lineWidth=2;
ctx.beginPath();ctx.moveTo(0,sy);ctx.lineTo(W,sy);ctx.stroke();
ctx.strokeStyle=`rgba(215,191,129,${alpha*2})`;ctx.lineWidth=1;
ctx.beginPath();ctx.moveTo(0,sy);ctx.lineTo(W,sy);ctx.stroke();}});
bassLines.forEach(p=>{const sy=-p*CFG.STAFF_SP+off+CFG.STAFF_GAP-cy;
if(sy>-40&&sy<H+40){
ctx.strokeStyle=`rgba(215,191,129,${alpha})`;ctx.lineWidth=2;
ctx.beginPath();ctx.moveTo(0,sy);ctx.lineTo(W,sy);ctx.stroke();
ctx.strokeStyle=`rgba(215,191,129,${alpha*2})`;ctx.lineWidth=1;
ctx.beginPath();ctx.moveTo(0,sy);ctx.lineTo(W,sy);ctx.stroke();}});
const ety=-10*CFG.STAFF_SP+off-cy,eby=10*CFG.STAFF_SP+off+CFG.STAFF_GAP-cy;
if(eby>-60&&ety<H+60){ctx.strokeStyle=`rgba(215,191,129,${alpha*3})`;ctx.lineWidth=2;
ctx.beginPath();ctx.moveTo(10,ety);ctx.lineTo(10,eby);ctx.stroke();}}
// Parallax starfield — moves proportionally to camera, giving depth illusion
const ms=Date.now();
ctx.save();
for(const s of this._sl){
const sx=((s.nx*W-cx*s.p%W)+W*10)%W;
const sy=((s.ny*H-cy*s.p*.18%H)+H*10)%H;
const tw=Math.sin(ms*s.tws+s.ph)*.18;
const a=Math.max(0,s.a+tw);
ctx.fillStyle=s.bc?`rgba(130,195,255,${a.toFixed(2)})`:`rgba(220,228,255,${a.toFixed(2)})`;
ctx.beginPath();ctx.arc(sx,sy,s.r,0,Math.PI*2);ctx.fill();}
ctx.restore();}

// Sticky clefs — drawn at world position or as fixed HUD indicator if off-screen
drawClefs(ctx,cy,H,realm){
const S=CFG.STAFF_SP;
const gY=-4*S-cy; // G4 line screen Y (treble, no gap)
const fY=4*S+CFG.STAFF_GAP-cy;  // F3 line screen Y (bass, shifted by gap)
const cx2=62;const cx3=66;
// Background mask
ctx.save();
const maskGrad=ctx.createLinearGradient(0,0,130,0);
maskGrad.addColorStop(0,'rgba(8,8,16,.97)');
maskGrad.addColorStop(.7,'rgba(8,8,16,.65)');
maskGrad.addColorStop(1,'rgba(8,8,16,0)');
ctx.fillStyle=maskGrad;ctx.fillRect(0,0,130,H);ctx.restore();
// Clefs: sticky horizontally (fixed X on left) but NOT vertically — follow staff Y
// Only draw when the staff line is actually on screen
if(gY>-400&&gY<H+400)this._trebleClef(ctx,cx2,gY);
if(fY>-400&&fY<H+400)this._bassClef(ctx,cx3,fY);
// Extra clefs for distant staves in Stratosphere/Abyss
if(realm===3||realm===4){
const off=realm===4?-150*S:150*S+CFG.STAFF_GAP;
const egY=-4*S+off-cy;
const efY=4*S+CFG.STAFF_GAP+off-cy;
if(egY>-400&&egY<H+400)this._trebleClef(ctx,cx2,egY,.7);
if(efY>-400&&efY<H+400)this._bassClef(ctx,cx3,efY,.7);}}

_trebleClef(ctx,x,yG,alpha){
// yG = screen Y of G4 line (staffPos 4)
// The treble clef curl sits on the G4 line
ctx.save();
const S=CFG.STAFF_SP;
const a=alpha||.95;
const col=`rgba(215,191,129,${a})`;
// Font spans treble staff height — proportional to staff spacing
const fontSize=Math.max(64,Math.round(S*11));
ctx.fillStyle=col;
ctx.font=`${fontSize}px "Times New Roman",serif`;
ctx.textAlign='center';ctx.textBaseline='alphabetic';
ctx.shadowColor=`rgba(215,191,129,${a*.63})`;ctx.shadowBlur=alpha?8:14;
// ── CLEF POSITION TUNING ──
// Proportional offset based on font size for consistent placement at any scale
const yOff=Math.round(fontSize*0.28)-Math.round(fontSize*0.08);
ctx.fillText('\u{1D11E}',x+Math.round(S*0.4),yG+yOff);
ctx.shadowBlur=0;ctx.restore();}

_bassClef(ctx,x,yF,alpha){
// yF = screen Y of F3 line (staffPos -4)
// The bass clef dots bracket F3
ctx.save();
const S=CFG.STAFF_SP;
const a=alpha||.95;
const col=`rgba(215,191,129,${a})`;
const fontSize=Math.max(52,Math.round(S*8.5));
ctx.fillStyle=col;
ctx.font=`${fontSize}px "Times New Roman",serif`;
ctx.textAlign='center';ctx.textBaseline='alphabetic';
ctx.shadowColor=`rgba(215,191,129,${a*.63})`;ctx.shadowBlur=alpha?8:14;
// ── CLEF POSITION TUNING ──
// Proportional offset: scales with font size for correct placement at all screen sizes
const yOff=Math.round(fontSize*0.18)+Math.round(fontSize*0.26);
ctx.fillText('\u{1D122}',x+Math.round(S*0.4),yF+yOff);
ctx.shadowBlur=0;ctx.restore();}}


// ── CANVAS PIANO — transparent piano overlay drawn on canvas, fixed screen position ──
class CanvasPiano{
constructor(){
this.pressedKeys={};this._fadeKeys={};
this.helpHintKeys={}; // keys highlighted by Help button (green)
this.WHITE_NOTES=['C','D','E','F','G','A','B'];
this.BLACK_NOTES=[{note:'C#',pos:0},{note:'D#',pos:1},{note:'F#',pos:3},{note:'G#',pos:4},{note:'A#',pos:5}];
this._clickNote=null;}
press(note){this.pressedKeys[note]=Date.now();this._fadeKeys[note]=1;}
release(note){delete this.pressedKeys[note];}
releaseAll(){this.pressedKeys={};this._fadeKeys={};}
hitTest(canvasX,canvasY,cvsW,cvsH){
const layout=this._layout(cvsW,cvsH);
if(canvasY<layout.y||canvasY>layout.y+layout.h)return null;
for(const bk of this.BLACK_NOTES){
const bx=layout.x+layout.wkW*(bk.pos+1)-layout.bkW/2;
const by=layout.y;const bh=layout.h*0.62;
if(canvasX>=bx&&canvasX<=bx+layout.bkW&&canvasY>=by&&canvasY<=by+bh)return bk.note;}
const relX=canvasX-layout.x;
if(relX<0||relX>layout.totalW)return null;
const idx=Math.floor(relX/layout.wkW);
if(idx>=0&&idx<7)return this.WHITE_NOTES[idx];
return null;}
_layout(cvsW,cvsH){
const isMob=cvsW<600;
const totalW=isMob?Math.min(cvsW*0.92,480):Math.min(cvsW*0.6,480);
const h=isMob?Math.min(cvsH*0.24,130):Math.min(cvsH*0.20,110);
const x=(cvsW-totalW)/2;
const y=cvsH-h-8;
const wkW=totalW/7;
const bkW=wkW*0.58;
return{x,y,h,totalW,wkW,bkW};}
draw(ctx,cvsW,cvsH,latin,pianistMode){
const L=this._layout(cvsW,cvsH);
ctx.save();
const hideLabels=!!pianistMode;
// "Play notes to move forward" text above piano
ctx.globalAlpha=0.5;
ctx.font='bold '+Math.round(L.wkW*0.24)+'px Montserrat,sans-serif';
ctx.textAlign='center';ctx.textBaseline='bottom';
ctx.fillStyle='#D7BF81';
ctx.shadowColor='rgba(215,191,129,.6)';ctx.shadowBlur=8;
ctx.fillText('\u266B  PLAY NOTES TO MOVE FORWARD  \u266B',cvsW/2,L.y-8);
ctx.shadowBlur=0;
// White keys — real piano: white with subtle gradient, rounded bottom
for(let i=0;i<7;i++){
const kx=L.x+i*L.wkW;
const note=this.WHITE_NOTES[i];
const isPressed=!!this.pressedKeys[note];
const isHelpHint=!!this.helpHintKeys[note];
const fade=this._fadeKeys[note]||0;
ctx.globalAlpha=isPressed?0.8:isHelpHint?0.9:0.5;
// White key body — bright ivory (green if help hint)
const wGrad=ctx.createLinearGradient(kx,L.y,kx,L.y+L.h);
if(isPressed){
wGrad.addColorStop(0,'rgba(255,225,120,1)');
wGrad.addColorStop(.4,'rgba(250,210,90,1)');
wGrad.addColorStop(1,'rgba(220,180,60,1)');
}else if(isHelpHint){
wGrad.addColorStop(0,'rgba(80,230,120,1)');
wGrad.addColorStop(.5,'rgba(50,200,90,1)');
wGrad.addColorStop(1,'rgba(30,160,60,1)');
}else{
wGrad.addColorStop(0,'rgba(255,255,252,1)');
wGrad.addColorStop(.5,'rgba(248,248,242,1)');
wGrad.addColorStop(1,'rgba(230,228,218,1)');}
ctx.fillStyle=wGrad;
rrect(ctx,kx+1,L.y,L.wkW-2,L.h-2,3);ctx.fill();
// Key border — dark gap between keys
ctx.strokeStyle=isPressed?'rgba(180,150,50,.6)':isHelpHint?'rgba(30,140,50,.7)':'rgba(80,80,80,.45)';
ctx.lineWidth=1.2;
rrect(ctx,kx+1,L.y,L.wkW-2,L.h-2,3);ctx.stroke();
// Bottom shadow for 3D depth
ctx.fillStyle='rgba(0,0,0,.12)';
ctx.fillRect(kx+3,L.y+L.h-6,L.wkW-6,4);
// Green glow for help hint
if(isHelpHint&&!isPressed){
ctx.shadowColor='rgba(50,200,80,.9)';ctx.shadowBlur=14;
ctx.fillStyle='rgba(50,200,80,.15)';
rrect(ctx,kx+1,L.y,L.wkW-2,L.h-2,3);ctx.fill();
ctx.shadowBlur=0;}
// Note label at bottom of key (hidden in pianist mode)
if(!hideLabels){
ctx.globalAlpha=isPressed?0.9:isHelpHint?0.9:0.45;
ctx.fillStyle=isPressed?'#6B4B00':'#444';
ctx.font='bold '+Math.round(L.wkW*0.26)+'px Montserrat,sans-serif';
ctx.textAlign='center';ctx.textBaseline='middle';
const LATIN={C:'Do',D:'Ré',E:'Mi',F:'Fa',G:'Sol',A:'La',B:'Si'};
const label=latin?(LATIN[note]||note):note;
ctx.fillText(label,kx+L.wkW/2,L.y+L.h*0.82);}}
// Black keys — real piano: dark with subtle shine
for(const bk of this.BLACK_NOTES){
const bx=L.x+L.wkW*(bk.pos+1)-L.bkW/2;
const bh=L.h*0.62;
const isPressed=!!this.pressedKeys[bk.note];
const isHelpHintBk=!!this.helpHintKeys[bk.note];
const fade=this._fadeKeys[bk.note]||0;
ctx.globalAlpha=isPressed?0.85:isHelpHintBk?0.9:0.7;
// Black key body — very dark, high contrast against white (green if help hint)
const bGrad=ctx.createLinearGradient(bx,L.y,bx,L.y+bh);
if(isPressed){
bGrad.addColorStop(0,'rgba(220,185,60,1)');
bGrad.addColorStop(.5,'rgba(180,150,40,1)');
bGrad.addColorStop(1,'rgba(140,110,25,1)');
}else if(isHelpHintBk){
bGrad.addColorStop(0,'rgba(40,180,70,1)');
bGrad.addColorStop(.5,'rgba(30,140,50,1)');
bGrad.addColorStop(1,'rgba(20,100,35,1)');
}else{
bGrad.addColorStop(0,'rgba(35,35,40,1)');
bGrad.addColorStop(.3,'rgba(18,18,22,1)');
bGrad.addColorStop(.8,'rgba(8,8,10,1)');
bGrad.addColorStop(1,'rgba(2,2,4,1)');}
ctx.fillStyle=bGrad;
rrect(ctx,bx,L.y-1,L.bkW,bh,2);ctx.fill();
// Subtle top shine
ctx.fillStyle=isPressed?'rgba(255,220,100,.3)':isHelpHintBk?'rgba(80,230,120,.3)':'rgba(255,255,255,.08)';
ctx.fillRect(bx+2,L.y,L.bkW-4,3);
// Green glow for help hint
if(isHelpHintBk&&!isPressed){
ctx.shadowColor='rgba(50,200,80,.9)';ctx.shadowBlur=12;
rrect(ctx,bx,L.y-1,L.bkW,bh,2);ctx.fill();
ctx.shadowBlur=0;}
// Border
ctx.strokeStyle=isHelpHintBk?'rgba(50,200,80,.6)':'rgba(0,0,0,.5)';ctx.lineWidth=1;
rrect(ctx,bx,L.y-1,L.bkW,bh,2);ctx.stroke();
// Label (hidden in pianist mode)
if(!hideLabels){
ctx.globalAlpha=isPressed?0.9:isHelpHintBk?0.9:0.35;
ctx.fillStyle=isPressed?'#FFD700':isHelpHintBk?'#AAFFCC':'#999';
ctx.font='bold '+Math.round(L.bkW*0.32)+'px Montserrat,sans-serif';
ctx.textAlign='center';ctx.textBaseline='middle';
const LATIN_BK={'C#':'Do#','D#':'Ré#','F#':'Fa#','G#':'Sol#','A#':'La#'};
const bkLabel=latin?(LATIN_BK[bk.note]||bk.note):bk.note;
ctx.fillText(bkLabel,bx+L.bkW/2,L.y+bh*0.7);}}
ctx.restore();
// Fade out pressed keys over time
for(const k in this._fadeKeys){
if(!this.pressedKeys[k]){
this._fadeKeys[k]-=0.03;
if(this._fadeKeys[k]<=0)delete this._fadeKeys[k];}}}}

// ── MAIN GAME ──
class Game{
constructor(){this.cvs=null;this.ctx=null;this.state='welcome';
this.player=null;this.world=null;this.bg=new BG();this.parts=new Particles();
this.canvasPiano=new CanvasPiano();
this.audio=new Audio();this.bgMusic=new BGMusic();this.vine=new VineRescue();this.midi=new MIDI();
this.cam={x:0,y:0};this.keys={left:false,right:false,up:false,down:false};this.jumpPress=false;
this.realm=1;this.difficulty='easy'; // 'easy' | 'pianist'
this.score=0;this.combo=0;this.maxCombo=0;this.lives=CFG.LIVES;
this.notesOk=0;this.notesAll=0;this.dist=0;this.best=0;
this.chalNote=null;this.glowLine=new GlowingStaffLine();
this.gameTime=0;this.lastT=0;this.unlocks=[true,true,true,false,false];
this.realmBestDist={1:0,2:0,3:0,4:0};
this.UNLOCK_DIST=500; // meters needed on Attic & Basement to unlock Stratosphere & Abyss
this.noteHintActive=false;this.noteHintTimer=0;this.lastLandedNote=null;
this._helpHintActive=false; // Help (?) button state
this._autoJumpTarget=null;
this._onVoidBar=null;this._activeBarrier=null;this._activeChordGroup=null;
this._levelKey=null;this._levelKeySpawned=false;
this._milestoneIdx=0;
this._MILESTONES=[10000,20000,30000,50000,80000,120000];
// Golden clef collectible counts per game
this.clefCounts={treble:{total:6,collected:0},bass:{total:5,collected:0},alto:{total:4,collected:0}};
this.noteTrail=new NoteTrail();
this._speedMult=1;this._gravFlip=false;
this._load();}

init(){this.cvs=document.getElementById('ll-canvas');if(!this.cvs)return;
this.ctx=this.cvs.getContext('2d');this._resize();
window.addEventListener('resize',()=>this._resize());
this._bindKeys();this._bindUI();this.midi.init();this.midi.cb=n=>this._midiNote(n);
this._updateHUD();
// Show/hide load save button
const lsb=document.getElementById('ll-load-save-btn');
if(lsb)lsb.style.display=this._hasSavedGame()?'inline-block':'none';
// Start in welcome mode — show site header/footer
this._setWelcomeMode(true);
this._volume=0.7;
// Fullscreen change listener
document.addEventListener('fullscreenchange',()=>{
const app=document.getElementById('ll-app');
if(!document.fullscreenElement&&!document.webkitFullscreenElement){
this._pseudoFs=false;if(app)app.classList.remove('fullscreen');}
setTimeout(()=>this._resize(),100);});
document.addEventListener('webkitfullscreenchange',()=>{
const app=document.getElementById('ll-app');
if(!document.webkitFullscreenElement){
this._pseudoFs=false;if(app)app.classList.remove('fullscreen');}
setTimeout(()=>this._resize(),100);});
this._loop(performance.now());}

_resize(){
const c=this.cvs.parentElement;
if(!c||c.clientWidth===0)return; // Skip resize when canvas hidden (welcome mode)
const newW=c.clientWidth,newH=c.clientHeight;
this.cvs.width=newW;this.cvs.height=newH;
// Scale visual dimensions proportionally with screen width
// Mobile gets a higher minimum so notes/symbols stay readable
const s=Math.min(1.0,Math.max(0.55,newW/900));
CFG.STAFF_SP=Math.round(20*s);
CFG.STAFF_GAP=Math.round(60*s);
CFG.NOTE_HEAD_RX=Math.round(22*s);
CFG.NOTE_HEAD_RY=Math.round(14*s);
CFG.NOTE_HITBOX_W=Math.round(56*s);
CFG.NOTE_HITBOX_H=Math.round(32*s);
CFG.ACC_EXTRA_W=Math.round(22*s);
CFG.PLAYER_W=Math.round(34*s);
CFG.PLAYER_H=Math.round(52*s);
// Gap scaling only before game starts
if(this.state!=='playing'&&this.state!=='challenged'){
CFG.GAP_MIN=Math.round(100*s);
CFG.GAP_MAX=Math.round(180*s);
CFG.GAP_MIN_EASY=Math.round(90*s);
CFG.GAP_MAX_EASY=Math.round(160*s);}}

_bindKeys(){
document.addEventListener('keydown',e=>{
if(e.code==='ArrowLeft'){this.keys.left=true;e.preventDefault();}
if(e.code==='ArrowRight'){this.keys.right=true;e.preventDefault();}
if(e.code==='ArrowDown'){this.keys.down=true;e.preventDefault();}
if(e.code==='ArrowUp'||e.code==='Space'){
if(!this.keys.up&&!this.keys.jump)this.jumpPress=true;
this.keys.up=true;this.keys.jump=true;e.preventDefault();}
if(e.code==='Escape'||e.code==='KeyP'){this._pause();e.preventDefault();}
if(CFG.KEYS[e.code]){
const n=CFG.KEYS[e.code];
this.canvasPiano.press(n);
if(this.state==='challenged')this._answer(n);
else if(this.state==='playing'){
// Priority 1: active chord barrier
if(this._activeBarrier&&this._activeBarrier.isBlocking()){
this._tryBarrierNote(n);
// Priority 2: active chord group
}else if(this._activeChordGroup&&this._activeChordGroup.state==='challenged'){
this._tryChordGroupNote(n);
}else if(this.vine.state==='available'){
if(!this.vine.tryAnswer(n,this.player))this.audio.playBad();
else{this.audio.playVine();
this.parts.emit(this.player.x+this.player.w/2,this.player.y,CFG.COLORS.VINE,12,{sp:5,life:30,sz:3});}
}else{this._tryAutoJump(n);this._tryNoteInput(n);this._tryRhythmHit(n);}}
e.preventDefault();}});
document.addEventListener('keyup',e=>{
if(e.code==='ArrowLeft')this.keys.left=false;
if(e.code==='ArrowRight')this.keys.right=false;
if(e.code==='ArrowDown')this.keys.down=false;
if(e.code==='ArrowUp'||e.code==='Space'){this.keys.up=false;this.keys.jump=false;}
if(CFG.KEYS[e.code])this.canvasPiano.release(CFG.KEYS[e.code]);});}

// Pitch class helper (shared)
_toPc(s){const PC={C:0,D:2,E:4,F:5,G:7,A:9,B:11};
const base=s.replace(/[#b]/g,'');const acc=s.includes('#')?1:s.includes('b')?-1:0;
return((PC[base]||0)+acc+12)%12;}

_tryAutoJump(noteName){
if(!this.player||!this.world)return;
if(!this.player.onG&&this.player.vy<0)return;
const px=this.player.x+this.player.w/2;
const inputPc=this._toPc(noteName);
const noteStr=n=>n.note+(n.accidental==='sharp'?'#':n.accidental==='flat'?'b':'');
// Check chord groups first — any note of chord validates and jumps to top
for(const cg of this.world.chordGroups){
if(cg.x>px+10&&cg.state!=='correct'&&cg.state!=='crumbling'){
const chordNotes=cg.notes||[];
for(const cn of chordNotes){
const cnStr=cn.note+(cn.accidental==='sharp'?'#':cn.accidental==='flat'?'b':'');
if(this._toPc(cnStr)===inputPc){
cg.state='correct';
this.score+=CFG.PTS_NOTE*2*(1+this.combo*CFG.COMBO_MULT);
this.combo++;this.maxCombo=Math.max(this.maxCombo,this.combo);this.notesOk++;
this.audio.playOk();this.parts.emit(cg.x,cg.minY,CFG.COLORS.GOLD,20,{sp:6,life:35,sz:3});
// Jump to topmost note of chord
this._jumpToNote({x:cg.x,y:cg.minY,note:cn.note,oct:cn.octave,
state:'correct',identified:true,isSolid:()=>true,accidental:cn.accidental||null});
return;}}}}
// Collect forward note candidates
const candidates=[];
this.world.noteBlocks
.filter(nb=>nb.x>px+10&&nb.isSolid()&&nb.state!=='wrong'&&!(nb.identified&&nb.state==='correct'))
.forEach(nb=>candidates.push({x:nb.x,nb}));
this.world.beamPairs.forEach(bp=>{
if(bp.x1>px+10&&bp.isSolid1()&&bp.state1!=='wrong'&&!(bp.identified1&&bp.state1==='correct'))
candidates.push({x:bp.x1,nb:bp.beamNote1});
if(bp.x2>px+10&&bp.isSolid2()&&bp.state2!=='wrong'&&!(bp.identified2&&bp.state2==='correct'))
candidates.push({x:bp.x2,nb:bp.beamNote2});});
candidates.sort((a,b)=>a.x-b.x);
if(candidates.length===0)return;
const next=candidates[0].nb;
const nextPc=this._toPc(noteStr(next));
if(inputPc===nextPc){this._jumpToNote(next);return;}
const hasVirus=this.world.enemies.some(e=>e.alive&&Math.abs(e.noteX-next.x)<5);
if(hasVirus&&candidates.length>=2){
const skip=candidates[1].nb;
if(inputPc===this._toPc(noteStr(skip)))this._jumpToNote(skip);}}

_jumpToNote(nb){
// Pre-validate target note and jump toward it
nb.state='correct';nb.identified=true;nb.requireGuess=false;
this.score+=CFG.PTS_NOTE*(1+this.combo*CFG.COMBO_MULT);
this.combo++;this.maxCombo=Math.max(this.maxCombo,this.combo);this.notesOk++;
this.audio.play(nb.note,nb.oct);
this.parts.emit(nb.x,nb.y,CFG.COLORS.GOLD,10,{sp:4,life:25,sz:2});
// Compute needed vertical velocity to always land ON TOP of the target note
// peakNeeded = how high above current feet position the note top is + buffer
const playerFeetY=this.player.y+this.player.h;
const noteTopY=nb.y-CFG.NOTE_HITBOX_H/2;
const heightNeeded=playerFeetY-noteTopY+48; // note top + generous buffer
let neededVy=CFG.JUMP;
if(heightNeeded>0){neededVy=-Math.sqrt(2*CFG.GRAVITY*heightNeeded);}
// Clamp: at least normal JUMP, at most 1.9× for very high targets
this.player.vy=Math.max(CFG.JUMP*1.9,Math.min(CFG.JUMP,neededVy));
// Physics-accurate horizontal velocity using actual air time
const T=Math.round(2*Math.abs(this.player.vy)/CFG.GRAVITY);
const sumFric=(1-Math.pow(CFG.AIR_FRIC,T))/(1-CFG.AIR_FRIC);
const dx=nb.x-(this.player.x+this.player.w/2);
const V=Math.abs(dx)/sumFric;
this.player.vx=Math.sign(dx)*Math.min(CFG.SPEED*5,Math.max(CFG.SPEED*0.6,V));
this.player.onG=false;
this.player.dj=true;this.player.tj=true;
this.player.sqX=1.3;this.player.sqY=.72;this.player.state='jump';
this.audio.playJmp();
// Store target for in-air guidance correction
this._autoJumpTarget=nb;
this._updateHUD();}

_bindUI(){
const $=id=>document.getElementById(id);
$('ll-start-btn')?.addEventListener('click',()=>{this.audio.init();this._start();});
// Canvas piano click handler — check if click/touch hits a piano key
const _pianoClick=(cx,cy)=>{
const note=this.canvasPiano.hitTest(cx,cy,this.cvs.width,this.cvs.height);
if(note){
this.canvasPiano.press(note);
setTimeout(()=>this.canvasPiano.release(note),300);
if(this.state==='challenged')this._answer(note);
else if(this.state==='playing'){
if(this._activeBarrier&&this._activeBarrier.isBlocking()){
this._tryBarrierNote(note);
}else if(this._activeChordGroup&&this._activeChordGroup.state==='challenged'){
this._tryChordGroupNote(note);
}else if(this.vine.state==='available'){
if(this.vine.tryAnswer(note,this.player)){this.audio.playVine();
this.parts.emit(this.player.x+this.player.w/2,this.player.y,CFG.COLORS.VINE,12,{sp:5,life:30});}
}else{this._tryAutoJump(note);this._tryNoteInput(note);this._tryRhythmHit(note);}}
if(this.audio.ok){const realmOct=[4,5,2,6,1][this.realm]||4;this.audio.play(note.replace('#',''),realmOct);}
return true;}return false;};
this.cvs.addEventListener('click',e=>{
const r=this.cvs.getBoundingClientRect();
const cx=e.clientX-r.left,cy2=e.clientY-r.top;
if((this.state==='playing'||this.state==='challenged')&&this._handleHUDClick(cx,cy2))return;
_pianoClick(cx,cy2);});
// Touch: multi-touch + swipe for movement
this._touchStarts={};this._swipeThresh=20;
this.cvs.addEventListener('touchstart',e=>{
e.preventDefault();
const r=this.cvs.getBoundingClientRect();
for(let i=0;i<e.changedTouches.length;i++){
const t=e.changedTouches[i];
const tx=t.clientX-r.left,ty=t.clientY-r.top;
this._touchStarts[t.identifier]={x:tx,y:ty,t:Date.now(),swiped:false};
if((this.state==='playing'||this.state==='challenged')&&this._handleHUDClick(tx,ty))continue;
if(_pianoClick(tx,ty)){this._touchStarts[t.identifier].piano=true;continue;}
if(this.state==='playing'||this.state==='challenged'){
this.jumpPress=true;this.keys.up=true;
this._jumpTouches=this._jumpTouches||new Set();
this._jumpTouches.add(t.identifier);}}
},{passive:false});
this.cvs.addEventListener('touchmove',e=>{
e.preventDefault();
for(let i=0;i<e.changedTouches.length;i++){
const t=e.changedTouches[i];
const start=this._touchStarts[t.identifier];
if(!start||start.piano)continue;
const dx=t.clientX-this.cvs.getBoundingClientRect().left-start.x;
if(!start.swiped&&Math.abs(dx)>this._swipeThresh){
start.swiped=true;
if(dx>0){this.keys.right=true;this.keys.left=false;}
else{this.keys.left=true;this.keys.right=false;}}}
},{passive:false});
this.cvs.addEventListener('touchend',e=>{
for(let i=0;i<e.changedTouches.length;i++){
const t=e.changedTouches[i];
const id=t.identifier;
const start=this._touchStarts[id];
// Stop swipe-driven movement
if(start&&start.swiped){this.keys.left=false;this.keys.right=false;}
delete this._touchStarts[id];
if(this._jumpTouches&&this._jumpTouches.has(id)){
this._jumpTouches.delete(id);
if(this._jumpTouches.size===0){this.keys.up=false;this.keys.jump=false;}}}
},{passive:false});
this.cvs.addEventListener('touchcancel',e=>{
this._touchStarts={};
if(this._jumpTouches)this._jumpTouches.clear();
this.keys.up=false;this.keys.jump=false;this.keys.left=false;this.keys.right=false;
},{passive:false});
// Difficulty buttons
document.querySelectorAll('.ll-diff-btn').forEach(b=>{
b.addEventListener('click',()=>{
this.difficulty=b.dataset.diff;
document.querySelectorAll('.ll-diff-btn').forEach(x=>x.classList.remove('active'));
b.classList.add('active');
// Update keyboard hint
const kh=$('ll-keys-hint');if(kh)kh.style.display=this.difficulty==='easy'?'none':'flex';
// Toggle pianist mode class on keyboard (hides note name labels)
const kb=$('ll-keyboard');if(kb)kb.classList.toggle('pianist-mode',this.difficulty==='pianist');});});
document.querySelectorAll('.ll-realm-card').forEach(c=>{c.addEventListener('click',()=>{
const r=parseInt(c.dataset.realm);if(this.unlocks[r]){this.realm=r;
document.querySelectorAll('.ll-realm-card').forEach(x=>x.classList.remove('active'));c.classList.add('active');}});});
this._updateRealmCards();
$('ll-resume-btn')?.addEventListener('click',()=>this._pause());
$('ll-restart-btn')?.addEventListener('click',()=>this._start());
$('ll-restart-btn-2')?.addEventListener('click',()=>this._start());
$('ll-back-to-menu-btn')?.addEventListener('click',()=>{
this.bgMusic.stop();this.state='welcome';this.chalNote=null;
$('ll-gameover-overlay').classList.add('hidden');
$('ll-welcome-overlay').classList.remove('hidden');
this._setWelcomeMode(true);
this._updateRealmCards();
const lsb2=$('ll-load-save-btn');if(lsb2)lsb2.style.display=this._hasSavedGame()?'inline-block':'none';});
$('ll-sound-btn')?.addEventListener('click',()=>{
const m=this.audio.toggle();this.bgMusic.toggle();
$('ll-sound-btn').innerHTML=m?
'<svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2"><path d="M11 5L6 9H2v6h4l5 4V5z"/><line x1="23" y1="9" x2="17" y2="15"/><line x1="17" y1="9" x2="23" y2="15"/></svg>':
'<svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2"><polygon points="11 5 6 9 2 9 2 15 6 15 11 19 11 5"/><path d="M19.07 4.93a10 10 0 0 1 0 14.14"/><path d="M15.54 8.46a5 5 0 0 1 0 7.07"/></svg>';});
// Mobile touch controls for left/right movement
const tleft=$('ll-touch-left'),tright=$('ll-touch-right');
if(tleft){
tleft.addEventListener('touchstart',e=>{e.preventDefault();this.keys.left=true;},{passive:false});
tleft.addEventListener('touchend',e=>{e.preventDefault();this.keys.left=false;},{passive:false});
tleft.addEventListener('mousedown',()=>{this.keys.left=true;});
tleft.addEventListener('mouseup',()=>{this.keys.left=false;});}
if(tright){
tright.addEventListener('touchstart',e=>{e.preventDefault();this.keys.right=true;},{passive:false});
tright.addEventListener('touchend',e=>{e.preventDefault();this.keys.right=false;},{passive:false});
tright.addEventListener('mousedown',()=>{this.keys.right=true;});
tright.addEventListener('mouseup',()=>{this.keys.right=false;});}
// Save button
$('ll-save-btn')?.addEventListener('click',()=>{this._saveGameState();});
// Load save button on welcome screen
$('ll-load-save-btn')?.addEventListener('click',()=>{this.audio.init();this._loadGameState();});
$('ll-fullscreen-btn')?.addEventListener('click',()=>{
const app=$('ll-app');
const isFs=document.fullscreenElement||document.webkitFullscreenElement;
if(!isFs){
  (app.requestFullscreen?.()??app.webkitRequestFullscreen?.())?.catch(()=>{});
  app.classList.add('fullscreen');
}else{
  (document.exitFullscreen??document.webkitExitFullscreen)?.call(document);
  app.classList.remove('fullscreen');}});
$('ll-help-btn')?.addEventListener('click',()=>$('ll-tutorial-modal').classList.remove('hidden'));
// Notation toggle: International (A,B,C) ↔ Latin (Do,Ré,Mi)
$('ll-notation-btn')?.addEventListener('click',()=>{
CFG.latin=!CFG.latin;
const lbl=$('ll-notation-label');if(lbl)lbl.textContent=CFG.latin?'Do·Ré':'A·B·C';
// Update piano keyboard key labels
document.querySelectorAll('.ll-key[data-note]').forEach(k=>{
const full=k.dataset.note;
const base=full.replace(/[#b]/g,'');
const acc=full.includes('#')?'sharp':full.includes('b')?'flat':null;
const nl=k.querySelector('.ll-key-note');
if(nl)nl.textContent=Mus.noteLabel(base,acc,CFG.latin);});});
// Chord barrier submit
$('ll-chord-submit')?.addEventListener('click',()=>this._submitChord());
$('ll-chord-input')?.addEventListener('keydown',e=>{if(e.key==='Enter'){e.preventDefault();this._submitChord();}});
$('ll-tutorial-close')?.addEventListener('click',()=>$('ll-tutorial-modal').classList.add('hidden'));
// Close tutorial modal on backdrop click
$('ll-tutorial-modal')?.addEventListener('click',e=>{
if(e.target.classList.contains('ll-modal')||e.target.classList.contains('ll-modal-backdrop'))
$('ll-tutorial-modal').classList.add('hidden');});
// Piano keyboard
document.querySelectorAll('.ll-key').forEach(k=>{
const h=()=>{const n=k.dataset.note;
if(n){
if(this.state==='challenged')this._answer(n);
else if(this.state==='playing'){
if(this._activeBarrier&&this._activeBarrier.isBlocking()){
this._tryBarrierNote(n);
}else if(this._activeChordGroup&&this._activeChordGroup.state==='challenged'){
this._tryChordGroupNote(n);
}else if(this.vine.state==='available'){
if(this.vine.tryAnswer(n,this.player)){this.audio.playVine();
this.parts.emit(this.player.x+this.player.w/2,this.player.y,CFG.COLORS.VINE,12,{sp:5,life:30});}
}else{this._tryAutoJump(n);this._tryNoteInput(n);this._tryRhythmHit(n);}}}
k.classList.add('pressed');setTimeout(()=>k.classList.remove('pressed'),150);
if(n&&this.audio.ok){const realmOct=[4,5,2,6,1][this.realm]||4;this.audio.play(n.replace('#',''),realmOct);}};
k.addEventListener('click',h);k.addEventListener('touchstart',e=>{e.preventDefault();h();});});}

_setWelcomeMode(on){
const app=document.getElementById('ll-app');
if(app){if(on){app.classList.add('welcome-mode');app.classList.remove('playing-mode');}
else{app.classList.remove('welcome-mode');app.classList.add('playing-mode');}}}

_start(){
this._setWelcomeMode(false);
// Re-measure canvas after switching from welcome mode
setTimeout(()=>this._resize(),50);
this.bgMusic.stop();
this.state='playing';this.score=0;this.combo=0;this.maxCombo=0;this.lives=CFG.LIVES;
this.notesOk=0;this.notesAll=0;this.dist=0;this.gameTime=0;this.chalNote=null;
this.noteHintActive=false;this.noteHintTimer=0;this.lastLandedNote=null;
this._helpHintActive=false;
this._autoJumpTarget=null;this._onVoidBar=null;this._activeBarrier=null;this._activeChordGroup=null;
this._levelKey=null;this._levelKeySpawned=false;this._milestoneIdx=0;
this.glowLine=new GlowingStaffLine();this.vine.reset();
// Start on a wide, solid rest platform — player placed precisely on top
this.world=new World(this.realm,this.difficulty);
// Start position adapted to chosen realm — player spawns near the realm's note range
const realmStartPos={0:2,1:12,2:-10,3:21,4:-17};
const startStaffPos=realmStartPos[this.realm]||2;
const startRestPlatform=new RestPlatform(20,startStaffPos);
startRestPlatform.w=80; // extra wide for stable start
this.world.restPlatforms.push(startRestPlatform);
this.world.chunk(200);this.world.chunk(this.world.genTo);
// Place player feet exactly on top edge of the platform rect (no falling through)
const spRect=startRestPlatform.rect();
this.player=new Player(20-CFG.PLAYER_W/2,spRect.y-CFG.PLAYER_H);
this.player.vy=0;this.player.vx=0;this.player.onG=true;
this.cam.x=0;this.cam.y=spRect.y-this.cvs.height/2+CFG.PLAYER_H;
// Toggle pianist mode class on keyboard
const kb=document.getElementById('ll-keyboard');
if(kb)kb.classList.toggle('pianist-mode',this.difficulty==='pianist');
document.querySelectorAll('.ll-overlay').forEach(o=>o.classList.add('hidden'));
this._updateHUD();
// Start BGMusic after short delay
setTimeout(()=>this.bgMusic.start(this.realm),800);}

_pause(){
if(this.state==='playing'){this.state='paused';this.bgMusic.stop();
document.getElementById('ll-pause-overlay').classList.remove('hidden');}
else if(this.state==='paused'){this.state='playing';this.bgMusic.start(this.realm);
document.getElementById('ll-pause-overlay').classList.add('hidden');}}

_over(){this.state='gameover';this.bgMusic.stop();this.audio.playEnd();
this._helpHintActive=false;
this.canvasPiano.helpHintKeys={};
if(this.score>this.best)this.best=this.score;
// Track best distance per realm
const prevUnlock3=this.unlocks[3],prevUnlock4=this.unlocks[4];
if(this.dist>this.realmBestDist[this.realm])this.realmBestDist[this.realm]=Math.round(this.dist);
// Recalculate unlocks: realms 1&2 always open; 3&4 unlock when both 1&2 reach threshold
this.unlocks[0]=true;this.unlocks[1]=true;this.unlocks[2]=true;
this.unlocks[3]=this.realmBestDist[1]>=this.UNLOCK_DIST&&this.realmBestDist[2]>=this.UNLOCK_DIST;
this.unlocks[4]=this.realmBestDist[1]>=this.UNLOCK_DIST&&this.realmBestDist[2]>=this.UNLOCK_DIST;
this._save();
const $=id=>document.getElementById(id);
if($('ll-final-score'))$('ll-final-score').textContent=Math.round(this.score);
if($('ll-final-best'))$('ll-final-best').textContent=this.best;
if($('ll-final-combo'))$('ll-final-combo').textContent=this.maxCombo;
if($('ll-final-accuracy'))$('ll-final-accuracy').textContent=(this.notesAll>0?Math.round(this.notesOk/this.notesAll*100):0)+'%';
if($('ll-final-distance'))$('ll-final-distance').textContent=Math.round(this.dist)+'m';
if($('ll-final-notes'))$('ll-final-notes').textContent=this.notesOk;
// Show unlock message if new realms just unlocked
let unlockMsg='';
const d1=this.realmBestDist[1]||0,d2=this.realmBestDist[2]||0;
if(this.unlocks[3]&&!prevUnlock3)unlockMsg='\uD83C\uDF1F Stratosphere & Abyss UNLOCKED!';
else if(!this.unlocks[3])unlockMsg='\uD83D\uDD12 Unlock: Attic '+Math.min(d1,this.UNLOCK_DIST)+'/'+this.UNLOCK_DIST+'m \u2022 Basement '+Math.min(d2,this.UNLOCK_DIST)+'/'+this.UNLOCK_DIST+'m';
const ue=$('ll-unlock-progress');if(ue)ue.textContent=unlockMsg;
else if(unlockMsg){const el=document.createElement('div');el.id='ll-unlock-progress';
el.style.cssText='text-align:center;font-size:12px;color:var(--ll-gold);margin-top:8px;';
el.textContent=unlockMsg;$('ll-gameover-overlay')?.querySelector('.ll-overlay-box')?.appendChild(el);}
$('ll-gameover-overlay').classList.remove('hidden');this._ajax();}

_loop(t){const dt=Math.min(t-(this.lastT||t),33);this.lastT=t;
if(this.state==='playing'||this.state==='challenged')this._update(dt);
this._draw();requestAnimationFrame(t2=>this._loop(t2));}

_update(dt){this.gameTime+=dt;

// Note hint timer
if(this.noteHintActive){this.noteHintTimer-=dt;if(this.noteHintTimer<=0)this.noteHintActive=false;}

// Sync player with moving void rescue bar BEFORE player.update to prevent trembling
const _prevVoidBarY=this._onVoidBar?this._onVoidBar.currentY():null;

// Jump
if(this.jumpPress&&this.state==='playing'&&this.vine.state!=='swinging'){
const superJump=this._onVoidBar&&this.player.onG;
if(this.player.jump()){this.audio.playJmp();
this.parts.emit(this.player.x+this.player.w/2,this.player.y+this.player.h,CFG.COLORS.GOLD,5,{sp:2,life:18,sz:2});
if(superJump){this.player.vy=CFG.JUMP*1.85;this._fb('SUPER JUMP!','perfect');}}}
this.jumpPress=false;

// Vine swinging
if(this.vine.state==='swinging'){this.vine.update(dt,this.player);this._updateCam();
this.dist=Math.max(this.dist,this.player.x/50);this._updateHUD();return;}

const mk=this.state==='challenged'?{left:false,right:false,up:false,down:false}:{...this.keys};
this.player.update(dt,mk);
// Apply speed zone: boost horizontal velocity
if(this._speedMult>1){
this.player.vx*=this._speedMult;this.player.x+=(this._speedMult-1)*this.player.vx*0.3;}
// Apply gravity flip: invert vertical velocity effect
if(this._gravFlip&&this.player.flyTime<=0){
this.player.vy=-Math.abs(this.player.vy)*0.8-0.5;
if(this.player.vy<-CFG.MAX_FALL)this.player.vy=-CFG.MAX_FALL;}
// Glass ceiling: prevent player from going too far above. Extends for Stratosphere/Abyss extra staves
const ceiling=(this.realm===3||this.realm===4)?-170*CFG.STAFF_SP:-60*CFG.STAFF_SP;
if(this.player.y<ceiling){this.player.y=ceiling;if(this.player.vy<0)this.player.vy=0;}

// Ticking — with spatial culling for performance
const zoom=this._getZoom();
const viewL=this.cam.x-800,viewR=this.cam.x+this.cvs.width/zoom+800;
const viewT=this.cam.y-600,viewB=this.cam.y+this.cvs.height/zoom+600;
this.world.ensure(this.cam.x+this.cvs.width/zoom+6000);
// Spatial-culled updates: only update entities near the viewport
for(let i=0,len=this.world.noteBlocks.length;i<len;i++){const n=this.world.noteBlocks[i];if(n.x>viewL&&n.x<viewR)n.update(dt);}
for(let i=0,len=this.world.beamPairs.length;i<len;i++){const b=this.world.beamPairs[i];if(b.x1>viewL&&b.x2<viewR)b.update(dt);}
for(let i=0,len=this.world.restPlatforms.length;i<len;i++){const r=this.world.restPlatforms[i];if(r.x>viewL&&r.x<viewR)r.update(dt);}
for(let i=0,len=this.world.enemies.length;i<len;i++){const e=this.world.enemies[i];if(e.x>viewL-200&&e.x<viewR+200)e.update(dt);}
for(let i=0,len=this.world.collectibles.length;i<len;i++){const c=this.world.collectibles[i];if(c.x>viewL&&c.x<viewR)c.update(dt);}
for(let i=0,len=this.world.restBonuses.length;i<len;i++){const r=this.world.restBonuses[i];if(r.x>viewL&&r.x<viewR)r.update(dt);}
for(let i=0,len=this.world.voidBars.length;i<len;i++){const b=this.world.voidBars[i];if(b.x>viewL-200&&b.x<viewR+200)b.update(dt);}
this.world.chordBarriers.forEach(b=>b.update(dt)); // always update (blocking)
for(let i=0,len=this.world.chordGroups.length;i<len;i++){const g=this.world.chordGroups[i];if(g.x>viewL&&g.x<viewR)g.update(dt);}
for(let i=0,len=this.world.cclefEnemies.length;i<len;i++){const e=this.world.cclefEnemies[i];if(e.x>viewL-300&&e.x<viewR+300)e.update(dt);}
this.world.fallingNotes.forEach(f=>f.update(dt)); // always update (moving)
for(let i=0,len=this.world.goldenClefs.length;i<len;i++){const gc=this.world.goldenClefs[i];if(gc.x>viewL&&gc.x<viewR)gc.update(dt);}
for(let i=0,len=this.world.speedZones.length;i<len;i++){const z=this.world.speedZones[i];if(z.x+z.w>viewL&&z.x<viewR)z.update(dt);}
for(let i=0,len=this.world.gravFlipZones.length;i<len;i++){const z=this.world.gravFlipZones[i];if(z.x+z.w>viewL&&z.x<viewR)z.update(dt);}
for(let i=0,len=this.world.rhythmSections.length;i<len;i++){const r=this.world.rhythmSections[i];if(r.x+r.w>viewL&&r.x<viewR)r.update(dt);}
this.noteTrail.update(dt,this.player,this.combo);
// Check if player is in a speed zone or gravity flip zone
this._speedMult=1;this._gravFlip=false;
const px=this.player.x+this.player.w/2;
this.world.speedZones.forEach(z=>{if(z.active&&z.contains(px))this._speedMult=z.mult;});
this.world.gravFlipZones.forEach(z=>{if(z.active&&z.contains(px))this._gravFlip=true;});
// Activate rhythm sections when player enters; reward on completion
this.world.rhythmSections.forEach(r=>{
if(r.state==='waiting'&&r.contains(px)){r.state='active';r.lastBeatTime=Date.now();}
if(r.state==='done'&&!r._rewarded){r._rewarded=true;
const pct=r.hitCount/r.totalBeats;
if(pct>=0.75){this.score+=200;this._fb('RHYTHM MASTER!','perfect');}
else if(pct>=0.5){this.score+=100;this._fb('GOOD RHYTHM!','ok');}}});
this.world.spawnFallingNote(this.player.x,this.cam.y,this.cvs.width/zoom);
if(this._levelKey&&!this._levelKey.collected)this._levelKey.update(dt);
this.glowLine.update(dt,this.realm);this.parts.update();
// Sync player Y with void bar movement to eliminate trembling (skip if player jumped)
if(this._onVoidBar&&_prevVoidBarY!==null&&this.player.onG){
const delta=this._onVoidBar.currentY()-_prevVoidBarY;
this.player.y+=delta;this.player.vy=0;}

// ── Collisions ──
const wasOnG=this.player.onG;
this.player.onG=false;this.player.lp=null;this._onVoidBar=null;
// Set coyote time when leaving ground (allows late jumps)
if(wasOnG)this.player.coyoteT=80; // 80ms coyote time
// Clear barrier ref if it dissolved or player moved away
if(this._activeBarrier&&!this._activeBarrier.isBlocking())this._activeBarrier=null;

// Note blocks
for(const nb of this.world.noteBlocks){
if((nb.state==='crumbling'&&nb.crumT>600)||!nb.isSolid())continue;
const r=nb.rect(),pl=this.player.rect();
if(this.player.vy>=0&&pl.x+pl.w>r.x&&pl.x<r.x+r.w&&
pl.y+pl.h>=r.y&&pl.y+pl.h<=r.y+r.h+12&&this.player.y+this.player.h-this.player.vy<=r.y+6){
this.player.y=r.y-this.player.h;this.player.vy=0;this.player.onG=true;
this.player.dj=true;this.player.tj=true;this.player.lp=nb;
if(this.player.state==='fall'){this.player.sqX=1.15;this.player.sqY=.85;}
// Highlight piano key if hint active
if(this.noteHintActive){
const a=nb.accidental;let ns=nb.note;
if(a==='sharp')ns+='#';
else if(a==='flat'){const f2s={Db:'C#',Eb:'D#',Gb:'F#',Ab:'G#',Bb:'A#'};ns=f2s[nb.note+'b']||nb.note;}
this._highlightKey(ns);}
// Challenge
if(nb.requireGuess&&!nb.identified&&nb.state!=='challenged'&&nb.state!=='wrong'&&this.state==='playing'){
nb.state='challenged';this.state='challenged';this.chalNote=nb;this.notesAll++;}
break;}}

// Rest platforms
for(const rp of this.world.restPlatforms){
const r=rp.rect(),pl=this.player.rect();
if(this.player.vy>=0&&pl.x+pl.w>r.x&&pl.x<r.x+r.w&&
pl.y+pl.h>=r.y&&pl.y+pl.h<=r.y+r.h+12&&this.player.y+this.player.h-this.player.vy<=r.y+6){
this.player.y=r.y-this.player.h;this.player.vy=0;this.player.onG=true;
this.player.dj=true;this.player.tj=true;
if(rp.state==='idle'){rp.state='used';this.score+=50;this._fb('+50 REST','good');}
// Rests are silence — no sound on landing
break;}}

// Beam pair — land on individual NOTE HEADS (not the beam bar or stems)
for(const bp of this.world.beamPairs){
if(bp.state==='crumbling'&&bp.crumT>600)continue;
const pl=this.player.rect();let landed=false;
// Note head 1
if(bp.isSolid1()){
const nr1=bp.noteRect1();
if(this.player.vy>=0&&pl.x+pl.w>nr1.x&&pl.x<nr1.x+nr1.w&&
pl.y+pl.h>=nr1.y&&pl.y+pl.h<=nr1.y+nr1.h+12&&this.player.y+this.player.h-this.player.vy<=nr1.y+6){
this.player.y=nr1.y-this.player.h;this.player.vy=0;this.player.onG=true;
this.player.dj=true;this.player.tj=true;this.player.lp=bp.beamNote1;
if(this.player.state==='fall'){this.player.sqX=1.15;this.player.sqY=.85;}
// Challenge beam note 1 if not yet identified
if(!bp.identified1&&bp.state1!=='wrong'&&this.state==='playing'){
  bp.state1='challenged';this.state='challenged';this.chalNote=bp.beamNote1;this.notesAll++;}
landed=true;}}
// Note head 2 (only if note 1 not landed)
if(!landed&&bp.isSolid2()){
const nr2=bp.noteRect2();
if(this.player.vy>=0&&pl.x+pl.w>nr2.x&&pl.x<nr2.x+nr2.w&&
pl.y+pl.h>=nr2.y&&pl.y+pl.h<=nr2.y+nr2.h+12&&this.player.y+this.player.h-this.player.vy<=nr2.y+6){
this.player.y=nr2.y-this.player.h;this.player.vy=0;this.player.onG=true;
this.player.dj=true;this.player.tj=true;this.player.lp=bp.beamNote2;
if(this.player.state==='fall'){this.player.sqX=1.15;this.player.sqY=.85;}
if(!bp.identified2&&bp.state2!=='wrong'&&this.state==='playing'){
  bp.state2='challenged';this.state='challenged';this.chalNote=bp.beamNote2;this.notesAll++;}
landed=true;}}
if(landed)break;}

// Ledger line platforms — stepping stones for notes above/below the main staff
if(!this.player.onG){
const S=CFG.STAFF_SP,hw=CFG.NOTE_HEAD_RX*1.55;
outerLL: for(const nb of this.world.noteBlocks){
if(!nb.isSolid()||(nb.state==='crumbling'&&nb.crumT>600))continue;
const sp=nb.staffPos;
if(sp<12&&sp>-12)continue;
const pl=this.player.rect();
const lx1=nb.x-hw,lx2=nb.x+hw;
if(pl.x+pl.w<=lx1||pl.x>=lx2)continue;
const lps=[];
if(sp>=12){const top=sp%2===0?sp-2:sp-1;for(let p=12;p<=top;p+=2)lps.push(p);}
else{const bot=sp%2===0?sp+2:sp+1;for(let p=-12;p>=bot;p-=2)lps.push(p);}
for(const lp of lps){
const lY=nb.y+(sp-lp)*S;
if(this.player.vy>=0&&pl.y+pl.h>=lY-2&&pl.y+pl.h<=lY+14&&
this.player.y+this.player.h-this.player.vy<=lY+4){
this.player.y=lY-this.player.h;this.player.vy=0;this.player.onG=true;
this.player.dj=true;this.player.tj=true;
if(this.player.state==='fall'){this.player.sqX=1.15;this.player.sqY=.85;}
this.player.lp=nb;
if(this.noteHintActive){
const a=nb.accidental;let ns=nb.note;
if(a==='sharp')ns+='#';
else if(a==='flat'){const f2s={Db:'C#',Eb:'D#',Gb:'F#',Ab:'G#',Bb:'A#'};ns=f2s[nb.note+'b']||nb.note;}
this._highlightKey(ns);}
if(nb.requireGuess&&!nb.identified&&nb.state!=='challenged'&&nb.state!=='wrong'&&this.state==='playing'){
nb.state='challenged';this.state='challenged';this.chalNote=nb;this.notesAll++;}
break outerLL;}}}}

// Glow line
if(this.glowLine.canLandOn()&&!this.player.onG){
const gly=this.glowLine.lineY;const pl=this.player.rect();
if(this.player.vy>=0&&pl.y+pl.h>=gly-4&&pl.y+pl.h<=gly+12&&this.player.y+this.player.h-this.player.vy<=gly){
this.player.y=gly-this.player.h-2;this.player.vy=0;this.player.onG=true;this.player.dj=true;this.player.tj=true;}}

// Void rescue bars — thick horizontal platforms oscillating far below note zone
for(const vb of this.world.voidBars){
const r=vb.rect(),pl=this.player.rect();
if(this.player.vy>=0&&pl.x+pl.w>r.x&&pl.x<r.x+r.w&&
pl.y+pl.h>=r.y&&pl.y+pl.h<=r.y+r.h+14&&this.player.y+this.player.h-this.player.vy<=r.y+8){
this.player.y=r.y-this.player.h;this.player.vy=0;this.player.onG=true;
this.player.dj=true;this.player.tj=true;this._onVoidBar=vb;
if(this.player.state==='fall'){this.player.sqX=1.15;this.player.sqY=.85;}
break;}}
// Level key platform collision
if(this._levelKey&&!this._levelKey.collected){
const pr=this._levelKey.platRect(),pl=this.player.rect();
if(this.player.vy>=0&&pl.x+pl.w>pr.x&&pl.x<pr.x+pr.w&&
pl.y+pl.h>=pr.y&&pl.y+pl.h<=pr.y+pr.h+12&&this.player.y+this.player.h-this.player.vy<=pr.y+6){
this.player.y=pr.y-this.player.h;this.player.vy=0;this.player.onG=true;this.player.dj=true;this.player.tj=true;}
// Collect the level key when player overlaps with clef symbol area
const kr=this._levelKey.rect();
if(this._ov(this.player.rect(),kr)){
this._levelKey.collected=true;
this.score+=2000;this._fb('LEVEL KEY!','perfect');
this.audio.playCoin();
this.parts.emit(this._levelKey.x,this._levelKey.y,CFG.COLORS.GOLD,40,{sp:8,life:55,sz:5});
this._updateHUD();}}

// Chord barriers — block player from passing through (no modal)
// Each barrier is independent — only collide if player is in the barrier's Y range
this.player._nearBarrier=false;this.player._nearBarrierDist=999;
for(const cb of this.world.chordBarriers){
if(!cb.isBlocking())continue;
const pr=this.player.rect(),br=cb.rect();
// Check BOTH X and Y overlap — barriers on different staves don't interact
const xOverlap=pr.x+pr.w>=br.x&&pr.x<br.x+br.w;
const yOverlap=pr.y+pr.h>=br.y&&pr.y<br.y+br.h;
// Proximity detection for electric aura (within ~120px ahead)
const distToBarrier=br.x-(pr.x+pr.w);
if(distToBarrier>0&&distToBarrier<120&&yOverlap){
this.player._nearBarrier=true;this.player._nearBarrierDist=distToBarrier;}
if(xOverlap&&yOverlap){
this.player.x=br.x-this.player.w-2;this.player.vx=0;this.player.vy=0;
this.player._nearBarrier=true;this.player._nearBarrierDist=0;
this.player._lockedToBarrier=true;
if(this.state==='playing'){
this._activeBarrier=cb;
// Start barrier timer for note blinking hint
if(!cb._lockTime)cb._lockTime=Date.now();}
break;}
if(!xOverlap||!yOverlap){if(cb===this._activeBarrier)this.player._lockedToBarrier=false;}}
// Clear lock flag if no active barrier
if(!this._activeBarrier||!this._activeBarrier.isBlocking())this.player._lockedToBarrier=false;

// Chord groups — land on them and challenge like notes
for(const cg of this.world.chordGroups){
if(!cg.isSolid())continue;
const cgr=cg.rect(),pl=this.player.rect();
// Landing on the chord group's bottom note area
if(this.player.vy>=0&&pl.x+pl.w>cgr.x&&pl.x<cgr.x+cgr.w){
const topNote=cg.maxY;
if(pl.y+pl.h>=topNote-CFG.NOTE_HITBOX_H/2&&pl.y+pl.h<=topNote+12&&
this.player.y+this.player.h-this.player.vy<=topNote-CFG.NOTE_HITBOX_H/2+6){
this.player.y=topNote-CFG.NOTE_HITBOX_H/2-this.player.h;this.player.vy=0;
this.player.onG=true;this.player.dj=true;this.player.tj=true;
if(cg.state==='idle'&&this.state==='playing'){
cg.state='challenged';this._activeChordGroup=cg;this.notesAll++;}
break;}}}

// C Clef enemies — 2 lives damage on contact
for(const ce of this.world.cclefEnemies){
if(!ce.alive)continue;
const cer=ce.rect(),pl=this.player.rect();
if(this._ov(pl,cer)){
if(this.player.vy>0&&pl.y+pl.h-8<cer.y+cer.h/2){
ce.alive=false;this.player.vy=CFG.JUMP*.6;this.score+=200;this.audio.playCoin();
this.parts.emit(cer.x+cer.w/2,cer.y+cer.h/2,'#FF4030',20,{sp:7});
this._fb('C-CLEF CRUSHED! +200','perfect');}
else if(!this.player.inv){
this._hit();if(this.lives>0)this._hit(); // 2nd hit for 2 damage
this._fb('\u{1D121} C-CLEF! -2 LIVES','miss');}}}

// Falling red notes — 1 life damage
for(const fn of this.world.fallingNotes){
if(!fn.alive)continue;
const fr=fn.rect(),pl=this.player.rect();
if(this._ov(pl,fr)){
fn.alive=false;
if(!this.player.inv){this._hit();this._fb('\u266A RED NOTE! -1 LIFE','miss');}
this.parts.emit(fn.x,fn.y,'#FF3020',10,{sp:5,life:25,sz:2});}}
// Golden clef rewards — huge bonus
for(const gc of this.world.goldenClefs){
if(gc.collected)continue;
const gr=gc.rect(),pl=this.player.rect();
if(this._ov(pl,gr)){
gc.collected=true;
this.score+=500;this.lives=Math.min(this.lives+2,CFG.MAX_LIVES);
// Track collectible count
const cType=gc.name.toLowerCase(); // 'treble', 'bass', 'alto'
if(this.clefCounts[cType])this.clefCounts[cType].collected++;
this.audio.playOk();
this.parts.emit(gc.x,gc.getY(),CFG.COLORS.GOLD,30,{sp:8,life:50,sz:5});
this._fb('\u2728 '+gc.name.toUpperCase()+' CLEF! +500 +2\u2665','perfect');}}

// Rest bonuses
for(const rb of this.world.restBonuses){if(rb.collected)continue;
const rr=rb.rect(),pl=this.player.rect();
if(this._ov(pl,rr)){rb.collected=true;this.score+=rb.points;
if(rb.givesLife){this.lives=Math.min(this.lives+1,CFG.MAX_LIVES);this._fb('+LIFE +'+rb.points,'good');}
else this._fb('+'+rb.points,'perfect');
this.audio.playCoin();this.parts.emit(rb.x,rb.y,CFG.COLORS.REST,15,{sp:5,life:30,sz:3});
this._updateHUD();}}

// Enemies
for(const e of this.world.enemies){if(!e.alive)continue;
const er=e.rect(),pl=this.player.rect();
if(this._ov(pl,er)){
if(this.player.vy>0&&pl.y+pl.h-8<er.y+er.h/2){
e.alive=false;this.player.vy=CFG.JUMP*.6;this.score+=50;this.audio.playCoin();
this.parts.emit(er.x+er.w/2,er.y+er.h/2,CFG.COLORS.ENEMY,12,{sp:5});}
else if(!this.player.inv)this._hit();}}

// Collectibles
for(const c of this.world.collectibles){if(c.collected)continue;
const cr=c.rect(),pl=this.player.rect();
if(this._ov(pl,cr)){c.collected=true;this.audio.playCoin();
this.parts.emit(cr.x+cr.w/2,cr.y+cr.h/2,CFG.COLORS.COIN,10,{sp:4,life:30});
if(c.type==='life'){this.lives=Math.min(this.lives+1,CFG.MAX_LIVES);this._fb('+1 LIFE!','good');}
else if(c.type==='fly'){this.player.startFly();this.audio.playFly();this._fb('FLY 3s!','perfect');}
else if(c.type==='hint'){this.noteHintActive=true;this.noteHintTimer=CFG.HINT_DURATION;this._fb('NOTE HINT 10s!','good');}
else{this.score+=CFG.PTS_COLLECT*(1+this.combo*CFG.COMBO_MULT);}
this._updateHUD();}}

// Fall detection — realm-aware: detect when player falls too far below their note zone
const fallLimits=[Mus.noteToY('C',1)+500,Mus.noteToY('C',1)+500,Mus.noteToY('E',1)+800,Mus.noteToY('C',1)+500,Mus.noteToY('A',0)+1200];
const fallLimit=fallLimits[this.realm]||Mus.noteToY('C',1)+500;
if(this.player.y>fallLimit){
this._hit();
if(this.state!=='gameover'){
// Respawn on nearest rest platform in the current visible area (safe, no challenge)
const cx=this.cam.x;
const safeRest=this.world.restPlatforms
  .filter(r=>r.x>cx&&r.x<cx+800)
  .sort((a,b)=>a.x-b.x)[0];
if(safeRest){const sr=safeRest.rect();
  this.player.x=safeRest.x-CFG.PLAYER_W/2;
  this.player.y=sr.y-CFG.PLAYER_H;}
else{
  // Fallback: nearest solid note block
  const solid=this.world.noteBlocks
    .filter(n=>n.isSolid()&&n.x>cx&&n.x<cx+800)
    .sort((a,b)=>a.x-b.x)[0];
  if(solid){const nr=solid.rect();
    this.player.x=solid.x-CFG.PLAYER_W/2;
    this.player.y=nr.y-CFG.PLAYER_H;}
  else{
    // Generate more world and drop onto start of it
    this.world.chunk(Math.max(this.world.genTo,cx+800));
    this.player.x=cx+200;
    this.player.y=-4*CFG.STAFF_SP-CFG.PLAYER_H;}}
this.player.vy=0;this.player.vx=0;this.player.onG=false;}}

// ── Play note ONCE on first landing — not repeated while staying on note ──
if(this.player.lp!==this.lastLandedNote){
this.lastLandedNote=this.player.lp;
if(this.player.lp){const nb2=this.player.lp;this.audio.play(nb2.note,nb2.oct);}}
if(!this.player.onG)this.lastLandedNote=null;

// ── Auto-jump target guidance: soft correction toward target note while in air ──
if(this._autoJumpTarget&&!this.player.onG&&this.player.flyTime<=0){
const tdx=this._autoJumpTarget.x-(this.player.x+this.player.w/2);
if(Math.abs(tdx)>8)this.player.vx+=Math.sign(tdx)*0.10; // gentle homing
}
if(this.player.onG&&this._autoJumpTarget)this._autoJumpTarget=null;

// ── Note attraction: gentle horizontal nudge toward nearest note when falling ──
if(!this.player.onG&&this.player.vy>0.8&&this.player.flyTime<=0){
const px=this.player.x+this.player.w/2,py=this.player.y+this.player.h;
let bestNote=null,bestDist=100;
for(const nb of this.world.noteBlocks){
if(!nb.isSolid())continue;const dx=nb.x-px,dy=nb.y-py;
if(Math.abs(dx)<100&&dy>0&&dy<200){const d=Math.abs(dx);if(d<bestDist){bestDist=d;bestNote=nb;}}}
if(bestNote){const dx=bestNote.x-px;
if(Math.abs(dx)>6)this.player.vx+=Math.sign(dx)*0.09;}}

// Vine check
if(this.state==='playing')this.vine.check(this.player,this.world.noteBlocks);

// Fly timer display
const flyBarWrap=document.getElementById('ll-fly-bar-wrap');
const flyBarFill=document.getElementById('ll-fly-bar-fill');
if(flyBarWrap)flyBarWrap.classList.toggle('active',this.player.flyTime>0);
if(flyBarFill)flyBarFill.style.width=(this.player.flyTime>0?Math.round(this.player.flyTime/CFG.FLY_DURATION*100):0)+'%';

// Note hint display
const hintIndicator=document.getElementById('ll-hint-indicator');
if(hintIndicator)hintIndicator.classList.toggle('active',!!this.noteHintActive);
if(this.noteHintActive)this._updateHintKeys();
if(this._helpHintActive)this._applyHelpHint();

this._updateCam();this.dist=Math.max(this.dist,this.player.x/50);
this.bgMusic.setTempo(this.dist,this.combo);
// Spawn level key once per session after 200m
if(!this._levelKeySpawned&&this.dist>700){
this._levelKeySpawned=true;
this._levelKey=new LevelKey(this.player.x+15000,this.realm);}
// Distance milestones
if(this._milestoneIdx<this._MILESTONES.length&&this.dist>=this._MILESTONES[this._milestoneIdx]){
const km=Math.round(this._MILESTONES[this._milestoneIdx]/1000);
this._milestoneIdx++;this.score+=km*10;
this._fb(km+'km MILESTONE! +'+km*10,'perfect');
this.audio.playCoin();
this.parts.emit(this.cvs.width/2+this.cam.x,this.player.y-60,CFG.COLORS.GOLD,30,{sp:7,life:50,sz:4});
this._updateHUD();}
this.world.clean(this.cam.x);this._updateHUD();}

_highlightKey(noteName){
// Highlight on canvas piano
this.canvasPiano.press(noteName);
setTimeout(()=>this.canvasPiano.release(noteName),500);}

_updateHintKeys(){
if(this.chalNote){
const acc=this.chalNote.accidental;
let noteStr=this.chalNote.note;
if(acc==='sharp')noteStr+='#';
else if(acc==='flat'){
const f2s={Db:'C#',Eb:'D#',Gb:'F#',Ab:'G#',Bb:'A#'};
noteStr=f2s[this.chalNote.note+'b']||this.chalNote.note;}
this._highlightKey(noteStr);}}

_getZoom(){
const base=(this.realm===3||this.realm===4)?0.52:0.62;
// In landscape or wide screens, zoom out more to see more of the staff
const aspect=this.cvs.width/Math.max(1,this.cvs.height);
if(aspect>1.4)return Math.max(0.38,base-0.08); // landscape: zoom out
if(this.cvs.height<400)return Math.max(0.35,base-0.12); // very short screen
return base;}
_updateCam(){
const zoom=this._getZoom();
const tx=this.player.x-this.cvs.width/zoom*0.45+(this.player.fr?30:-30);
const ty=this.player.y-this.cvs.height/zoom*0.38+(this.player.vy>2?20:-10);
this.cam.x+=(tx-this.cam.x)*CFG.CAM_LERP;this.cam.y+=(ty-this.cam.y)*CFG.CAM_LERP;}

_tryNoteInput(n){if(this.chalNote&&this.state==='challenged')this._answer(n);}

_applyHelpHint(){
if(!this._helpHintActive){this.canvasPiano.helpHintKeys={};return;}
// Determine correct note(s) to highlight
const targets=[];
const noteToKey=(note,acc)=>{
let s=note;
if(acc==='sharp')s+='#';
else if(acc==='flat'){const f2s={Db:'C#',Eb:'D#',Gb:'F#',Ab:'G#',Bb:'A#'};s=f2s[note+'b']||note;}
return s;};
if(this.state==='challenged'&&this.chalNote){
targets.push(noteToKey(this.chalNote.note,this.chalNote.accidental));
}else if(this._activeBarrier&&this._activeBarrier.isBlocking()){
this._activeBarrier.chordNotes.forEach(cn=>{
if(!cn.validated)targets.push(noteToKey(cn.note,cn.accidental));});
}else if(this._activeChordGroup&&this._activeChordGroup.state==='challenged'){
this._activeChordGroup.notes.forEach(cn=>{
if(!cn.validated)targets.push(noteToKey(cn.note,cn.accidental));});
}
// Build helpHintKeys map for CanvasPiano
const hk={};
if(targets.length>0){
const PC={C:0,D:2,E:4,F:5,G:7,A:9,B:11};
const toPc=s=>{const base=s.replace(/[#b]/g,'');const acc=s.includes('#')?1:s.includes('b')?-1:0;return((PC[base]||0)+acc+12)%12;};
const targetPcs=targets.map(t=>toPc(t));
const allKeys=['C','D','E','F','G','A','B','C#','D#','F#','G#','A#'];
allKeys.forEach(k=>{if(targetPcs.includes(toPc(k)))hk[k]=true;});}
this.canvasPiano.helpHintKeys=hk;}

_tryRhythmHit(n){
if(!this.world)return;
const PC={C:0,D:2,E:4,F:5,G:7,A:9,B:11};
const toPc=(name)=>{const base=name.replace(/[#b0-9]/g,'');return((PC[base]||0)+(name.includes('#')?1:name.includes('b')?-1:0)+12)%12;};
this.world.rhythmSections.forEach(r=>{
if(r.state==='active'&&r.tryHit(n,toPc)){
this.score+=50;this.combo++;this._fb('BEAT!','perfect');
this.parts.emit(this.player.x+this.player.w/2,this.player.y,CFG.COLORS.GOLD,8,{sp:4,life:20,sz:3});}});}

_answer(n){
if(!this.chalNote||this.state!=='challenged')return;
const nb=this.chalNote;
// Pitch class comparison — handles enharmonic equivalents (C#=Db, F#=Gb, etc.)
const PC={C:0,D:2,E:4,F:5,G:7,A:9,B:11};
const toPc=s=>{const base=s.replace(/[#b]/g,'');const acc=s.includes('#')?1:s.includes('b')?-1:0;return((PC[base]||0)+acc+12)%12;};
const nbPc=toPc(nb.note+(nb.accidental==='sharp'?'#':nb.accidental==='flat'?'b':''));
const ok=toPc(n)===nbPc;
this.audio.play(n.replace('#',''),nb.oct);
if(ok){nb.state='correct';nb.identified=true;
this.score+=CFG.PTS_NOTE*(1+this.combo*CFG.COMBO_MULT);
this.combo++;this.maxCombo=Math.max(this.maxCombo,this.combo);this.notesOk++;
this.audio.playOk();this.parts.emit(nb.x,nb.y,CFG.COLORS.GOLD,15,{sp:6,life:35,sz:3});
this._fb('PERFECT!','perfect');
if(this.combo>0&&this.combo%5===0){this.lives=Math.min(this.lives+1,CFG.MAX_LIVES);this._fb('+1 LIFE!','good');}
// NO auto-jump — player stays on note and chooses when to move
document.querySelectorAll('.ll-key.hint-active').forEach(k=>k.classList.remove('hint-active'));
this.canvasPiano.helpHintKeys={};
this._helpHintActive=false;
// Flash the pressed key GOLD
document.querySelectorAll('.ll-key').forEach(k=>{if(k.dataset.note===n){k.classList.add('key-correct');setTimeout(()=>k.classList.remove('key-correct'),500);}});
this.state='playing';this.chalNote=null;this._updateHUD();
}else{nb.state='wrong';nb.crumT=0;this.combo=0;this.audio.playBad();
this.parts.emit(nb.x,nb.y,CFG.COLORS.ENEMY,10,{sp:4});this._fb('WRONG!','miss');
// Flash the pressed key RED
document.querySelectorAll('.ll-key').forEach(k=>{if(k.dataset.note===n){k.classList.add('key-wrong');setTimeout(()=>k.classList.remove('key-wrong'),600);}});
// Stay blocked — flash red for 600ms then revert to challenged state
setTimeout(()=>{if(nb.state==='wrong'){nb.state='challenged';nb.crumT=0;}},600);}}

_midiNote(n){
if(this.state==='challenged')this._answer(n.full||n.note);
else if(this.state==='playing'){
if(this._activeBarrier&&this._activeBarrier.isBlocking()){
this._tryBarrierNote(n.full||n.note);
}else if(this._activeChordGroup&&this._activeChordGroup.state==='challenged'){
this._tryChordGroupNote(n.full||n.note);
}else if(this.vine.state==='available'){
if(this.vine.tryAnswer(n.note,this.player)){this.audio.playVine();
this.parts.emit(this.player.x+this.player.w/2,this.player.y,CFG.COLORS.VINE,12,{sp:5,life:30});}}
else{this._tryAutoJump(n.full||n.note);this._tryNoteInput(n.full||n.note);this._tryRhythmHit(n.full||n.note);}}}

_hit(){if(this.player.inv)return;this.lives--;this.combo=0;
this.player.inv=true;this.player.invT=CFG.INVINCE;this.audio.playBad();
this.parts.emit(this.player.x+this.player.w/2,this.player.y+this.player.h/2,CFG.COLORS.ENEMY,15,{sp:6});
const app=document.getElementById('ll-app');
if(app){app.classList.add('ll-shake');setTimeout(()=>app.classList.remove('ll-shake'),400);}
this._updateHUD();if(this.lives<=0)this._over();}

_ov(a,b){return a.x<b.x+b.w&&a.x+a.w>b.x&&a.y<b.y+b.h&&a.y+a.h>b.y;}

_draw(){
const ctx=this.ctx,W=this.cvs.width,H=this.cvs.height;
ctx.clearRect(0,0,W,H);
// Zoom-out: camera always zoomed out to see lots of notes and staff
const zoom=this._getZoom();
const VW=W/zoom,VH=H/zoom;
ctx.save();ctx.scale(zoom,zoom);
this.bg.draw(ctx,this.cam.x,this.cam.y,VW,VH,this.realm);
if(this.state==='welcome'){this.bg.drawClefs(ctx,this.cam.y,VH,this.realm);ctx.restore();return;}
this.glowLine.draw(ctx,this.cam.x,this.cam.y,VW);
this.world.voidBars.forEach(b=>b.draw(ctx,this.cam.x,this.cam.y));
this.world.restBonuses.forEach(r=>r.draw(ctx,this.cam.x,this.cam.y));
if(this._levelKey&&!this._levelKey.collected)this._levelKey.draw(ctx,this.cam.x,this.cam.y);
this.world.restPlatforms.forEach(r=>r.draw(ctx,this.cam.x,this.cam.y));
this.world.noteBlocks.forEach(n=>n.draw(ctx,this.cam.x,this.cam.y));
this.world.beamPairs.forEach(b=>b.draw(ctx,this.cam.x,this.cam.y));
this.world.chordBarriers.forEach(b=>b.draw(ctx,this.cam.x,this.cam.y));
this.world.chordGroups.forEach(g=>g.draw(ctx,this.cam.x,this.cam.y));
this.world.enemies.forEach(e=>e.draw(ctx,this.cam.x,this.cam.y));
this.world.cclefEnemies.forEach(e=>e.draw(ctx,this.cam.x,this.cam.y));
this.world.fallingNotes.forEach(f=>f.draw(ctx,this.cam.x,this.cam.y));
this.world.goldenClefs.forEach(gc=>gc.draw(ctx,this.cam.x,this.cam.y));
this.world.collectibles.forEach(c=>c.draw(ctx,this.cam.x,this.cam.y));
this.world.speedZones.forEach(z=>z.draw(ctx,this.cam.x,this.cam.y));
this.world.gravFlipZones.forEach(z=>z.draw(ctx,this.cam.x,this.cam.y));
this.world.rhythmSections.forEach(r=>r.draw(ctx,this.cam.x,this.cam.y));
this.vine.draw(ctx,this.cam.x,this.cam.y,this.player,this.difficulty!=='pianist');
if(this.player)this.player.draw(ctx,this.cam.x,this.cam.y);
this.noteTrail.draw(ctx,this.cam.x,this.cam.y);
this.parts.draw(ctx,this.cam.x,this.cam.y);
// Sticky clefs drawn LAST (on top of everything)
this.bg.drawClefs(ctx,this.cam.y,VH,this.realm);
this._drawGuideMessages(ctx,VW,VH);
ctx.restore();
// Draw HUD and piano overlay in screen coords (after zoom restore)
if(this.state==='playing'||this.state==='challenged'){
this._drawCanvasHUD(ctx,W,H);
this.canvasPiano.draw(ctx,W,H,CFG.latin,this.difficulty==='pianist');}}

_drawGuideMessages(ctx,W,H){
const t=Date.now();ctx.save();ctx.textAlign='center';
const zoom=this._getZoom();
const isMobMsg=W<500/zoom;
const hudPx=isMobMsg?76:52; // must match _drawCanvasHUD hudH
const msgPad=isMobMsg?14:28; // more padding on desktop for better visibility
const msgY=Math.round(hudPx/zoom)+msgPad; // well below the HUD bar on all platforms
const msgScale=isMobMsg?1:1.2; // larger text on desktop
if(this.state==='challenged'&&this.chalNote){
const ms=msgScale;
const bw=Math.round(380*ms),bh=Math.round((this._helpHintActive?78:62)*ms);
ctx.fillStyle='rgba(5,5,15,.82)';
rrect(ctx,W/2-bw/2,msgY-12,bw,bh,12);ctx.fill();
ctx.strokeStyle=this._helpHintActive?'#50C878':CFG.COLORS.GOLD;ctx.lineWidth=1.5;
rrect(ctx,W/2-bw/2,msgY-12,bw,bh,12);ctx.stroke();
ctx.font='bold '+Math.round(24*ms)+'px Montserrat,sans-serif';ctx.fillStyle=CFG.COLORS.GOLD;
ctx.shadowColor=CFG.COLORS.GOLD;ctx.shadowBlur=18;
ctx.fillText('IDENTIFY THE NOTE!',W/2,msgY+Math.round(20*ms));ctx.shadowBlur=0;
if(this._helpHintActive){
ctx.font='bold '+Math.round(14*ms)+'px Montserrat,sans-serif';ctx.fillStyle='#50C878';
ctx.shadowColor='#50C878';ctx.shadowBlur=8;
ctx.fillText('\u25B6 PUSH THE GREEN KEY TO MOVE FORWARD',W/2,msgY+Math.round(40*ms));ctx.shadowBlur=0;
ctx.font='500 '+Math.round(11*ms)+'px Montserrat,sans-serif';ctx.fillStyle='rgba(80,200,120,.7)';
ctx.fillText('Look at the highlighted key on the piano below!',W/2,msgY+Math.round(56*ms));
}else{
ctx.font='500 '+Math.round(12*ms)+'px Montserrat,sans-serif';ctx.fillStyle='rgba(255,255,255,.6)';
ctx.fillText('Piano  \u00B7  Keys A-J  \u00B7  MIDI',W/2,msgY+Math.round(40*ms));}
if(this.noteHintActive&&this.chalNote&&!this._helpHintActive){
ctx.font='bold '+Math.round(12*ms)+'px Montserrat,sans-serif';ctx.fillStyle='rgba(200,150,255,.9)';
ctx.fillText('HINT: look at the piano below!',W/2,msgY+Math.round(56*ms));}
}else if(this.state==='playing'&&this.vine.state==='available'){
const pulse=Math.sin(t*.005)*.3+.7;
ctx.fillStyle=`rgba(20,15,0,${.82*pulse})`;
rrect(ctx,W/2-210,msgY-12,420,48,10);ctx.fill();
ctx.strokeStyle=`rgba(255,215,0,${pulse})`;ctx.lineWidth=2;
rrect(ctx,W/2-210,msgY-12,420,48,10);ctx.stroke();
ctx.font='bold 17px Montserrat,sans-serif';ctx.fillStyle=`rgba(255,215,0,${pulse})`;
ctx.shadowColor='#FFD700';ctx.shadowBlur=12;
ctx.fillText('\u26A1 TYPE A NOTE ABOVE — GRAB A VINE!',W/2,msgY+12);ctx.shadowBlur=0;
ctx.font='500 10px Montserrat,sans-serif';ctx.fillStyle=`rgba(255,215,0,${pulse*.7})`;
ctx.fillText('Keys A–J  ·  Piano  ·  MIDI',W/2,msgY+28);
}else if(this.state==='playing'&&this._activeBarrier&&this._activeBarrier.isBlocking()){
const pulse=Math.sin(t*.006)*.3+.7;
const bw2=Math.round(400*msgScale),bh2=Math.round(56*msgScale);
ctx.fillStyle=`rgba(30,5,5,${.85*pulse})`;
rrect(ctx,W/2-bw2/2,msgY-12,bw2,bh2,10);ctx.fill();
ctx.strokeStyle=`rgba(255,80,60,${pulse})`;ctx.lineWidth=2;
rrect(ctx,W/2-bw2/2,msgY-12,bw2,bh2,10);ctx.stroke();
ctx.font='bold '+Math.round(20*msgScale)+'px Montserrat,sans-serif';ctx.fillStyle=`rgba(255,80,60,${pulse})`;
ctx.shadowColor='#FF4444';ctx.shadowBlur=12;
ctx.fillText('\u26A0 PLAY THE CHORD TO PASS!',W/2,msgY+Math.round(16*msgScale));ctx.shadowBlur=0;
if(this._helpHintActive){
ctx.font='bold '+Math.round(12*msgScale)+'px Montserrat,sans-serif';ctx.fillStyle='#50C878';
ctx.fillText('\u25B6 Push the green keys on the piano!',W/2,msgY+Math.round(34*msgScale));
}else{
ctx.font='500 '+Math.round(11*msgScale)+'px Montserrat,sans-serif';ctx.fillStyle=`rgba(255,150,130,${pulse*.7})`;
ctx.fillText('Identify each note of the chord on the barrier',W/2,msgY+Math.round(34*msgScale));}
}else if(this.state==='playing'&&this._speedMult>1){
const pulse=Math.sin(t*.005)*.3+.7;
ctx.fillStyle='rgba(20,15,0,.7)';rrect(ctx,W/2-130,msgY,260,30,8);ctx.fill();
ctx.font='bold 14px Montserrat,sans-serif';ctx.fillStyle=`rgba(215,191,129,${pulse})`;
ctx.fillText('\u00BB SPEED ZONE \u00D7'+this._speedMult.toFixed(1),W/2,msgY+20);
}else if(this.state==='playing'&&this._gravFlip){
const pulse=Math.sin(t*.005)*.4+.6;
ctx.fillStyle='rgba(15,5,25,.7)';rrect(ctx,W/2-130,msgY,260,30,8);ctx.fill();
ctx.font='bold 14px Montserrat,sans-serif';ctx.fillStyle=`rgba(180,120,255,${pulse})`;
ctx.fillText('\u21C5 GRAVITY FLIP!',W/2,msgY+20);
}else if(this.state==='playing'&&this.world&&this.world.rhythmSections.some(r=>r.state==='active')){
const pulse=Math.sin(t*.006)*.3+.7;
ctx.fillStyle='rgba(20,15,0,.7)';rrect(ctx,W/2-150,msgY,300,42,8);ctx.fill();
ctx.strokeStyle=`rgba(215,191,129,${pulse*.5})`;ctx.lineWidth=1;
rrect(ctx,W/2-150,msgY,300,42,8);ctx.stroke();
ctx.font='bold 15px Montserrat,sans-serif';ctx.fillStyle=`rgba(215,191,129,${pulse})`;
ctx.fillText('\u266A PLAY NOTES ON THE BEAT!',W/2,msgY+16);
const rs=this.world.rhythmSections.find(r=>r.state==='active');
if(rs){ctx.font='500 10px Montserrat,sans-serif';ctx.fillStyle=`rgba(215,191,129,${pulse*.6})`;
ctx.fillText(rs.hitCount+'/'+rs.totalBeats+' hits \u2022 '+rs.bpm+' BPM',W/2,msgY+32);}
}else if(this.state==='playing'){
const pulse=Math.sin(t*.0018)*.25+.75;
ctx.fillStyle='rgba(0,0,0,.38)';rrect(ctx,W/2-118,msgY,236,30,8);ctx.fill();
ctx.font='700 13px Montserrat,sans-serif';
ctx.fillStyle=`rgba(215,191,129,${pulse})`;ctx.fillText('\u2192  ADVANCE  \u2192',W/2,msgY+20);
if(this.combo>=3){ctx.font='600 11px Montserrat,sans-serif';ctx.fillStyle='rgba(255,160,50,.88)';
ctx.fillText('COMBO \xD7'+(1+this.combo*CFG.COMBO_MULT).toFixed(1)+'!',W/2,msgY+40);}
if(this.player?.flyTime>0){
ctx.font='bold 13px Montserrat,sans-serif';ctx.fillStyle='rgba(140,230,255,.9)';
ctx.fillText('\u2708 FLY MODE  '+(Math.ceil(this.player.flyTime/1000))+'s',W/2,msgY+58);}}
ctx.restore();}

_drawCanvasHUD(ctx,W,H){
ctx.save();
const isMob=W<500;const isTab=W>=500&&W<900;
const hudH=isMob?76:52; // two rows on mobile, taller on desktop
const isFs=this._pseudoFs||document.fullscreenElement||document.webkitFullscreenElement;
// Bar background
ctx.fillStyle='rgba(10,10,18,.92)';
ctx.fillRect(0,0,W,hudH);
ctx.fillStyle='rgba(215,191,129,.4)';ctx.fillRect(0,hudH-1,W,1);
ctx.textBaseline='middle';
const row1Y=isMob?20:hudH/2;
const row2Y=isMob?56:hudH/2;
const cy=row1Y;
const margin=isMob?8:16; // margin from edges
// Helper: draw separator
const sep=(x)=>{const sepY=isMob?4:10;const sepH=isMob?30:hudH-20;ctx.fillStyle='rgba(255,255,255,.1)';ctx.fillRect(x,sepY,1,sepH);};
// ═══════════ LEFT SECTION (row 1) ═══════════
let xL=margin;
// Back arrow
const fSz=isMob?14:18;const fSzL=isMob?12:16;const fSzS=isMob?10:13;const fSzXS=isMob?9:11;
ctx.fillStyle='#D7BF81';ctx.font='bold '+fSz+'px Montserrat,sans-serif';
ctx.textAlign='left';ctx.fillText('\u2190',xL,cy);
this._hudBackBtn={x:0,y:0,w:margin+24,h:hudH};xL+=isMob?22:28;
sep(xL);xL+=8;
// Lives
for(let i=0;i<Math.min(this.lives,CFG.MAX_LIVES);i++){
ctx.fillStyle='#E84040';ctx.font=(isMob?'11':'14')+'px sans-serif';ctx.fillText('\u2665',xL,cy);xL+=isMob?12:15;}
xL+=6;
// Score
ctx.fillStyle='#D7BF81';ctx.font='bold '+fSzL+'px Montserrat,sans-serif';
ctx.fillText(Math.round(this.score),xL,cy);
xL+=ctx.measureText(Math.round(this.score)+'').width+8;
// Combo
if(this.combo>0){
ctx.fillStyle=this.combo>=5?'#FF6020':this.combo>=3?'#FF9030':'#CCC';
ctx.font='bold '+fSzS+'px Montserrat,sans-serif';
ctx.fillText('x'+this.combo,xL,cy);xL+=ctx.measureText('x'+this.combo).width+8;}
sep(xL);xL+=8;
// Distance
ctx.fillStyle='#999';ctx.font='bold '+fSzS+'px Montserrat,sans-serif';
ctx.fillText(Math.round(this.dist)+'m',xL,cy);
xL+=ctx.measureText(Math.round(this.dist)+'m').width+8;
sep(xL);xL+=8;
// Clef collectibles
const cc=this.clefCounts;
ctx.font='bold '+fSzXS+'px Montserrat,sans-serif';
// Treble
ctx.fillStyle='#D7BF81';
const t1='\u{1D11E} '+cc.treble.collected+'/'+cc.treble.total;
ctx.fillText(t1,xL,cy);xL+=ctx.measureText(t1).width+6;
// Bass
const t2='\u{1D122} '+cc.bass.collected+'/'+cc.bass.total;
ctx.fillText(t2,xL,cy);xL+=ctx.measureText(t2).width+6;
// Alto
const t3='\u{1D121} '+cc.alto.collected+'/'+cc.alto.total;
ctx.fillText(t3,xL,cy);xL+=ctx.measureText(t3).width+8;
// Realm name (desktop/tablet)
if(!isMob){
sep(xL);xL+=8;
const rn=CFG.REALMS[this.realm];
if(rn){ctx.fillStyle=rn.color||'#AAA';ctx.font='bold 10px Montserrat,sans-serif';
ctx.fillText(rn.name,xL,cy);xL+=ctx.measureText(rn.name).width+8;}}
// Help (?) button — golden, left side near realm info
sep(xL);xL+=isMob?6:10;
const helpBtnSz=isMob?13:16;
const helpBtnW=helpBtnSz+10;
const helpBtnH=helpBtnSz+6;
const helpBtnX=xL;
const helpBtnY=cy-helpBtnH/2;
ctx.save();
if(this._helpHintActive){
ctx.fillStyle='rgba(50,200,80,.3)';
rrect(ctx,helpBtnX,helpBtnY,helpBtnW,helpBtnH,5);ctx.fill();
ctx.strokeStyle='#50C878';ctx.lineWidth=2;
rrect(ctx,helpBtnX,helpBtnY,helpBtnW,helpBtnH,5);ctx.stroke();
ctx.fillStyle='#50C878';ctx.shadowColor='#50C878';ctx.shadowBlur=8;
}else{
ctx.fillStyle='rgba(215,191,129,.12)';
rrect(ctx,helpBtnX,helpBtnY,helpBtnW,helpBtnH,5);ctx.fill();
ctx.strokeStyle='rgba(215,191,129,.5)';ctx.lineWidth=1.2;
rrect(ctx,helpBtnX,helpBtnY,helpBtnW,helpBtnH,5);ctx.stroke();
ctx.fillStyle='#D7BF81';ctx.shadowColor='#D7BF81';ctx.shadowBlur=3;}
ctx.font='bold '+helpBtnSz+'px Montserrat,sans-serif';
ctx.textAlign='center';ctx.textBaseline='middle';
ctx.fillText('?',helpBtnX+helpBtnW/2,cy);
ctx.shadowBlur=0;ctx.restore();
this._hudHelpBtn={x:helpBtnX-2,y:0,w:helpBtnW+4,h:hudH};
xL+=helpBtnW+6;
// Row divider on mobile
if(isMob){ctx.fillStyle='rgba(215,191,129,.25)';ctx.fillRect(0,38,W,1);}
// ═══════════ RIGHT SECTION (row 2 on mobile) ═══════════
const sp=isMob?14:20;
const rcy=row2Y;
ctx.textAlign='center';
let rx=W-margin;
// Fullscreen
const fsS=14;const fsX=rx-fsS,fsY=rcy-fsS/2;
ctx.strokeStyle='#D7BF81';ctx.lineWidth=1.8;
if(isFs){
ctx.beginPath();ctx.moveTo(fsX+2,fsY+2);ctx.lineTo(fsX+fsS-2,fsY+fsS-2);ctx.stroke();
ctx.beginPath();ctx.moveTo(fsX+fsS-2,fsY+2);ctx.lineTo(fsX+2,fsY+fsS-2);ctx.stroke();
}else{
ctx.beginPath();ctx.moveTo(fsX,fsY+4);ctx.lineTo(fsX,fsY);ctx.lineTo(fsX+4,fsY);ctx.stroke();
ctx.beginPath();ctx.moveTo(fsX+fsS-4,fsY);ctx.lineTo(fsX+fsS,fsY);ctx.lineTo(fsX+fsS,fsY+4);ctx.stroke();
ctx.beginPath();ctx.moveTo(fsX,fsY+fsS-4);ctx.lineTo(fsX,fsY+fsS);ctx.lineTo(fsX+4,fsY+fsS);ctx.stroke();
ctx.beginPath();ctx.moveTo(fsX+fsS-4,fsY+fsS);ctx.lineTo(fsX+fsS,fsY+fsS);ctx.lineTo(fsX+fsS,fsY+fsS-4);ctx.stroke();}
this._hudFsBtn={x:fsX-6,y:0,w:fsS+12,h:hudH};
rx-=fsS+sp;
// Pause
ctx.fillStyle='#D7BF81';
ctx.fillRect(rx-5,rcy-6,3,12);ctx.fillRect(rx,rcy-6,3,12);
this._hudPauseBtn={x:rx-10,y:0,w:16,h:hudH};
rx-=12+sp;
// Notation (A/Do)
ctx.font='bold '+(isMob?11:13)+'px Montserrat,sans-serif';ctx.fillStyle='#D7BF81';
const notLabel=CFG.latin?'Do':'ABC';
ctx.fillText(notLabel,rx,rcy);
const notW=ctx.measureText(notLabel).width;
this._hudNotationBtn={x:rx-notW/2-6,y:0,w:notW+12,h:hudH};
rx-=notW/2+sp+12;
if(!isMob){sep(rx+6);rx-=6;}
// Mode (Easy/Pro)
const modeLabel=this.difficulty==='pianist'?'PRO':'EASY';
ctx.font='bold '+(isMob?10:12)+'px Montserrat,sans-serif';
const modeCol=this.difficulty==='pianist'?'#E6735C':'#50C878';
const modeW=ctx.measureText(modeLabel).width;
ctx.save();
ctx.fillStyle=this.difficulty==='pianist'?'rgba(230,115,92,.18)':'rgba(80,200,120,.18)';
rrect(ctx,rx-modeW/2-5,rcy-9,modeW+10,18,5);ctx.fill();
ctx.strokeStyle=modeCol;ctx.lineWidth=1;
rrect(ctx,rx-modeW/2-5,rcy-9,modeW+10,18,5);ctx.stroke();
ctx.restore();
ctx.fillStyle=modeCol;
ctx.fillText(modeLabel,rx,rcy);
this._hudModeBtn={x:rx-modeW/2-6,y:0,w:modeW+12,h:hudH};
rx-=modeW/2+sp+6;
// Account (desktop/tablet)
if(!isMob){
ctx.font='bold 10px Montserrat,sans-serif';ctx.fillStyle='rgba(215,191,129,.7)';
ctx.fillText('\u2606',rx,rcy);
this._hudAccountBtn={x:rx-10,y:0,w:20,h:hudH};
rx-=10+sp;}
// Mute
ctx.textAlign='left';
const vol=this._volume!==undefined?this._volume:0.7;
const vx=rx,vy=rcy;
ctx.strokeStyle=this.audio.muted?'rgba(215,191,129,.4)':'#D7BF81';ctx.lineWidth=1.5;
ctx.beginPath();ctx.moveTo(vx-2,vy-4);ctx.lineTo(vx-6,vy-4);ctx.lineTo(vx-9,vy-7);
ctx.lineTo(vx-9,vy+7);ctx.lineTo(vx-6,vy+4);ctx.lineTo(vx-2,vy+4);ctx.closePath();ctx.stroke();
if(!this.audio.muted){
ctx.beginPath();ctx.arc(vx,vy,5,-.6,.6);ctx.stroke();
if(vol>0.3){ctx.beginPath();ctx.arc(vx,vy,9,-.5,.5);ctx.stroke();}
}else{ctx.beginPath();ctx.moveTo(vx+1,vy-5);ctx.lineTo(vx+7,vy+5);ctx.stroke();}
this._hudMuteBtn={x:vx-14,y:0,w:22,h:hudH};
rx-=16+sp;
// Volume slider (desktop only)
if(!isMob){
const slW=50,slH=3,slX=rx-slW,slY=rcy-1;
ctx.fillStyle='rgba(215,191,129,.18)';
rrect(ctx,slX,slY,slW,slH,2);ctx.fill();
ctx.fillStyle='#D7BF81';
rrect(ctx,slX,slY,slW*vol,slH,2);ctx.fill();
ctx.shadowColor='rgba(215,191,129,.5)';ctx.shadowBlur=4;
ctx.fillStyle='#D7BF81';
ctx.beginPath();ctx.arc(slX+slW*vol,slY+slH/2,4,0,Math.PI*2);ctx.fill();
ctx.shadowBlur=0;
ctx.fillStyle='#0A0A18';
ctx.beginPath();ctx.arc(slX+slW*vol,slY+slH/2,1.5,0,Math.PI*2);ctx.fill();
this._hudVolSlider={x:slX,y:slY-8,w:slW,h:20};}
else{this._hudVolSlider=null;}
ctx.textAlign='left';
ctx.restore();}

_handleHUDClick(canvasX,canvasY){
// Back to menu
if(this._hudBackBtn&&canvasX<this._hudBackBtn.w&&canvasY<this._hudBackBtn.h){
this.bgMusic.stop();this.state='welcome';this.chalNote=null;
this._helpHintActive=false;
this.canvasPiano.helpHintKeys={};
const $=id=>document.getElementById(id);
$('ll-gameover-overlay')?.classList.add('hidden');
$('ll-welcome-overlay')?.classList.remove('hidden');
this._setWelcomeMode(true);this._updateRealmCards();
const lsb=$('ll-load-save-btn');if(lsb)lsb.style.display=this._hasSavedGame()?'inline-block':'none';
return true;}
// Help (?) toggle
if(this._hudHelpBtn&&canvasX>=this._hudHelpBtn.x&&canvasX<=this._hudHelpBtn.x+this._hudHelpBtn.w&&canvasY<this._hudHelpBtn.h){
this._helpHintActive=!this._helpHintActive;
if(this._helpHintActive){this._applyHelpHint();}
else{this.canvasPiano.helpHintKeys={};}
return true;}
// Fullscreen toggle
if(this._hudFsBtn&&canvasX>=this._hudFsBtn.x&&canvasX<=this._hudFsBtn.x+this._hudFsBtn.w&&canvasY<this._hudFsBtn.h){
this._toggleFullscreen();return true;}
// Pause button
if(this._hudPauseBtn&&canvasX>=this._hudPauseBtn.x&&canvasX<=this._hudPauseBtn.x+this._hudPauseBtn.w&&canvasY<this._hudPauseBtn.h){
this._pause();return true;}
// Notation toggle
if(this._hudNotationBtn&&canvasX>=this._hudNotationBtn.x&&canvasX<=this._hudNotationBtn.x+this._hudNotationBtn.w&&canvasY<this._hudNotationBtn.h){
CFG.latin=!CFG.latin;
const lbl=document.getElementById('ll-notation-label');if(lbl)lbl.textContent=CFG.latin?'Do·Ré':'A·B·C';
return true;}
// Mode toggle (Easy/Pianist)
if(this._hudModeBtn&&canvasX>=this._hudModeBtn.x&&canvasX<=this._hudModeBtn.x+this._hudModeBtn.w&&canvasY<this._hudModeBtn.h){
this.difficulty=this.difficulty==='pianist'?'easy':'pianist';
// Update note label visibility on existing notes
const showL=this.difficulty==='easy';
this.world.noteBlocks.forEach(n=>{n.showLabel=showL;});
this.world.beamPairs.forEach(b=>{b.showLabel=showL;});
this.world.chordGroups.forEach(g=>{g.showLabel=showL;});
return true;}
// Account button
if(this._hudAccountBtn&&canvasX>=this._hudAccountBtn.x&&canvasX<=this._hudAccountBtn.x+this._hudAccountBtn.w&&canvasY<this._hudAccountBtn.h){
// Navigate to account page if available
if(window.pmAccountUrl){window.location.href=window.pmAccountUrl;}
return true;}
// Mute toggle
if(this._hudMuteBtn&&canvasX>=this._hudMuteBtn.x&&canvasX<=this._hudMuteBtn.x+this._hudMuteBtn.w&&canvasY<this._hudMuteBtn.h){
this.audio.toggle();this.bgMusic.toggle();return true;}
// Volume slider
if(this._hudVolSlider&&canvasX>=this._hudVolSlider.x&&canvasX<=this._hudVolSlider.x+this._hudVolSlider.w&&canvasY>=this._hudVolSlider.y&&canvasY<=this._hudVolSlider.y+this._hudVolSlider.h){
const vol=Math.max(0,Math.min(1,(canvasX-this._hudVolSlider.x)/this._hudVolSlider.w));
this._volume=vol;this._setVolume(vol);return true;}
return false;}

_toggleFullscreen(){
const app=document.getElementById('ll-app');
const isFs=document.fullscreenElement||document.webkitFullscreenElement||this._pseudoFs;
if(!isFs){
// Try native fullscreen first
const req=app.requestFullscreen||app.webkitRequestFullscreen||app.mozRequestFullScreen||app.msRequestFullscreen;
if(req){try{req.call(app).then(()=>{app.classList.add('fullscreen');}).catch(()=>{
this._pseudoFs=true;app.classList.add('fullscreen');this._applyPseudoFs(app,true);});}
catch(e){this._pseudoFs=true;app.classList.add('fullscreen');this._applyPseudoFs(app,true);}}
else{this._pseudoFs=true;app.classList.add('fullscreen');this._applyPseudoFs(app,true);}
}else{
if(this._pseudoFs){this._pseudoFs=false;app.classList.remove('fullscreen');this._applyPseudoFs(app,false);}
else{const exit=document.exitFullscreen||document.webkitExitFullscreen||document.mozCancelFullScreen||document.msExitFullscreen;
if(exit)try{exit.call(document);}catch(e){}
app.classList.remove('fullscreen');}}
setTimeout(()=>this._resize(),150);}

_applyPseudoFs(app,on){
if(on){
app.style.position='fixed';app.style.top='0';app.style.left='0';
app.style.width='100vw';app.style.height='100vh';app.style.height='100dvh';
app.style.zIndex='99999';app.style.marginTop='0';
document.documentElement.style.setProperty('--ll-total-offset','0px');
document.body.style.overflow='hidden';
// Scroll to top to hide Safari address bar
window.scrollTo(0,1);
}else{
app.style.position='';app.style.top='';app.style.left='';
app.style.width='';app.style.height='';
app.style.zIndex='';app.style.marginTop='';
document.body.style.overflow='';
// Re-measure header
const h=document.querySelector('.piano-header')||document.querySelector('header');
if(h){const hh=Math.round(h.getBoundingClientRect().height);
app.style.marginTop=hh+'px';
document.documentElement.style.setProperty('--ll-total-offset',hh+'px');}}}

_setVolume(v){
if(this._volume===undefined)this._volume=0.7;
this._volume=v;
try{if(Tone.getDestination)Tone.getDestination().volume.value=v<=0?-Infinity:-40+v*40;}catch(e){}}

_tryBarrierNote(noteName){
if(!this._activeBarrier||!this._activeBarrier.isBlocking())return;
const ok=this._activeBarrier.tryNote(noteName);
if(ok){
this.audio.playOk();this.score+=100;this.combo++;
this.maxCombo=Math.max(this.maxCombo,this.combo);
this.parts.emit(this._activeBarrier.x,this.player.y,CFG.COLORS.GOLD,8,{sp:4,life:25,sz:2});
this._fb('NOTE OK! '+this._activeBarrier.validatedCount+'/'+this._activeBarrier.chordNotes.length,'good');
// All notes validated — barrier dissolves
if(this._activeBarrier.allValidated()){
this.score+=300;
this.parts.emit(this._activeBarrier.x,this.player.y-40,CFG.COLORS.GOLD,30,{sp:8,life:40,sz:4});
this._fb('BARRIER BROKEN! +300','perfect');
this._activeBarrier=null;
this._helpHintActive=false;
this.canvasPiano.helpHintKeys={};
}else if(this._helpHintActive){this._applyHelpHint();}
this._updateHUD();
}else{
this.audio.playBad();this.combo=0;this._fb('WRONG NOTE!','miss');}}

_tryChordGroupNote(noteName){
if(!this._activeChordGroup||this._activeChordGroup.state!=='challenged')return;
const cg=this._activeChordGroup;
const ok=cg.tryNote(noteName);
if(ok){
this.audio.playOk();this.score+=80;this.combo++;
this.maxCombo=Math.max(this.maxCombo,this.combo);
this._fb('NOTE '+cg.validatedCount+'/'+cg.notes.length,'good');
if(cg.allValidated()){
this.score+=200;this.notesOk++;
this.parts.emit(cg.x,cg.minY,CFG.COLORS.GOLD,20,{sp:6,life:35,sz:3});
this._fb('CHORD COMPLETE! +200','perfect');
this._activeChordGroup=null;
this._helpHintActive=false;
this.canvasPiano.helpHintKeys={};
}else if(this._helpHintActive){this._applyHelpHint();}
this._updateHUD();
}else{this.audio.playBad();this.combo=0;this._fb('WRONG!','miss');}}

_fb(txt,type){const el=document.getElementById('ll-feedback');if(!el)return;
el.textContent=txt;el.className='ll-feedback '+type+' show';setTimeout(()=>{el.className='ll-feedback';},700);}

_updateHUD(){const $=id=>document.getElementById(id);
if($('ll-score'))$('ll-score').textContent=Math.round(this.score);
if($('ll-best'))$('ll-best').textContent=this.best;
if($('ll-combo'))$('ll-combo').textContent=this.combo;
if($('ll-distance'))$('ll-distance').textContent=Math.round(this.dist)+'m';
const lv=$('ll-lives');if(lv){let h='';for(let i=0;i<CFG.MAX_LIVES;i++){
if(i<this.lives)h+='<span class="ll-life">\u2665</span>';
else if(i<CFG.LIVES)h+='<span class="ll-life lost">\u2665</span>';}lv.innerHTML=h;}
if($('ll-realm-name')){const r=CFG.REALMS[this.realm];$('ll-realm-name').textContent=r?r.name:'';}
if($('ll-diff-display'))$('ll-diff-display').textContent=this.difficulty==='easy'?'EASY':'PIANIST';
const mb=$('ll-midi-badge');if(mb&&this.midi.on){mb.classList.add('connected');
const lb=mb.querySelector('.ll-midi-label');if(lb)lb.textContent=this.midi.dev;}}

_updateRealmCards(){
document.querySelectorAll('.ll-realm-card').forEach(c=>{
const r=parseInt(c.dataset.realm);
const info=c.querySelector('.ll-realm-info');
if(this.unlocks[r]){c.classList.remove('locked');
const lo=c.querySelector('.ll-realm-lock');if(lo)lo.remove();
}else{c.classList.add('locked');c.classList.remove('active');
if(!c.querySelector('.ll-realm-lock')&&info){
const d1=Math.min(this.realmBestDist[1]||0,this.UNLOCK_DIST);
const d2=Math.min(this.realmBestDist[2]||0,this.UNLOCK_DIST);
const lo=document.createElement('div');lo.className='ll-realm-lock';
lo.textContent='\uD83D\uDD12 Attic '+d1+'/'+this.UNLOCK_DIST+'m & Basement '+d2+'/'+this.UNLOCK_DIST+'m';
info.appendChild(lo);}}});}

_save(){try{localStorage.setItem(CFG.SAVE_KEY,JSON.stringify({
best:this.best,unlocks:this.unlocks,realmBestDist:this.realmBestDist,
clefCounts:this.clefCounts}));}catch(e){}}
_load(){try{const d=JSON.parse(localStorage.getItem(CFG.SAVE_KEY));
if(d){this.best=d.best||0;
if(d.realmBestDist)this.realmBestDist=d.realmBestDist;
// Recalculate unlocks from distance data
this.unlocks[0]=true;this.unlocks[1]=true;this.unlocks[2]=true;
this.unlocks[3]=this.realmBestDist[1]>=this.UNLOCK_DIST&&this.realmBestDist[2]>=this.UNLOCK_DIST;
this.unlocks[4]=this.realmBestDist[1]>=this.UNLOCK_DIST&&this.realmBestDist[2]>=this.UNLOCK_DIST;
// Restore collectible clef counts (accumulated across games)
if(d.clefCounts){
for(const k of ['treble','bass','alto']){
if(d.clefCounts[k]&&this.clefCounts[k]){
this.clefCounts[k].collected=d.clefCounts[k].collected||0;}}}}}catch(e){}}

// Full game state save/load for mid-game resume
_saveGameState(){
if(this.state!=='playing'&&this.state!=='challenged')return;
try{
const gs={
v:7,realm:this.realm,difficulty:this.difficulty,
score:this.score,combo:this.combo,maxCombo:this.maxCombo,
lives:this.lives,notesOk:this.notesOk,notesAll:this.notesAll,
dist:this.dist,gameTime:this.gameTime,
playerX:this.player.x,playerY:this.player.y,
camX:this.cam.x,camY:this.cam.y,
ts:Date.now()};
localStorage.setItem(CFG.SAVE_KEY+'_state',JSON.stringify(gs));
this._fb('GAME SAVED!','good');}catch(e){this._fb('SAVE FAILED','miss');}}

_loadGameState(){
try{const raw=localStorage.getItem(CFG.SAVE_KEY+'_state');
if(!raw)return false;
const gs=JSON.parse(raw);
if(!gs||gs.v!==7)return false;
// Only load saves less than 24h old
if(Date.now()-gs.ts>86400000)return false;
// Restore game state
this.realm=gs.realm||0;this.difficulty=gs.difficulty||'easy';
this._start(); // initialize world with realm/difficulty
this.score=gs.score||0;this.combo=gs.combo||0;this.maxCombo=gs.maxCombo||0;
this.lives=gs.lives||CFG.LIVES;this.notesOk=gs.notesOk||0;this.notesAll=gs.notesAll||0;
this.dist=gs.dist||0;this.gameTime=gs.gameTime||0;
// Position player at saved location and generate world ahead
this.player.x=gs.playerX||0;this.player.y=gs.playerY||0;
this.cam.x=gs.camX||0;this.cam.y=gs.camY||0;
this.world.ensure(this.cam.x+this.cvs.width/0.52+6000);
this._updateHUD();
this._fb('SAVE LOADED!','good');return true;}catch(e){return false;}}

_hasSavedGame(){
try{const raw=localStorage.getItem(CFG.SAVE_KEY+'_state');
if(!raw)return false;const gs=JSON.parse(raw);
return gs&&gs.v===7&&(Date.now()-gs.ts<86400000);}catch(e){return false;}}

_ajax(){if(typeof ledgerLineData==='undefined'||ledgerLineData.isLoggedIn!=='1')return;
const fd=new FormData();fd.append('action','save_ledger_line_score');fd.append('nonce',ledgerLineData.nonce);
fd.append('score',Math.round(this.score));fd.append('realm',this.realm);
fd.append('combo',this.maxCombo);
fd.append('accuracy',this.notesAll>0?Math.round(this.notesOk/this.notesAll*100):0);
fd.append('total_notes',this.notesAll);
fd.append('distance',Math.round(this.dist));
fd.append('best_score',Math.round(this.best));
fd.append('difficulty',this.difficulty);
fd.append('notes_correct',this.notesOk);
fd.append('game_time',Math.round(this.gameTime/1000));
// Send realm best distances for cross-session tracking
fd.append('realm_best_dist',JSON.stringify(this.realmBestDist));
// Send clef counts
fd.append('clef_counts',JSON.stringify(this.clefCounts));
fetch(ledgerLineData.ajaxurl,{method:'POST',body:fd}).catch(()=>{});}}

document.addEventListener('DOMContentLoaded',()=>{const g=new Game();g.init();window.ledgerLineLegend=g;});
})();