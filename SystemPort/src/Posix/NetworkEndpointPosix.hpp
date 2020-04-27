#ifndef SYSTEM_ABSTRACTIONS_NETWORK_ENDPOINT_POSIX_HPP
#define SYSTEM_ABSTRACTIONS_NETWORK_ENDPOINT_POSIX_HPP

/**
 * @file NetworkEndpointPosix.hpp
 *
 * This module declares the POSIX implementation of the
 * PhoenixWays::Core::NetworkEndpointPlatform structure.
 *
 * Copyright (c) 2020 by LiuJ
 */

#include "PipeSignal.hpp"

#include <list>
#include <mutex>
#include <stdint.h>
#include <SystemPort/NetworkEndpoint.hpp>
#include <thread>
#include <vector>

namespace SystemAbstractions {

    struct NetworkEndpoint::Platform {
        struct Packet {
            uint32_t address;
            uint16_t port;
            std::vector< uint8_t > body;
        };

        int sock = -1;

        std::thread processor;

        PipeSignal processorStateChangeSignal;

        bool processorStop = false;

        std::recursive_mutex processingMutex;

        std::list< Packet > outputQueue;
    };

} // namespace SystemAbstractions

#endif