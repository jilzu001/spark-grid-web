const assert=require('node:assert/strict');
const pixels=require('../web/pixel.js');
const input=new Uint8ClampedArray([250,205,165,255,1,2,3,0,15,20,30,100]);
const same=pixels.quantize(input);
assert.equal(same,input);
assert.deepEqual([...input],[250,205,165,255,15,20,30,0,15,20,30,0]);
const photo=new Uint8ClampedArray(32*32*4);
for(let i=0;i<photo.length;++i)photo[i]=(i*37+101)%256;
pixels.quantize(photo);
for(let i=0;i<photo.length;i+=4){
  assert(pixels.palette.some(c=>c[0]===photo[i]&&c[1]===photo[i+1]&&c[2]===photo[i+2]));
  assert(photo[i+3]===0||photo[i+3]===255);
}
assert.equal(pixels.palette.length,32);
assert.throws(()=>pixels.quantize([1,2,3]));
console.log('PASS: 32-colour conversion, skin tones, alpha, fixed size, in-place reuse');
