#ifndef SYSTEM_ABSTRACTIONS_FILE_IMPL_HPP
#define SYSTEM_ABSTRACTIONS_FILE_IMPL_HPP

/**
 * @file FileImpl.hpp
 *
 * This module contains the platform-independent part of the
 * implementation of the SystemAbstractions::File class.
 *
 * © 2020 by LiuJ
 */

#include <memory>
#include <SystemPort/File.hpp>

namespace SystemAbstractions {

    struct File::Impl {

        std::string path;

        std::unique_ptr< Platform > platform;

        // Lifecycle Management
        ~Impl() noexcept;
        Impl(const Impl&) = delete;
        Impl(Impl&&) noexcept;
        Impl& operator=(const Impl&) = delete;
        Impl& operator=(Impl&&);

        Impl();

        static bool CreatePath(std::string path);
    };
}

#endif