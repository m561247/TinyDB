/**
 * @file DynamicLibraryMach.cpp
 *
 * This module contains the Mach-specific parts of the
 * SystemAbstractions::DynamicLibrary class.
 *
 * Copyright (c) 2020 by LiuJ
 */

#include "../Posix/DynamicLibraryImpl.hpp"

namespace SystemAbstractions {

    std::string DynamicLibraryImpl::GetDynamicLibraryFileExtension() {
        return "dylib";
    }

}
