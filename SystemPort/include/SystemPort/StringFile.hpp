#ifndef SYSTEM_ABSTRACTIONS_STRING_FILE_HPP
#define SYSTEM_ABSTRACTIONS_STRING_FILE_HPP

/**
 * @file StringFile.hpp
 *
 * This module declares the SystemAbstractions::StringFile class.
 *
 * Copyright © 2020 by LiuJ
 */

#include "IFile.hpp"

#include <memory>
#include <stdint.h>
#include <string>
#include <vector>

namespace SystemAbstractions {

    /**
     * This class reprsesents a file stored in a string
     */
    class StringFile: public IFile {
        // Lifecycle management
    public:
        ~StringFile() noexcept;
        StringFile(const StringFile&);
        StringFile(StringFile&&) noexcept;
        StringFile& operator=(const StringFile&);
        StringFile& operator=(StringFile&&) noexcept;

    public:
        /**
         *  This is an instance constructor.
         * 
         *  @param[in] initialValue
         *      This is the initial contents of the file
         */
        StringFile(std::string initialValue = "");

        /**
         *  This is an instance constructor
         *  
         *  @param[in] initialValue
         *      This is the initial contents of the file.
         */
        StringFile(std::vector< uint8_t > initialValue);

        /**
         *  This is the typecast to std::string operator
         */
        operator std::string() const;

        /**
         *  This is the typecast to std::vector< uint8_t > oprtator.
         */
        operator std::vector< uint8_t >() const;

        StringFile& operator=(const std::string &b);

        StringFile& operator=(const std::vector< uint8_t > &b);

        void Remove(size_t numBytes);

    public:
        // IFile
        virtual uint64_t GetSize() const override;
        virtual bool SetSize(uint64_t size) override;
        virtual uint64_t GetPosition() const override;
        virtual void SetPosition(uint64_t position) override;
        virtual size_t Peek(Buffer& buffer, size_t numBytes = 0, size_t offset = 0) const override;
        virtual size_t Peek(void* buffer, size_t numBytes) const override;
        virtual size_t Read(Buffer& buffer, size_t numBytes = 0, size_t offset = 0) override;
        virtual size_t Read(void* buffer, size_t numBytes) override;
        virtual size_t Write(const Buffer& buffer, size_t numBytes = 0, size_t offset = 0) override;
        virtual size_t Write(const void* buffer, size_t numBytes) override;
        virtual std::shared_ptr< IFile > Clone() override;

    private:

        struct Impl;

        std::unique_ptr< Impl > impl_;

    };

}

#endif /* SYSTEM_ABSTRACTIONS_STRING_FILE_HPP */