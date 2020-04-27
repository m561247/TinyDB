#ifndef SYSTEM_ABSTRACTIONS_NETWORK_CONNECT_IMPL_HPP
#define SYSTEM_ABSTRACTIONS_NETWORK_CONNECT_IMPL_HPP

/**
 * @file NetworkConnectionImpl.hpp
 *
 * This module declares the SystemAbstractions::NetworkConnectionImpl
 * structure.
 *
 * © 2020 by LiuJ
 */

#include <memory>
#include <stdint.h>
#include <SystemPort/DiagnosticsSender.hpp>
#include <SystemPort/NetworkConnection.hpp>
#include <vector>

namespace SystemAbstractions {

    struct NetworkConnection::Impl
        : public std::enable_shared_from_this< NetworkConnection::Impl >
    {

        enum class CloseProcedure {

            ImmediateDoNotStopProcessor,

            ImmediateAndStopProcessor,

            Graceful,
        };

        std::unique_ptr< Platform > platform;

        MessageReceivedDelegate messageReceivedDelegate;

        BrokenDelegate brokenDelegate;

        uint32_t peerAddress = 0;

        uint16_t peerPort = 0;

        uint32_t boundAddress = 0;

        uint16_t boundPort = 0;

        DiagnosticsSender diagnosticsSender;

        // Lifecycle Management
        ~Impl() noexcept;
        Impl(const Impl&) = delete;
        Impl(Impl&&) noexcept = delete;
        Impl& operator=(const Impl&) = delete;
        Impl& operator=(Impl&&) noexcept = delete;

        Impl();

        bool Connect();

        /**
         * This method starts message processing on the connection
         *  listening for incoming messages and sending outgoing messages
         * 
         * @return
         *   An indication of whether or not the method was 
         *   successful is returned
         * 
         */
        bool Process();

        /**
         * This is the main function called for the worker thread
         *  of the object. It does all the actual sending and
         *  receiving of messages, using the underlying operating
         *  system network handle.
         */
        void Processor();

        /**
         *  This method returns an indication of whether or not there
         *  is a connection currently established with a peer.
         * 
         * @return
         *  An indication of whether or not there
         *  is a connection currently established with a peer
         *  is returned
         */
        bool IsConnected() const;

        /**
         * This method appends the given data to the queue of data
         * currently being sent to the peer. the actual sending
         * is performed by the processor worker thread.
         * 
         * @param[in] message
         *  This holds the data to be appended to the send queue.
         */
        void SendMessage(const std::vector< uint8_t >& message);

        bool Close(CloseProcedure procedure);

        void CloseImmediately();

        static uint32_t GetAddressOfHost(const std::string& host);

    };

} // namespace SystemAbstractions

#endif