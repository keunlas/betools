cmake_minimum_required(VERSION 3.21)
project(betools VERSION 1.5.4)

include(cmake/build_option.cmake)
include(cmake/compile_setting.cmake)

add_library(betools_interface INTERFACE)
target_include_directories(betools_interface INTERFACE ${PROJECT_SOURCE_DIR}/deps/include)
target_include_directories(betools_interface INTERFACE ${PROJECT_SOURCE_DIR}/include)
# target_compile_features(betools_interface INTERFACE cxx_std_23)
add_library(betools::betools ALIAS betools_interface)

if(${PROJECT_NAME}_BUILD_TEST)
  add_subdirectory(test)
endif()

if(${PROJECT_NAME}_BUILD_EXAMPLE)
  add_subdirectory(example)
endif()

include(cmake/install.cmake)
include(cmake/cpack.cmake)
