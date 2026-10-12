#include "Diagnostics.hpp"
#include <windows.h>
#include <psapi.h>
#include <fstream>
#include <iomanip>
#include <sstream>
namespace acid {
namespace {
std::uint64_t ticks(FILETIME t){return (static_cast<std::uint64_t>(t.dwHighDateTime)<<32)|t.dwLowDateTime;}
bool processTime(std::uint64_t& value) {FILETIME created{},exit{},kernel{},user{}; if(!GetProcessTimes(GetCurrentProcess(),&created,&exit,&kernel,&user))return false;value=ticks(kernel)+ticks(user);return true;}
std::string escaped(const std::string& value){std::string s;for(unsigned char c:value){if(c=='"'||c=='\\'){s+='\\';s+=static_cast<char>(c);}else if(c=='\n')s+="\\n";else if(c>=32)s+=static_cast<char>(c);}return s;}
}
Diagnostics::Diagnostics() {
    SYSTEM_INFO info{};GetNativeSystemInfo(&info);processors_=std::max(1UL,info.dwNumberOfProcessors);
    MEMORYSTATUSEX memory{};memory.dwLength=sizeof(memory);
    std::ostringstream s;s<<"Windows | "<<processors_<<" logical processors | "<<(sizeof(void*)*8)<<"-bit application";
    if(GlobalMemoryStatusEx(&memory))s<<" | "<<(memory.ullTotalPhys/(1024ULL*1024ULL))<<" MiB physical RAM";
    system=s.str();cpuAvailable=processTime(lastCpu_);log("Diagnostics initialized; measuring this application's process only.");
}
void Diagnostics::tick() {
    const auto now=Clock::now();frameMs=std::chrono::duration<float,std::milli>(now-previous_).count();previous_=now;frames.push(frameMs);
    const double elapsed=std::chrono::duration<double>(now-sampled_).count();if(elapsed<0.5)return;
    std::uint64_t current=0;const bool valid=processTime(current);
    if(valid&&cpuAvailable) {cpuPercent=static_cast<float>(100.0*static_cast<double>(current-lastCpu_)/(elapsed*1e7*processors_));cpuHistory.push(cpuPercent);}
    cpuAvailable=valid;lastCpu_=current;sampled_=now;
    PROCESS_MEMORY_COUNTERS_EX counters{};counters.cb=sizeof(counters);
    memoryAvailable=GetProcessMemoryInfo(GetCurrentProcess(),reinterpret_cast<PROCESS_MEMORY_COUNTERS*>(&counters),sizeof(counters))!=FALSE;
    if(memoryAvailable){workingMiB=static_cast<double>(counters.WorkingSetSize)/1048576.0;privateMiB=static_cast<double>(counters.PrivateUsage)/1048576.0;memoryHistory.push(static_cast<float>(workingMiB));}
}
void Diagnostics::resume(){previous_=sampled_=Clock::now();cpuAvailable=processTime(lastCpu_);}
double Diagnostics::uptime()const{return std::chrono::duration<double>(Clock::now()-start_).count();}
void Diagnostics::log(const std::string& message){std::ostringstream s;s<<'['<<std::fixed<<std::setprecision(1)<<uptime()<<"s] "<<message;if(logs.size()==200)logs.pop_front();logs.push_back(s.str());}
bool Diagnostics::exportReport(const std::filesystem::path& path,std::string& error)const {
    try {
        std::filesystem::create_directories(path.parent_path());std::ofstream f(path,std::ios::binary|std::ios::trunc);f.imbue(std::locale::classic());
        f<<std::setprecision(9)<<"{\n  \"schema_version\": 1,\n  \"scope\": \"Acid External process\",\n  \"system\": \""<<escaped(system)<<"\",\n  \"gpu\": \""<<escaped(gpu)<<"\",\n  \"uptime_seconds\": "<<uptime()<<",\n  \"process_cpu_percent\": ";
        if(cpuAvailable)f<<cpuPercent;else f<<"null";
        f<<",\n  \"working_set_mib\": ";if(memoryAvailable)f<<workingMiB;else f<<"null";
        f<<",\n  \"private_commit_mib\": ";if(memoryAvailable)f<<privateMiB;else f<<"null";
        auto history=[&](const char* name,const auto& h){f<<",\n  \""<<name<<"\": [";for(std::size_t i=0;i<h.size();++i){if(i)f<<", ";f<<h.at(i);}f<<']';};
        history("frame_time_ms",frames);history("process_cpu_history_percent",cpuHistory);history("working_set_history_mib",memoryHistory);
        f<<",\n  \"log\": [";for(std::size_t i=0;i<logs.size();++i){if(i)f<<", ";f<<'"'<<escaped(logs[i])<<'"';}f<<"]\n}\n";f.flush();if(!f){error="Cannot write diagnostic report";return false;}f.close();if(!f){error="Cannot close diagnostic report";return false;}error.clear();return true;
    }catch(const std::exception& e){error=e.what();return false;}
}
}
