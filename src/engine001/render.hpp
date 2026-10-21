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
    void resize(Resolution resolution);
    std::size_t bytes() const { return pixels.size()*sizeof(Pixel); }
};
struct Viewport { int x,y,w,h; };
Viewport letterbox(int outputW,int outputH,Resolution world);
struct RayHit { double depth; int x,y,side; double u; };
RayHit castRay(const World& world,Vec2 position,Vec2 direction);
enum class Art { Creature, Signal, Arch };
struct Sprite { Vec2 position; double width=1,height=1; Art art=Art::Creature; };
struct Projection { double depth,left,top,width,height; };
bool project(const Camera& camera,const World& world,Resolution size,const Sprite& sprite,Projection& out);
struct DrawStats { int creatureCount=1,visibleSprites=0; std::size_t spritePixels=0; double worldMs=0,spriteMs=0; };
class Renderer {
public:
    Renderer();
    DrawStats render(Buffer& buffer,const World& world,const Camera& camera,bool stress,double seconds);
    std::size_t drawSprite(Buffer& buffer,const World& world,const Camera& camera,const Sprite& sprite,double seconds) const;
    static constexpr std::size_t textureBytes=6*64*64*sizeof(Pixel);
private:
    using Texture=std::array<Pixel,64*64>;
    Texture wall_,floor_,ceiling_,creature_,signal_,arch_;
};
void savePpm(const Buffer& buffer,const std::filesystem::path& path);
}
