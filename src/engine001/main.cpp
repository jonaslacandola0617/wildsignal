#include "platform_sdl.hpp"
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <exception>
#include <iostream>

int main(int argc,char** argv) {
    try {
        const auto o=ws::options(argc,argv,false);
        if(o.help) { ws::help();return 0; }
        return ws::runSdl(o);
    } catch(const std::exception& e) {
        std::cerr<<"ENGINE-001: "<<e.what()<<'\n';
        SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR,"WILDSIGNAL ENGINE-001",e.what(),nullptr);
        return 1;
    }
}
