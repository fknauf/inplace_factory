project(inplace_factory)

cmake_minimum_required(VERSION 3.24)

option(USE_ASAN "use adress sanitizer" on)
option(USE_STACK_PROTECTOR "use stack protector" on)

set(CMAKE_BUILD_TYPE Debug)
set(CMAKE_CXX_STANDARD 20)

add_compile_options(-Wall -Wextra -Werror)

if(USE_STACK_PROTECTOR)
  add_compile_options(-fstack-protector -fstack-protector-all)
endif(USE_STACK_PROTECTOR)

if(USE_ASAN) 
  add_compile_options(-fsanitize=address -fsanitize=undefined)
  add_link_options(-fsanitize=address -fsanitize=undefined)
endif(USE_ASAN)

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

enable_testing()

add_executable(factory_test
  tests/test.cc
  tests/group_exceptions.cc
  tests/group_mixed.cc
  tests/group_multi.cc
  tests/group_nocopy.cc
  tests/group_nocopy_nomove.cc
  tests/group_nomove.cc
  tests/group_plain.cc
  tests/group_references.cc
)
target_include_directories(factory_test BEFORE PRIVATE .)
target_link_libraries(factory_test boost_unit_test_framework)
target_compile_options(factory_test PRIVATE -Wno-self-assign-overloaded)
add_test(NAME test COMMAND factory_test)

add_executable(example examples/example.cc)
target_include_directories(example BEFORE PRIVATE .)
