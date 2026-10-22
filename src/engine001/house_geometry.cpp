#include "house_geometry.hpp"
#include "sampling.hpp"
#include <algorithm>
#include <chrono>
#include <cmath>
#include <fstream>
#include <iterator>
#include <limits>
#include <stdexcept>
#include <tuple>

namespace ws {
namespace {
double number(const Json& j,const char* k,double lo,double hi){
    const double v=j.at(k).number();
    if(!std::isfinite(v)||v<lo||v>hi)throw std::runtime_error("Invalid house geometry numeric field");
    return v;
}
MaterialRegion region(const Json& j,const SamplingPack& p){
    MaterialRegion r;const double slot=number(j,"material",0,static_cast<double>(p.materials.size())-1);
    if(slot!=std::floor(slot))throw std::runtime_error("Invalid house material slot");
    r.material=static_cast<int>(slot);r.u0=number(j,"u0",0,1);r.u1=number(j,"u1",0,1);r.v0=number(j,"v0",0,1);r.v1=number(j,"v1",0,1);
    if(r.u1<=r.u0||r.v1<=r.v0)throw std::runtime_error("Empty house material region");
    const auto& t=p.materials[static_cast<std::size_t>(r.material)];
    r.x0=std::max(0,static_cast<int>(std::ceil(r.u0*t.w-.5)));r.x1=std::min(t.w-1,static_cast<int>(std::ceil(r.u1*t.w-.5))-1);
    r.y0=std::max(0,static_cast<int>(std::ceil(r.v0*t.h-.5)));r.y1=std::min(t.h-1,static_cast<int>(std::ceil(r.v1*t.h-.5))-1);
    if(r.x1<r.x0||r.y1<r.y0)throw std::runtime_error("Unsampled house material region");
    for(int y=r.y0;y<=r.y1;++y)for(int x=r.x0;x<=r.x1;++x)
        if((t.pixels[static_cast<std::size_t>(y*t.w+x)]>>24)!=255)throw std::runtime_error("House surfaces require opaque material regions");
    return r;
}
struct Vertex { double x,y,d,u,v; };
Vertex mix(Vertex a,Vertex b,double t){return {a.x+(b.x-a.x)*t,a.y+(b.y-a.y)*t,a.d+(b.d-a.d)*t,a.u+(b.u-a.u)*t,a.v+(b.v-a.v)*t};}
// Canonical direction makes shared-edge intersections identical for opposite windings.
Vertex intersect(Vertex a,Vertex b,double fa,double fb){
    if(std::tie(a.x,a.y,a.d)>std::tie(b.x,b.y,b.d)){std::swap(a,b);std::swap(fa,fb);}
    return mix(a,b,fa/(fa-fb));
}
struct Screen { std::int64_t x,y; double q,u,v; };
std::int64_t edge(const Screen& a,const Screen& b,std::int64_t x,std::int64_t y){return (b.x-a.x)*(y-a.y)-(b.y-a.y)*(x-a.x);}
bool inclusive(const Screen& a,const Screen& b){return b.y<a.y||(b.y==a.y&&b.x>a.x);}
double fract(double n){return n-std::floor(n);}
}
HouseGeometry HouseGeometry::load(const std::filesystem::path& path,const SamplingPack& p,const FacadeCalibration& f){
    if(f.scale!="human"||!f.treatment||f.treatment->look!="c"||f.treatment->plaster<0)throw std::runtime_error("House geometry requires VIS-003 C human baseline");
    std::ifstream in(path,std::ios::binary|std::ios::ate);
    if(!in||in.tellg()>16384)throw std::runtime_error("Missing or oversized house geometry input");
    in.seekg(0);const auto j=parseJson(std::string{std::istreambuf_iterator<char>(in),{}});
    if(j.at("schema").string()!="wildsignal-house-geometry-experiment-v1")throw std::runtime_error("Unsupported house geometry schema");
    HouseGeometry h;h.roof=region(j.at("roof"),p);h.wood=region(j.at("wood"),p);
    h.rise=number(j,"rise",.6,2);h.eave=number(j,"eave",.1,.5);h.thickness=number(j,"thickness",.06,.18);h.recess=number(j,"recess",0,.25);
    h.roofStatus=j.at("roofStatus").string();
    if(h.roofStatus!="UNAPPROVED_CANDIDATE"&&h.roofStatus!="TEMPORARY_ENGINEERING")throw std::runtime_error("Explicit experimental roof status required");
    h.build(f);return h;
}
bool HouseGeometry::opening(double x,double z,const FacadeCalibration& f) const {
    return recess>0&&x>=f.treatment->doorX-.68&&x<f.treatment->doorX+.68&&z>=.03&&z<2.07;
}
void HouseGeometry::build(const FacadeCalibration& f){
    count=0;
    const auto tri=[&](SurfaceVertex a,SurfaceVertex b,SurfaceVertex c,HouseMaterial m){
        if(count==capacity)throw std::runtime_error("Static house triangle budget exceeded");
        triangles[count++]={{a,b,c},m};
    };
    const auto quad=[&](SurfaceVertex a,SurfaceVertex b,SurfaceVertex c,SurfaceVertex d,HouseMaterial m){tri(a,b,c,m);tri(a,c,d,m);};
    const double left=9-eave,right=14+eave,back=7-eave,front=11+eave,ridge=wallHeight+rise;
    const double edgeZ=wallHeight-rise*eave/2.5,slope=std::hypot(2.5+eave,rise*(2.5+eave)/2.5);
    const auto roofPoint=[&](double x,double y,double z){return SurfaceVertex{x,y,z,(y-back)/.9,std::abs(x-11.5)/(2.5+eave)*slope/(.9/roof.aspect())};};
    for(double x:{left,right}){
        const auto a=roofPoint(11.5,back,ridge),b=roofPoint(11.5,front,ridge),c=roofPoint(x,front,edgeZ),d=roofPoint(x,back,edgeZ);
        quad(a,b,c,d,HouseMaterial::Roof);
        auto aa=a,bb=b,cc=c,dd=d;aa.z-=thickness;bb.z-=thickness;cc.z-=thickness;dd.z-=thickness;
        // Undersides retain the same physical planes; fascia uses native timber aspect.
        for(auto* v:{&aa,&bb,&cc,&dd}){v->u=(v->y-back)/(.12*wood.aspect());v->v=std::abs(v->x-11.5)/(2.5+eave)*slope/.12;}
        quad(aa,bb,cc,dd,HouseMaterial::Wood);
        const auto strip=[&](SurfaceVertex p,SurfaceVertex q,SurfaceVertex r,SurfaceVertex s){
            const double length=std::hypot(std::hypot(q.x-p.x,q.y-p.y),q.z-p.z);
            p.u=0;p.v=0;q.u=length/(thickness*wood.aspect());q.v=0;r.u=q.u;r.v=1;s.u=0;s.v=1;
            quad(p,q,r,s,HouseMaterial::Wood);
        };
        strip(b,c,cc,bb);strip(d,a,aa,dd);strip(c,d,dd,cc);
    }
    for(double y:{7.0,11.0}){
        tri({9,y,wallHeight,0,1-wallHeight/2.5},{14,y,wallHeight,2,1-wallHeight/2.5},{11.5,y,ridge,1,1-ridge/2.5},HouseMaterial::Plaster);
    }
    // A small solid triangular ridge cap. Same shingle material, no camera-facing cards.
    const double half=.075,capZ=ridge+.055,baseZ=ridge-rise*half/2.5;
    const auto cap=[&](double x,double y,double z){return SurfaceVertex{x,y,z,(y-back)/.9,(x-11.5)/.9};};
    for(double x:{11.5-half,11.5+half})quad(cap(11.5,back,capZ),cap(11.5,front,capZ),cap(x,front,baseZ),cap(x,back,baseZ),HouseMaterial::Roof);
    tri(cap(11.5-half,front,baseZ),cap(11.5+half,front,baseZ),cap(11.5,front,capZ),HouseMaterial::Roof);
    tri(cap(11.5-half,back,baseZ),cap(11.5+half,back,baseZ),cap(11.5,back,capZ),HouseMaterial::Roof);
    if(recess>0){
        const double l=f.treatment->doorX-.68,r=f.treatment->doorX+.68,lo=.03,hi=2.07,y=11-recess;
        // Door canvas X/Z and scale are unchanged; only its supporting plane moves inward.
        quad({l,y,lo,l,lo},{r,y,lo,r,lo},{r,y,hi,r,hi},{l,y,hi,l,hi},HouseMaterial::Door);
        const auto jamb=[&](SurfaceVertex a,SurfaceVertex b,SurfaceVertex c,SurfaceVertex d){
            const double length=std::hypot(b.x-a.x,b.z-a.z);
            a.u=0;a.v=0;b.u=length/(recess*wood.aspect());b.v=0;c.u=b.u;c.v=1;d.u=0;d.v=1;quad(a,b,c,d,HouseMaterial::Wood);
        };
        jamb({l,11,lo},{l,11,hi},{l,y,hi},{l,y,lo});jamb({r,11,lo},{r,11,hi},{r,y,hi},{r,y,lo});
        jamb({l,11,hi},{r,11,hi},{r,y,hi},{l,y,hi});jamb({l,11,lo},{r,11,lo},{r,y,lo},{l,y,lo});
    }
}
GeometryStats HouseGeometry::draw(Buffer& b,const World& w,const Camera& c,const SamplingPack& p,const FacadeCalibration& f) const {
    const auto start=std::chrono::steady_clock::now();GeometryStats stats;
    const double focal=b.size.w/(2*std::tan(w.fov*pi/360)),scale=b.size.w/8.0,eye=w.eye*wallHeight;
    const Vec2 forward{std::sin(c.yaw),-std::cos(c.yaw)},right{std::cos(c.yaw),std::sin(c.yaw)};
    const bool ortho=f.orthographic;
    const auto distance=[&](const Vertex& a,int plane){
        const double extentX=b.size.w/(2*(ortho?scale:focal)),extentY=b.size.h/(2*(ortho?scale:focal));
        switch(plane){case 0:return a.d-.08;case 1:return 40-a.d;case 2:return a.x+extentX*(ortho?1:a.d);case 3:return extentX*(ortho?1:a.d)-a.x;case 4:return a.y+extentY*(ortho?1:a.d);default:return extentY*(ortho?1:a.d)-a.y;}
    };
    const auto pixel=[&](HouseMaterial m,double u,double v){
        if(m==HouseMaterial::Roof)return roof.at(p,fract(u),fract(v));
        if(m==HouseMaterial::Wood)return wood.at(p,fract(u),fract(v));
        if(m==HouseMaterial::Plaster)return p.materials[static_cast<std::size_t>(f.treatment->plaster)].at(u,v);
        Pixel color=p.materials[static_cast<std::size_t>(f.treatment->plaster)].at((u-9)/2.5,1-v/2.5);
        const auto panel=facadeDoor(f);const double du=(u-panel.left)/panel.size,dv=(panel.top-v)/panel.size;
        if(du>=0&&du<1&&dv>=0&&dv<1){const auto ink=p.materials[static_cast<std::size_t>(f.doorMaterial)].at(du,dv);if(ink>>24)color=ink;}
        return color;
    };
    for(std::size_t t=0;t<count;++t){
        std::array<Vertex,16> a{},scratch{};int n=3;
        for(int k=0;k<3;++k){const auto& v=triangles[t].vertices[static_cast<std::size_t>(k)];const double dx=v.x-c.position.x,dy=v.y-c.position.y;
            a[static_cast<std::size_t>(k)]={dx*right.x+dy*right.y,v.z-eye,dx*forward.x+dy*forward.y,v.u,v.v};}
        for(int plane=0;plane<6&&n;++plane){int out=0;
            for(int k=0;k<n;++k){const auto& v=a[static_cast<std::size_t>(k)];const auto& next=a[static_cast<std::size_t>((k+1)%n)];const double d=distance(v,plane),e=distance(next,plane);
                if(d>=0)scratch[static_cast<std::size_t>(out++)]=v;
                if((d>=0)!=(e>=0))scratch[static_cast<std::size_t>(out++)]=intersect(v,next,d,e);
            }
            a=scratch;n=out;
        }
        for(int fan=1;fan+1<n;++fan){
            std::array<Screen,3> s{};const std::array<int,3> ids{0,fan,fan+1};
            for(int k=0;k<3;++k){const auto& v=a[static_cast<std::size_t>(ids[static_cast<std::size_t>(k)])];const double factor=ortho?scale:focal/v.d,q=ortho?v.d:1/v.d;
                s[static_cast<std::size_t>(k)]={static_cast<std::int64_t>(std::llround((b.size.w*.5+v.x*factor)*256)),static_cast<std::int64_t>(std::llround((b.size.h*.5-v.y*factor)*256)),q,ortho?v.u:v.u*q,ortho?v.v:v.v*q};}
            auto area=edge(s[0],s[1],s[2].x,s[2].y);if(!area)continue;if(area<0){std::swap(s[1],s[2]);area=-area;}
            ++stats.triangles;
            const int x0=std::max(0,static_cast<int>(std::floor(std::min({s[0].x,s[1].x,s[2].x})/256.0))),x1=std::min(b.size.w-1,static_cast<int>(std::floor(std::max({s[0].x,s[1].x,s[2].x})/256.0)));
            const int y0=std::max(0,static_cast<int>(std::floor(std::min({s[0].y,s[1].y,s[2].y})/256.0))),y1=std::min(b.size.h-1,static_cast<int>(std::floor(std::max({s[0].y,s[1].y,s[2].y})/256.0)));
            const std::array<std::int64_t,3> bias{inclusive(s[1],s[2])?0:-1,inclusive(s[2],s[0])?0:-1,inclusive(s[0],s[1])?0:-1};
            std::array<std::int64_t,3> row{edge(s[1],s[2],x0*256+128,y0*256+128),edge(s[2],s[0],x0*256+128,y0*256+128),edge(s[0],s[1],x0*256+128,y0*256+128)};
            const double invArea=1.0/static_cast<double>(area);
            for(int y=y0;y<=y1;++y){auto edges=row;
                for(int x=x0;x<=x1;++x){
                    if(edges[0]+bias[0]>=0&&edges[1]+bias[1]>=0&&edges[2]+bias[2]>=0){
                        ++stats.tests;const double a0=edges[0]*invArea,a1=edges[1]*invArea,a2=edges[2]*invArea;
                        const double q=a0*s[0].q+a1*s[1].q+a2*s[2].q,depth=ortho?q:1/q;const auto i=static_cast<std::size_t>(y*b.size.w+x);
                        if(depth<b.pixelDepth[i]){const double factor=ortho?1:depth;const double u=(a0*s[0].u+a1*s[1].u+a2*s[2].u)*factor,v=(a0*s[0].v+a1*s[1].v+a2*s[2].v)*factor;
                            b.pixels[i]=pixel(triangles[t].material,u,v);b.pixelDepth[i]=static_cast<float>(depth);b.owner[i]=0;++stats.writes;}
                    }
                    edges[0]-=(s[2].y-s[1].y)*256;edges[1]-=(s[0].y-s[2].y)*256;edges[2]-=(s[1].y-s[0].y)*256;
                }
                row[0]+=(s[2].x-s[1].x)*256;row[1]+=(s[0].x-s[2].x)*256;row[2]+=(s[1].x-s[0].x)*256;
            }
        }
    }
    stats.ms=std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-start).count();return stats;
}
}
