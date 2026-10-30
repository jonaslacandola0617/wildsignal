#include "meadow_patches.hpp"
#include <fstream>
#include <iostream>
int main(){try{
 auto root=std::filesystem::temp_directory_path()/"wildsignal-meadow-weight-validation";std::filesystem::create_directories(root);
 auto cfg=ws::parseJson(R"({"path":"weights.bin","world_min":[-30,-30],"world_size":[60,60],"colors_linear":[[0.3,0.7,0.1],[0.1,0.4,0.05],[0.05,0.3,0.2]]})");
 std::string bytes="WSP1";for(int v:{2,2})for(int i=0;i<4;i++)bytes+=char((v>>(i*8))&255);
 for(int k=0;k<4;k++){bytes+=char(k==0?255:0);bytes+=char(k==1?255:0);bytes+=char(k>1?255:0);bytes+=char(255);}
 auto write=[&]{std::ofstream f(root/"weights.bin",std::ios::binary);f.write(bytes.data(),bytes.size());};write();
 p3::MeadowPatches valid;valid.load(root,cfg);if(!valid.enabled||valid.width!=2||valid.height!=2||valid.weights.size()!=16)throw std::runtime_error("valid control data rejected");

 // Older packs stay regional. Optional lighting ramps are bounded, finite RGB.
 if(valid.lightDriven)throw std::runtime_error("legacy pack changed lighting");
 auto base=std::get<ws::Json::Object>(cfg.value);
 auto rampTest=[&](std::string values,bool accept){auto object=base;object["lighting_colors_linear"]=ws::parseJson(values);ws::Json c;c.value=object;bool ok=true;try{p3::MeadowPatches p;p.load(root,c);if(!p.lightDriven||p.lightColors[1].y!=.4f)throw std::runtime_error("lighting ramp missing");}catch(...){ok=false;}if(ok!=accept)throw std::runtime_error("lighting palette validation");};
 rampTest("[[0.4,0.8,0.1],[0.1,0.4,0.01],[0.01,0.1,0.03]]",true);
 rampTest("[[0.4,0.8,0.1],[0.1,0.4,0.01]]",false);
 rampTest("[[0.4,0.8,0.1],[0.1,0.4,0.01],[0.01,1.1,0.03]]",false);
 rampTest("[[0.4,0.8,0.1],[0.1,0.4,0.01],[0.01,-0.1,0.03]]",false);
 rampTest("[[0.4,0.8,0.1],[0.1,0.4,0.01],[0.01,0.1]]",false);

 // Bounded numeric light apertures are optional; no image or geometry asset.
 auto beamTest=[&](std::string apertures,std::string colors,bool accept,bool palette=true){auto object=base;if(palette)object["lighting_colors_linear"]=ws::parseJson("[[0.4,0.8,0.1],[0.1,0.4,0.01],[0.01,0.1,0.03]]");object["sun_beams"]=ws::parseJson("{\"apertures\":"+apertures+",\"colors_linear\":"+colors+"}");ws::Json c;c.value=object;bool ok=true;try{p3::MeadowPatches p;p.load(root,c);if(!p.beamDriven||p.beamCount<1||p.apertures[0][0]!=3)throw std::runtime_error("beam configuration missing");}catch(...){ok=false;}if(ok!=accept)throw std::runtime_error("beam validation");};
 std::string opening=R"({"center_xz":[3,4],"radii_xz":[2,3]})",colors="[[0.4,0.8,0.1],[0.1,0.4,0.01],[0.01,0.1,0.03]]";
 beamTest("["+opening+"]",colors,true);
 beamTest("[]",colors,false);
 beamTest("["+opening+","+opening+","+opening+","+opening+","+opening+"]",colors,false);
 beamTest(R"([{"center_xz":[3,4],"radii_xz":[0,3]}])",colors,false);
 beamTest(R"([{"center_xz":[3,4],"radii_xz":[2,13]}])",colors,false);
 beamTest(R"([{"center_xz":[3,400],"radii_xz":[2,3]}])",colors,false);
 beamTest(R"([{"center_xz":[3],"radii_xz":[2,3]}])",colors,false);
 beamTest("["+opening+"]","[[0,0,0],[0,0,0],[0,1.1,0]]",false);
 beamTest("["+opening+"]",colors,false,false);
 auto reject=[&]{write();bool failed=false;try{p3::MeadowPatches p;p.load(root,cfg);}catch(...){failed=true;}if(!failed)throw std::runtime_error("corrupt regional weights accepted");};
 bytes[12]=0;reject();bytes[12]=char(255);bytes[0]='X';reject();bytes[0]='W';bytes.pop_back();reject();
 std::filesystem::remove_all(root);std::cout<<"PASS valid numeric regional weights and bad sums/header/truncation fail closed\n";return 0;
 }catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
