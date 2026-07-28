cmake_minimum_required(VERSION 3.15)
project(WalkingSim VERSION 0.1.0 LANGUAGES CXX)

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
    src/camera/Camera.cpp
    src/core/Application.cpp
    src/core/Config.cpp
    src/formation/CircleFormation.cpp
    src/formation/LineFormation.cpp
    src/formation/SquareFormation.cpp
    src/formation/WedgeFormation.cpp
    src/input/InputManager.cpp
    src/pathfinding/AStarPathfinder.cpp
    src/pathfinding/DijkstraPathfinder.cpp
    src/pathfinding/FlowFieldPathfinder.cpp
    src/pathfinding/Grid.cpp
    src/pathfinding/NavMesh.cpp
    src/pathfinding/ThetaStarPathfinder.cpp
    src/rendering/Renderer.cpp
    src/simulation/Agent.cpp
    src/simulation/Barrier.cpp
    src/simulation/World.cpp
    src/spatial/SpatialHash.cpp
    src/steering/CollisionAvoidance.cpp
    src/steering/FlockingBehavior.cpp
    src/steering/ORCABehavior.cpp
    src/steering/SeekBehavior.cpp
    src/steering/SeparationBehavior.cpp
)

file(GLOB_RECURSE HEADERS
    src/camera/Camera.h
    src/core/Application.h
    src/core/Config.h
    src/formation/CircleFormation.h
    src/formation/IFormation.h
    src/formation/LineFormation.h
    src/formation/SquareFormation.h
    src/formation/WedgeFormation.h
    src/input/InputManager.h
    src/pathfinding/AStarPathfinder.h
    src/pathfinding/DijkstraPathfinder.h
    src/pathfinding/FlowFieldPathfinder.h
    src/pathfinding/Grid.h
    src/pathfinding/IPathfinder.h
    src/pathfinding/NavMesh.h
    src/pathfinding/ThetaStarPathfinder.h
    src/rendering/Renderer.h
    src/simulation/Agent.h
    src/simulation/Barrier.h
    src/simulation/World.h
    src/spatial/SpatialHash.h
    src/steering/CollisionAvoidance.h
    src/steering/FlockingBehavior.h
    src/steering/ISteeringBehavior.h
    src/steering/ORCABehavior.h
    src/steering/SeekBehavior.h
    src/steering/SeparationBehavior.h
)

add_executable(${PROJECT_NAME} ${SOURCES} ${HEADERS})

target_include_directories(${PROJECT_NAME} PRIVATE 
    src
    ${OPENAL_INCLUDE_DIR}
)

find_package(glm REQUIRED)

target_link_libraries(${PROJECT_NAME} PRIVATE
    glfw
    OpenGL::GL
    imgui
    glm::glm
    glad
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