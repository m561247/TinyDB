#ifndef SYSTEM_ABSTRACTIONS_DYNAMIC_LIBRARY_IMPL_HPP
#define SYSTEM_ABSTRACTIONS_DYNAMIC_LIBRARY_IMPL_HPP

/**
 * @file DynamicLibraryImpl.hpp
 *
 * This module contains the POSIX implementation of the
 * SystemAbstractions::DynamicLibraryImpl structure.
 *
 * Copyright (c) 2020 by LiuJ
 */

#include <string>

namespace SystemAbstractions {

    struct DynamicLibraryImpl {

        void* libraryHandle = NULL;

        static std::string GetDynamicLibraryFileExtension();

    };

}


#endif /* SYSTEM_ABSTRACTIONS_DYNAMIC_LIBRARY_IMPL_HPP */