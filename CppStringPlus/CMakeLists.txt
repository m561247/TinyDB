# CMakeLists.txt for CppStringPlus
#
# © 2018-2019 by LiuJ

cmake_minimum_required(VERSION 3.8)
set(This CppStringPlus)
project(${This} C CXX)

set(CMAKE_C_STANDARD 99)
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

enable_testing()

set(Headers
    include/CppStringPlus/CppStringPlus.hpp
)

set(Sources
    src/CppStringPlus.cpp
)

add_library(${This} STATIC ${Sources} ${Headers})
set_target_properties(${This} PROPERTIES
    FOLDER Libraries
)

target_include_directories(${This} PUBLIC include)

# Google Test not need as a lib
# set(BUILD_GTEST ON CACHE BOOL "Builds the googletest subproject" FORCE)
# set(BUILD_GMOCK OFF CACHE BOOL "Builds the googlemock subproject" FORCE)
# add_subdirectory(googletest)
# set_target_properties(gtest gtest_main PROPERTIES
#     FOLDER libraries
# )

add_subdirectory(test)

