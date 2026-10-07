/* Pure pixel conversion; shared by camera UI and Node tests. No image uploads. */
(function (root) {
  'use strict';
  const palette = [
    [15,20,30],[40,45,55],[75,80,90],[120,125,135],[180,185,195],[240,240,235],
    [60,35,30],[100,55,35],[150,90,55],[195,130,85],[225,165,120],[250,205,165],
    [85,30,55],[145,45,70],[205,70,100],[245,130,150],
    [100,65,25],[175,115,30],[230,175,55],[255,225,115],
    [25,60,45],[40,110,70],[70,170,105],[135,220,155],
    [20,65,85],[30,120,145],[75,190,195],[150,230,225],
    [35,40,90],[65,80,160],[110,145,220],[180,195,250]
  ];
  function quantize(data) {
    if (data.length % 4) throw new Error('RGBA length must be a multiple of 4');
    for (let i=0;i<data.length;i+=4) {
      let best=palette[0], minimum=Infinity;
      for (const color of palette) {
        const r=data[i]-color[0], g=data[i+1]-color[1], b=data[i+2]-color[2];
        const distance=r*r+g*g+b*b;
        if (distance<minimum) {minimum=distance;best=color;}
      }
      data[i]=best[0];data[i+1]=best[1];data[i+2]=best[2];
      data[i+3]=data[i+3]<128?0:255;
    }
    return data;
  }
  const api={palette,quantize};
  if (typeof module==='object' && module.exports) module.exports=api;
  else root.SparkPixels=api;
})(typeof globalThis!=='undefined'?globalThis:this);
