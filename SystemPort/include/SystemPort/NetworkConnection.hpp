#ifndef SYSTEM_ABSTRACTIONS_NETWORK_CONNECTION_HPP
#define SYSTEM_ABSTRACTIONS_NETWORK_CONNECTION_HPP

/**
 * @file NetworkConnection.hpp
 *
 * This module declares the SystemAbstractions::NetworkConnection class.
 *
 * © 2020 by LiuJ
 */

#include "DiagnosticsSender.hpp"
#include "INetworkConnection.hpp"

#include <memory>
#include <stdint.h>
#include <vector>

namespace SystemAbstractions
{
    /**
     *  This class handles the exchanging of messages between its owner and a
     *  remote object, over a network.
     */
    class NetworkConnection
        : public INetworkConnection
    {
    public:

        struct Impl;

        struct Platform;

    public:
        ~NetworkConnection() noexcept;
        NetworkConnection(const NetworkConnection&) = delete;
        NetworkConnection(NetworkConnection&& other) noexcept = delete;
        NetworkConnection& operator=(const NetworkConnection&) = delete;
        NetworkConnection& operator=(NetworkConnection&& other) noexcept = delete;

    public:

        /**
         * This is an instance constructor
         */
        NetworkConnection();

        static uint32_t GetAddressOfHost(const std::string& host);

    public:

        virtual DiagnosticsSender::UnsubscribeDelegate SubscribeToDiagnostics(
            DiagnosticsSender::DiagnosticsMessageDelegate delegate,
            size_t minLevel = 0
        ) override;

        virtual bool Connect(uint32_t peerAddress, uint16_t peerPort) override;

        virtual bool Process(
            MessageReceivedDelegate messageReceivedDElegate,
            BrokenDelegate brokenDelegate
        ) override;

        virtual uint32_t GetPeerAddress() const override;

        virtual uint16_t GetPeerPort() const override;

        virtual bool IsConnected() const override;

        virtual uint32_t GetBoundAddress() const override;

        virtual uint16_t GetBoundPort() const override;

        virtual void SendMessage(const std::vector< uint8_t >& message) override;

        virtual void Close(bool clean = false) override;

    private:

        std::shared_ptr< Impl > impl_;

    }; 

} // namespace SystemAbstractions

#endif