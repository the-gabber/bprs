cmake_minimum_required(VERSION 3.24)
project(bprs VERSION 1.0.0 LANGUAGES CXX)

add_library(bprs INTERFACE)
add_library(bprs::bprs ALIAS bprs)
target_include_directories(bprs INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)
target_compile_features(bprs INTERFACE cxx_std_23)

#Conditionally add tests if this is the top-level project
if(CMAKE_PROJECT_NAME STREQUAL PROJECT_NAME)
    option(ENABLE_COVERAGE "Enable code coverage reporting" OFF)
    enable_testing() # Required to activate CTest
    add_subdirectory(tests)
endif()

option(BUILD_EXAMPLE "Build example programs" ON)

if(BUILD_EXAMPLE)
    add_subdirectory(example)
endif()
