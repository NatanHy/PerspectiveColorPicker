cmake_minimum_required(VERSION 3.16)
project(PerspectiveColorPicker CXX)

set(CMAKE_CXX_STANDARD 20)

add_executable(app
    src/main.cpp
    src/Renderer.cpp
    src/ColorMatching.cpp
    src/utils.cpp
    src/ClipboardWatcher.cpp

    external/imgui/imgui.cpp
    external/imgui/imgui_draw.cpp
    external/imgui/imgui_widgets.cpp
    external/imgui/imgui_tables.cpp

    external/rlImGui/rlImGui.cpp
)

target_include_directories(app PRIVATE
    include
    external/imgui
    external/rlImGui
    external/raylib/src
)

target_link_directories(app PRIVATE
    external/raylib/src
)

target_link_libraries(app PRIVATE
    raylib
    opengl32
    gdi32
    winmm
)

find_package(OpenMP REQUIRED)
target_link_libraries(app PRIVATE OpenMP::OpenMP_CXX)