#include "pixel_camera.hpp"
#include <iostream>
#include <stdexcept>
using namespace p3;
static void check(bool b, const char *text) { if (!b) throw std::runtime_error(text); }
static float distance(Vec a, Vec b) { auto d=a-b; return std::sqrt(dot(d,d)); }
static Vec projected(const Camera &c, Vec p, int w, int h) {
  auto m=perspective(pi/3,float(w)/h)*c.view();
  auto v=point(m,p);
  float clipW=m.v[3]*p.x+m.v[7]*p.y+m.v[11]*p.z+m.v[15];
  return {(v.x/clipW*.5f+.5f)*w,(v.y/clipW*.5f+.5f)*h,0};
}
int main() {
  try {
    for (int w : {480,640}) {
      int h=w*9/16;
      Camera c; c.position={.237f,2.123f,11.347f};c.yaw=.23f;c.pitch=-.13f;
      auto off=pixelCamera(c,w,h,PixelCameraMode::Off);
      check(distance(off.render.position,c.position)==0,"OFF changes camera");
      auto raw=pixelCamera(c,w,h,PixelCameraMode::Raw);
      auto comp=pixelCamera(c,w,h,PixelCameraMode::Compensated);
      check(distance(raw.render.position,comp.render.position)==0,"RAW/COMP use different geometry camera");
      check(distance(raw.render.position,sampledCamera(c,h,true).position)<1e-5f,"legacy RAW changed");
      check(comp.render.yaw==c.yaw&&comp.render.pitch==c.pitch,"input orientation was quantized");
      check(std::abs(comp.offsetPixels.x)<=.501f&&std::abs(comp.offsetPixels.y)<=.501f,"fractional offset bound");
      for(int i=0;i<100;i++) check(distance(pixelCamera(c,w,h,PixelCameraMode::Compensated).render.position,comp.render.position)==0,"stationary jitter");
      auto reference=c.position+c.forward()*10;
      auto desired=projected(c,reference,w,h),snapped=projected(raw.render,reference,w,h);
      check(distance(snapped+comp.offsetPixels,desired)<.003f,"reference-plane compensation sign/magnitude");
      bool parallax=false;
      for(float z:{2.f,30.f}) {
        auto p=c.position+c.forward()*z;
        auto residual=projected(raw.render,p,w,h)+comp.offsetPixels-projected(c,p,w,h);
        parallax|=distance(residual,{})>.01f;
      }
      check(parallax,"test must expose perspective near/far residual");
      for(int routeId=1;routeId<=3;routeId++) for(int f=0;f<600;f++) {
        auto game=studyRoute(routeId,f,600),before=game;
        auto p=pixelCamera(game,w,h,PixelCameraMode::Compensated);
        check(finite(p.render.position)&&finite(p.offsetPixels),"route produced invalid camera");
        check(game.yaw==before.yaw&&game.pitch==before.pitch&&distance(game.position,before.position)==0,"game state mutated");
        check(std::abs(p.offsetPixels.x)<.502f&&std::abs(p.offsetPixels.y)<.502f,"rotation/rapid turn compensation exploded");
      }
      // Very slow strafe, not frame-sized movement: preserve unquantized input.
      for(int i=0;i<1000;i++) {
        c.position.x+=.00001f;
        auto p=pixelCamera(c,w,h,PixelCameraMode::Compensated);
        check(finite(p.render.position),"slow translation invalid");
      }
    }
    for(int which=0;which<9;which++) {
      Camera c; int w=480,h=270;float d=10;
      if(which==0) w=0; if(which==1)h=-1; if(which==2)h=1;
      if(which==3)c.position.x=NAN;if(which==4)c.yaw=INFINITY;
      if(which==5)c.pitch=pi/2;if(which==6)d=NAN;if(which==7)d=0;if(which==8)d=201;
      bool rejected=false;try{pixelCamera(c,w,h,PixelCameraMode::Compensated,d);}catch(...){rejected=true;}
      check(rejected,"invalid pixel camera accepted");
    }
    std::cout<<"PASS OFF/RAW/COMP, unchanged input, stationary/slow strafe/orbit/look/rapid turn, reference alignment and explicit parallax residual, invalid inputs\n";
    return 0;
  }catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}
}
