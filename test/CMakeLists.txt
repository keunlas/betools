include(CTest)

function(config_target_compile target_name)
  set_target_properties(${target_name} PROPERTIES FOLDER "TestCases")

  if(MSVC) # MSVC
    target_compile_options(${target_name} PRIVATE /utf-8)
    target_compile_options(${target_name} PRIVATE /W3)
    target_compile_options(${target_name} PRIVATE /WX)
  else() # GCC / Clang
    target_compile_options(${target_name} PRIVATE -Wall)
    target_compile_options(${target_name} PRIVATE -Werror)
  endif()

  target_link_libraries(${target_name} PRIVATE betools::betools)
endfunction()

function(add_test_executable target_name)
  add_executable(${target_name} ${ARGN})
  config_target_compile(${target_name})
endfunction()

function(add_ctest_executable target_name)
  add_executable(${target_name} ${ARGN})
  config_target_compile(${target_name})
  add_test(NAME ${target_name} COMMAND ${target_name})
endfunction()

### Here is examples ###
# add_test_executable(my_test1 my_test1.cpp my_test1a.cpp code.cpp)
# add_ctest_executable(my_ctest2 my_ctest2.cpp my_test2a.cpp code.cpp)

add_ctest_executable(test_base62 test_base62.cpp)

add_ctest_executable(test_base64 test_base64.cpp)

add_ctest_executable(test_lock_based_queue test_lock_based_queue.cpp)

add_ctest_executable(test_threadpool test_threadpool.cpp)
target_compile_features(test_threadpool PRIVATE cxx_std_20)

add_ctest_executable(test_config test_config.cpp)

add_ctest_executable(test_singleton test_singleton.cpp)





