cmake_minimum_required(VERSION 3.25)
project(pathFind LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(pathFind main.cpp Cell.cpp Map.cpp Unit.cpp)