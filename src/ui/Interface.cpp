#include "Interface.hpp"
#include "imgui.h"
#include <algorithm>
#include <cmath>
#include <chrono>
#include <sstream>
namespace acid {
namespace {
ImVec4 accent(const Config& c,float alpha=1){return {c.accent[0],c.accent[1],c.accent[2],alpha};}
void muted(const char* text){ImGui::TextColored({0.55f,0.60f,0.70f,1},"%s",text);}
void heading(const char* title,const char* subtitle){ImGui::SetWindowFontScale(1.45f);ImGui::TextUnformatted(title);ImGui::SetWindowFontScale(1.f);muted(subtitle);ImGui::Spacing();ImGui::Spacing();}
void panel(const char* name,ImVec2 size){ImGui::BeginChild(name,size,ImGuiChildFlags_Borders,ImGuiWindowFlags_NoSavedSettings);}
void metric(const char* id,const char* label,const std::string& value,float width,const Config& c){panel(id,{width,112});muted(label);ImGui::Spacing();ImGui::SetWindowFontScale(1.65f);ImGui::TextColored(accent(c),"%s",value.c_str());ImGui::SetWindowFontScale(1);ImGui::EndChild();}
std::string number(double value,int precision,const char* unit){std::ostringstream s;s.setf(std::ios::fixed);s.precision(precision);s<<value<<unit;return s.str();}
void graph(const char* name,History<240>& history,float height,float maximum=0){if(!history.size()){muted("Waiting for measured samples...");return;}if(maximum==0){maximum=1;for(std::size_t i=0;i<history.size();++i)maximum=std::max(maximum,history.at(i)*1.15f);}ImGui::PlotLines(name,History<240>::plot,&history,static_cast<int>(history.size()),0,nullptr,0,maximum,{-1,height});}
}
void Interface::configureStyle(){
    auto& s=ImGui::GetStyle();s.WindowPadding={20,16};s.FramePadding={12,9};s.ItemSpacing={12,12};s.WindowBorderSize=0;s.ChildBorderSize=1;s.WindowRounding=0;s.ChildRounding=12;s.FrameRounding=8;s.GrabRounding=8;s.PopupRounding=10;s.ScrollbarRounding=8;
    auto* colors=s.Colors;const float b=config_.background;
    colors[ImGuiCol_WindowBg]={b,b+0.005f,b+0.025f,1};colors[ImGuiCol_ChildBg]={b+0.018f,b+0.025f,b+0.045f,1};colors[ImGuiCol_Border]={0.19f,0.22f,0.30f,0.6f};colors[ImGuiCol_Text]={0.91f,0.93f,0.98f,1};colors[ImGuiCol_TextDisabled]={0.5f,0.55f,0.65f,1};
    colors[ImGuiCol_FrameBg]={0.14f,0.16f,0.23f,1};colors[ImGuiCol_FrameBgHovered]=accent(config_,0.35f);colors[ImGuiCol_FrameBgActive]=accent(config_,0.5f);
    colors[ImGuiCol_Button]=accent(config_,0.65f);colors[ImGuiCol_ButtonHovered]=accent(config_,0.85f);colors[ImGuiCol_ButtonActive]=accent(config_);colors[ImGuiCol_Header]=accent(config_,0.3f);colors[ImGuiCol_HeaderHovered]=accent(config_,0.4f);colors[ImGuiCol_HeaderActive]=accent(config_,0.6f);colors[ImGuiCol_CheckMark]=accent(config_);colors[ImGuiCol_SliderGrab]=accent(config_);colors[ImGuiCol_SliderGrabActive]=accent(config_);colors[ImGuiCol_PlotLines]=accent(config_);colors[ImGuiCol_PlotLinesHovered]={0.65f,0.75f,1,1};
}
void Interface::notify(const std::string& message){notice_=message;diagnostics_.log(message);}
void Interface::draw(HWND hwnd){
    configureStyle();auto& io=ImGui::GetIO();ImGui::SetNextWindowPos({0,0});ImGui::SetNextWindowSize(io.DisplaySize);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding,{0,0});ImGui::Begin("Acid workspace",nullptr,ImGuiWindowFlags_NoDecoration|ImGuiWindowFlags_NoMove|ImGuiWindowFlags_NoSavedSettings|ImGuiWindowFlags_NoBringToFrontOnFocus);ImGui::PopStyleVar();
    // The native hit-test exposes the empty title area as HTCAPTION for drag / snap / double-click.
    const float scale=static_cast<float>(GetDpiForWindow(hwnd))/96.f;
    ImGui::SetCursorPos({22,13});ImGui::TextColored(accent(config_),"A C I D");ImGui::SameLine();muted("/  EXTERNAL V2");
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding,{0,0});ImGui::SetCursorPos({io.DisplaySize.x-144*scale,0});
    if(ImGui::Button("_",{48*scale,48*scale}))ShowWindow(hwnd,SW_MINIMIZE);ImGui::SameLine(0,0);
    if(ImGui::Button(IsZoomed(hwnd)?"[]":"[ ]",{48*scale,48*scale}))ShowWindow(hwnd,IsZoomed(hwnd)?SW_RESTORE:SW_MAXIMIZE);ImGui::SameLine(0,0);
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered,{0.8f,0.2f,0.3f,1});if(ImGui::Button("X",{48*scale,48*scale}))PostMessageW(hwnd,WM_CLOSE,0,0);ImGui::PopStyleColor();ImGui::PopStyleVar();
    ImGui::SetCursorPos({16,56*scale});
    panel("Sidebar",{190*scale,std::max(1.f,io.DisplaySize.y-72*scale)});muted("WORKSPACE");ImGui::Spacing();
    constexpr const char* tabs[]={"Dashboard","Visualization","Diagnostics","Settings"};
    for(int i=0;i<4;++i){ImGui::PushID(i);const auto p=ImGui::GetCursorScreenPos();const ImVec2 size{ImGui::GetContentRegionAvail().x,48*scale};
        if(ImGui::InvisibleButton(tabs[i],size)){if(tab_!=i){tab_=i;transition_=config_.animations?0:1;}}
        const float desired=ImGui::IsItemHovered()?1.f:0.f;hover_[i]=config_.animations?hover_[i]+(desired-hover_[i])*std::min(1.f,io.DeltaTime*12):desired;
        auto* draw=ImGui::GetWindowDrawList();const float alpha=tab_==i?0.28f:0.03f+hover_[i]*0.13f;
        draw->AddRectFilled(p,{p.x+size.x,p.y+size.y},ImGui::ColorConvertFloat4ToU32(accent(config_,alpha)),8*scale);
        if(tab_==i)draw->AddRectFilled({p.x,p.y+12*scale},{p.x+3*scale,p.y+size.y-12*scale},ImGui::ColorConvertFloat4ToU32(accent(config_)),2);
        draw->AddText({p.x+15*scale,p.y+15*scale},ImGui::GetColorU32(tab_==i?ImGuiCol_Text:ImGuiCol_TextDisabled),tabs[i]);ImGui::PopID();}
    ImGui::Spacing();ImGui::Separator();ImGui::Spacing();muted("LOCAL DIAGNOSTICS");ImGui::TextWrapped("DirectX 11\nC++20 / Dear ImGui");ImGui::Spacing();ImGui::TextColored({0.3f,0.85f,0.6f,1},"Session active");ImGui::EndChild();
    ImGui::SameLine();panel("Page",{0,std::max(1.f,io.DisplaySize.y-72*scale)});
    transition_=config_.animations?std::min(1.f,transition_+io.DeltaTime*5.f):1;
    ImGui::PushStyleVar(ImGuiStyleVar_Alpha,0.25f+0.75f*transition_);ImGui::SetCursorPosY(ImGui::GetCursorPosY()+(1-transition_)*8*scale);
    switch(tab_){case 0:dashboard();break;case 1:visualization();break;case 2:diagnostics();break;default:settings();break;}
    if(!notice_.empty()){ImGui::Spacing();ImGui::Separator();ImGui::TextWrapped("%s",notice_.c_str());}
    ImGui::PopStyleVar();ImGui::EndChild();ImGui::End();
}
void Interface::dashboard(){
    heading("Overview","Your application, measured in real time.");const float width=(ImGui::GetContentRegionAvail().x-12)/2;
    const float mean=diagnostics_.frames.mean();metric("FPS","APPLICATION FPS",mean>0?number(1000.0/mean,1," fps"):"Sampling...",width,config_);ImGui::SameLine();metric("Time","MEAN FRAME INTERVAL",number(mean,2," ms"),width,config_);
    metric("CPU","PROCESS CPU / ALL LOGICAL CORES",diagnostics_.cpuAvailable&&diagnostics_.cpuHistory.size()?number(diagnostics_.cpuPercent,1," %"):"Sampling...",width,config_);ImGui::SameLine();metric("Memory","PROCESS WORKING SET",diagnostics_.memoryAvailable?number(diagnostics_.workingMiB,1," MiB"):"Sampling...",width,config_);
    panel("Frame cadence",{0,185});ImGui::TextUnformatted("Frame cadence");muted("Last 240 active frame intervals, including presentation wait");graph("##cadence",diagnostics_.frames,100);ImGui::EndChild();
    panel("Session",{0,0});ImGui::TextColored({0.3f,0.85f,0.6f,1},"Operational");ImGui::SameLine();ImGui::Text("/ Session uptime: %.0f s",diagnostics_.uptime());ImGui::TextWrapped("%s",diagnostics_.system.c_str());ImGui::TextWrapped("Renderer: %s",diagnostics_.gpu.c_str());muted(config_.vsync?"Presentation: synchronized":"Presentation: unsynchronized, capped near 240 fps");ImGui::EndChild();
}
void Interface::visualization(){
    heading("Visualization","An independent local canvas for circle and crosshair design.");
    ImGui::Checkbox("FOV circle",&config_.circle);ImGui::SameLine();ImGui::Checkbox("Crosshair",&config_.crosshair);
    ImGui::SliderFloat("Radius",&config_.radius,20,300,"%.0f px");ImGui::SliderFloat("Stroke",&config_.thickness,1,8,"%.1f px");ImGui::SliderFloat("Crosshair size",&config_.crosshairSize,4,40,"%.0f px");ImGui::ColorEdit4("Preview color",config_.previewColor.data(),ImGuiColorEditFlags_NoInputs);
    const auto origin=ImGui::GetCursorScreenPos();const auto available=ImGui::GetContentRegionAvail();const ImVec2 size{std::max(1.f,available.x),std::max(160.f,available.y-8)};
    ImGui::InvisibleButton("Independent canvas",size);auto* draw=ImGui::GetWindowDrawList();const ImVec2 end{origin.x+size.x,origin.y+size.y};draw->AddRectFilled(origin,end,IM_COL32(12,16,27,255),12);draw->PushClipRect(origin,end,true);
    for(float x=origin.x+24;x<end.x;x+=24)draw->AddLine({x,origin.y},{x,end.y},IM_COL32(32,39,56,120));for(float y=origin.y+24;y<end.y;y+=24)draw->AddLine({origin.x,y},{end.x,y},IM_COL32(32,39,56,120));
    const ImVec2 center{origin.x+size.x/2,origin.y+size.y/2};const auto color=ImGui::ColorConvertFloat4ToU32({config_.previewColor[0],config_.previewColor[1],config_.previewColor[2],config_.previewColor[3]});
    if(config_.circle)draw->AddCircle(center,config_.radius,color,128,config_.thickness);
    if(config_.crosshair){const auto length=config_.crosshairSize;draw->AddLine({center.x-length,center.y},{center.x+length,center.y},color,config_.thickness);draw->AddLine({center.x,center.y-length},{center.x,center.y+length},color,config_.thickness);}
    draw->AddText({origin.x+16,origin.y+14},IM_COL32(130,145,175,255),"PREVIEW CANVAS / PIXEL UNITS");draw->PopClipRect();
}
void Interface::diagnostics(){
    heading("Diagnostics","Performance history, system details and a session event console.");
    if(ImGui::Button("Export JSON report")){auto path=configPath_.parent_path()/"reports"/("diagnostics-"+std::to_string(std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count())+".json");std::string error;if(diagnostics_.exportReport(path,error))notify("Report exported to "+path.string());else notify("Export failed: "+error);}
    ImGui::SameLine();if(ImGui::Button("Clear performance history")){diagnostics_.frames.clear();diagnostics_.cpuHistory.clear();diagnostics_.memoryHistory.clear();notify("Performance history cleared.");}
    panel("Performance",{0,340});ImGui::TextUnformatted("Frame interval / milliseconds");graph("##frame",diagnostics_.frames,70);ImGui::TextUnformatted("Process CPU / percent of all logical cores");graph("##cpu",diagnostics_.cpuHistory,70,100);ImGui::TextUnformatted("Working set / MiB (0.5 second samples)");graph("##memory",diagnostics_.memoryHistory,70);ImGui::EndChild();
    ImGui::TextWrapped("%s",diagnostics_.system.c_str());ImGui::TextWrapped("%s",diagnostics_.gpu.c_str());if(diagnostics_.memoryAvailable)ImGui::Text("Private committed memory: %.1f MiB",diagnostics_.privateMiB);
    if(ImGui::Button("Clear console")){diagnostics_.logs.clear();}ImGui::SameLine();muted("EVENT CONSOLE / LAST 200 EVENTS");
    panel("Console",{0,180});const bool atBottom=ImGui::GetScrollY()>=ImGui::GetScrollMaxY();for(const auto& line:diagnostics_.logs)ImGui::TextUnformatted(line.c_str());if(atBottom)ImGui::SetScrollHereY(1);ImGui::EndChild();
}
void Interface::settings(){
    heading("Settings","Tune the workspace. Preferences persist between sessions.");
    panel("Appearance",{0,280});ImGui::TextUnformatted("Appearance");muted("ACCENT PRESETS");const std::array<std::array<float,3>,3> presets={{{0.43f,0.38f,0.96f},{0.2f,0.55f,0.98f},{0.64f,0.32f,0.94f}}};const char* names[]={"Indigo","Blue","Purple"};
    for(int i=0;i<3;++i){if(i)ImGui::SameLine();if(ImGui::Button(names[i]))config_.accent=presets[i];}
    ImGui::ColorEdit3("Custom accent",config_.accent.data(),ImGuiColorEditFlags_NoInputs);ImGui::SliderFloat("Dark background",&config_.background,0.025f,0.12f,"%.3f");ImGui::Checkbox("Navigation and hover animations",&config_.animations);ImGui::Checkbox("Synchronize presentation (VSync)",&config_.vsync);ImGui::EndChild();
    panel("Persistence",{0,0});ImGui::TextUnformatted("Configuration");ImGui::TextWrapped("%s",configPath_.string().c_str());
    if(ImGui::Button("Save configuration")){std::string error;notify(saveConfig(configPath_,config_,error)?"Configuration saved.":"Save failed: "+error);}ImGui::SameLine();
    if(ImGui::Button("Load configuration")){std::string error;notify(loadConfig(configPath_,config_,error)?"Configuration loaded.":"Load failed: "+error);}
    if(ImGui::Button("Reset to defaults")){config_=Config{};notify("Defaults restored. Save to persist them immediately.");}muted("Current settings also save when the application exits.");ImGui::EndChild();
}
}
