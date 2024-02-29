cmake_minimum_required(VERSION 3.10) 
project(MyProject) 


# set(CMAKE_CXX_STANDARD 20)
# set(CMAKE_C_COMPILER "D:/mingw64/bin")
# set(CMAKE_C_STANDARD_REQUIRED True)
set(CMAKE_CXX_COMPILER "D:/mingw64/bin/g++.exe")
set(CMAKE_CXX_STANDARD_REQUIRED True)
set(CMAKE_TOOLCHAIN_FILE "D:/code/vcpkg/scripts/buildsystems/vcpkg.cmake" CACHE STRING "Vcpkg toolchain file")
include_directories("D:/code/vcpkg/installed/x64-windows/include")

add_executable(MyApp vs_examples/basic_1/basic_1.cpp)
