cmake_minimum_required(VERSION 3.15)
project(ThInK_AbOuT_NaMe_Of_ThE_GaMe VERSION 1.0.0 LANGUAGES CXX)

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



# Add stb_image for texture loading
add_library(stb_image INTERFACE)
target_include_directories(stb_image INTERFACE external/stb)

# Add OpenAL for audio (you may need to find it)
find_package(OpenAL REQUIRED)

# Collect source files
file(GLOB_RECURSE SOURCES 
    src/main.cpp
    src/AI/AIController.cpp
    src/AI/AIState.cpp
    src/AI/BuildOrder.cpp
    src/Audio/Audiomanager.cpp
    src/Core/Game.cpp
    src/Core/GameState.cpp
    src/Entities/Building.cpp
    src/Entities/Entity.cpp
    src/Entities/Fighter.cpp
    src/Entities/Obstacle.cpp
    src/Entities/Peasant.cpp
    src/Entities/Unit.cpp
    src/Graphics/Camera.cpp
    src/Graphics/Renderer.cpp
    src/Graphics/Sprite.cpp
    src/Graphics/Texture.cpp
    src/Input/InputHandler.cpp
    src/Map/FogOfWar.cpp
    src/Map/Map.cpp
    src/Map/Pathfinding.cpp
    src/Map/Tile.cpp
    src/Navigation/NavMesh.cpp
    src/Navigation/SteeringSystem.cpp
    src/Serialization/SaveSystem.cpp
    src/Systems/CombatSystem.cpp
    src/Systems/CommandSystem.cpp
    src/Systems/MovementSystem.cpp
    src/Systems/PopulationSystem.cpp
    src/Systems/ResourceManager.cpp
    src/Systems/SelectionSystem.cpp
    src/Systems/TechTree.cpp
    src/UI/HUD.cpp
    src/UI/MenuSystem.cpp
    src/UI/SelectionPanel.cpp
    src/Utils/Timer.cpp
)

file(GLOB_RECURSE HEADERS 
    src/AI/AIController.h
    src/AI/AIState.h
    src/AI/BuildOrder.h
    src/Audio/AudioManager.h
    src/Core/Game.h
    src/Core/GameState.h
    src/Entities/Building.h
    src/Entities/Entity.h
    src/Entities/Fighter.h
    src/Entities/Obstacle.h
    src/Entities/Peasant.h
    src/Entities/Unit.h
    src/Graphics/Camera.h
    src/Graphics/Renderer.h
    src/Graphics/Sprite.h
    src/Graphics/Texture.h
    src/Input/InputHandler.h
    src/Map/FogOfWar.h
    src/Map/Map.h
    src/Map/Pathfinding.h
    src/Map/Tile.h
    src/Navigation/NavMesh.h
    src/Navigation/SteeringSystem.h
    src/Serialization/SaveSystem.h
    src/Systems/CombatSystem.h
    src/Systems/CommandSystem.h
    src/Systems/MovementSystem.h
    src/Systems/PopulationSystem.h
    src/Systems/ResourceManager.h
    src/Systems/SelectionSystem.h
    src/Systems/TechTree.h
    src/UI/HUD.h
    src/UI/MenuSystem.h
    src/UI/SelectionPanel.h
    src/Utils/Math.h
    src/Utils/Timer.h
)

# Create executable
add_executable(${PROJECT_NAME} ${SOURCES} ${HEADERS})

# Include directories
target_include_directories(${PROJECT_NAME} PRIVATE 
    src
    ${OPENAL_INCLUDE_DIR}
)

#glm
find_package(glm REQUIRED)

# Link libraries
target_link_libraries(${PROJECT_NAME} PRIVATE
    glfw
    OpenGL::GL
    imgui
    stb_image
    glm::glm
    ${OPENAL_LIBRARY}
    glad
    vorbis
    vorbisfile
    ogg
    mpg123
)

# Copy assets to build directory
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    ${CMAKE_SOURCE_DIR}/assets $<TARGET_FILE_DIR:${PROJECT_NAME}>/assets
)

# Create saves directory
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory
    $<TARGET_FILE_DIR:${PROJECT_NAME}>/saves
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