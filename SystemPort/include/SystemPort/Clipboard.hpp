#ifndef SYSTEM_PORT_CLIPBOARD_HPP
#define SYSTEM_PORT_CLIPBOARD_HPP

/**
 * @file Clipboard.hpp
 *
 * This module declares the SystemPort::Clipboard class.
 *
 * © 2020 by LiuJ
 */
#include <memory>
#include <string>

#ifdef CLIPBOARD_REVEAL_OS_API

#ifdef _WIN32
#include <Windows.h>
#elif defined(__APPLE__)
#else 
// Linux
#endif

/*
* This is used to replace actual aoperating system calls to
* test framework function, if desired
*/
class ClipboardOperatingSystemInterface {
public:
#ifdef _WIN32

    virtual BOOL OpenClipboard(HWND hWndNewOwner);
    virtual BOOL EmptyClipboard();
    virtual BOOL IsClipboardFormatAvailble(UINT format);
    virtual HANDLE GetClipboardData(UINT uFormat);
    virtual void SetClipboardData(
        UINT uFormat,
        HANDLE hMem
    );
    virtual BOOL CloseClipboard();

#elif defined(__APPLE__)

    virtual void Copy(const std::string& s);
    virtual bool HasString();
    virtual std::string PasteString();

#else /* Linux */

    virtual void Copy(const std::string& s);
    virtual bool HasString();
    virtual std::string PasteString();

#endif
};

extern ClipboardOperatingSystemInterface* selectedClipboardOperatingSystemInterface;

#endif /* CLIPBOARD_REVEAL_OS_API*/

namespace SystemAbstractions {

    /**
     *  This class represents a file accessed through the
     *  native operating system.
     */
    class Clipboard {
    // Lifecycle management
    public:
        ~Clipboard() noexcept;
        Clipboard(const Clipboard& other) = default;
        Clipboard(Clipboard&& other) noexcept = default;
        Clipboard& operator=(const Clipboard& other) = default;
        Clipboard& operator=(Clipboard&& other) noexcept = default;

        Clipboard();

        /**
         * This puts the given string into the Clipboard.
         * 
         * @param[in] s
         *  This is the string to put into the clipboard.
         */
        void Copy(const std::string& s);

        /**
         * This method determines whether or not the clipboard's contents
         * can be represented by a string
         * 
         * @return
         * 
         *  An indication of whether or not the clipboard's contents
         *  can be represented by a string is returned.
         */
        bool HasString();

        /**
         * This method returns the contents of the clipboard as a string.
         * 
         * @return
         *  The contents of the clipboard as a string is returned.
         */
        std::string PasteString();

    private:
        
        /**
         * This contains any platform-specific state for object.
         */
        struct Impl;

        /**
         * This contains any platform-specific state for the object.
         */
        std::unique_ptr< Impl > impl_;
    };

}

#endif /* SYSTEM_PORT_CLIPBOARD_HPP */