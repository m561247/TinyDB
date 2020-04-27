/**
 * @file NetworkEndpointTests.cpp
 *
 * This module contains the unit tests of the
 * SystemAbstractions::NetworkEndpoint class.
 *
 * © 2020 by LiuJ
 */

#include <gtest/gtest.h>
#include <condition_variable>
#include <mutex>
#include <SystemPort/NetworkEndpoint.hpp>
#include <iostream>
#include <netinet/ip.h>
#include <sys/socket.h>
#define IPV4_ADDRESS_IN_SOCKADDR sin_addr.s_addr
#define SOCKADDR_LENGTH_TYPE socklen_t

namespace {
    
    struct Packet {

        std::vector< uint8_t > payload;

        uint32_t address;

        uint16_t port;

        Packet(
            const std::vector< uint8_t >& newPayload,
            uint32_t newAddress,
            uint16_t newPort
        ) 
            : payload(newPayload)
            , address(newAddress)
            , port(newPort)
        {
        }

    };


    struct Owner {

        std::condition_variable_any condition; // https://www.cnblogs.com/haippy/p/3252041.html

        /**
         *  This is used to synchronize access to the class.
         */
        std::mutex mutex;

        /**
         *  This holds a copy of each packet received
         */
        std::vector< Packet > packetsReceived;

        std::vector< uint8_t > streamReceived;

        std::vector< std::shared_ptr< SystemAbstractions::NetworkConnection > > connections;

        /**
         *  This flag indicates whether or not a connection
         *  to the network endpoint has been broken
         */
        bool connectionBroken = false;

        /*
         * template <class Rep, class Period, class Predicate>
              bool wait_for (unique_lock<mutex>& lck,
              const chrono::duration<Rep,Period>& rel_time, Predicate pred); 
              wait_for 可以指定一个时间段，在当前线程收到通知或者指定的时间 rel_time 超时之前，
              该线程都会处于阻塞状态。而一旦超时或者收到了其他线程的通知，wait_for 返回
              最后一个参数 pred 表示 wait_for 的预测条件，只有当 pred 条件为 false 时调用 wait() 才会阻塞当前线程，
              并且在收到其他线程的通知后只有当 pred 为 true 时才会被解除阻塞，因此相当于如下代码：
         */
        bool AwaitPacket() {
            std::unique_lock< decltype(mutex) > lock(mutex);
            return condition.wait_for(
                lock,
                std::chrono::seconds(1),
                [this]{
                    return !packetsReceived.empty();
                }
            );
        }

        bool AwaitStream(size_t numBytes) {
            std::unique_lock< decltype(mutex) > lock(mutex);
            return condition.wait_for(
                lock,
                std::chrono::seconds(1),
                [this, numBytes] {
                    return (streamReceived.size() >=numBytes);
                }
            );
        }

        bool AwaitConnection() {
            std::unique_lock< decltype(mutex) > lock(mutex);
            return condition.wait_for(
                lock,
                std::chrono::seconds(1),
                [this]{
                    return !connections.empty();
                }
            );
        }

        /**
         *  This method waits up to a second for the given number
         *  of connections to be receoved at the network endpoint
         */
        bool AwaitConnections(size_t numConnections) {
            std::unique_lock< decltype(mutex) > lock(mutex);
            return condition.wait_for(
                lock,
                std::chrono::seconds(1),
                [this, numConnections] {
                    return (connections.size() >= numConnections);
                }
            );
        }

        /**
         *  This method waits up to a second for a connection
         *  to the network endpoint to be broken.
         */
        bool AwaitDisconnection() {
            std::unique_lock< decltype(mutex) > lock(mutex);
            return condition.wait_for(
                lock,
                std::chrono::seconds(1),
                [this] {
                    return connectionBroken;
                }
            );
        }

        /**
         * This is the call back function to be called whenever
         *  a new client connects to the network endpoint.
         */
        void NetworkEndpointNewConnection(std::shared_ptr< SystemAbstractions::NetworkConnection > newConnection) {
            std::unique_lock< decltype(mutex) > lock(mutex);
            connections.push_back(newConnection);
            condition.notify_all();
            (void)newConnection->Process(
                [this](const std::vector< uint8_t >& message) {
                    NetworkConnectionMessageReceived(message);
                },
                [this](bool) {
                    NetworkConnectionBroken();
                }
            );
        }

        /**
         *  This is the type of callback function to be called whenever
         *  a new datagram-oriented message is received by network endpoint
         */
        void NetworkEndpointPacketReceived(
            uint32_t address,
            uint16_t port,
            const std::vector< uint8_t >&body
        ) {
            std::unique_lock< decltype(mutex) > lock(mutex);
            packetsReceived.emplace_back(body, address, port);
            condition.notify_all();
        }

        /**
         * This is the callback issued whenever more
         * data is received from the peer of the connection 
         */
        void NetworkConnectionMessageReceived(const std::vector< uint8_t >& message) {
            std::unique_lock< decltype(mutex) > lock(mutex);
            streamReceived.insert(
                streamReceived.end(),
                message.begin(),
                message.end()
            );
            condition.notify_all();
        }

        /**
         *  This is the callback issued whenever
         *  the connection is broken
         */
        void NetworkConnectionBroken() {
            std::unique_lock< decltype(mutex) > lock(mutex);
            connectionBroken = true;
            condition.notify_all();
        }

    };

}

struct NetworkEndpointTests
    : public ::testing::Test
{

    bool wsaStarted = false;

    virtual void SetUp() {

    }

    virtual void TearDown() {

    }

};

TEST_F(NetworkEndpointTests, DatagramSending){
    auto receiver = socket(AF_INET, SOCK_DGRAM, 0);

    ASSERT_FALSE(receiver < 0);

    struct sockaddr_in receiverAddress;
    (void)memset(&receiverAddress, 0, sizeof(receiverAddress));
    receiverAddress.sin_family = AF_INET;
    receiverAddress.IPV4_ADDRESS_IN_SOCKADDR = 0;
    receiverAddress.sin_port = 0;
    ASSERT_TRUE(bind(receiver, (struct sockaddr*)&receiverAddress, sizeof(receiverAddress)) == 0);
    SOCKADDR_LENGTH_TYPE receiverAddressLength = sizeof(receiverAddress);
    uint16_t port;
    ASSERT_TRUE(getsockname(receiver, (struct sockaddr*)&receiverAddress, &receiverAddressLength) == 0);
    port = ntohs(receiverAddress.sin_port);

    SystemAbstractions::NetworkEndpoint endpoint;
    Owner owner;
    endpoint.Open(
        [&owner](
            std::shared_ptr< SystemAbstractions::NetworkConnection > newConnection
        ) { owner.NetworkEndpointNewConnection(newConnection); },
        [&owner](
            uint32_t address,
            uint16_t port,
            const std::vector< uint8_t >& body
        ){
            owner.NetworkEndpointPacketReceived(address, port, body);
        },
        SystemAbstractions::NetworkEndpoint::Mode::Datagram,
        0,
        0,
        0
    );
    std::cout << port << std::endl;
    const std::vector< uint8_t > testPacket{ 0x12, 0x34, 0x56, 0x78 };
    endpoint.SendPacket(0x7F000001, port, testPacket);

    struct sockaddr_in senderAddress;
    SOCKADDR_LENGTH_TYPE senderAddressSize = sizeof(senderAddress);
    std::vector< uint8_t > buffer(testPacket.size() * 2);
    const int amountReceived = recvfrom(
        receiver,
        (char*)buffer.data(),
        (int)buffer.size(),
        0,
        (struct sockaddr*)&senderAddress,
        &senderAddressSize
    );
    ASSERT_EQ(testPacket.size(), amountReceived);
    buffer.resize(amountReceived);
    ASSERT_EQ(testPacket, buffer);
    ASSERT_EQ(0x7F000001, ntohl(senderAddress.IPV4_ADDRESS_IN_SOCKADDR));
    ASSERT_EQ(endpoint.GetBoundPort(), ntohs(senderAddress.sin_port));
}

TEST_F(NetworkEndpointTests, DatagramReceving) {

    auto sender = socket(AF_INET, SOCK_DGRAM, 0);

    ASSERT_FALSE(sender < 0);
    struct sockaddr_in senderAddress;
    (void)memset(&senderAddress, 0, sizeof(senderAddress));
    senderAddress.sin_family = AF_INET;
    senderAddress.IPV4_ADDRESS_IN_SOCKADDR = 0;
    senderAddress.sin_port = 0;
    ASSERT_TRUE(bind(sender, (struct sockaddr*)&senderAddress, sizeof(senderAddress)) == 0);
    SOCKADDR_LENGTH_TYPE senderAddressLength = sizeof(senderAddress);
    uint16_t port;
    ASSERT_TRUE(getsockname(sender, (struct sockaddr*)&senderAddress, &senderAddressLength) == 0);
    port = ntohs(senderAddress.sin_port);

    // Set up the NetworkEndpoint
    SystemAbstractions::NetworkEndpoint endpoint;
    Owner owner;
    endpoint.Open(
        [&owner](
            std::shared_ptr< SystemAbstractions::NetworkConnection > newConnection
        ) { owner.NetworkEndpointNewConnection(newConnection); },
        [&owner](
            uint32_t address,
            uint16_t port,
            const std::vector< uint8_t >& body
        ){
            owner.NetworkEndpointPacketReceived(address, port, body);
        },
        SystemAbstractions::NetworkEndpoint::Mode::Datagram,
        0,
        0,
        0        
    );

    const std::vector< uint8_t > testPacket{ 0x12, 0x34, 0x56, 0x78 };

    struct sockaddr_in receiverAddress;
    (void)memset(&receiverAddress, 0, sizeof(receiverAddress));
    receiverAddress.sin_family = AF_INET;
    receiverAddress.IPV4_ADDRESS_IN_SOCKADDR = htonl(0x7F000001);
    receiverAddress.sin_port = htons(endpoint.GetBoundPort());
    (void)sendto(
        sender,
        (const char*)testPacket.data(),
        (int)testPacket.size(),
        0,
        (const sockaddr*)&receiverAddress,
        sizeof(receiverAddress)
    );

    // Verify that we received the datagram.
    ASSERT_TRUE(owner.AwaitPacket());
    ASSERT_EQ(testPacket, owner.packetsReceived[0].payload);
    ASSERT_EQ(0x7F000001, owner.packetsReceived[0].address);
    ASSERT_EQ(ntohs(senderAddress.sin_port), owner.packetsReceived[0].port);

}

TEST_F(NetworkEndpointTests, ConnectionSending) {

    auto receiver = socket(AF_INET, SOCK_STREAM, 0);

    ASSERT_FALSE(receiver < 0);

    struct sockaddr_in receiverAddress;
    (void)memset(&receiverAddress, 0, sizeof(receiverAddress));
    receiverAddress.sin_family = AF_INET;
    receiverAddress.IPV4_ADDRESS_IN_SOCKADDR = 0;
    receiverAddress.sin_port = 0;
    ASSERT_TRUE(bind(receiver, (struct sockaddr*)&receiverAddress, sizeof(receiverAddress)) == 0);
    SOCKADDR_LENGTH_TYPE receiverAddressLength = sizeof(receiverAddress);
    uint16_t port;
    ASSERT_TRUE(getsockname(receiver, (struct sockaddr*)&receiverAddress, &receiverAddressLength) == 0);
    port = ntohs(receiverAddress.sin_port);

    // Set up the NetworkEndpoint.
    SystemAbstractions::NetworkEndpoint endpoint;
    Owner owner;
    endpoint.Open(
        [&owner](
            std::shared_ptr< SystemAbstractions::NetworkConnection > newConnection
        ) {
            owner.NetworkEndpointNewConnection(newConnection);
        },
        [&owner](
            uint32_t address,
            uint16_t port,
            const std::vector< uint8_t >& body
        ) {
            owner.NetworkEndpointPacketReceived(address, port, body);
        },
        SystemAbstractions::NetworkEndpoint::Mode::Connection,
        0,
        0,
        0
    );

    // Connect to the NetworkEndpoint
    struct sockaddr_in senderAddress;
    (void)memset(&senderAddress, 0, sizeof(senderAddress));
    senderAddress.sin_family = AF_INET;
    senderAddress.IPV4_ADDRESS_IN_SOCKADDR = htonl(0x7F000001);
    senderAddress.sin_port = htons(endpoint.GetBoundPort());
    ASSERT_TRUE(
        connect(
            receiver,
            (const sockaddr*)&senderAddress,
            sizeof(senderAddress)
        ) == 0
    );
    owner.AwaitConnection();
    struct sockaddr_in socketAddress;
    SOCKADDR_LENGTH_TYPE socketAddressLength = sizeof(socketAddress);
    ASSERT_TRUE(getsockname(receiver, (struct sockaddr*)&socketAddress, &socketAddressLength) == 0);
    ASSERT_EQ(ntohl(senderAddress.IPV4_ADDRESS_IN_SOCKADDR), owner.connections[0]->GetBoundAddress());
    ASSERT_EQ(ntohs(senderAddress.sin_port), owner.connections[0]->GetBoundPort());

    // Test sending a message from the unit under test.
    std::vector< uint8_t > testPacket{ 0x12, 0x34, 0x56, 0x78 };
    owner.connections[0]->SendMessage(testPacket);

    // Verify that we received the message from the unit under test.
    std::vector< uint8_t > buffer(testPacket.size());
    const int amountReceived = recv(
        receiver,
        (char*)buffer.data(),
        (int)buffer.size(),
        0
    );
    ASSERT_EQ(testPacket.size(), amountReceived);
    ASSERT_EQ(testPacket, buffer);
}

TEST_F(NetworkEndpointTests, ConnectionReceiving) {
    // Set up to connection-oriented socket to test sending to NetworkEndpoint
    auto sender = socket(AF_INET, SOCK_STREAM, 0);

    ASSERT_FALSE(sender < 0);

    struct sockaddr_in senderAddress;
    (void)memset(&senderAddress, 0, sizeof(senderAddress));
    senderAddress.sin_family = AF_INET;
    senderAddress.IPV4_ADDRESS_IN_SOCKADDR = 0;
    senderAddress.sin_port = 0;
    ASSERT_TRUE(bind(sender, (struct sockaddr*)&senderAddress, sizeof(senderAddress)) == 0);
    SOCKADDR_LENGTH_TYPE senderAddressLength = sizeof(senderAddress);
    uint16_t port;
    ASSERT_TRUE(getsockname(sender, (struct sockaddr*)&senderAddress, &senderAddressLength) == 0);
    port = ntohs(senderAddress.sin_port);

    // Set up the NetworkEndpoint
    SystemAbstractions::NetworkEndpoint endpoint;
    Owner owner;
    endpoint.Open(
        [&owner](
            std::shared_ptr< SystemAbstractions::NetworkConnection > newConnection
        ) { owner.NetworkEndpointNewConnection(newConnection); },
        [&owner](
            uint32_t address,
            uint16_t port,
            const std::vector< uint8_t >& body
        ) { owner.NetworkEndpointPacketReceived(address, port, body); },
        SystemAbstractions::NetworkEndpoint::Mode::Connection,
        0,
        0,
        0
    );

    // Connect to the NetworkEndpoint.
    struct sockaddr_in receiverAddress;
    (void)memset(&receiverAddress, 0, sizeof(receiverAddress));
    receiverAddress.sin_family = AF_INET;
    receiverAddress.IPV4_ADDRESS_IN_SOCKADDR = htonl(0x7F000001);
    receiverAddress.sin_port = htons(endpoint.GetBoundPort());
    ASSERT_TRUE(
        connect(
            sender,
            (const sockaddr*)&receiverAddress,
            sizeof(receiverAddress)
        ) == 0
    );
    owner.AwaitConnection();

    const std::vector< uint8_t > testPacket{ 0x12, 0x34, 0x56, 0x78 };
    (void)send(
        sender,
        (char*)testPacket.data(),
        (int)testPacket.size(),
        0
    );

    // Verify that we received the message at the unit under test.
    owner.AwaitStream(testPacket.size());
    ASSERT_EQ(testPacket, owner.streamReceived);
}

TEST_F(NetworkEndpointTests, ConnectionBroken) {
    auto client = socket(AF_INET, SOCK_STREAM, 0);

    ASSERT_FALSE(client < 0);
    struct sockaddr_in clientAddress;
    (void)memset(&clientAddress, 0, sizeof(clientAddress));
    clientAddress.sin_family = AF_INET;
    clientAddress.IPV4_ADDRESS_IN_SOCKADDR = 0;
    clientAddress.sin_port = 0;
    ASSERT_TRUE(bind(client, (struct sockaddr*)&clientAddress, sizeof(clientAddress)) == 0);
    SOCKADDR_LENGTH_TYPE clientAddressLength = sizeof(clientAddress);
    uint16_t port;
    ASSERT_TRUE(getsockname(client, (struct sockaddr*)&clientAddress, &clientAddressLength) == 0);
    port = ntohs(clientAddress.sin_port);

    // Set up the NetworkEndpoint.
    SystemAbstractions::NetworkEndpoint endpoint;
    Owner owner;
    endpoint.Open(
        [&owner](
            std::shared_ptr< SystemAbstractions::NetworkConnection > newConnection
        ) { owner.NetworkEndpointNewConnection(newConnection); },
        [&owner](
            uint32_t address,
            uint16_t port,
            const std::vector< uint8_t >& body 
        ) { owner.NetworkEndpointPacketReceived(address, port, body); },
        SystemAbstractions::NetworkEndpoint::Mode::Connection,
        0,
        0,
        0
    );

    // Connect to the NetworkEndpoint.
    struct sockaddr_in serverAddress;
    (void)memset(&serverAddress, 0, sizeof(serverAddress));
    serverAddress.sin_family = AF_INET;
    serverAddress.IPV4_ADDRESS_IN_SOCKADDR = htonl(0x7F000001);
    serverAddress.sin_port = htons(endpoint.GetBoundPort());
    ASSERT_TRUE(
        connect(
            client,
            (const sockaddr*)&serverAddress,
            sizeof(serverAddress)
        ) == 0
    );
    owner.AwaitConnection();

    (void)close(client);

    owner.AwaitDisconnection();
}

TEST_F(NetworkEndpointTests, MultipleConnections) {

    auto sender1 = socket(AF_INET, SOCK_STREAM, 0);
    auto sender2 = socket(AF_INET, SOCK_STREAM, 0);

    ASSERT_FALSE(sender1 < 0);
    ASSERT_FALSE(sender2 < 0);

    struct sockaddr_in sender1Address;
    (void)memset(&sender1Address, 0, sizeof(sender1Address));
    sender1Address.sin_family = AF_INET;
    sender1Address.IPV4_ADDRESS_IN_SOCKADDR = 0;
    sender1Address.sin_port = 0;
    struct sockaddr_in sender2Address = sender1Address;
    ASSERT_TRUE(bind(sender1, (struct sockaddr*)&sender1Address, sizeof(sender1Address)) == 0);
    ASSERT_TRUE(bind(sender2, (struct sockaddr*)&sender2Address, sizeof(sender2Address)) == 0);
    SOCKADDR_LENGTH_TYPE sender1AddressLength = sizeof(sender1Address);
    SOCKADDR_LENGTH_TYPE sender2AddressLength = sizeof(sender2Address);
    ASSERT_TRUE(getsockname(sender1, (struct sockaddr*)&sender1Address, &sender1AddressLength) == 0);
    ASSERT_TRUE(getsockname(sender2, (struct sockaddr*)&sender2Address, &sender2AddressLength) == 0);

    // Set up the NetworkEndpoint.
    SystemAbstractions::NetworkEndpoint endpoint;
    Owner owner;
    endpoint.Open(
        [&owner](
            std::shared_ptr< SystemAbstractions::NetworkConnection > newConnection
        ) { owner.NetworkEndpointNewConnection(newConnection); },
        [&owner](
            uint32_t address,
            uint16_t port,
            const std::vector< uint8_t >& body
        ) { owner.NetworkEndpointPacketReceived(address, port, body); },
        SystemAbstractions::NetworkEndpoint::Mode::Connection,
        0,
        0,
        0
    );

    // Connect to the NetworkEndpoint.
    struct sockaddr_in receiverAddress;
    (void)memset(&receiverAddress, 0, sizeof(receiverAddress));
    receiverAddress.sin_family = AF_INET;
    receiverAddress.IPV4_ADDRESS_IN_SOCKADDR = htonl(0x7F000001);
    receiverAddress.sin_port = htons(endpoint.GetBoundPort());
    ASSERT_TRUE(
        connect(
            sender1,
            (const sockaddr*)&receiverAddress,
            sizeof(receiverAddress)
        ) == 0
    );
    ASSERT_TRUE(
        connect(
            sender2,
            (const sockaddr*)&receiverAddress,
            sizeof(receiverAddress)           
        ) == 0
    );
    owner.AwaitConnections(2);

    // Test receiving a message at the unit under test.
    const std::vector< uint8_t > testPacket{ 0x12, 0x34, 0x56, 0x78 };
    (void)send(
        sender1,
        (char*)testPacket.data(),
        (int)testPacket.size() / 2,
        0
    );
    owner.AwaitStream(testPacket.size() / 2);
    (void)send(
        sender2,
        (char*)testPacket.data() + testPacket.size() / 2,
        (int)(testPacket.size() / 2),
        0
    );

    // Verify that we received the message at the unit under test.
    owner.AwaitStream(testPacket.size());
    ASSERT_EQ(testPacket, owner.streamReceived);
}
