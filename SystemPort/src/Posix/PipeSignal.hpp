#ifndef FILES_PIPE_SIGNAL_HPP
#define FILES_PIPE_SIGNAL_HPP

/**
 * @file PipeSignal.hpp
 *
 * This module declares the SystemAbstractions::PipeSignal class.
 *
 * Copyright (c) 2020 by LiuJ
 */
#include <memory>
#include <string>

namespace SystemAbstractions {

    /**
     *  This class implements a level-sensitive signal that exposes
     *  a file handle which may be used in select() to wait for
     *  the signal
     */
    class PipeSignal {
    public:
        PipeSignal();

        ~PipeSignal();

        bool Initialize();

        std::string GetLastError() const;

        void Set();

        void Clear();

        bool IsSet() const;

        int GetSelectHandle() const;

    private:

        std::unique_ptr< struct PipeSignalImpl > impl_;

    };
    
} // namespace SystemAbstractions



#endif /* FILES_PIPE_SIGNAL_HPP */