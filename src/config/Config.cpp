#include "Config.hpp"
#include <charconv>
#include <cmath>
#include <fstream>
#include <iomanip>
#include <map>
#include <sstream>
#include <vector>
#ifdef _WIN32
#include <windows.h>
#endif
namespace acid {
namespace {
// Deliberately narrow JSON schema: a flat object of numbers, booleans and numeric arrays.
// Reject duplicate/unknown keys, trailing data and invalid ranges; never partially apply a file.
struct Reader {
    const std::string& s;
    std::size_t pos = 0;
    void space() { while (pos < s.size() && (s[pos]==' ' || s[pos]=='\n' || s[pos]=='\r' || s[pos]=='\t')) ++pos; }
    bool take(char c) { space(); if (pos<s.size() && s[pos]==c) { ++pos; return true; } return false; }
    bool number(float& out) {
        space(); const auto start=pos;
        if (pos<s.size() && s[pos]=='-') ++pos;
        if (pos>=s.size()) return false;
        if (s[pos]=='0') ++pos;
        else { if(s[pos]<'1'||s[pos]>'9') return false; while(pos<s.size() && s[pos]>='0' && s[pos]<='9') ++pos; }
        if(pos<s.size() && s[pos]=='.') { ++pos; auto n=pos; while(pos<s.size()&&s[pos]>='0'&&s[pos]<='9') ++pos; if(n==pos) return false; }
        if(pos<s.size()&&(s[pos]=='e'||s[pos]=='E')) { ++pos; if(pos<s.size()&&(s[pos]=='+'||s[pos]=='-')) ++pos; auto n=pos; while(pos<s.size()&&s[pos]>='0'&&s[pos]<='9') ++pos; if(n==pos) return false; }
        auto result=std::from_chars(s.data()+start,s.data()+pos,out);
        return result.ec==std::errc{} && std::isfinite(out);
    }
    bool boolean(bool& v) { space(); if(s.compare(pos,4,"true")==0){pos+=4;v=true;return true;} if(s.compare(pos,5,"false")==0){pos+=5;v=false;return true;} return false; }
    template<std::size_t N> bool array(std::array<float,N>& a) { if(!take('['))return false; for(std::size_t i=0;i<N;++i) {if(i&&!take(','))return false; if(!number(a[i])||a[i]<0||a[i]>1)return false;} return take(']'); }
    bool key(std::string& key) { if(!take('"'))return false; auto start=pos; while(pos<s.size()&&s[pos]!='"') {if(s[pos]<' '||s[pos]=='\\')return false;++pos;} if(pos>=s.size())return false;key=s.substr(start,pos-start);++pos;return take(':'); }
};
}
std::string serialize(const Config& c) {
    std::ostringstream s; s.imbue(std::locale::classic()); s << std::setprecision(9) << std::boolalpha;
    s << "{\n  \"version\": 1,\n  \"accent\": [" << c.accent[0] << ", " << c.accent[1] << ", " << c.accent[2]
      << "],\n  \"previewColor\": [" << c.previewColor[0] << ", " << c.previewColor[1] << ", " << c.previewColor[2] << ", " << c.previewColor[3]
      << "],\n  \"radius\": " << c.radius << ",\n  \"thickness\": " << c.thickness << ",\n  \"crosshairSize\": " << c.crosshairSize
      << ",\n  \"background\": " << c.background << ",\n  \"circle\": " << c.circle << ",\n  \"crosshair\": " << c.crosshair
      << ",\n  \"animations\": " << c.animations << ",\n  \"vsync\": " << c.vsync << "\n}\n"; return s.str();
}
bool parseConfig(const std::string& s, Config& out, std::string& error) {
    Reader r{s}; Config c; std::map<std::string,bool> keys;
    auto fail=[&] {error="Invalid configuration JSON, schema, or value range near byte " + std::to_string(r.pos);return false;};
    if(s.size()>65536||!r.take('{')) return fail();
    if(!r.take('}')) { do {
        std::string key; if(!r.key(key)||!keys.emplace(key,true).second)return fail(); bool ok=false;
        if(key=="version") {float v=0;ok=r.number(v)&&v==1;}
        else if(key=="accent") ok=r.array(c.accent);
        else if(key=="previewColor") ok=r.array(c.previewColor);
        else if(key=="radius") ok=r.number(c.radius)&&c.radius>=20&&c.radius<=300;
        else if(key=="thickness") ok=r.number(c.thickness)&&c.thickness>=1&&c.thickness<=8;
        else if(key=="crosshairSize") ok=r.number(c.crosshairSize)&&c.crosshairSize>=4&&c.crosshairSize<=40;
        else if(key=="background") ok=r.number(c.background)&&c.background>=0.025f&&c.background<=0.12f;
        else if(key=="circle") ok=r.boolean(c.circle);
        else if(key=="crosshair") ok=r.boolean(c.crosshair);
        else if(key=="animations") ok=r.boolean(c.animations);
        else if(key=="vsync") ok=r.boolean(c.vsync);
        if(!ok) return fail();
    }while(r.take(',')); if(!r.take('}'))return fail(); }
    r.space(); if(r.pos!=s.size()||!keys.contains("version"))return fail(); out=c;error.clear();return true;
}
bool loadConfig(const std::filesystem::path& path, Config& c, std::string& error) {
    try { std::ifstream f(path,std::ios::binary); if(!f) {error="Cannot open configuration";return false;}
        std::string s(65537,'\0'); f.read(s.data(),static_cast<std::streamsize>(s.size())); s.resize(static_cast<std::size_t>(f.gcount()));
        if(f.bad()){error="Cannot read configuration";return false;} return parseConfig(s,c,error);
    } catch(const std::exception& e) {error=e.what();return false;}
}
bool saveConfig(const std::filesystem::path& path, const Config& c, std::string& error) {
    Config validated; if(!parseConfig(serialize(c),validated,error))return false;
    auto temp=path; temp += ".tmp";
    try {
        if(!path.parent_path().empty())std::filesystem::create_directories(path.parent_path());
        {std::ofstream f(temp,std::ios::binary|std::ios::trunc); if(!f){error="Cannot create configuration";return false;} f<<serialize(c); f.flush(); if(!f){error="Cannot write configuration";return false;} f.close(); if(!f){error="Cannot close configuration";return false;}}
#ifdef _WIN32
        if(!MoveFileExW(temp.c_str(),path.c_str(),MOVEFILE_REPLACE_EXISTING|MOVEFILE_WRITE_THROUGH)) {error="Cannot replace configuration (Windows error " + std::to_string(GetLastError()) + ")";std::filesystem::remove(temp);return false;}
#else
        std::filesystem::rename(temp,path);
#endif
        error.clear();return true;
    }catch(const std::exception& e){error=e.what();std::error_code ec;std::filesystem::remove(temp,ec);return false;}
}
}
