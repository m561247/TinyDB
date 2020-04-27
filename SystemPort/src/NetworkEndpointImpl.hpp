#ifndef SYSTEM_ABSTRACTIONS_NETWORK_ENDPOINT_IMPL_HPP
#define SYSTEM_ABSTRACTIONS_NETWORK_ENDPOINT_IMPL_HPP

/**
 * @file NetworkEndpoint::Impl.hpp
 *
 * This module declares the SystemAbstractions::NetworkEndpoint::Impl
 * structure.
 *
 * © 2020 by LiuJ
 */

#include <memory>
#include <stdint.h>
#include <SystemPort/DiagnosticsSender.hpp>
#include <SystemPort/NetworkEndpoint.hpp>
#include <vector>

namespace SystemAbstractions {

    struct NetworkEndpoint::Impl {

        std::unique_ptr< Platform > platform;

        NewConnectionDelegate newConnectionDelegate;

        PacketReceivedDelegate packetReceivedDelegate;

        uint32_t localAddress = 0;

        uint32_t groupAddress = 0;

        uint16_t port = 0;

        Mode mode = Mode::Datagram;

        /**
         *  This is a helper object used to publish diagnostic messages.
         */
        DiagnosticsSender diagnosticsSender;

        ~Impl() noexcept;
        Impl(const Impl&) = delete;
        Impl(Impl&&) noexcept = delete;
        Impl& operator=(const Impl&) = delete;
        Impl& operator=(Impl&&) noexcept = delete;

        Impl();

        /**
         * This method starts message or connection processing  on
         * the endpoint, depending on the configured mode. 
        */
       bool Open();

       /**
        *  This is the main function called for the worker thread
        *   of the object. It does all the actual sending and receiving
        *   of messages, using  the underlying operating
        *   system network handle
        */
       void Processor();

       /**
        * This method is used when the network endpoint is configured
        *  to send datagram messages 
        *   It is called to send a message to one or more recipients.
        */
       void SendPacket(
           uint32_t address,
           uint16_t port,
           const std::vector< uint8_t >& body
       );

       /**
        *  This method is the opposite of the Open method. It stops
        * any and all network activity associated with the endpoint,
        * and releases any network resources previously acquired.
        */
       void Close(bool stopProcessing);

       static std::vector< uint32_t > GetInterfaceAddresses();
    
    };

}


#endif