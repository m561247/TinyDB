# CMakeLists.txt for CppStringPlus
#
# © 2018-2019 by LiuJ

cmake_minimum_required(VERSION 3.8)
set(This CppStringPlusTests)

set(Sources
    src/CppStringPlusTests.cpp
)

# add_executable(${This} ${Sources})

# set_target_properties(${This} PROPERTIES
#     FOLDER Tests
# )

# target_include_directories(${This} PRIVATE ../src)

# target_link_libraries(${This} PUBLIC
#     gtest_main
#     CppStringPlus
# )

add_test(
    NAME ${This}
    COMMAND ${This}
)