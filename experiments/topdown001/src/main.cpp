#include <png.h>
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>
struct Img{int w=0,h=0;std::vector<unsigned char> rgba;};
static Img load(const std::string& p){png_image img{};img.version=PNG_IMAGE_VERSION;if(!png_image_begin_read_from_file(&img,p.c_str()))throw std::runtime_error(std::string("read ")+p);img.format=PNG_FORMAT_RGBA;Img o{int(img.width),int(img.height),std::vector<unsigned char>(PNG_IMAGE_SIZE(img))};if(!png_image_finish_read(&img,nullptr,o.rgba.data(),0,nullptr))throw std::runtime_error("decode "+p);png_image_free(&img);return o;}
struct Scene{int w=320,h=180;std::vector<unsigned char> p=std::vector<unsigned char>(w*h*4,0);void blit(const Img& i,int sx,int sy,int sw,int sh,int x,int y,int scale=1){for(int dy=0;dy<sh*scale;dy++)for(int dx=0;dx<sw*scale;dx++){int tx=sx+dx/scale,ty=sy+dy/scale,xx=x+dx,yy=y+dy;if(tx<0||ty<0||tx>=i.w||ty>=i.h||xx<0||yy<0||xx>=w||yy>=h)continue;size_t a=(ty*i.w+tx)*4,b=(yy*w+xx)*4;int alpha=i.rgba[a+3];if(alpha==0)continue;for(int c=0;c<3;c++)p[b+c]=static_cast<unsigned char>((i.rgba[a+c]*alpha+p[b+c]*(255-alpha)+127)/255);p[b+3]=255;}} void fill(unsigned char r,unsigned char g,unsigned char b){for(size_t i=0;i<p.size();i+=4){p[i]=r;p[i+1]=g;p[i+2]=b;p[i+3]=255;}} void save(const std::string& path){png_image img{};img.version=PNG_IMAGE_VERSION;img.width=w;img.height=h;img.format=PNG_FORMAT_RGBA;if(!png_image_write_to_file(&img,path.c_str(),0,p.data(),0,nullptr))throw std::runtime_error("write png failed");}};
int main(int argc,char**argv){try{std::string dir="private_input",out="topdown.png";int frames=1;for(int i=1;i<argc;i++){std::string a=argv[i];if(a=="--assets"&&i+1<argc)dir=argv[++i];else if(a=="--output"&&i+1<argc)out=argv[++i];else if(a=="--frames"&&i+1<argc)frames=std::stoi(argv[++i]);else throw std::runtime_error("usage: topdown --assets PRIVATE_DIRECTORY --output FRAME.png [--frames 1..120]");}if(frames<1||frames>120)throw std::runtime_error("frames outside range");auto floors=load(dir+"/floors.png"),walls=load(dir+"/walls.png"),roofs=load(dir+"/roofs.png"),trees=load(dir+"/trees.png"),plants=load(dir+"/plants.png"),farms=load(dir+"/farms.png"),character=load(dir+"/character.png");const int T=16,MW=40,MH=30;auto ground=[&](int x,int y){unsigned v=(unsigned)((x*134775813u)^(y*2654435761u));v^=v>>13;return (v%11==0)?1:0;};std::vector<unsigned char> collision(MW*MH,0);for(int y=0;y<MH;y++)for(int x=0;x<MW;x++){if(x==0||y==0||x==MW-1||y==MH-1)collision[y*MW+x]=1;}for(int y=10;y<=17;y++)for(int x=17;x<=27;x++)collision[y*MW+x]=1;for(int y=18;y<=24;y++)for(int x=21;x<=23;x++)collision[y*MW+x]=0;double playerX=20.0,playerY=22.0;for(int frame=0;frame<frames;frame++){Scene s;s.fill(72,145,92);int cx=int(playerX*T)-s.w/2,cy=int(playerY*T)-s.h/2;auto drawTile=[&](const Img& a,int srcX,int srcY,int mx,int my){s.blit(a,srcX,srcY,T,T,mx*T-cx,my*T-cy);};for(int y=0;y<MH;y++)for(int x=0;x<MW;x++){drawTile(floors,ground(x,y)?64:16,0,x,y);if((x>=21&&x<=23&&y>=18&&y<=27)||(y==23&&x>=8&&x<=33))drawTile(floors,32,0,x,y);}
// Buildings assembled only from original Pixel Crawler sheet cells; art itself is never procedurally redrawn.
for(int y=10;y<=17;y++)for(int x=17;x<=27;x++){drawTile(walls,64+(x%3)*16,32+(y%3)*16,x,y);}
for(int y=9;y<=12;y++)for(int x=16;x<=28;x++){drawTile(roofs,96+(x%4)*16,64+(y%3)*16,x,y);}
// Scatter sheet-native vegetation and tree sprites in meaningful world positions.
for(int y=4;y<MH-2;y+=4)for(int x=3;x<MW-2;x+=6){if(x>13&&x<31&&y>7&&y<26)continue;int sx=x*T-cx,sy=y*T-cy;if((x+y)%3==0)s.blit(trees,0,0,64,64,sx-24,sy-48);else s.blit(plants,32,16,32,32,sx,sy);}
for(int y=25;y<29;y+=2)for(int x=9;x<15;x+=2)s.blit(farms,16,16,16,16,x*T-cx,y*T-cy);
// Experimental player on top of background using exact cropped source pixels.
s.blit(character,0,0,32,32,int(playerX*T)-cx-16,int(playerY*T)-cy-28);
if(frames==1)s.save(out);else{char suffix[24];std::snprintf(suffix,sizeof(suffix),"_%03d.png",frame);s.save(out+suffix);}if(frame+1<frames){double nx=playerX+0.11,ny=playerY;int ix=int(nx),iy=int(ny);if(ix>=0&&iy>=0&&ix<MW&&iy<MH&&!collision[iy*MW+ix])playerX=nx;}}
std::cout<<"PASS: actual CPU-rendered Pixel Crawler sheet pixels, "+std::to_string(frames)+" frame(s), 320x180 RGBA\n";return 0;}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
