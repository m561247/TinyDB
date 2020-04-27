#ifndef SYSTEM_ABSTRACTIONS_NETWORK_CONNECTION_POSIX_HPP
#define SYSTEM_ABSTRACTIONS_NETWORK_CONNECTION_POSIX_HPP

/**
 * @file NetworkConnectionPosix.hpp
 *
 * This module declares the POSIX implementation of the
 * SystemAbstractions::NetworkConnectionPlatform structure.
 *
 * Copyright (c) 2020 by LiuJ
 */

#include "../DataQueue.hpp"
#include "PipeSignal.hpp"

#include <deque>
#include <memory>
#include <mutex>
#include <stdint.h>
#include <SystemPort/DiagnosticsSender.hpp>
#include <thread>

namespace SystemAbstractions {

    struct NetworkConnection::Platform {

        int sock = -1;

        bool peerClosed = false;

        bool closing = false;

        bool shutdownSent = false;

        /**
         *  This is the thread which performs all the actual
         *  sending and receving of data over the network.
         */
        std::thread processor;

        /**
         * This is used 
         */
        PipeSignal processorStateChangeSignal;

        bool processorStop = false;

        /** https://zh.cppreference.com/w/cpp/thread/recursive_mutex
         *  防止死锁
         *  This is used to synchronize access to the object
         */
        std::recursive_mutex processingMutex;

        DataQueue outputQueue;

        /**
         * This is a factory method for creating a new NetworkConnection
         * object out of an already established connection.
         */
        static std::shared_ptr< NetworkConnection > MakeConnectionFromExistingSocket(
            int sock,
            uint32_t boundAddress,
            uint16_t boundPort,
            uint32_t peerAddress,
            uint16_t peerPort
        );

        void CloseImmediately();

    };
}

#endif