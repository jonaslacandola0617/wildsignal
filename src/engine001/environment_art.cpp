#include "render.hpp"
#include <algorithm>
#include <cmath>

namespace ws {
void Renderer::makeEnvironmentArt() {
    for(auto& t:props_) t.fill(0);
    for(int y=0;y<64;++y)for(int x=0;x<64;++x) {
        const auto i=static_cast<std::size_t>(y*64+x);
        const unsigned grain=static_cast<unsigned>((x*7+y*11+(x*y)%13)%7);
        const bool beam=x<3||x>60||y<3||y>60||y==43||y==44;
        materials_[0][i]=beam?rgb(100,69,48):rgb(226+grain,207+grain,170+grain);
        materials_[1][i]=beam?rgb(107,72,49):rgb(89+grain,135+grain,127+grain);
        const bool window=x>17&&x<46&&y>10&&y<37;
        if(window) for(int m:{0,1}) materials_[static_cast<std::size_t>(m)][i]=
            (x==30||x==31||y==23)?rgb(185,151,92):rgb(42,81,86);
        if(x>24&&x<40&&y>46) for(int m:{0,1}) materials_[static_cast<std::size_t>(m)][i]=
            x%6<2?rgb(83,59,43):rgb(130,94,58);
        materials_[2][i]=x%13<3?rgb(60,52,37):rgb(99+grain,82+grain,50+grain);
        materials_[3][i]=x%32<2||y%24<2?rgb(82,92,77):rgb(119+grain,129+grain,103+grain);
        materials_[4][i]=y<18?(x%16<3?rgb(185,153,85):rgb(255,221,126)):
            (y%16<2?rgb(173,151,109):rgb(231+grain,216+grain,177+grain));
        materials_[5][i]=x%32<2||y%32<2?rgb(155,154,129):rgb(192+grain,183+grain,151+grain);
        materials_[6][i]=y%16<2?rgb(91,66,45):rgb(155+grain,116+grain,71+grain);
        if(x%31==3&&y%16==6) materials_[6][i]=rgb(62,69,59);
        materials_[7][i]=(y+(x/9))%17<2?rgb(137,185,178):rgb(66+grain,132+grain,149+grain);
        materials_[8][i]=(x*3+y*7)%61<3?rgb(113,150,94):rgb(75+grain,112+grain,72+grain);
        materials_[9][i]=(x+y*3)%53<2?rgb(113,111,73):rgb(153+grain,137+grain,89+grain);
        auto& npc=props_[0][i];
        if((x-32)*(x-32)+(y-20)*(y-20)<72) npc=rgb(218,169,112);
        if(y>9&&y<16&&x>20&&x<44) npc=rgb(51,104,112);
        if(y>=29&&y<54&&std::abs(x-32)<13) npc=x<31?rgb(83,132,129):rgb(48,98,108);
        if(y>=28&&y<33&&std::abs(x-32)<13) npc=rgb(232,198,113);
        if(y>=53&&y<63&&((x>21&&x<29)||(x>35&&x<43))) npc=rgb(63,62,54);
        if(y==20&&(x==28||x==36)) npc=rgb(54,46,40);
        if(y>=8+std::abs(x-32)*1.3&&y<63) props_[1][i]=y%10<2?rgb(102,62,49):rgb(166,100+grain,72);
        if(y>=59) props_[1][i]=rgb(213,173,108);
        if(x>=30&&x<=33&&y>19) props_[2][i]=rgb(125,99,60);
        if(y>=10&&y<20&&std::abs(x-32)<(y-8)) props_[2][i]=rgb(196,151,76);
        if(x>21&&x<43&&y>=20&&y<38) props_[2][i]=x<25||x>39?rgb(111,87,54):rgb(255,224,141);
        if(y>=38&&y<=41&&x>20&&x<44) props_[2][i]=rgb(160,116,60);
        const auto ellipse=[&](double cx,double cy,double rx,double ry) { return (x-cx)*(x-cx)/(rx*rx)+(y-cy)*(y-cy)/(ry*ry)<1; };
        if(ellipse(21,25,21,18)||ellipse(43,26,20,17)||ellipse(32,15,18,14)||ellipse(32,37,23,15)) {
            if((x*5+y*7)%29!=0) props_[3][i]=y<19?rgb(115+grain,164+grain,94):y<35?rgb(71+grain,132+grain,78):rgb(46+grain,103+grain,69);
        }
        if(y>45&&y<60&&std::abs(x-32)<(60-y)/3+1) props_[3][i]=rgb(102,88,52);
        const double ring=(x-32)*(x-32)/484.0+(y-45)*(y-45)/225.0;
        const bool trunk=std::abs(x-(30+3*std::sin(y*0.14)))<7+(y>37?3:0);
        if((trunk&&y>7&&y<59)||(ring<1.22&&ring>0.5&&y>30)||
           (y>8&&y<30&&(std::abs(x-32-(30-y)*0.7)<4||std::abs(x-32+(30-y)*0.7)<4)))
            props_[5][i]=x%9<2?rgb(90,73,43):rgb(149+grain,120+grain,63+grain);
        if(y<17&&(ellipse(18,11,15,8)||ellipse(44,10,18,9))) props_[5][i]=rgb(74+grain,136+grain,81);
        if(y>=30&&y<=43&&std::abs(x-33)<=(y-30)/2&&
           (std::abs(x-33)>=(y-30)/2-1||y>=41)) props_[5][i]=rgb(116,224,170);
        if(x>=23&&x<=27&&y>30&&y<59) props_[6][i]=rgb(222,201,143);
        if(ellipse(25,29,17,9)&&y<32) props_[6][i]=(x+y)%13<2?rgb(247,224,175):rgb(189,99,69);
        if(x>=43&&x<=46&&y>46&&y<62) props_[6][i]=rgb(207,190,139);
        if(ellipse(44,44,10,6)&&y<47) props_[6][i]=rgb(207,147,82);
        if(x>=20&&x<=22) props_[7][i]=rgb(151,119,70);
        if(x>22&&x<55&&y>=6&&y<29-(x-22)/4) props_[7][i]=y%9<3?rgb(232,199,116):rgb(53,119,120);
    }
    // Five original fronds drawn once. Transparency is binary cutout, with no colored border.
    auto dot=[&](int x,int y,Pixel color) {if(x>=0&&y>=0&&x<64&&y<64) props_[4][static_cast<std::size_t>(y*64+x)]=color;};
    for(int frond=-2;frond<=2;++frond) for(int step=0;step<=36;++step) {
        const double t=step/36.0;
        const int x=32+static_cast<int>(frond*t*14),y=59-static_cast<int>(t*(44-std::abs(frond)*5)-t*t*4);
        dot(x,y,rgb(131,164,89));
        if(step%4==0&&step>3)for(int leaf=-5;leaf<=5;++leaf) {
            dot(x+leaf,y+std::abs(leaf)/2,rgb(68,135+static_cast<unsigned>(std::abs(leaf))*3,77));
            dot(x+leaf,y+std::abs(leaf)/2+1,rgb(52,115,70));
        }
    }
    for(int style=0;style<2;++style)for(int y=0;y<128;++y)for(int x=0;x<256;++x) {
        Pixel p;
        if(style==0) {
            p=rgb(static_cast<unsigned>(105+y*0.9),static_cast<unsigned>(157+y*0.45),static_cast<unsigned>(178-y*0.1));
            const int cloud=(x+17)%83;
            if(y>24&&y<39&&cloud<31&&y>30-cloud/7) p=rgb(240,226,185);
            const int ridge=105-std::abs((x%64)-32)/3;
            if(y>ridge) p=rgb(103,150,147);
            if(y>116&&((x+11)%41)<23) p=rgb(69,117,122);
        } else {
            p=rgb(static_cast<unsigned>(69+y*0.45),static_cast<unsigned>(115+y*0.4),static_cast<unsigned>(106+y*0.25));
            const int crown=80+static_cast<int>(12*std::sin(x*0.12)+8*std::sin(x*0.31));
            if(y>crown) p=rgb(61,107,79);
            if(y<16+static_cast<int>(8*std::sin(x*0.17))) p=rgb(38,82,66);
        }
        skies_[static_cast<std::size_t>(style)][static_cast<std::size_t>(y*256+x)]=p;
    }
}
}
