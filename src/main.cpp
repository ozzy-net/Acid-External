#include "core/Application.hpp"
#include <exception>
#include <string_view>
int WINAPI wWinMain(HINSTANCE instance,HINSTANCE,PWSTR commandLine,int show){
    try{acid::Application app(instance);return app.run(show,std::wstring_view(commandLine)==L"--smoke-test");}
    catch(const std::exception& error){MessageBoxA(nullptr,error.what(),"Acid External / unexpected error",MB_OK|MB_ICONERROR);return 1;}
}
