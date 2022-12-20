# CMakeLists.txt for CppStringPlus
#
# © 2018-2019 by LiuJ

cmake_minimum_required(VERSION 3.8)
set(This protocol)
project(${This} C CXX)

set(CMAKE_C_STANDARD 99)
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

enable_testing()

set(Headers
    include/protocol/Common.hpp
    include/protocol/Greeting.hpp
    include/protocol/Auth.hpp
    include/protocol/Eof.hpp
    include/protocol/Buffer.hpp
    include/protocol/Field.hpp
    include/protocol/Type.hpp
    include/protocol/Ok.hpp
    include/protocol/Err.hpp
    include/protocol/mysql_common.h
)

set(Sources
    src/Greeting.cpp
    src/Auth.cpp
    src/Eof.cpp
    src/Buffer.cpp
    src/Field.cpp
    src/Type.cpp
    src/Ok.cpp
    src/Err.cpp
)

add_library(${This} STATIC ${Sources} ${Headers})
set_target_properties(${This} PROPERTIES
    FOLDER Libraries
)

target_include_directories(${This} PUBLIC include)

target_link_libraries(${This} PUBLIC
    SystemPort
)

add_subdirectory(test)
