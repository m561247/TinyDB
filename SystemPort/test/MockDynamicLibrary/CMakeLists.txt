# CMakeLists.txt for MockDynamicLibrary
#
# © 2020 by LiuJ

cmake_minimum_required(VERSION 3.8)
set(This MockDynamicLibrary)

set(Sources 
    src/MockDynamicLibrary.cpp
)

add_library(${This} SHARED ${Sources})
set_target_properties(${This} PROPERTIES
    FOLDER TestHelpers
)