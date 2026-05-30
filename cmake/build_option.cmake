#
# 构建时配置选项
#

#[[

# 设置库的编译类型，默认为动态库
set(MY_LIBRARY_TYPE "SHARED" CACHE STRING "STATIC | SHARED")

# 编译出的静态库需要提供给动态库链接时启用，可能会降低代码运行性能（编译动态库时 CMake 会强制启用）
option(MY_POSITION_INDEPENDENT_CODE "启用 -fPIC 编译选项" ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ${MY_POSITION_INDEPENDENT_CODE})

# 控制是否编译 test 案例和引入 CTest 工具
option(MY_BUILD_TEST "是否编译测试代码" ON)

# 控制是否编译 example 案例
option(MY_BUILD_EXAMPLE "是否编译示例代码" ON)

# 打印调试信息
message(STATUS "MY_LIBRARY_TYPE: ${MY_LIBRARY_TYPE}")
message(STATUS "MY_POSITION_INDEPENDENT_CODE: ${MY_POSITION_INDEPENDENT_CODE}")

]]
