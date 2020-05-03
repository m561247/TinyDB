# CMakeLists.txt for SystemAbstractionsTests
#
# © 2020 by LiuJ

cmake_minimum_required(VERSION 3.8)
set(This SystemAbstractionsTests)

# set(Sources
    # src/CryptoRandomTests.cpp
    # src/DiagnosticsContextTests.cpp
    # src/DiagnosticsSenderTests.cpp
    # src/DiagnosticsStreamReporterTests.cpp
    # src/FileTests.cpp
    # src/StringFileTests.cpp
    # src/DynamicLibraryTests.cpp
    # src/DataQueueTests.cpp
    # src/NetworkEndpointTests.cpp
    # src/NetworkConnectionTests.cpp
# )

# add_executable(${This} ${Sources})
# set_target_properties(${This} PROPERTIES
#     FOLDER Tests
# )

# target_include_directories(${This} PRIVATE ../src)

# target_link_libraries(${This} PUBLIC
#     gtest_main
#     SystemPort
#     CppStringPlus
# )

# add_subdirectory(MockDynamicLibrary) 
# add_custom_command(TARGET ${This} POST_BUILD
#     COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:MockDynamicLibrary> $<TARGET_FILE_DIR:${This}>
# )

add_test(
    NAME ${This}
    COMMAND ${This}
)
