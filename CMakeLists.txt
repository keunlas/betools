cmake_minimum_required(VERSION 3.21)
project(betools VERSION 2.0.0)

# 控制是否编译 test 案例和引入 CTest 工具
option(${PROJECT_NAME}_BUILD_TEST "是否编译测试代码" ON)

# 控制是否编译 example 案例
option(${PROJECT_NAME}_BUILD_EXAMPLE "是否编译示例代码" ON)

# 接口目标
add_library(betools_interface INTERFACE)
target_include_directories(betools_interface INTERFACE ${PROJECT_SOURCE_DIR}/include)
add_library(betools::betools ALIAS betools_interface)

# 测试案例
if(${PROJECT_NAME}_BUILD_TEST)
  add_subdirectory(test)
endif()

# 示例代码
if(${PROJECT_NAME}_BUILD_EXAMPLE)
  add_subdirectory(example)
endif()

# 打印调试信息
message(STATUS "${PROJECT_NAME}_BUILD_TEST: ${${PROJECT_NAME}_BUILD_TEST}")
message(STATUS "${PROJECT_NAME}_BUILD_EXAMPLE: ${${PROJECT_NAME}_BUILD_EXAMPLE}")
