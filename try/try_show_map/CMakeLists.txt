cmake_minimum_required(VERSION 3.25)
project(try_show_map LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_subdirectory(lib)

find_package(OpenGL REQUIRED)
add_executable(try_show_map main.cpp)


target_link_libraries(try_show_map PRIVATE glad glfw glm)
