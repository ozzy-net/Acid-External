#include "config/Config.hpp"
#include "diagnostics/History.hpp"
#include <iostream>
#include <chrono>
#include <stdexcept>
#include <limits>
void require(bool condition,const char* message){if(!condition)throw std::runtime_error(message);}
int main(){try{
    acid::Config c;c.accent={0.1f,0.2f,0.3f};c.radius=237;c.animations=false;c.vsync=false;
    acid::Config result;std::string error;
    require(acid::parseConfig(acid::serialize(c),result,error)&&result==c,"JSON round trip");
    for(const auto* invalid:{"{}","{\"version\":2}","{\"version\":1,\"radius\":-20}","{\"version\":1,\"radius\":01}","{\"version\":1,\"radius\":1e999}","{\"version\":1,\"circle\":0}","{\"version\":1,\"accent\":[1,2,3]}","{\"version\":1,\"version\":1}","{\"version\":1,}","{\"version\":1} garbage","{\"version\":1,\"unknown\":true}"}){
        result=c;require(!acid::parseConfig(invalid,result,error),"Reject malformed schema");require(result==c,"Invalid load must not change live config");}
    require(!acid::parseConfig(std::string(70000,' '),result,error),"Reject oversized file");
    require(acid::parseConfig("{\"version\":1}",result,error)&&result==acid::Config{},"Missing optional fields default");
    const auto folder=std::filesystem::temp_directory_path()/("acid-tests-"+std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
    const auto path=folder/"config.json";
    require(acid::saveConfig(path,c,error),"Save config");require(acid::loadConfig(path,result,error)&&result==c,"Load config");
    c.radius=42;require(acid::saveConfig(path,c,error)&&acid::loadConfig(path,result,error)&&result==c,"Replace existing config atomically");
    c.radius=std::numeric_limits<float>::quiet_NaN();require(!acid::saveConfig(path,c,error),"Reject nonfinite values on save");
    require(acid::loadConfig(path,result,error)&&result.radius==42,"Failed save preserves old file");std::filesystem::remove_all(folder);
    acid::History<3> history;require(history.size()==0&&history.mean()==0,"Empty history");history.push(1);history.push(2);history.push(3);history.push(4);
    require(history.size()==3&&history.at(0)==2&&history.at(2)==4&&history.mean()==3,"Ring wrap chronology and average");history.clear();history.push(9);require(history.size()==1&&history.at(0)==9,"Clear ring");
    std::cout<<"All configuration and history checks passed\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
