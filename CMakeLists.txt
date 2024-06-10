cmake_minimum_required(VERSION 3.10)
project(MyProject)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# 使用 vcpkg 工具链文件
set(CMAKE_TOOLCHAIN_FILE "D:/code/vcpkg/scripts/buildsystems/vcpkg.cmake" CACHE STRING "Vcpkg toolchain file")

set(CMAKE_PREFIX_PATH "D:/code/vcpkg/installed/x64-windows" CACHE STRING "Vcpkg installation prefix")
set(fmt_DIR "D:/code/vcpkg/installed/x64-windows/include/fmt")
set(BOOST_ROOT "D:/code/vcpkg/installed/x64-windows")
set(BOOST_LIBRARYDIR "D:/code/vcpkg/installed/x64-windows/lib")
set(libfort_DIR "D:/code/vcpkg/installed/x64-windows/include/libfort")

# 查找 pybind11
find_package(pybind11 CONFIG REQUIRED)

find_package(fmt CONFIG REQUIRED)

# 查找并链接 Boost 库
set(Boost_USE_STATIC_LIBS ON) # 强制使用静态库
find_package(Boost 1.85.0 REQUIRED COMPONENTS system filesystem serialization)

# 查找并链接 libfort 库（确保您的 vcpkg 已安装 libfort）
find_package(libfort CONFIG REQUIRED)

# 包含 range-v3 库的头文件路径
find_package(range-v3 CONFIG REQUIRED)

include_directories(${DataFrame_INCLUDE_DIRS})
include_directories("D:/code/vcpkg/packages/dataframe_x64-windows/include")
find_package(DataFrame REQUIRED)

# 设置编译标志
add_compile_options("/bigobj") # 添加编译选项 /bigobj

# 创建共享库
pybind11_add_module(backtradercpp
    vs_examples/basic_1/basic_1.cpp
)

# 链接您的库与 fmt, Boost, 和 libfort
target_link_libraries(backtradercpp PRIVATE range-v3)
target_link_libraries(backtradercpp ${DataFrame_LIBRARIES})
target_link_libraries(backtradercpp PRIVATE fmt::fmt Boost::filesystem Boost::system Boost::serialization libfort::fort)
target_link_libraries(backtradercpp PRIVATE pybind11::module)
