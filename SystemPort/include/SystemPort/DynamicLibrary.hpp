#ifndef SYSTEM_PORT_DYNAMIC_LIBRARY_HPP
#define SYSTEM_PORT_DYNAMIC_LIBRARY_HPP

/**
 * @file DynamicLibrary.hpp
 *
 * This module declares the SystemAbstractions::DynamicLibrary class.
 *
 * Copyright (c) 2020 by LiuJ
 */

#include <memory>
#include <string>

namespace SystemAbstractions {

    class DynamicLibrary {
        // Lifecycle management
    public:
        ~DynamicLibrary();
        DynamicLibrary(const DynamicLibrary&) = delete;
        DynamicLibrary(DynamicLibrary&& other) noexcept;
        DynamicLibrary& operator=(const DynamicLibrary& other) = delete;
        DynamicLibrary& operator=(DynamicLibrary&& other) noexcept;

    public:

        DynamicLibrary();

        /**
         * 
         * @param[in] path
         *     This is the path to the directory containing
         *     the dynamic library
         * 
         * @param[in] name
         *      This is the name of the dynamic library containing
         *      the dynamic library
         */
        bool Load(const std::string& path, const std::string& name);

        /**
         *  This method unload the dynamic library from the running program
         */
        void Unload();

        /**
         * This method locates the procedure that has the given
         * name in the library, and returns its address.
         * 
         * @param[in] name
         *   This is the name od the function in the library to locate
         * 
         */
        void* GetProcedure(const std::string& name);

        /**
         *  This method returns a human-readable string describing
         *  the last error that occurred calling another method on the object
         */
        std::string GetLastError();

    private:
        std::unique_ptr< struct DynamicLibraryImpl > impl_;
        
    };

} // SystemAbstractions

#endif