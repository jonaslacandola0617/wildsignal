#include "sampling.hpp"
#include <algorithm>
#include <cmath>
#include <chrono>
#include <fstream>
#include <limits>
#include <stdexcept>
#include <iterator>

namespace ws {
namespace {
std::uint64_t read(std::istream& in,int bytes) {
    std::uint64_t n=0;
    for(int i=0;i<bytes;++i){const auto c=in.get();if(c==EOF)throw std::runtime_error("Truncated sampling pack");n|=static_cast<std::uint64_t>(c)<<(8*i);}
    return n;
}
SampleTexture texture(std::istream& in,int mode,bool fixed) {
    const auto width=read(in,4),height=read(in,4),expected=read(in,8);
    if(width==0||height==0||width>1024||height>1024||(!fixed&&(width!=static_cast<unsigned>(mode)||height!=width)))
        throw std::runtime_error("Invalid sampling texture dimensions");
    SampleTexture t;t.w=static_cast<int>(width);t.h=static_cast<int>(height);t.pixels.resize(static_cast<std::size_t>(width*height));
    std::uint64_t hash=14695981039346656037ull;
    for(auto& pixel:t.pixels){std::array<unsigned,4> c{};for(auto& channel:c){channel=static_cast<unsigned>(read(in,1));hash^=channel;hash*=1099511628211ull;}
        if(c[3]!=0&&c[3]!=255)throw std::runtime_error("Sampling proof supports binary alpha only");
        pixel=(c[3]<<24)|(c[0]<<16)|(c[1]<<8)|c[2];}
    if(hash!=expected)throw std::runtime_error("Sampling texture checksum mismatch");
    return t;
}
}
Pixel SampleTexture::at(double u,double v) const {
    const int x=std::clamp(static_cast<int>((u-std::floor(u))*w),0,w-1);
    const int y=std::clamp(static_cast<int>((v-std::floor(v))*h),0,h-1);
    return pixels[static_cast<std::size_t>(y*w+x)];
}
std::size_t SamplingPack::bytes() const {
    std::size_t n=fixedProp.pixels.size()*sizeof(Pixel);for(const auto& t:materials)n+=t.pixels.size()*sizeof(Pixel);return n;
}
SamplingPack SamplingPack::load(const std::filesystem::path& file) {
    std::ifstream in(file,std::ios::binary);char magic[8]{};in.read(magic,8);
    if(std::string_view(magic,8)!="WSTEX001")throw std::runtime_error("Invalid sampling pack header");
    SamplingPack p;p.mode=static_cast<int>(read(in,4));const auto count=read(in,4);
    if((p.mode!=64&&p.mode!=128&&p.mode!=256)||count<1||count>16)throw std::runtime_error("Unsupported sampling pack mode/count");
    p.materials.reserve(static_cast<std::size_t>(count));
    for(std::uint64_t i=0;i<count;++i)p.materials.push_back(texture(in,p.mode,false));
    if(std::any_of(p.materials[0].pixels.begin(),p.materials[0].pixels.end(),[](Pixel px){return (px>>24)!=255;}))throw std::runtime_error("Background material must be opaque");
    p.fixedProp=texture(in,p.mode,true);
    if(in.peek()!=EOF)throw std::runtime_error("Trailing sampling pack data");
    return p;
}
FacadeCalibration FacadeCalibration::load(const std::filesystem::path& path,std::string_view name,bool ortho,const SamplingPack& pack) {
    std::ifstream in(path,std::ios::binary|std::ios::ate);
    if(!in||in.tellg()>16384)throw std::runtime_error("Missing or oversized facade layout");
    in.seekg(0);const std::string text{std::istreambuf_iterator<char>(in),{}};
    const auto j=parseJson(text);
    if(j.at("schema").string()!="wildsignal-facade-calibration-v1")throw std::runtime_error("Invalid facade layout schema");
    FacadeCalibration f;f.scale=name;f.orthographic=ortho;
    if(name=="a"){f.doorScale=1.5;f.windowScale=1.4;}
    else if(name=="b"){f.doorScale=1.8;f.windowScale=1.7;}
    else if(name=="c"){f.doorScale=2.1;f.windowScale=2;}
    else if(name=="human"){f.doorScale=3.2;f.windowScale=2.4;}
    else if(name!="current")throw std::runtime_error("Invalid facade scale");
    const auto slot=[&](const char* key){const double n=j.at(key).number();
        if(!std::isfinite(n)||n<1||n>=static_cast<double>(pack.materials.size())||std::floor(n)!=n)throw std::runtime_error("Invalid facade material slot");
        return static_cast<int>(n);};
    const auto anchor=[&](const char* key){const double n=j.at(key).number();
        if(!std::isfinite(n)||n<0||n>1)throw std::runtime_error("Invalid facade source anchor");
        return n;};
    f.frameMaterial=slot("frameMaterial");f.doorMaterial=slot("doorMaterial");f.windowMaterial=slot("windowMaterial");
    if(f.frameMaterial==f.doorMaterial||f.frameMaterial==f.windowMaterial||f.doorMaterial==f.windowMaterial)
        throw std::runtime_error("Facade material slots must be distinct");
    f.doorAnchorU=anchor("doorAnchorU");f.doorAnchorV=anchor("doorAnchorV");
    f.windowAnchorU=anchor("windowAnchorU");f.windowAnchorV=anchor("windowAnchorV");return f;
}
FacadePanel facadeDoor(const FacadeCalibration& f) {
    // Anchor the original silhouette's bottom at z=.05; preserve its full square canvas.
    return {10.25-f.doorAnchorU*f.doorScale,.05+f.doorAnchorV*f.doorScale,f.doorScale};
}
FacadePanel facadeWindow(const FacadeCalibration& f) {
    return {12.75-f.windowAnchorU*f.windowScale,1.35+f.windowAnchorV*f.windowScale,f.windowScale};
}
World samplingWorld(bool facade) {
    World w;w.kind=SceneKind::Sampling;w.id="VIS002_CALIBRATION_001";w.name="SAMPLING / EXPERIMENTAL UNITS";w.fixtureVersion="vis002-calibration-v1";
    for(int y=0;y<24;++y)w.rows[static_cast<std::size_t>(y)]=y==0||y==23?std::string(24,'#'):"#"+std::string(22,'.')+"#";
    for(int y=7;y<=10;++y)for(int x=9;x<=13;++x)w.rows[static_cast<std::size_t>(y)][static_cast<std::size_t>(x)]='#';
    if(facade){w.id="VIS002_SCALE_CALIBRATION_001";w.name="FACADE / PROVISIONAL UNITS";w.fixtureVersion="vis002-facade-v1";}
    w.spawn={{12,16.5},0};return w;
}
DrawStats Renderer::renderSampling(Buffer& b,const World& w,const Camera& c) {
    if(b.size.w<=0||b.pixels.size()!=static_cast<std::size_t>(b.size.w*b.size.h)||b.depth.size()!=static_cast<std::size_t>(b.size.w)||b.pixelDepth.size()!=b.pixels.size()||b.owner.size()!=b.pixels.size())throw std::runtime_error("Invalid sampling framebuffer");
    using Clock=std::chrono::steady_clock;const auto start=Clock::now();const auto& pack=*sampling_;
    const double plane=std::tan(w.fov*pi/360),focal=b.size.w/(2*plane),eye=w.eye*wallHeight;
    const bool ortho=facade_&&facade_->orthographic;
    const double orthoScale=b.size.w/8.0;
    const auto doorPanel=facade_?facadeDoor(*facade_):FacadePanel{};
    const auto windowPanel=facade_?facadeWindow(*facade_):FacadePanel{};
    const Vec2 forward{std::sin(c.yaw),-std::cos(c.yaw)},right{std::cos(c.yaw),std::sin(c.yaw)};
    std::fill(b.owner.begin(),b.owner.end(),std::uint8_t{0});
    for(int y=0;y<b.size.h;++y){
        const double vertical=y+.5-b.size.h*.5,depth=vertical>0?eye*focal/vertical:std::numeric_limits<double>::infinity();
        for(int x=0;x<b.size.w;++x){const auto i=static_cast<std::size_t>(y*b.size.w+x);b.pixelDepth[i]=static_cast<float>(depth);
            if(ortho){b.pixels[i]=vertical<=eye*orthoScale?rgb(50,65,70):rgb(66,75,73);b.pixelDepth[i]=std::numeric_limits<float>::infinity();continue;}
            if(vertical<=0||depth>40){b.pixels[i]=rgb(50,65,70);continue;}
            const double offset=(2*(x+.5)/b.size.w-1)*plane;
            b.pixels[i]=pack.materials[0].at(c.position.x+depth*(forward.x+right.x*offset),c.position.y+depth*(forward.y+right.y*offset));}
    }
    for(int x=0;x<b.size.w;++x){
        const double offset=(2*(x+.5)/b.size.w-1)*plane;
        const double lateral=(x+.5-b.size.w*.5)/orthoScale;
        const Vec2 origin=ortho?Vec2{c.position.x+right.x*lateral,c.position.y+right.y*lateral}:c.position;
        // Parallel rays for a genuine in-world orthographic elevation; original DDA for perspective.
        const Vec2 direction=ortho?forward:Vec2{forward.x+right.x*offset,forward.y+right.y*offset};
        // An ortho ray can start in a solid cell when the camera approaches an edge.
        if(ortho&&w.solid(static_cast<int>(std::floor(origin.x)),static_cast<int>(std::floor(origin.y)))){
            b.depth[static_cast<std::size_t>(x)]=0;
            for(int y=0;y<b.size.h;++y)b.pixelDepth[static_cast<std::size_t>(y*b.size.w+x)]=0;
            continue;
        }
        const auto hit=castRay(w,origin,direction);b.depth[static_cast<std::size_t>(x)]=hit.depth;
        const double pixelsPerUnit=ortho?orthoScale:focal/hit.depth;
        const double top=ortho?b.size.h*.5-(wallHeight-eye)*orthoScale:b.size.h*.5-(wallHeight-eye)*focal/hit.depth;
        const double size=ortho?wallHeight*orthoScale:wallHeight*focal/hit.depth;
        const int lo=static_cast<int>(std::clamp(std::ceil(top-.5),0.0,static_cast<double>(b.size.h))),hi=static_cast<int>(std::clamp(std::ceil(top+size-.5),0.0,static_cast<double>(b.size.h)));
        // One unit square band at z=[1,2]. This is a calibration transform, not a house/module contract.
        const int cell=hit.side?hit.x-9:hit.y-7;const auto index=static_cast<std::size_t>((cell%static_cast<int>(pack.materials.size())+static_cast<int>(pack.materials.size()))%static_cast<int>(pack.materials.size()));
        const double worldX=origin.x+direction.x*hit.depth;
        for(int y=lo;y<hi;++y){const auto i=static_cast<std::size_t>(y*b.size.w+x);
            // Preserve perspective operation order, including texel-boundary rounding in old captures.
            const double z=ortho?eye+(b.size.h*.5-y-.5)/pixelsPerUnit:eye+(b.size.h*.5-y-.5)*hit.depth/focal;
            const bool perimeter=hit.x==0||hit.y==0||hit.x==23||hit.y==23;
            Pixel color=perimeter?rgb(50,65,70):pack.materials[0].at(hit.u,-z);
            if(facade_){
                // Fixed square panels: two 2.5 x 2.5 samples, never stretched to the 3.2-high wall.
                // Only the actual south-facing surface carries facade art; side views reveal a solid block.
                if(hit.side==1&&hit.y==10&&direction.y<0&&worldX>=9&&worldX<14){
                    if(z>=0&&z<2.5){const auto px=pack.materials[static_cast<std::size_t>(facade_->frameMaterial)].at((worldX-9)/2.5,1-z/2.5);if(px>>24)color=px;}
                    const auto overlay=[&](int material,FacadePanel panel){
                        const double u=(worldX-panel.left)/panel.size,v=(panel.top-z)/panel.size;
                        if(u>=0&&u<1&&v>=0&&v<1){const auto px=pack.materials[static_cast<std::size_t>(material)].at(u,v);if(px>>24)color=px;}
                    };
                    overlay(facade_->doorMaterial,doorPanel);overlay(facade_->windowMaterial,windowPanel);
                }
            }else if(!perimeter&&z>=1&&z<2){const Pixel overlay=pack.materials[index].at(hit.u,2-z);if(overlay>>24)color=overlay;}
            b.pixels[i]=color;b.pixelDepth[i]=static_cast<float>(hit.depth);}
    }
    DrawStats stats;stats.creatureCount=0;stats.worldMs=std::chrono::duration<double,std::milli>(Clock::now()-start).count();const auto spriteStart=Clock::now();
    // Fixed native-aspect prop. No resampling/cache downgrade between the three modes.
    const Sprite prop{{14.65,12},2.0*pack.fixedProp.w/pack.fixedProp.h,2,Art::Lantern};Projection p{};
    bool visible=false;
    if(ortho){const Vec2 delta{prop.position.x-c.position.x,prop.position.y-c.position.y};
        p.depth=delta.x*forward.x+delta.y*forward.y;p.width=prop.width*orthoScale;p.height=prop.height*orthoScale;
        p.left=b.size.w*.5+(delta.x*right.x+delta.y*right.y)*orthoScale-p.width*.5;
        p.top=b.size.h*.5+(eye-prop.height)*orthoScale;
        visible=p.depth>0&&p.left<b.size.w&&p.left+p.width>0&&p.top<b.size.h&&p.top+p.height>0;
    }else visible=project(c,w,b.size,prop,p);
    if(visible){
        const int x0=static_cast<int>(std::max(0.0,std::ceil(p.left-.5))),x1=static_cast<int>(std::min(static_cast<double>(b.size.w),std::ceil(p.left+p.width-.5)));
        const int y0=static_cast<int>(std::max(0.0,std::ceil(p.top-.5))),y1=static_cast<int>(std::min(static_cast<double>(b.size.h),std::ceil(p.top+p.height-.5)));
        for(int y=y0;y<y1;++y)for(int x=x0;x<x1;++x){++stats.spriteTests;const auto i=static_cast<std::size_t>(y*b.size.w+x);if(p.depth>=b.pixelDepth[i])continue;
            const Pixel pixel=pack.fixedProp.at((x+.5-p.left)/p.width,(y+.5-p.top)/p.height);if(!(pixel>>24))continue;
            b.pixels[i]=pixel;b.pixelDepth[i]=static_cast<float>(p.depth);b.owner[i]=1;++stats.spritePixels;}
    }
    stats.visibleSpritePixels=stats.spritePixels;stats.visibleSprites=stats.spritePixels?1:0;stats.scenery=1;stats.submitted=1;
    stats.spriteMs=std::chrono::duration<double,std::milli>(Clock::now()-spriteStart).count();return stats;
}
}
