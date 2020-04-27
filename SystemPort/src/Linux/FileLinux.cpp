/**
 * @file FileLinux.cpp
 *
 * This module contains the Linux specific part of the
 * implementation of the SystemAbstractions::File class.
 *
 * Copyright (c) 2020 by LiuJ
 */

#include "../Posix/FilePosix.hpp"

#include <dirent.h>
#include <errno.h>
#include <pwd.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <string.h>
#include <CppStringPlus/CppStringPlus.hpp>
#include <sys/stat.h>
#include <sys/types.h>
#include <SystemPort/File.hpp>
#include <unistd.h>
#include <vector>

namespace SystemAbstractions {

    std::string File::GetExeImagePath() {
        /**
         *  Path to self is always available through procfs /proc/self/exe
         */
        std::vector< char > buffer(PATH_MAX);
        (void)realpath("/proc/self/exe", &buffer[0]);
        return std::string(&buffer[0]);
    }

    std::string File::GetExeParentDirectory() {
        std::vector< char > buffer(PATH_MAX);
        (void)realpath("/proc/self/exe", &buffer[0]);
        auto length = strlen(&buffer[0]);
        while (--length > 0) {
            if (buffer[length] == '/') {
                break;
            }
        }
        if (length == 0) {
            ++length;
        }
        buffer[length] = '\0';
        return std::string(&buffer[0]);
    }

    std::string File::GetResourceFilePath(const std::string &name) {
        return CppStringPlus::sprintf("%s/%s", GetExeParentDirectory().c_str(), name.c_str());
    }

    std::string File::GetLocalPerUserConfigDirectory(const std::string &nameKey) {
        return CppStringPlus::sprintf("%s/.%s", GetUserHomeDirectory().c_str(), nameKey.c_str());
    }

}