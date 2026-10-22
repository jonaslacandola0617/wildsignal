#pragma once
#include "world.hpp"
#include <array>
#include <cstdint>
#include <span>
#include <vector>

namespace ws {
using Pixel=std::uint32_t; // numeric 0xAARRGGBB, SDL_PIXELFORMAT_ARGB8888; 4 bytes/pixel.
constexpr Pixel rgb(unsigned r,unsigned g,unsigned b) { return 0xFF000000u|(r<<16)|(g<<8)|b; }
constexpr double wallHeight=3.2;
struct Buffer {
    Resolution size{0,0};
    std::vector<Pixel> pixels;
    std::vector<double> depth;
    std::vector<float> pixelDepth;
    std::vector<std::uint8_t> owner;
    void resize(Resolution resolution);
    std::size_t bytes() const { return pixels.size()*sizeof(Pixel); }
};
struct Viewport { int x,y,w,h; };
Viewport letterbox(int outputW,int outputH,Resolution world);
struct RayHit { double depth; int x,y,side; double u; };
RayHit castRay(const World& world,Vec2 position,Vec2 direction);
struct Projection { double depth,left,top,width,height; };
bool project(const Camera& camera,const World& world,Resolution size,const Sprite& sprite,Projection& out);
struct DrawStats {
    int creatureCount=1,visibleSprites=0,visibleActors=0,submitted=0,scenery=0;
    std::size_t spritePixels=0,spriteTests=0,visibleSpritePixels=0;
    double worldMs=0,spriteMs=0;
};
struct SamplingPack;
class Renderer {
public:
    Renderer();
    void sampling(const SamplingPack* pack) { sampling_=pack; }
    DrawStats render(Buffer& buffer,const World& world,const Camera& camera,bool stress,double seconds);
    DrawStats render(Buffer& buffer,const World& world,const Camera& camera,int actors,double seconds);
    std::size_t drawSprite(Buffer& buffer,const World& world,const Camera& camera,const Sprite& sprite,double seconds) const;
    static constexpr std::size_t textureBytes=(6+10+8)*64*64*sizeof(Pixel)+2*256*128*sizeof(Pixel);
private:
    const SamplingPack* sampling_=nullptr;
    DrawStats renderSampling(Buffer&,const World&,const Camera&);
    using Texture=std::array<Pixel,64*64>;
    Texture wall_,floor_,ceiling_,creature_,signal_,arch_;
    std::array<Texture,10> materials_;
    std::array<Texture,8> props_;
    std::array<std::array<Pixel,256*128>,2> skies_;
    void makeEnvironmentArt();
    DrawStats renderOutdoor(Buffer&,const World&,const Camera&,int,double);
    std::size_t paintSprite(Buffer&,const World&,const Camera&,const Sprite&,double,std::uint8_t,std::size_t*) const;
    void finalVisibility(const Buffer&,std::span<const Sprite>,DrawStats&) const;
};
void savePpm(const Buffer& buffer,const std::filesystem::path& path);
}
