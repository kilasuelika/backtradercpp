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

# 设置 Python3 路径
# set(Python3_ROOT_DIR "C:/Users/scott/AppData/Local/Programs/Python/Python39")
# set(Python3_INCLUDE_DIR "C:/Users/scott/AppData/Local/Programs/Python/Python39/include")
# set(Python3_LIBRARIES "C:/Users/scott/AppData/Local/Programs/Python/Python39/libs/python39.lib")
# set(Python3_LIBRARIES "C:/Users/scott/AppData/Local/Programs/Python/Python311/libs/python311.lib")

# 查找 pybind11
find_package(pybind11 CONFIG REQUIRED)

# set(Python3_NumPy_INCLUDE_DIRS "D:/code/quantitative-trading/venv/Lib/site-packages/numpy/core/include")


# 查找 Python3
# find_package(Python3 REQUIRED COMPONENTS Interpreter Development NumPy)
# link_directories(${Python3_LIBRARIES})

find_package(fmt CONFIG REQUIRED)
# find_package(Python3 REQUIRED COMPONENTS Interpreter Development NumPy)

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
# add_compile_options(-Wa,-mbig-obj)
# add_link_options(-Wl,--allow-multiple-definition) # 添加链接器标志

# 设置编译标志
add_compile_options("/bigobj") # 添加编译选项 /bigobj

# 创建共享库
# add_library(backtradercpp SHARED vs_examples/basic_1/basic_1.cpp)
add_library(backtradercpp SHARED 
    # vs_examples/basic_1/basic_1_main.cpp
    vs_examples/basic_1/basic_1.cpp
)

# 链接您的库与 fmt, Boost, 和 libfort
target_link_libraries(backtradercpp PRIVATE range-v3)
target_link_libraries(backtradercpp ${DataFrame_LIBRARIES})
target_link_libraries(backtradercpp PRIVATE fmt::fmt Boost::filesystem Boost::system Boost::serialization libfort::fort)
target_link_libraries(backtradercpp PRIVATE pybind11::module)
# 包含 Python 头文件路径
# include_directories(${Python3_INCLUDE_DIR} ${Python3_NumPy_INCLUDE_DIRS})
# target_link_libraries(backtradercpp PRIVATE ${Python3_LIBRARIES})



