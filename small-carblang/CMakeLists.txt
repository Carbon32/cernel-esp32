cmake_minimum_required(VERSION 3.15)
project(small_carblang LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreadedDLL")
add_library(carblang_vm STATIC src/core/vm.cpp src/core/module.cpp src/core/bytecode.cpp)
target_include_directories(carblang_vm PUBLIC ${CMAKE_SOURCE_DIR}/include)

add_library(carblang_frontend STATIC src/core/core.cpp src/core/error.cpp src/core/scanner.cpp src/core/token.cpp src/core/parser.cpp src/core/compiler.cpp)
target_include_directories(carblang_frontend PUBLIC ${CMAKE_SOURCE_DIR}/include)
target_link_libraries(carblang_frontend PUBLIC carblang_vm)

add_executable(small_carblang src/vm_main.cpp)
target_link_libraries(small_carblang PRIVATE carblang_vm)

add_executable(small_carblangc src/compiler_main.cpp)
target_link_libraries(small_carblangc PRIVATE carblang_frontend)

set_target_properties(small_carblang small_carblangc PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
