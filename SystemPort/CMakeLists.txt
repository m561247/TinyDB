# CMakeLists.txt for SystemPort
#
# © 2020 by LiuJ

cmake_minimum_required(VERSION 3.8)
set(This SystemPort)

set(Headers
    include/SystemPort/CryptoRandom.hpp
    include/SystemPort/DiagnosticsContext.hpp
    include/SystemPort/DiagnosticsSender.hpp
    include/SystemPort/DiagnosticsStreamReporter.hpp
    include/SystemPort/DynamicLibrary.hpp
    include/SystemPort/File.hpp
    include/SystemPort/IFile.hpp
    include/SystemPort/Subprocess.hpp
    include/SystemPort/Clipboard.hpp
    include/SystemPort/INetworkConnection.hpp
    include/SystemPort/NetworkConnection.hpp
    include/SystemPort/NetworkEndpoint.hpp
    include/SystemPort/Service.hpp
    include/SystemPort/StringFile.hpp
    include/SystemPort/Time.hpp
)

set(Sources
    src/DiagnosticsContext.cpp
    src/DiagnosticsSender.cpp
    src/DiagnosticsStreamReporter.cpp
    src/File.cpp
    src/FileImpl.hpp
    src/StringFile.cpp
    src/SubprocessInternal.hpp
    src/DataQueue.hpp
    src/DataQueue.cpp
    src/NetworkConnectionImpl.hpp
    src/NetworkConnection.cpp
    src/NetworkEndpointImpl.hpp
    src/NetworkEndpoint.cpp
)

if(APPLE)
    list(APPEND Sources
        src/Mac/TimeMac.cpp
        src/Mac/FileMac.cpp
        src/Mac/DynamicLibraryMac.cpp
        src/Mac/SubprocessMac.cpp
    )
elseif(UNIX)
    list(APPEND Sources    
        src/Linux/TimeLinux.cpp
        src/Linux/FileLinux.cpp
        src/Linux/DynamicLibraryLinux.cpp
        src/Linux/SubprocessLinux.cpp
    )
endif()

if(UNIX)
    list(APPEND Sources
        src/Posix/CryptoRandomPosix.cpp
        src/Posix/TimePosix.cpp
        src/Posix/DynamicLibraryImpl.hpp
        src/Posix/DynamicLibraryPosix.cpp
        src/Posix/FilePosix.cpp
        src/Posix/FilePosix.hpp
        src/Posix/SubprocessPosix.cpp
        src/Posix/NetworkConnectionPosix.hpp
        src/Posix/NetworkConnectionPosix.cpp
        src/Posix/NetworkEndpointPosix.hpp
        src/Posix/NetworkEndpointPosix.cpp
        src/Posix/PipeSignal.hpp
        src/Posix/PipeSignal.cpp
    )
endif(UNIX)

add_library(${This} STATIC ${Sources} ${Headers})
set_target_properties(${This} PROPERTIES
    FOLDER Libraries
)

if(APPLE)
    find_library(AppKit AppKit)
    find_library(CF CoreFoundation)
    target_link_libraries(${This} PUBLIC ${CF} ${AppKit})
endif(APPLE)
if(UNIX)
    target_link_libraries(${This} PUBLIC
        pthread
        dl
    )
endif(UNIX)

target_compile_definitions(${This} PRIVATE CLIPBOARD_REVEAL_OS_API)
target_include_directories(${This} PUBLIC include)

target_link_libraries(${This} PUBLIC
    CppStringPlus
)

add_subdirectory(test)
