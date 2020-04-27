#ifndef PHOENIX_WAYS_CORE_NETWORK_ENDPOINT_HPP
#define PHOENIX_WAYS_CORE_NETWORK_ENDPOINT_HPP

/**
 * @file NetworkEndpoint.hpp
 *
 * This module declares the SystemAbstractions::NetworkEndpoint class.
 *
 * © 2020 by LiuJ
 */

#include "DiagnosticsSender.hpp"
#include "NetworkConnection.hpp"

#include <memory>
#include <stddef.h>
#include <stdint.h>
#include <vector>

namespace SystemAbstractions {

    /**
     * This class listens for incoming connections from remote objects，
     * constructing network connection objects for them and passing
     * them along to the owner
     */

    class NetworkEndpoint {
    public:
        /**
         *  This is the type of callback function to be called whenever
         *  a new client connects to the network endpoint.
         * 
         *  @param[in] newConnection
         *      This represents the connection to the new client.
         */
        typedef std::function< void(std::shared_ptr< NetworkConnection > newConnection) > NewConnectionDelegate;

        /**
         * This is the type of callback function to be called whenever
         *  a new datagram-oriented message is received by the network endpoint.
         * 
         * @param[in] address
         *  This is the IPv4 address of the client who sent the message.
         * 
         * @param[in] port
         *  This is the port number of the client who sent the message
         * 
         * @param[in] body
         *  This is the contents of the datagram sent by the client.
         * 
         */
        typedef std::function<
            void(
                uint32_t address,
                uint16_t port,
                const std::vector< uint8_t >& body 
            )
        > PacketReceivedDelegate;

        enum class Mode {

            Datagram,   // UDP

            Connection,   // TCP

            MulticastSend,

            MulticastRecevie,

        };

    public:
        ~NetworkEndpoint() noexcept;
        NetworkEndpoint(const NetworkEndpoint&) = delete;
        NetworkEndpoint(NetworkEndpoint&& other) noexcept;
        NetworkEndpoint& operator=(const NetworkEndpoint&) = delete;
        NetworkEndpoint& operator=(NetworkEndpoint&& other) noexcept;
    
    public:
        NetworkEndpoint();

        /**
         *  This method forms a new subscription to diagnostic
         *  messages published by the sender.
         *  
         */
        DiagnosticsSender::UnsubscribeDelegate SubscribeToDiagnostics(
            DiagnosticsSender::DiagnosticsMessageDelegate delegate,
            size_t minLevel = 0
        );

        /**
         * This method starts message or connection processing on the endpoint,
         *  depending on the given mode.
         * 
         * @param[in] newConnectionDelegate
         *      This is the callback function to be called whenever
         *      a new client connects to the network endpoint.
         * 
         * @param[in] packetReceivedDelegate
         *      This is the callback function to be called whenever
         *      a new datagram-oriented message is received by
         *      the network endpoint.
         * 
         * @param[in] mode
         *      This selects the kind of processing to perform with
         *      the endpoint
         * 
         * @param[in] localAddress
         *      This is the address to use on the network for the endpoint.
         *      It is only required fro multicast send mode. It is not
         *      used at all for multicast recevice mode, since in this mode
         *      the socket requests memebrship in the multicast group on
         *      all interfaces. For datagram and connection modes, if an
         *      address is specified, it limits the traffic to a single
         *      interface
         * 
         * @param[in] groupAddress
         * 
         *  
         * @param[in] port
         * 
         */

        bool Open(
            NewConnectionDelegate newConnectionDelegate,
            PacketReceivedDelegate packetReceivedDelegate,
            Mode mode,
            uint32_t localAddress,
            uint32_t groupAddress,
            uint16_t port
        );

        uint16_t GetBoundPort() const;

        void SendPacket(
            uint32_t address,
            uint16_t port,
            const std::vector< uint8_t >& body  
        );

        void Close();

        static std::vector< uint32_t > GetInterfaceAddresses();

    private:

        struct Impl;

        struct Platform;

        std::unique_ptr< Impl > impl_;
 
    };

}

#endif