#ifndef SYSTEM_ABSTRACTIONS_FILE_POSIX_HPP
#define SYSTEM_ABSTRACTIONS_FILE_POSIX_HPP

/**
 * @file FilePosix.hpp
 *
 * This module declares the POSIX implementation of the
 * SystemAbstractions::FileImpl structure.
 *
 * Copyright (c) 2020 by LiuJ
 */

#include <string>
#include <SystemPort/File.hpp>

namespace SystemAbstractions {

    struct File::Platform {

        int handle = -1;

        bool writeAccess = false;

    };

}

#endif