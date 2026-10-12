cmake_minimum_required(VERSION 3.21)
project(AcidExternal VERSION 2.0.0 LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)
FetchContent_Declare(imgui
  GIT_REPOSITORY https://github.com/ocornut/imgui.git
  GIT_TAG v1.91.9b
  GIT_SHALLOW TRUE)
FetchContent_MakeAvailable(imgui)

add_executable(AcidExternal WIN32
  src/main.cpp
  src/ui/premium_ui.cpp
  ${imgui_SOURCE_DIR}/imgui.cpp
  ${imgui_SOURCE_DIR}/imgui_draw.cpp
  ${imgui_SOURCE_DIR}/imgui_tables.cpp
  ${imgui_SOURCE_DIR}/imgui_widgets.cpp
  ${imgui_SOURCE_DIR}/backends/imgui_impl_win32.cpp
  ${imgui_SOURCE_DIR}/backends/imgui_impl_dx11.cpp)
target_include_directories(AcidExternal PRIVATE
  ${imgui_SOURCE_DIR} ${imgui_SOURCE_DIR}/backends)
target_link_libraries(AcidExternal PRIVATE d3d11 dxgi)
target_compile_definitions(AcidExternal PRIVATE UNICODE _UNICODE WIN32_LEAN_AND_MEAN NOMINMAX)
if(MSVC)
  target_compile_options(AcidExternal PRIVATE /W4 /EHsc)
endif()

option(ACID_UI_SMOKE_TEST "Build the headless UI smoke test" OFF)
if(ACID_UI_SMOKE_TEST)
  enable_testing()
  add_executable(AcidUiSmoke
    src/ui/smoke_test.cpp src/ui/premium_ui.cpp
    ${imgui_SOURCE_DIR}/imgui.cpp ${imgui_SOURCE_DIR}/imgui_draw.cpp
    ${imgui_SOURCE_DIR}/imgui_tables.cpp ${imgui_SOURCE_DIR}/imgui_widgets.cpp)
  target_include_directories(AcidUiSmoke PRIVATE ${imgui_SOURCE_DIR})
  # Keep assertions enabled for the smoke test in Release configurations.
  if(MSVC)
    target_compile_options(AcidUiSmoke PRIVATE /UNDEBUG)
  else()
    target_compile_options(AcidUiSmoke PRIVATE -UNDEBUG)
  endif()
  add_test(NAME AcidUiSmoke COMMAND AcidUiSmoke)
endif()

