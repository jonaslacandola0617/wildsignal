#include "tree_study.hpp"
#include <bit>
#include <fstream>
#include <iostream>

using namespace p3;
static void require(bool b,const char *message){if(!b)throw std::runtime_error(message);}
int main(int argc,char **argv){try{
 static_assert(sizeof(TreeCard)==64);
 auto root=std::filesystem::temp_directory_path()/"wildsignal-tree-study-schema";
 std::filesystem::create_directories(root);
 World world;Model m;m.id="SOURCE_TREE";Primitive p;
 p.vertices.resize(4);p.vertices[0].p={-1,0,0};p.vertices[1].p={1,0,0};p.vertices[2].p={0,2,0};p.vertices[3].p={0,3,0};p.indices={0,1,2,1,2,3};
 m.parts.push_back(p);m.bounds.add(Vec{-1,0,-1});m.bounds.add(Vec{1,3,1});world.models.push_back(m);Instance i;i.model=0;i.id="selected";i.matrix=Mat::identity();world.instances.push_back(i);
 const std::string valid=R"({"revision":1,"canopy_color_linear":[0.1,0.3,0.03],"leaf_mean_linear":0.3,"instance":"selected","model":"SOURCE_TREE","leaf_texture":"leaf.png","cards_path":"cards.bin","groups":[{"center":[0,2,0],"radii":[1,1,1]}],"trunk_triangles":[[0]]})";
 std::ofstream(root/"leaf.png")<<"parser fixture only; never rendered";
 auto writeCards=[&](float width,float normal,int count=1){std::ofstream f(root/"cards.bin",std::ios::binary);f.write("WST1",4);auto word=[&](std::uint32_t v){for(int k=0;k<4;k++)f.put(char((v>>(8*k))&255));};word(count);std::array<float,16>a{0,2,0,.8f,1,0,0,width,0,1,0,0,0,normal,0,0};for(float x:a)word(std::bit_cast<std::uint32_t>(x));};
 auto check=[&](std::string text,bool expected){std::ofstream(root/"tree_study.json")<<text;bool ok=true;try{auto t=TreeStudy::load(root,world);require(t.cards.size()==1&&t.trunk[0]==std::vector<std::uint32_t>{0,1,2},"source trunk selection");require(t.canopyTriangles==1&&t.localBounds.hi.y>=3,"conservative bounds");}catch(...){ok=false;}require(ok==expected,"tree schema acceptance");};
 writeCards(.4f,1);check(valid,true);
 auto edit=[&](std::string a,std::string b){auto s=valid;auto at=s.find(a);require(at!=s.npos,"fixture edit");s.replace(at,a.size(),b);return s;};
 check(edit("leaf.png","../leaf.png"),false);check(edit("[1,1,1]","[1,0,1]"),false);check(edit("[[0]]","[[0,0]]"),false);check(edit("[[0]]","[[0,1]]"),false);check(edit("[[0]]","[[2]]"),false);check(edit("selected","missing"),false);check(edit("SOURCE_TREE","wrong"),false);
 writeCards(.4f,1,2);check(valid,false);writeCards(.4f,0);check(valid,false);writeCards(std::numeric_limits<float>::quiet_NaN(),1);check(valid,false);writeCards(3,1);check(valid,false);writeCards(.4f,1);check(valid,true);
 require(world.models[0].parts[0].indices==p.indices,"source indices changed");
 if(argc==2){auto a=std::filesystem::path(argv[1]);auto w=World::load(a/"pack.json",a/"tree_scene.json",false);auto before=w.cpuGeometryBytes();auto t=TreeStudy::load(a,w);require(t.cards.size()>100&&t.centers.size()>1,"actual source canopy groups/cards");require(w.cpuGeometryBytes()==before,"private source mutation");
  require(t.attached&&!t.rimCards.empty()&&t.rimCards.size()<t.cards.size()/2,"verified sparse rooted leaf data");
  require(t.leafUV[2]<1&&t.leafBase[1]>.8f,"exclude neighboring fragment and define base pivot");
  auto again=TreeStudy::load(a,w);require(t.rimCards.size()==again.rimCards.size(),"stable attachment count");
  for(std::size_t n=0;n<t.rimCards.size();n++){
    const auto &leaf=t.rimCards[n];require(leaf.centerHeight==again.rimCards[n].centerHeight&&leaf.upRoll==again.rimCards[n].upRoll,"deterministic attachment");
    require(leaf.upRoll[3]==0,"no random leaf roll");
    Vec c{leaf.centerHeight[0],leaf.centerHeight[1],leaf.centerHeight[2]};float radius=leaf.centerHeight[3]+leaf.rightWidth[3];
    require(t.localBounds.lo.x<=c.x-radius&&t.localBounds.hi.x>=c.x+radius&&t.localBounds.lo.y<=c.y-radius&&t.localBounds.hi.y>=c.y+radius&&t.localBounds.lo.z<=c.z-radius&&t.localBounds.hi.z>=c.z+radius,"rooted card rotation inside conservative bounds");
  }
  std::cout<<"branch-directed leaves: "<<t.rimCards.size()<<" / source tree unchanged\n";std::cout<<"actual licensed source: "<<t.cards.size()<<" cards / "<<t.centers.size()<<" canopy groups\n";}
 std::filesystem::remove_all(root);std::cout<<"tree selection, dependency, binary, bounds and source-preservation tests passed\n";return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
