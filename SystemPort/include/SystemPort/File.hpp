#ifndef SYSTEM_ABSTRACTIONS_FILE_HPP
#define SYSTEM_ABSTRACTIONS_FILE_HPP

/**
 * @file File.hpp
 *
 * This module declares the SystemAbstractions::File class.
 *
 * © 2013-2018 by LiuJ
 */

#include "IFile.hpp"

#include <memory>
#include <string>
#include <vector>

namespace SystemAbstractions {

    /**
     *  This class represents a file accessed through the
     *  native operating system
     */
    class File: public IFile {
        // Lifecycle Management
    public:
        ~File() noexcept;
        File(const File&) = delete;
        File(File&& other) noexcept;
        File& operator=(const File&) = delete;
        File& operator=(File&& other) noexcept;

    public:

        File(std::string path);

        bool IsExisting();

        bool IsDirectory();

        bool OpenReadOnly();

        void Close();

        bool OpenReadWrite();

        void Destroy();

        bool Move(const std::string& newPath);

        bool Copy(const std::string& detination);

        time_t GetLastModifiedTime() const;

        std::string GetPath() const;

        static bool IsAbsolutePath(const std::string& path);

        static std::string GetExeImagePath();

        static std::string GetExeParentDirectory();

        static std::string GetResourceFilePath(const std::string& name);

        static std::string GetUserHomeDirectory();

        static std::string GetLocalPerUserConfigDirectory(const std::string& nameKey);

        static std::string GetUserSavedGamesDirectory(const std::string& nameKey);

        static void ListDirectory(const std::string& directory, std::vector< std::string >& list);

        static bool CreateDirectory(const std::string& directory);

        static bool DeleteDirectory(const std::string& directory);

        static bool CopyDirectory(
            const std::string& existingDirectory,
            const std::string& newDirectory
        );

        static std::vector< std::string > GetDirectoryRoots();

        static std::string GetWorkingDirectory();

        static void SetWorkingDirectory(const std::string& workingDirectory);
    
    public:
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

        struct Platform;

        std::unique_ptr< Impl > impl_;

    };

}

#endif /* SYSTEM_ABSTRACTIONS_FILE_HPP */