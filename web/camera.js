'use strict';
let runtimeReady=false;
const photoBytes=Array(18).fill(null), photoUrls=Array(18).fill(null), photoGeneration=Array(18).fill(0);
const heldPointers=new Map();
const statusLine=document.getElementById('status');
const gameCanvas=document.getElementById('canvas');
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
    say('준비 완료. 방향키로 선택하고 확인을 누르세요.');
    // Emscripten calls this before C++ main; apply queued photos after its first frame.
    requestAnimationFrame(()=>requestAnimationFrame(()=>{
      try{photoBytes.forEach((bytes,i)=>{if(bytes)applyPhoto(i);});invoke('WebPhotoEditing',[document.querySelector('details').open?1:0]);}catch(error){say(error.message);}
    }));
  },
  onAbort(){runtimeReady=false;say('게임을 불러오지 못했습니다. 새로고침하거나 연결을 확인하세요.');},
  onGameClosed(){releaseAll();runtimeReady=false;document.querySelectorAll('[data-action]').forEach(button=>button.disabled=true);say('게임을 종료했습니다. 다시 실행하려면 새로고침하세요.');}
};
function releaseAll(){
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
document.querySelector('details').addEventListener('toggle',event=>{releaseAll();invoke('WebPhotoEditing',[event.target.open?1:0]);});
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
