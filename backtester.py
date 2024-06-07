import ctypes
import os
# 加载依赖的 DLL
ctypes.CDLL("D:/code/backtradercpp/build/Debug/boost_serialization-vc143-mt-gd-x64-1_85.dll")
ctypes.CDLL("D:/code/backtradercpp/build/Debug/fmtd.dll")
ctypes.CDLL("D:/code/backtradercpp/build/Debug/python311_d.dll")

# 加载主 DLL
dll = ctypes.CDLL("D:/code/backtradercpp/build/Debug/backtradercpp.dll")

# 定义函数的参数类型和返回类型
dll.runBacktrader.argtypes = []  # 如果函数不接受任何参数
dll.runBacktrader.restype = None  # 如果函数没有返回值

os.chdir("D:/code/backtradercpp/build/Debug")
# 调用函数
dll.runBacktrader()  # 调用函数