# cmake_minimum_required(VERSION 3.10) 
# project(MyProject) 


# set(CMAKE_CXX_STANDARD 20)
# # set(CMAKE_C_COMPILER "D:/mingw64/bin")
# # set(CMAKE_C_STANDARD_REQUIRED True)
# set(CMAKE_CXX_COMPILER "D:/mingw64/bin/g++.exe")
# set(CMAKE_CXX_STANDARD_REQUIRED True)


# set(CMAKE_TOOLCHAIN_FILE "D:/code/vcpkg/scripts/buildsystems/vcpkg.cmake" CACHE STRING "Vcpkg toolchain file")
# include_directories("D:/code/vcpkg/installed/x64-windows/include")
# include_directories("D:/code/vcpkg/installed/x64-windows/include/boost")
# set(CMAKE_PREFIX_PATH "${CMAKE_PREFIX_PATH};D:/code/vcpkg/installed/x64-windows/include/fmt")

# # add_executable(MyApp vs_examples/basic_1/basic_1.cpp)
# add_library(backtradercpp SHARED vs_examples/basic_1/basic_1.cpp)

# find_package(fmt REQUIRED)
# target_link_libraries(MyApp PRIVATE fmt::fmt)
# # find_package(fort REQUIRED)
# target_include_directories(MyApp PRIVATE "D:/code/vcpkg/installed/x64-windows/include")

# target_link_libraries(MyApp PRIVATE "D:/code/vcpkg/installed/x64-windows/lib/fort.lib")


# find_package(Boost REQUIRED COMPONENTS system filesystem serialization)
# if(Boost_FOUND)
#   include_directories(${Boost_INCLUDE_DIRS})
#   target_link_libraries(MyApp PRIVATE ${Boost_LIBRARIES})
# endif()


cmake_minimum_required(VERSION 3.10) 
project(MyProject) 

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)
set(CMAKE_CXX_COMPILER "D:/mingw64/bin/g++.exe")
set(CMAKE_TOOLCHAIN_FILE "D:/code/vcpkg/scripts/buildsystems/vcpkg.cmake" CACHE STRING "Vcpkg toolchain file")
set(CMAKE_PREFIX_PATH "${CMAKE_PREFIX_PATH};D:/code/vcpkg/installed/x64-windows/include/fmt")

include_directories("D:/code/vcpkg/installed/x64-windows/include")
include_directories("D:/code/vcpkg/installed/x64-windows/include/boost")


# 只有当您需要创建可执行文件时
# add_executable(MyApp vs_examples/basic_1/basic_1.cpp)

# 或者，如果您需要创建一个共享库
# add_executable(backtradercpp vs_examples/csv_directory_data_extra/csv_directory_data_extra.cpp)
add_library(backtradercpp SHARED vs_examples/csv_directory_data_extra/csv_directory_data_extra.cpp)

# target_include_directories(backtradercpp PRIVATE "D:/code/vcpkg/installed/x64-windows/lib/fort.lib")
# target_link_libraries(backtradercpp PRIVATE "D:/code/vcpkg/installed/x64-windows/lib/fort.lib")
set(libfort_DIR "D:/code/vcpkg/packages/libfort_x64-windows/share/libfort/libfort-config.cmake")
find_package(libfort)
target_link_libraries(backtradercpp PRIVATE libfort::fort)

find_package(fmt REQUIRED)
target_link_libraries(backtradercpp PRIVATE fmt::fmt)

set(BOOST_ROOT "D:/code/vcpkg/installed/x64-windows/include")
set(BOOST_LIBRARYDIR "D:/code/vcpkg/installed/x64-windows/lib")
find_package(Boost REQUIRED COMPONENTS system filesystem serialization)
if(Boost_FOUND)
  target_include_directories(backtradercpp PRIVATE ${Boost_INCLUDE_DIRS})
  target_link_libraries(backtradercpp PRIVATE ${Boost_LIBRARIES})
endif()




