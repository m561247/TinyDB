#ifndef SYSTEM_ABSTRACTIONS_I_FILE_HPP
#define SYSTEM_ABSTRACTIONS_I_FILE_HPP

/**
 * @file IFile.hpp
 *
 * This module declares the SystemAbstractions::IFile interface.
 *
 * Copyright (c) 2020 by LiuJ
 */

#include <memory>
#include <stddef.h>
#include <stdint.h>
#include <string>
#include <vector>

namespace SystemAbstractions {

    /**
     *  This is the interface to an object holding a mutable array of
     *  bytes and a movable pointer into it.
     */
    class IFile {

    public:
        typedef std::vector< uint8_t > Buffer;

    public:
        virtual ~IFile() {}

        virtual uint64_t GetSize() const = 0;

        virtual bool SetSize(uint64_t size) = 0;

        virtual uint64_t GetPosition() const = 0;

        virtual void SetPosition(uint64_t position) = 0;

        /**
         * This method reads a region of the file without advancing the
         * current position in the file.
         * 
         * @param[in,out] buffer
         *      This will be modified to contain bytes read from the file.
         * 
         * @param[in] numBytes
         *      This is the number of bytes to read from the file.
         * 
         * @param[in] offset
         *      This is the byte offset in the buffer where to store the
         *      first byte read from the file.
         * 
         * @return
         *      The number of bytes actually read is returned.
         * 
         */

        virtual size_t Peek(Buffer& buffer, size_t numBytes = 0, size_t offset = 0) const = 0;

        /**
         *  This method reads a region of the file without advancing the 
         *  current position in the file.
         * 
         *  @param[out] buffer
         *      This is where to put the bytes read from the file.
         * 
         *  @param[in] numBytes
         *      This is the number of bytes read from the file.
         * 
         *  @return
         *      This number of bytes actually read from the file.
         */
        
        virtual size_t Peek(void* buffer, size_t numBytes) const = 0;

        virtual size_t Read(Buffer& buffer, size_t numBytes = 0, size_t offset = 0) = 0;

        virtual size_t Read(void* buffer, size_t numBytes) = 0;

        virtual size_t Write(const Buffer& buffer, size_t numBytes = 0, size_t offset = 0) = 0;
 
        virtual size_t Write(const void* buffer, size_t numBytes) = 0;

        virtual std::shared_ptr< IFile > Clone() = 0;

    };  

}

#endif /* SYSTEM_ABSTRACTIONS_I_FILE_HPP */