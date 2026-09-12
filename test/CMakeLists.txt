include(CTest)

function(add_test_executable target_name)
  add_executable(${target_name} ${ARGN})
  set_target_properties(${target_name} PROPERTIES FOLDER "TestCases")
endfunction()

function(add_ctest_executable target_name)
  add_executable(${target_name} ${ARGN})
  add_test(NAME ${target_name} COMMAND ${target_name})
  set_target_properties(${target_name} PROPERTIES FOLDER "TestCases")
endfunction()

### Here is examples ###
# add_test_executable(my_test1 my_test1.cpp my_test1a.cpp code.cpp)
# add_ctest_executable(my_ctest2 my_ctest2.cpp my_test2a.cpp code.cpp)

add_ctest_executable(test_base62 test_base62.cpp)
target_link_libraries(test_base62 PRIVATE betools::betools)

add_ctest_executable(test_base64 test_base64.cpp)
target_link_libraries(test_base64 PRIVATE betools::betools)

add_ctest_executable(test_lock_based_queue test_lock_based_queue.cpp)
target_link_libraries(test_lock_based_queue PRIVATE betools::betools)

add_ctest_executable(test_threadpool test_threadpool.cpp)
target_link_libraries(test_threadpool PRIVATE betools::betools)

add_ctest_executable(test_config test_config.cpp)
target_link_libraries(test_config PRIVATE betools::betools)

add_ctest_executable(test_singleton test_singleton.cpp)
target_link_libraries(test_singleton PRIVATE betools::betools)

### Endian ###
# 自动分类：验证语言与编译器到后端的映射
add_ctest_executable(test_endian test_endian.cpp)
target_link_libraries(test_endian PRIVATE betools::betools)
target_compile_definitions(test_endian PRIVATE BETOOLS_ENDIAN_TEST_AUTO)

# 兼容 C++11：验证标准版本相关的条件编译
add_ctest_executable(test_endian_cxx11 test_endian.cpp)
target_link_libraries(test_endian_cxx11 PRIVATE betools::betools)
target_compile_definitions(test_endian_cxx11 PRIVATE BETOOLS_ENDIAN_TEST_AUTO)
set_target_properties(test_endian_cxx11 PROPERTIES CXX_STANDARD 11 CXX_STANDARD_REQUIRED ON)

# 关闭不带前缀的同名接口后，带前缀的接口依然可用
add_ctest_executable(test_endian_no_bare_names test_endian_no_bare_names.cpp)
target_link_libraries(test_endian_no_bare_names PRIVATE betools::betools)

# C 语言接口
add_ctest_executable(test_endian_c test_endian.c)
target_link_libraries(test_endian_c PRIVATE betools::betools)
target_compile_definitions(test_endian_c PRIVATE BETOOLS_ENDIAN_TEST_AUTO)

# 强制大端分类：在小端机器上同样覆盖大端分支，源码同时按 C 与 C++ 编译
add_ctest_executable(test_endian_forced_big_c test_endian_forced_big.c)
target_link_libraries(test_endian_forced_big_c PRIVATE betools::betools)
target_compile_definitions(test_endian_forced_big_c PRIVATE
  BETOOLS_ENDIAN_FORCE_HOST_ORDER=BETOOLS_ENDIAN_BIG)

add_ctest_executable(test_endian_forced_big_cpp test_endian_forced_big.cpp)
target_link_libraries(test_endian_forced_big_cpp PRIVATE betools::betools)
target_compile_definitions(test_endian_forced_big_cpp PRIVATE
  BETOOLS_ENDIAN_FORCE_HOST_ORDER=BETOOLS_ENDIAN_BIG)

# 后端覆盖：同一份用例分别强制走各条已有设施的分支
add_ctest_executable(test_endian_manual test_endian.cpp)
target_link_libraries(test_endian_manual PRIVATE betools::betools)
target_compile_definitions(test_endian_manual PRIVATE
  BETOOLS_ENDIAN_BACKEND=BETOOLS_ENDIAN_BACKEND_MANUAL
  BETOOLS_ENDIAN_EXPECT_BACKEND=BETOOLS_ENDIAN_BACKEND_MANUAL)

if(MSVC)
  add_ctest_executable(test_endian_msvc test_endian.cpp)
  target_link_libraries(test_endian_msvc PRIVATE betools::betools)
  target_compile_definitions(test_endian_msvc PRIVATE
    BETOOLS_ENDIAN_BACKEND=BETOOLS_ENDIAN_BACKEND_MSVC
    BETOOLS_ENDIAN_EXPECT_BACKEND=BETOOLS_ENDIAN_BACKEND_MSVC)
else()
  add_ctest_executable(test_endian_builtin test_endian.cpp)
  target_link_libraries(test_endian_builtin PRIVATE betools::betools)
  target_compile_definitions(test_endian_builtin PRIVATE
    BETOOLS_ENDIAN_BACKEND=BETOOLS_ENDIAN_BACKEND_BUILTIN
    BETOOLS_ENDIAN_EXPECT_BACKEND=BETOOLS_ENDIAN_BACKEND_BUILTIN)
endif()

if(APPLE)
  add_ctest_executable(test_endian_osswap test_endian.cpp)
  target_link_libraries(test_endian_osswap PRIVATE betools::betools)
  target_compile_definitions(test_endian_osswap PRIVATE
    BETOOLS_ENDIAN_BACKEND=BETOOLS_ENDIAN_BACKEND_OSSWAP
    BETOOLS_ENDIAN_EXPECT_BACKEND=BETOOLS_ENDIAN_BACKEND_OSSWAP)
endif()

if("cxx_std_23" IN_LIST CMAKE_CXX_COMPILE_FEATURES)
  add_ctest_executable(test_endian_std test_endian.cpp)
  target_link_libraries(test_endian_std PRIVATE betools::betools)
  target_compile_features(test_endian_std PRIVATE cxx_std_23)
  target_compile_definitions(test_endian_std PRIVATE
    BETOOLS_ENDIAN_BACKEND=BETOOLS_ENDIAN_BACKEND_STD
    BETOOLS_ENDIAN_EXPECT_BACKEND=BETOOLS_ENDIAN_BACKEND_STD)
endif()
