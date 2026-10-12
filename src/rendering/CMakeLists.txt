cmake_minimum_required(VERSION 3.21)
project(AcidRendering LANGUAGES CXX)
enable_testing()
add_executable(frame_state_test tests/frame_state_test.cpp)
target_compile_features(frame_state_test PRIVATE cxx_std_20)
target_include_directories(frame_state_test PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
add_test(NAME frame_state COMMAND frame_state_test)

if(WIN32)
  include(FetchContent)
  FetchContent_Declare(imgui
    GIT_REPOSITORY https://github.com/ocornut/imgui.git
    GIT_TAG v1.91.9b
    GIT_SHALLOW TRUE)
  FetchContent_MakeAvailable(imgui)
  add_library(acid_rendering STATIC dx11_renderer.cpp
    ${imgui_SOURCE_DIR}/imgui.cpp
    ${imgui_SOURCE_DIR}/imgui_draw.cpp
    ${imgui_SOURCE_DIR}/imgui_tables.cpp
    ${imgui_SOURCE_DIR}/imgui_widgets.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_dx11.cpp)
  target_compile_features(acid_rendering PUBLIC cxx_std_20)
  target_include_directories(acid_rendering PUBLIC ${CMAKE_CURRENT_SOURCE_DIR} ${imgui_SOURCE_DIR})
  target_compile_definitions(acid_rendering PUBLIC UNICODE _UNICODE WIN32_LEAN_AND_MEAN NOMINMAX)
  target_link_libraries(acid_rendering PUBLIC d3d11 dxgi)
  if(MSVC)
    target_compile_options(acid_rendering PRIVATE /W4 /EHsc)
  endif()
  add_executable(renderer_smoke tests/renderer_smoke.cpp)
  target_link_libraries(renderer_smoke PRIVATE acid_rendering)
  add_test(NAME renderer_smoke COMMAND renderer_smoke)
endif()
