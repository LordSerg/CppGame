cmake_minimum_required(VERSION 3.15)
project(MapGenerator VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Options
option(BUILD_SHARED_LIBS "Build shared libraries" OFF)

#
add_subdirectory(lib)

# Find packages
find_package(OpenGL REQUIRED)
find_package(glfw3 REQUIRED)

# ImGui as subdirectory or find_package
# Assuming ImGui is in external/imgui
add_library(imgui STATIC
    external/imgui/imgui.cpp
    external/imgui/imgui_demo.cpp
    external/imgui/imgui_draw.cpp
    external/imgui/imgui_tables.cpp
    external/imgui/imgui_widgets.cpp
    external/imgui/backends/imgui_impl_glfw.cpp
    external/imgui/backends/imgui_impl_opengl3.cpp
)
target_include_directories(imgui PUBLIC 
    external/imgui
    external/imgui/backends
)
target_link_libraries(imgui PUBLIC glfw OpenGL::GL)

# Collect source files — use GLOB_RECURSE on the actual src/ directory
file(GLOB_RECURSE SOURCES
    src/main.cpp
    src/app.cpp
    src/map_generator.cpp
    src/map_data.cpp
    src/renderer.cpp
    src/noise.cpp
    src/fractal_generator.cpp
    src/pattern_cell.cpp
    src/pattern_star.cpp
    src/pattern_archipelago.cpp
)

file(GLOB_RECURSE HEADERS
    src/app.h
    src/map_generator.h
    src/map_data.h
    src/renderer.h
    src/noise.h
    src/fractal_generator.h
    src/pattern_cell.h
    src/pattern_star.h
    src/pattern_archipelago.h
    src/pathfinding.h
)

add_executable(${PROJECT_NAME} ${SOURCES} ${HEADERS})

target_include_directories(${PROJECT_NAME} PRIVATE 
    src
    ${OPENAL_INCLUDE_DIR}
)

find_package(glm REQUIRED)
find_package(Threads REQUIRED)

target_link_libraries(${PROJECT_NAME} PRIVATE
    glfw
    OpenGL::GL
    imgui
    glm::glm
    glad
    Threads::Threads
)

# Platform-specific settings
if(WIN32)
    target_compile_definitions(${PROJECT_NAME} PRIVATE PLATFORM_WINDOWS)
elseif(UNIX)
    target_compile_definitions(${PROJECT_NAME} PRIVATE PLATFORM_LINUX)
endif()

# Compiler warnings
if(MSVC)
    target_compile_options(${PROJECT_NAME} PRIVATE /W4)
else()
    target_compile_options(${PROJECT_NAME} PRIVATE -Wall -Wextra -Wpedantic)
endif()