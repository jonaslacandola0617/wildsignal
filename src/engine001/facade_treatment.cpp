#include "facade_treatment.hpp"
#include "sampling.hpp"
#include <algorithm>
#include <cmath>
#include <fstream>
#include <iterator>
#include <stdexcept>

namespace ws {
namespace {
double number(const Json& j,const char* key,double lo,double hi){
    const double n=j.at(key).number();
    if(!std::isfinite(n)||n<lo||n>hi)throw std::runtime_error("Invalid facade treatment numeric field");
    return n;
}
int slot(const Json& j,const char* key,const SamplingPack& p,bool optional=false){
    const double n=number(j,key,optional?-1:0,static_cast<double>(p.materials.size())-1);
    if(std::floor(n)!=n)throw std::runtime_error("Non-integral treatment material slot");
    return static_cast<int>(n);
}
MaterialRegion region(const Json& j,const SamplingPack& p){
    MaterialRegion r;r.material=slot(j,"material",p);
    r.u0=number(j,"u0",0,1);r.v0=number(j,"v0",0,1);r.u1=number(j,"u1",0,1);r.v1=number(j,"v1",0,1);
    if(r.u1<=r.u0||r.v1<=r.v0)throw std::runtime_error("Empty material UV region");
    const auto& t=p.materials[static_cast<std::size_t>(r.material)];
    r.x0=std::max(0,static_cast<int>(std::ceil(r.u0*t.w-.5)));r.x1=std::min(t.w-1,static_cast<int>(std::ceil(r.u1*t.w-.5))-1);
    r.y0=std::max(0,static_cast<int>(std::ceil(r.v0*t.h-.5)));r.y1=std::min(t.h-1,static_cast<int>(std::ceil(r.v1*t.h-.5))-1);
    if(r.x1<r.x0||r.y1<r.y0)throw std::runtime_error("UV region contains no sampled pixel centers");
    return r;
}
}
Pixel MaterialRegion::at(const SamplingPack& p,double u,double v) const {
    const auto& t=p.materials[static_cast<std::size_t>(material)];
    const int x=std::clamp(static_cast<int>((u0+(u1-u0)*u)*t.w),x0,x1);
    const int y=std::clamp(static_cast<int>((v0+(v1-v0)*v)*t.h),y0,y1);
    return t.pixels[static_cast<std::size_t>(y*t.w+x)];
}
FacadeTreatment FacadeTreatment::load(const std::filesystem::path& path,std::string_view look,const SamplingPack& p){
    if(look!="a"&&look!="b"&&look!="c")throw std::runtime_error("Invalid facade look");
    std::ifstream in(path,std::ios::binary|std::ios::ate);
    if(!in||in.tellg()>16384)throw std::runtime_error("Missing or oversized facade treatment");
    in.seekg(0);const auto j=parseJson(std::string{std::istreambuf_iterator<char>(in),{}});
    if(j.at("schema").string()!="wildsignal-facade-treatment-v1")throw std::runtime_error("Unsupported facade treatment schema");
    FacadeTreatment t;t.look=look;t.plaster=slot(j,"plasterMaterial",p,true);
    if(t.plaster>=0&&std::any_of(p.materials[static_cast<std::size_t>(t.plaster)].pixels.begin(),p.materials[static_cast<std::size_t>(t.plaster)].pixels.end(),[](Pixel px){return (px>>24)!=255;}))
        throw std::runtime_error("Treatment plaster must be opaque");
    t.ground=region(j.at("ground"),p);t.vertical=region(j.at("vertical"),p);t.horizontal=region(j.at("horizontal"),p);
    if(t.vertical.aspect()<.02||t.vertical.aspect()>.4||t.horizontal.aspect()<2.5||t.horizontal.aspect()>50)
        throw std::runtime_error("Timber regions must keep their native vertical/horizontal proportions");
    const auto& ground=p.materials[static_cast<std::size_t>(t.ground.material)];
    for(int y=t.ground.y0;y<=t.ground.y1;++y)for(int x=t.ground.x0;x<=t.ground.x1;++x)
        if((ground.pixels[static_cast<std::size_t>(y*ground.w+x)]>>24)!=255)throw std::runtime_error("Ground UV region must be opaque");
    t.frameBorder=number(j,"frameBorder",.02,.25);t.groundRepeat=number(j,"groundRepeat",.2,4);
    t.base=number(j,"baseHeight",0,.4);t.doorX=number(j,"doorX",10,10.5);
    t.windowX=number(j,"windowX",12.5,13);t.windowZ=number(j,"windowZ",1.2,1.6);
    return t;
}
Pixel FacadeTreatment::floor(const SamplingPack& p,double x,double y) const {
    const double u=x/groundRepeat,v=y/(groundRepeat/ground.aspect());
    return ground.at(p,u-std::floor(u),v-std::floor(v));
}
Pixel FacadeTreatment::surface(const SamplingPack& p,int frameMaterial,Pixel baseline,double along,double z,bool front) const {
    Pixel color=plaster<0?baseline:p.materials[static_cast<std::size_t>(plaster)].at((along-9)/2.5,1-z/2.5);
    if(look=="c"&&z<base)color=floor(p,along,-z);
    if(!front||z<0||z>=2.5)return color;
    const double u=(along-9)/2.5,v=1-z/2.5;
    if(look=="b"){
        const double local=u-std::floor(u);
        if(plaster<0||local<frameBorder||local>=1-frameBorder||v<frameBorder||v>=1-frameBorder){
            const auto px=p.materials[static_cast<std::size_t>(frameMaterial)].at(u,v);if(px>>24)color=px;
        }
        return color;
    }
    // Original rectangular strips, uniform scale in each quad; no diagonal/volume claims.
    const auto quad=[&](const MaterialRegion& r,double left,double top,double width,double height){
        if(along<left||along>=left+width||z>top||z<=top-height)return;
        const double a=(along-left)/width,b=(top-z)/height;
        const auto px=r.at(p,a,b);if(px>>24)color=px;
    };
    const double railHeight=2.5/horizontal.aspect(),postHeight=2.5-base,postWidth=postHeight*vertical.aspect();
    for(double left:{9.0,11.5}){
        quad(horizontal,left,2.5,2.5,railHeight);quad(horizontal,left,base+railHeight,2.5,railHeight);
    }
    for(double left:{9.0,11.5-postWidth*.5,14-postWidth})quad(vertical,left,2.5,postWidth,postHeight);
    return color;
}
}
