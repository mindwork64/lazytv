cmake_minimum_required(VERSION 3.21)
project(minimal_client LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)

find_package(Qt6 REQUIRED COMPONENTS Core)
find_package(LgRemote REQUIRED)

qt_standard_project_setup()

qt_add_executable(minimal_client main.cpp)
target_link_libraries(minimal_client PRIVATE LgRemote::Core)