'use strict';
let runtimeReady=false;
const photoBytes=Array(18).fill(null), photoUrls=Array(18).fill(null), photoGeneration=Array(18).fill(0);
const heldPointers=new Map();
let joyPointer=null,joyAction=null;
const joystick=document.getElementById('joystick'),joyKnob=document.getElementById('joy-knob');
function releaseJoystick(){
  if(joyAction!==null)invoke('WebAction',[joyAction,0]);
  joyAction=null;joyPointer=null;joyKnob.style.transform='';
}
function steerJoystick(event){
  const rect=joystick.getBoundingClientRect(),dx=event.clientX-(rect.left+rect.width/2),dy=event.clientY-(rect.top+rect.height/2);
  const length=Math.hypot(dx,dy),scale=length>40?40/length:1;
  joyKnob.style.transform='translate('+dx*scale+'px,'+dy*scale+'px)';
  const action=length<12?null:Math.abs(dx)>Math.abs(dy)?(dx>0?1:0):(dy>0?3:2);
  if(action===joyAction)return;
  if(joyAction!==null)invoke('WebAction',[joyAction,0]);
  joyAction=action;if(action!==null)invoke('WebAction',[action,1]);
}
joystick.addEventListener('pointerdown',event=>{
  if(!runtimeReady||joyPointer!==null)return;
  event.preventDefault();joyPointer=event.pointerId;joystick.setPointerCapture(event.pointerId);invoke('WebAudioStart');steerJoystick(event);
});
joystick.addEventListener('pointermove',event=>{if(event.pointerId===joyPointer)steerJoystick(event);});
for(const type of ['pointerup','pointercancel','lostpointercapture'])joystick.addEventListener(type,event=>{if(event.pointerId===joyPointer)releaseJoystick();});
const statusLine=document.getElementById('status');
const gameCanvas=document.getElementById('canvas');
let currentScreen='title';
function showScreen(screen){
  releaseAll();currentScreen=screen;document.body.dataset.screen=screen;
  document.getElementById('result-screen').hidden=true;
  for(const name of ['title','help','character','game','pause'])document.getElementById(name+'-screen').hidden=!(name===(screen==='playing'||screen==='paused'?'game':screen)||name==='pause'&&screen==='paused');
  invoke('WebPhotoEditing',[screen==='character'||screen==='paused'?1:0]);
  say('');window.scrollTo(0,0);
  if(screen==='playing')gameCanvas.focus();
}
document.getElementById('start-game').addEventListener('click',()=>{invoke('WebAudioStart');invoke('WebNavigate',[2]);});
document.getElementById('open-characters').addEventListener('click',()=>showScreen('character'));
document.getElementById('open-help').addEventListener('click',()=>showScreen('help'));
document.querySelectorAll('[data-back]').forEach(button=>button.addEventListener('click',()=>showScreen('title')));
function pulse(action){invoke('WebAction',[action,1]);invoke('WebAction',[action,0]);}
document.getElementById('pause-game').addEventListener('click',()=>showScreen('paused'));
document.getElementById('resume-game').addEventListener('click',()=>showScreen('playing'));
document.getElementById('toggle-sound').addEventListener('click',()=>pulse(8));
document.getElementById('restart-game').addEventListener('click',()=>{pulse(6);showScreen('playing');});
document.getElementById('leave-game').addEventListener('click',()=>invoke('WebNavigate',[0]));
document.getElementById('next-stage').addEventListener('click',()=>invoke('WebNavigate',[3]));
document.getElementById('retry-stage').addEventListener('click',()=>pulse(6));
document.getElementById('result-title-back').addEventListener('click',()=>invoke('WebNavigate',[0]));
document.addEventListener('keydown',event=>{
  if(currentScreen==='title'&&event.key==='Enter'){
    event.stopImmediatePropagation();
    if(!event.target.closest('button')){event.preventDefault();document.getElementById('start-game').click();}
    return;
  }
  if(currentScreen==='title'&&['ArrowUp','ArrowDown','ArrowLeft','ArrowRight',' '].includes(event.key)){
    event.stopImmediatePropagation();
    if(!event.target.closest('button'))event.preventDefault();
    return;
  }
  if(event.key==='Escape'&&(currentScreen==='playing'||currentScreen==='paused')){
    event.preventDefault();event.stopImmediatePropagation();showScreen(currentScreen==='paused'?'playing':'paused');return;
  }
  if(currentScreen==='help'||currentScreen==='character'||currentScreen==='paused'){
    if(event.target.matches('input'))return;
    event.stopImmediatePropagation();
    if(event.key==='Escape'){event.preventDefault();showScreen('title');}
  }
},true);
function addImageCard(host,title,slot,filename){
  const card=document.createElement('section');card.className='skin';
  const heading=document.createElement('h2');heading.textContent=title;card.append(heading);
  const preview=document.createElement('canvas');preview.id='preview-'+slot;preview.className='preview';preview.width=preview.height=32;card.append(preview);
  for(const capture of [true,false]){
    const label=document.createElement('label');label.className='photo-label';label.append(capture?'카메라 촬영':'사진 선택');
    const input=document.createElement('input');input.type='file';input.accept='image/*';input.dataset.actor=String(slot);
    if(capture)input.setAttribute('capture','user');label.append(input);card.append(label,document.createTextNode(' '));
  }
  const size=document.createElement('small');size.className='size';size.id='size-'+slot;size.textContent='32×32 도트 PNG로 변환됩니다.';card.append(size);
  const download=document.createElement('a');download.id='download-'+slot;download.download=filename;download.hidden=true;download.textContent='도트 PNG 저장';card.append(download);
  host.append(card);
}
addImageCard(document.getElementById('player-skins'),'플레이어 · 정상',0,'player.png');
addImageCard(document.getElementById('player-skins'),'플레이어 · 사망',1,'player-dead.png');
let enemySets=0;
function addEnemySet(){
  if(enemySets>=8)return;
  const number=++enemySets,normal=2+(number-1)*2;
  addImageCard(document.getElementById('enemy-skins'),'적 '+number+' · 정상',normal,'enemy-'+number+'.png');
  addImageCard(document.getElementById('enemy-skins'),'적 '+number+' · 사망',normal+1,'enemy-'+number+'-dead.png');
  if(enemySets===8)document.getElementById('add-enemy-set').disabled=true;
}
for(let i=0;i<3;++i)addEnemySet();
document.getElementById('add-enemy-set').addEventListener('click',addEnemySet);
function say(message){statusLine.textContent=message;}
function invoke(name,args=[]){if(runtimeReady)return Module.ccall(name,'number',args.map(()=> 'number'),args);}
function applyPhoto(actor){
  if(!runtimeReady||!photoBytes[actor])return false;
  Module.FS.mkdirTree('/skins');
  Module.FS.writeFile('/skins/slot-'+actor+'.png',photoBytes[actor]);
  if(invoke('ApplyPhotoSkin',[actor])!==1)throw new Error('게임 이미지 적용에 실패했습니다.');
  return true;
}
var Module={
  canvas:gameCanvas,
  setStatus(text){if(text&&!runtimeReady)say('게임 파일을 불러오는 중입니다: '+text);},
  print(text){console.log(text);},printErr(text){console.error(text);},
  onRuntimeInitialized(){
    runtimeReady=true;
    document.querySelectorAll('[data-action]').forEach(button=>button.disabled=false);
    document.getElementById('start-game').disabled=false;
    say('');
    // Emscripten calls this before C++ main; apply queued photos after its first frame.
    requestAnimationFrame(()=>requestAnimationFrame(()=>{
      try{photoBytes.forEach((bytes,i)=>{if(bytes)applyPhoto(i);});invoke('WebPhotoEditing',[currentScreen==='character'?1:0]);}catch(error){say(error.message);}
    }));
  },
  onWebScreen(screen){showScreen(screen===1?'playing':'title');},
  onHud(bombs,maxBombs,range,enemies,muted,stage,phase){
    document.getElementById('hud').textContent=stage+'/10 · 폭탄 '+bombs+'/'+maxBombs+' · 범위 '+range+' · 적 '+enemies;
    document.getElementById('toggle-sound').textContent=muted?'소리: 꺼짐':'소리: 켜짐';
    gameCanvas.style.setProperty('--game-ratio',gameCanvas.width/gameCanvas.height);
    document.getElementById('result-screen').hidden=currentScreen!=='playing'||phase===0;
    document.getElementById('result-title').textContent=phase===1?'다시 도전해 보세요':stage===10?'10개 스테이지 모두 클리어!':stage+' 스테이지 클리어!';
    document.getElementById('next-stage').hidden=phase!==2||stage===10;
  },
  onAbort(){runtimeReady=false;say('게임을 불러오지 못했습니다. 새로고침하거나 연결을 확인하세요.');},
  onGameClosed(){releaseAll();runtimeReady=false;document.querySelectorAll('[data-action]').forEach(button=>button.disabled=true);say('게임을 종료했습니다. 다시 실행하려면 새로고침하세요.');}
};
function releaseAll(){
  releaseJoystick();
  for(const action of new Set(heldPointers.values()))invoke('WebAction',[action,0]);
  heldPointers.clear();
}
document.querySelectorAll('[data-action]').forEach(button=>{
  button.addEventListener('pointerdown',event=>{
    if(!runtimeReady)return;
    event.preventDefault();button.setPointerCapture(event.pointerId);
    const action=Number(button.dataset.action);
    heldPointers.set(event.pointerId,action);invoke('WebAction',[action,1]);
  });
  function release(event){
    const action=heldPointers.get(event.pointerId);
    if(action===undefined)return;
    heldPointers.delete(event.pointerId);
    if(![...heldPointers.values()].includes(action))invoke('WebAction',[action,0]);
  }
  button.addEventListener('pointerup',release);button.addEventListener('pointercancel',release);button.addEventListener('lostpointercapture',release);
  button.addEventListener('click',event=>{if(event.detail===0){const action=Number(button.dataset.action);invoke('WebAction',[action,1]);invoke('WebAction',[action,0]);}});
});
gameCanvas.addEventListener('pointerdown',()=>{invoke('WebAudioStart');gameCanvas.focus();});
document.addEventListener('keydown',()=>invoke('WebAudioStart'));
window.addEventListener('blur',releaseAll);
document.addEventListener('visibilitychange',()=>{if(document.hidden)releaseAll();});
function loadImage(file){
  return new Promise((resolve,reject)=>{
    const url=URL.createObjectURL(file),image=new Image();
    image.onload=()=>{URL.revokeObjectURL(url);resolve(image);};
    image.onerror=()=>{URL.revokeObjectURL(url);reject(new Error('읽을 수 없는 사진입니다. JPG 또는 PNG로 다시 선택하세요.'));};
    image.src=url;
  });
}
document.addEventListener('change',async event=>{
  const input=event.target;
  if(!input.matches('input[data-actor]'))return;
  const file=input.files[0];if(!file)return;
  const actor=Number(input.dataset.actor),generation=++photoGeneration[actor];
  try{
    if(!file.type.startsWith('image/'))throw new Error('사진 파일을 선택하세요.');
    if(file.size>20*1024*1024)throw new Error('20MB 이하 사진을 선택하세요.');
    say('사진을 32×32 도트 이미지로 바꾸는 중입니다.');
    const image=await loadImage(file);
    if(image.naturalWidth*image.naturalHeight>40*1024*1024)throw new Error('사진 해상도가 너무 큽니다. 작은 사진을 선택하세요.');
    if(generation!==photoGeneration[actor])return;
    const converted=document.createElement('canvas');converted.width=converted.height=32;
    const context=converted.getContext('2d',{willReadFrequently:true});
    const side=Math.min(image.naturalWidth,image.naturalHeight);
    context.imageSmoothingEnabled=true;context.imageSmoothingQuality='high';
    context.drawImage(image,(image.naturalWidth-side)/2,(image.naturalHeight-side)/2,side,side,0,0,32,32);
    const rgba=context.getImageData(0,0,32,32);SparkPixels.quantize(rgba.data);context.putImageData(rgba,0,0);
    const blob=await new Promise(resolve=>converted.toBlob(resolve,'image/png'));
    if(!blob)throw new Error('PNG 변환에 실패했습니다.');
    const bytes=new Uint8Array(await blob.arrayBuffer());
    if(generation!==photoGeneration[actor])return;
    photoBytes[actor]=bytes;
    const preview=document.getElementById('preview-'+actor).getContext('2d');preview.clearRect(0,0,32,32);preview.drawImage(converted,0,0);
    if(photoUrls[actor])URL.revokeObjectURL(photoUrls[actor]);
    photoUrls[actor]=URL.createObjectURL(blob);
    const download=document.getElementById('download-'+actor);download.href=photoUrls[actor];download.hidden=false;
    document.getElementById('size-'+actor).textContent='32×32 · 32색 PNG · '+bytes.length.toLocaleString()+'바이트';
    const applied=applyPhoto(actor);
    say(applied?'도트 캐릭터를 게임에 적용했습니다.':'도트 이미지가 준비됐습니다. 게임 로딩 후 적용됩니다.');
  }catch(error){if(generation===photoGeneration[actor])say(error.message);}
  finally{input.value='';}
});

