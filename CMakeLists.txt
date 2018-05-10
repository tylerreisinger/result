cmake_minimum_required(VERSION 2.8)
project(cpp_result)

if(MSVC)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /std:c++17")
else()
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++17")
endif()

include_directories(${CMAKE_SOURCE_DIR})
add_subdirectory("test")

