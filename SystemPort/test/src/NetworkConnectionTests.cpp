/**
 * @file NetworkConnectionTests.cpp
 *
 * This module contains the unit tests of the
 * SystemAbstractions::NetworkConnection class.
 *
 * © 2020 by LiuJ
 */

#include <algorithm>
#include <condition_variable>
#include <functional>
#include <future>
#include <gtest/gtest.h>
#include <mutex>
#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>
#include <string>
#include <CppStringPlus/CppStringPlus.hpp>
#include <SystemPort/NetworkConnection.hpp>
#include <SystemPort/NetworkEndpoint.hpp>
#include <thread>
#include <time.h>
#include <vector>

#include <fcntl.h>
#include <netinet/ip.h>
#include <sys/socket.h>
#define IPV4_ADDRESS_IN_SOCKADDR sin_addr.s_addr
#define SOCKADDR_LENGTH_TYPE socklen_t
#define SOCKET int
#define closesocket close

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

        /**
         * This flag indicates whether or not a connection
         * to the network endpoint has been broken gracefully.
         */
        bool connectionBrokenGracefully = false;

        /**
         * This is a function to call when the connection is closed.
         *
         * @param[in] graceful
         *     This indicates whether or not the peer of connection
         *     has closed the connection gracefully (meaning we can
         *     continue to send our data back to the peer).
         */
        std::function< void(bool graceful) > connectionBrokenDelegate;
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
        void NetworkConnectionNewConnection(std::shared_ptr< SystemAbstractions::NetworkConnection > newConnection) {
            std::unique_lock< decltype(mutex) > lock(mutex);
            connections.push_back(newConnection);
            condition.notify_all();
            (void)newConnection->Process(
                [this](const std::vector< uint8_t >& message){
                    NetworkConnectionMessageReceived(message);
                },
                [this](bool graceful){
                    NetworkConnectionBroken(graceful);
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
        void NetworkConnectionBroken(bool graceful) {
            if (connectionBrokenDelegate != nullptr) {
                connectionBrokenDelegate(graceful);
            }
            {
                std::unique_lock< decltype(mutex) > lock(mutex);
                connectionBroken = true;
                connectionBrokenGracefully = graceful;
                condition.notify_all();
            }
        }

    };

} // namespace

/**
 *  This is the test fixture for these tests, providing common
 * setup and teardown for each test.
 */
struct NetworkConnectionTests 
    : public ::testing::Test
{

    SystemAbstractions::NetworkConnection client;

    std::shared_ptr< Owner > clientOwner = std::make_shared< Owner >();

    std::vector< std::string > diagnosticMessage;

    SystemAbstractions::DiagnosticsSender::UnsubscribeDelegate diagnosticUnsubscribeDelegate;

    bool printDiagnosticMessages = false;

    virtual void SetUp() {
        diagnosticUnsubscribeDelegate = client.SubscribeToDiagnostics(
            [this](
                std::string senderName,
                size_t level,
                std::string message 
            ) {
                diagnosticMessage.push_back(
                    CppStringPlus::sprintf(
                        "%s[%zu]: %s",
                        senderName.c_str(),
                        level,
                        message.c_str()
                    )
                );
                if (printDiagnosticMessages) {
                    printf(
                        "%s[%zu]: %s\n",
                        senderName.c_str(),
                        level,
                        message.c_str()
                    );
                }
            },
            1
        );
    }

    virtual void TearDown() {
        diagnosticUnsubscribeDelegate();
    }

};

TEST_F(NetworkConnectionTests, EstablishConnection) {
    SystemAbstractions::NetworkEndpoint server;
    std::vector< std::shared_ptr< SystemAbstractions::NetworkConnection > > clients;
    std::condition_variable_any callbackCondition;
    std::mutex callbackMutex;
    const auto newConnectionDelegate = [&clients, &callbackCondition, &callbackMutex](
        std::shared_ptr< SystemAbstractions::NetworkConnection > newConnection
    ) {
        std::unique_lock< std::mutex > lock(callbackMutex);
        clients.push_back(newConnection);
        callbackCondition.notify_all();
    };
    const auto packetReceivedDelegate =[](
        uint32_t address,
        uint16_t port,
        const std::vector< uint8_t >& body
    ){};

    ASSERT_TRUE(
        server.Open(
            newConnectionDelegate,
            packetReceivedDelegate,
            SystemAbstractions::NetworkEndpoint::Mode::Connection,
            0x7F000001,
            0,
            0
        )
    );
    ASSERT_FALSE(client.IsConnected());
    ASSERT_TRUE(client.Connect(0x7F000001, server.GetBoundPort()));
    {
        std::unique_lock< decltype(callbackMutex) > lock(callbackMutex);
        ASSERT_TRUE(
            callbackCondition.wait_for(
                lock,
                std::chrono::seconds(1),
                [&clients]{
                    return !clients.empty();
                }
            )
        );
    }
    ASSERT_EQ(server.GetBoundPort(), client.GetPeerPort());
    ASSERT_EQ(0x7F000001, client.GetPeerAddress());
    ASSERT_TRUE(client.IsConnected());
    ASSERT_EQ(client.GetBoundPort(), clients[0]->GetPeerPort());
    ASSERT_EQ(client.GetBoundAddress(), clients[0]->GetPeerAddress());
}

TEST_F(NetworkConnectionTests, SendingMessage) {
   SystemAbstractions::NetworkEndpoint server;
    Owner serverConnectionOwner;
    std::vector< std::shared_ptr< SystemAbstractions::NetworkConnection > > clients;
    std::condition_variable_any callbackCondition;
    std::mutex callbackMutex;
    const auto newConnectionDelegate = [
        &clients,
        &callbackCondition,
        &callbackMutex,
        &serverConnectionOwner
    ](
        std::shared_ptr< SystemAbstractions::NetworkConnection > newConnection
    ){
        std::unique_lock< std::mutex > lock(callbackMutex);
        clients.push_back(newConnection);
        ASSERT_TRUE(
            newConnection->Process(
                [&serverConnectionOwner](const std::vector< uint8_t >& message){
                    serverConnectionOwner.NetworkConnectionMessageReceived(message);
                },
                [&serverConnectionOwner](bool graceful){
                    serverConnectionOwner.NetworkConnectionBroken(graceful);
                }
            )
        );
        callbackCondition.notify_all();
    };
    const auto packetReceivedDelegate = [](
        uint32_t address,
        uint16_t port,
        const std::vector< uint8_t >& body
    ){
    };
    ASSERT_TRUE(
        server.Open(
            newConnectionDelegate,
            packetReceivedDelegate,
            SystemAbstractions::NetworkEndpoint::Mode::Connection,
            0x7F000001,
            0,
            0
        )
    );
    ASSERT_TRUE(client.Connect(0x7F000001, server.GetBoundPort()));
    auto clientOwnerCopy = clientOwner;
    ASSERT_TRUE(
        client.Process(
            [clientOwnerCopy](const std::vector< uint8_t >& message){
                clientOwnerCopy->NetworkConnectionMessageReceived(message);
            },
            [clientOwnerCopy](bool graceful){
                clientOwnerCopy->NetworkConnectionBroken(graceful);
            }
        )
    );
    const std::string messageAsString("Hello, World!");
    const std::vector< uint8_t > messageAsVector(messageAsString.begin(), messageAsString.end());
    client.SendMessage(messageAsVector);
    ASSERT_TRUE(serverConnectionOwner.AwaitStream(messageAsVector.size()));
    ASSERT_EQ(messageAsVector, serverConnectionOwner.streamReceived);
}

TEST_F(NetworkConnectionTests, RecevingMessage) {
    SystemAbstractions::NetworkEndpoint server;
    Owner serverConnectionOwner;
    std::vector< std::shared_ptr< SystemAbstractions::NetworkConnection > > clients;
    std::condition_variable_any callbackCondition;
    std::mutex callbackMutex;
    const auto newConnectionDelegate = [
        &clients,
        &callbackCondition,
        &callbackMutex,
        &serverConnectionOwner
    ](
        std::shared_ptr< SystemAbstractions::NetworkConnection > newConnection
    ){
        std::unique_lock< std::mutex > lock(callbackMutex);
        clients.push_back(newConnection);
        ASSERT_TRUE(
            newConnection->Process(
                [&serverConnectionOwner](const std::vector< uint8_t >& message) {
                    serverConnectionOwner.NetworkConnectionMessageReceived(message);
                },
                [&serverConnectionOwner](bool graceful) {
                    serverConnectionOwner.NetworkConnectionBroken(graceful);
                }
            )
        );
        callbackCondition.notify_all();
    };
    const auto packetReceivedDelegate = [](
        uint32_t address,
        uint16_t port,
        const std::vector< uint8_t >& body
    ){};
    ASSERT_TRUE(
        server.Open(
            newConnectionDelegate,
            packetReceivedDelegate,
            SystemAbstractions::NetworkEndpoint::Mode::Connection,
            0x7F000001,
            0,
            0
        )
    );
    ASSERT_TRUE(client.Connect(0x7F000001, server.GetBoundPort()));
    auto clientOwnerCopy = clientOwner;
    ASSERT_TRUE(
        client.Process(
            [clientOwnerCopy](const std::vector< uint8_t >& message) {
                clientOwnerCopy->NetworkConnectionMessageReceived(message);
            },
            [clientOwnerCopy](bool graceful) {
                clientOwnerCopy->NetworkConnectionBroken(graceful);
            }
        )
    );
    const std::string messageAsString("Hello, World!");
    const std::vector< uint8_t > messageAsVector(messageAsString.begin(), messageAsString.end());
    {
        std::unique_lock< decltype(callbackMutex) > lock(callbackMutex);
        ASSERT_TRUE(
            callbackCondition.wait_for(
                lock,
                std::chrono::seconds(2),
                [&clients]{
                    return !clients.empty();
                }
            )
        );
    }
    clients[0]->SendMessage(messageAsVector);
    ASSERT_TRUE(clientOwner->AwaitStream(messageAsVector.size()));
    ASSERT_EQ(messageAsVector, clientOwner->streamReceived);
}

TEST_F(NetworkConnectionTests, Close) {
    SystemAbstractions::NetworkEndpoint server;
    Owner serverConnectionOwner;
    std::vector< std::shared_ptr< SystemAbstractions::NetworkConnection > > clients;
    std::condition_variable_any callbackCondition;
    std::mutex callbackMutex;
    const auto newConnectionDelegate = [
        &clients,
        &callbackCondition,
        &callbackMutex,
        &serverConnectionOwner
    ](
        std::shared_ptr< SystemAbstractions::NetworkConnection > newConnection
    ){
        std::unique_lock< std::mutex > lock(callbackMutex);
        clients.push_back(newConnection);
        ASSERT_TRUE(
            newConnection->Process(
                [&serverConnectionOwner](const std::vector< uint8_t >& message){
                    serverConnectionOwner.NetworkConnectionMessageReceived(message);
                },
                [&serverConnectionOwner](bool graceful){
                    serverConnectionOwner.NetworkConnectionBroken(graceful);
                }
            )
        );
        callbackCondition.notify_all();
    };
    const auto packetReceivedDelegate = [](
        uint32_t address,
        uint16_t port,
        const std::vector< uint8_t >& body
    ){
    };
    ASSERT_TRUE(
        server.Open(
            newConnectionDelegate,
            packetReceivedDelegate,
            SystemAbstractions::NetworkEndpoint::Mode::Connection,
            0x7F000001,
            0,
            0
        )
    );
    ASSERT_TRUE(client.Connect(0x7F000001, server.GetBoundPort()));
    auto clientOwnerCopy = clientOwner;
    ASSERT_TRUE(
        client.Process(
            [clientOwnerCopy](const std::vector< uint8_t >& message){
                clientOwnerCopy->NetworkConnectionMessageReceived(message);
            },
            [clientOwnerCopy](bool graceful){
                clientOwnerCopy->NetworkConnectionBroken(graceful);
            }
        )
    );
    const std::string messageAsString("Hello, World!");
    const std::vector< uint8_t > messageAsVector(messageAsString.begin(), messageAsString.end());
    {
        std::unique_lock< decltype(callbackMutex) > lock(callbackMutex);
        ASSERT_TRUE(
            callbackCondition.wait_for(
                lock,
                std::chrono::seconds(1),
                [&clients]{
                    return !clients.empty();
                }
            )
        );
    }
    ASSERT_FALSE(serverConnectionOwner.connectionBroken);
    client.Close();
    ASSERT_TRUE(serverConnectionOwner.AwaitDisconnection());
    ASSERT_TRUE(serverConnectionOwner.connectionBroken);
}

TEST_F(NetworkConnectionTests, CloseDuringBrokenConnectionCallback) {
    SystemAbstractions::NetworkEndpoint server;
    Owner serverConnectionOwner;
    std::vector< std::shared_ptr< SystemAbstractions::NetworkConnection > > clients;
    std::condition_variable_any callbackCondition;
    std::mutex callbackMutex;
    const auto newConnectionDelegate = [
        &clients,
        &callbackCondition,
        &callbackMutex,
        &serverConnectionOwner
    ](
        std::shared_ptr< SystemAbstractions::NetworkConnection > newConnection
    ){
        std::unique_lock< std::mutex > lock(callbackMutex);
        clients.push_back(newConnection);
        ASSERT_TRUE(
            newConnection->Process(
                [&serverConnectionOwner](const std::vector< uint8_t >& message){
                    serverConnectionOwner.NetworkConnectionMessageReceived(message);
                },
                [&serverConnectionOwner](bool graceful){
                    serverConnectionOwner.NetworkConnectionBroken(graceful);
                }
            )
        );
        callbackCondition.notify_all();
    };
    const auto packetReceivedDelegate = [](
        uint32_t address,
        uint16_t port,
        const std::vector< uint8_t >& body
    ){
    };
    ASSERT_TRUE(
        server.Open(
            newConnectionDelegate,
            packetReceivedDelegate,
            SystemAbstractions::NetworkEndpoint::Mode::Connection,
            0x7F000001,
            0,
            0
        )
    );
    clientOwner->connectionBrokenDelegate = [this](bool){ client.Close(); };
    (void)client.Connect(0x7F000001, server.GetBoundPort());
    auto clientOwnerCopy = clientOwner;
    (void)client.Process(
        [clientOwnerCopy](const std::vector< uint8_t >& message){
            clientOwnerCopy->NetworkConnectionMessageReceived(message);
        },
        [clientOwnerCopy](bool graceful){
            clientOwnerCopy->NetworkConnectionBroken(graceful);
        }
    );
    {
        std::unique_lock< decltype(callbackMutex) > lock(callbackMutex);
        ASSERT_TRUE(
            callbackCondition.wait_for(
                lock,
                std::chrono::seconds(1),
                [&clients]{
                    return !clients.empty();
                }
            )
        );
    }
    clients[0]->Close();
    clientOwner->AwaitDisconnection();
}

TEST_F(NetworkConnectionTests, InitiateCloseGracefullyWithDataQueued) {
    // Set up a connection-oriented socket to receive
    // a connection from the unit under test.
    auto server = socket(AF_INET, SOCK_STREAM, 0);
    std::shared_ptr< SOCKET > serverReference(
        &server,
        [&server](SOCKET* s){
            if (server >= 0) {
                closesocket(server);
            }
        }
    );
    ASSERT_FALSE(server < 0);
    struct sockaddr_in serverAddress;
    (void)memset(&serverAddress, 0, sizeof(serverAddress));
    serverAddress.sin_family = AF_INET;
    serverAddress.IPV4_ADDRESS_IN_SOCKADDR = 0;
    serverAddress.sin_port = 0;
    ASSERT_TRUE(bind(server, (struct sockaddr*)&serverAddress, sizeof(serverAddress)) == 0);
    SOCKADDR_LENGTH_TYPE serverAddressLength = sizeof(serverAddress);
    uint16_t serverPort;
    ASSERT_TRUE(getsockname(server, (struct sockaddr*)&serverAddress, &serverAddressLength) == 0);
    serverPort = ntohs(serverAddress.sin_port);
    ASSERT_TRUE(listen(server, SOMAXCONN) == 0);

    // Have unit under test connect to the server.
    SOCKET serverConnection;
    std::shared_ptr< SOCKET > serverConnectionReference(
        &serverConnection,
        [&serverConnection](SOCKET* s){
            if (serverConnection >= 0) {
                closesocket(serverConnection);
            }
        }
    );
    std::thread serverAccept(
        [server, &serverConnection]{
            struct sockaddr_in clientAddress;
            SOCKADDR_LENGTH_TYPE clientAddressLength = sizeof(clientAddress);
            serverConnection = accept(server, (struct sockaddr*)&clientAddress, &clientAddressLength);
            ASSERT_FALSE(serverConnection < 0);
        }
    );
    ASSERT_TRUE(client.Connect(0x7F000001, serverPort));
    serverAccept.join();
    auto clientOwnerCopy = clientOwner;
    (void)client.Process(
        [clientOwnerCopy](const std::vector< uint8_t >& message){
            clientOwnerCopy->NetworkConnectionMessageReceived(message);
        },
        [clientOwnerCopy](bool graceful){
            clientOwnerCopy->NetworkConnectionBroken(graceful);
        }
    );

    // Queue up to send a large amount of data to the server.
    std::vector< uint8_t > data(10000000, 'X');
    client.SendMessage(data);

    // Issue a graceful close on the client.
    client.Close(true);

    // Verify server connection is not broken until all the
    // data is received.
    std::vector< uint8_t > buffer(100000);
    size_t totalBytesReceived = 0;
    while (totalBytesReceived < data.size()) {
        const auto bytesToReceive = std::min(
            buffer.size(),
            data.size() - totalBytesReceived
        );
        const auto bytesReceived = recv(
            serverConnection,
            (char*) buffer.data(),
            (int)bytesToReceive,
            MSG_WAITALL
        );
        ASSERT_FALSE(bytesReceived <= 0) << totalBytesReceived;
        totalBytesReceived += bytesReceived;
    }

    // Verify unit under test has not yet indicated
    // that the connection is broken.
    ASSERT_FALSE(clientOwner->connectionBroken);

    // Close the server connection and verify the connection
    // is finally broken.
    (void)closesocket(serverConnection);
    serverConnection = -1;
    ASSERT_TRUE(clientOwner->AwaitDisconnection());
}

TEST_F(NetworkConnectionTests, ReceiveCloseGracefully) {
    // Set up a connection-oriented socket to receive
    // a connection from the unit under test.
    auto server = socket(AF_INET, SOCK_STREAM, 0);
    std::shared_ptr< SOCKET > serverReference(
        &server,
        [&server](SOCKET* s){
            if (server >= 0) {
                closesocket(server);
            }
        }
    );
    ASSERT_FALSE(server < 0);
    struct sockaddr_in serverAddress;
    (void)memset(&serverAddress, 0, sizeof(serverAddress));
    serverAddress.sin_family = AF_INET;
    serverAddress.IPV4_ADDRESS_IN_SOCKADDR = 0;
    serverAddress.sin_port = 0;
    ASSERT_TRUE(bind(server, (struct sockaddr*)&serverAddress, sizeof(serverAddress)) == 0);
    SOCKADDR_LENGTH_TYPE serverAddressLength = sizeof(serverAddress);
    uint16_t serverPort;
    ASSERT_TRUE(getsockname(server, (struct sockaddr*)&serverAddress, &serverAddressLength) == 0);
    serverPort = ntohs(serverAddress.sin_port);
    ASSERT_TRUE(listen(server, SOMAXCONN) == 0);

    // Have unit under test connect to the server.
    SOCKET serverConnection;
    std::shared_ptr< SOCKET > serverConnectionReference(
        &serverConnection,
        [&serverConnection](SOCKET* s){
            if (serverConnection >= 0) {
                closesocket(serverConnection);
            }
        }
    );
    std::thread serverAccept(
        [server, &serverConnection]{
            struct sockaddr_in clientAddress;
            SOCKADDR_LENGTH_TYPE clientAddressLength = sizeof(clientAddress);
            serverConnection = accept(server, (struct sockaddr*)&clientAddress, &clientAddressLength);
            ASSERT_FALSE(serverConnection < 0);
        }
    );
    ASSERT_TRUE(client.Connect(0x7F000001, serverPort));
    serverAccept.join();
    auto clientOwnerCopy = clientOwner;
    (void)client.Process(
        [clientOwnerCopy](const std::vector< uint8_t >& message){
            clientOwnerCopy->NetworkConnectionMessageReceived(message);
        },
        [clientOwnerCopy](bool graceful){
            clientOwnerCopy->NetworkConnectionBroken(graceful);
        }
    );

    // Queue up to send a large amount of data to the server.
    std::vector< uint8_t > data(10000000, 'X');
    client.SendMessage(data);

    (void)shutdown(serverConnection, SHUT_WR);
    // Verify client receives the graceful close notification.
    EXPECT_TRUE(clientOwner->AwaitDisconnection());
    EXPECT_TRUE(clientOwner->connectionBrokenGracefully);
    clientOwner->connectionBroken = false;
    clientOwner->connectionBrokenGracefully = false;

    // Close the client end of the connection.
    client.Close(true);

    // Verify client connection is not broken until all the
    // data is sent.
    std::vector< uint8_t > buffer(100000);
    size_t totalBytesReceived = 0;
    while (totalBytesReceived < data.size()) {
        const auto bytesToReceive = std::min(
            buffer.size(),
            data.size() - totalBytesReceived
        );
        const auto bytesReceived = recv(
            serverConnection,
            (char*) buffer.data(),
            (int)bytesToReceive,
            MSG_WAITALL
        );
        ASSERT_FALSE(bytesReceived <= 0) << totalBytesReceived << " recv returned: " << bytesReceived;
        totalBytesReceived += bytesReceived;
    }

    // Verify the server closes its end after sending
    // the last message.
    const auto bytesToReceive = buffer.size();
    const auto bytesReceived = recv(
        serverConnection,
        (char*) buffer.data(),
        (int)bytesToReceive,
        MSG_WAITALL
    );
    EXPECT_TRUE(bytesReceived == 0) << bytesReceived;

    // Verify unit under test receives the indication
    // that the connection is broken.
    ASSERT_TRUE(clientOwner->AwaitDisconnection());
    EXPECT_FALSE(clientOwner->connectionBrokenGracefully);
    ASSERT_EQ(
        (std::vector< std::string >{
            "NetworkConnection[1]: connection closed gracefully by peer",
            "NetworkConnection[1]: closing connection",
            "NetworkConnection[1]: closed connection",
        }),
        diagnosticMessage
    );
}

TEST_F(NetworkConnectionTests, InitiateCloseAbruptly) {
    // Set up a connection-oriented socket to receive
    // a connection from the unit under test.
    auto server = socket(AF_INET, SOCK_STREAM, 0);
    std::shared_ptr< SOCKET > serverReference(
        &server,
        [&server](SOCKET* s){
            if (server >= 0) {
                closesocket(server);
            }
        }
    );
    ASSERT_FALSE(server < 0);
    struct sockaddr_in serverAddress;
    (void)memset(&serverAddress, 0, sizeof(serverAddress));
    serverAddress.sin_family = AF_INET;
    serverAddress.IPV4_ADDRESS_IN_SOCKADDR = 0;
    serverAddress.sin_port = 0;
    ASSERT_TRUE(bind(server, (struct sockaddr*)&serverAddress, sizeof(serverAddress)) == 0);
    SOCKADDR_LENGTH_TYPE serverAddressLength = sizeof(serverAddress);
    uint16_t serverPort;
    ASSERT_TRUE(getsockname(server, (struct sockaddr*)&serverAddress, &serverAddressLength) == 0);
    serverPort = ntohs(serverAddress.sin_port);
    ASSERT_TRUE(listen(server, SOMAXCONN) == 0);

    // Have unit under test connect to the server.
    SOCKET serverConnection;
    std::shared_ptr< SOCKET > serverConnectionReference(
        &serverConnection,
        [&serverConnection](SOCKET* s){
            if (serverConnection >= 0) {
                closesocket(serverConnection);
            }
        }
    );
    std::thread serverAccept(
        [server, &serverConnection]{
            struct sockaddr_in clientAddress;
            SOCKADDR_LENGTH_TYPE clientAddressLength = sizeof(clientAddress);
            serverConnection = accept(server, (struct sockaddr*)&clientAddress, &clientAddressLength);
            ASSERT_FALSE(serverConnection < 0);
        }
    );
    ASSERT_TRUE(client.Connect(0x7F000001, serverPort));
    serverAccept.join();
    auto clientOwnerCopy = clientOwner;
    (void)client.Process(
        [clientOwnerCopy](const std::vector< uint8_t >& message){
            clientOwnerCopy->NetworkConnectionMessageReceived(message);
        },
        [clientOwnerCopy](bool graceful){
            clientOwnerCopy->NetworkConnectionBroken(graceful);
        }
    );

    // Queue up to send a large amount of data to the server.
    std::vector< uint8_t > data(10000000, 'X');
    client.SendMessage(data);

    // Issue an abrupt close on the client.
    client.Close(false);

    // Verify server connection is broken before all the
    // data is received.
    std::vector< uint8_t > buffer(100000);
    size_t totalBytesReceived = 0;
    while (totalBytesReceived < data.size()) {
        const auto bytesToReceive = std::min(
            buffer.size(),
            data.size() - totalBytesReceived
        );
        const auto bytesReceived = recv(
            serverConnection,
            (char*) buffer.data(),
            (int)bytesToReceive,
            MSG_WAITALL
        );
        EXPECT_FALSE(bytesReceived == 0);
        if (bytesReceived <= 0) {
            break;
        }
        totalBytesReceived += bytesReceived;
    }
    EXPECT_LT(totalBytesReceived, data.size());

    // Verify unit under test has indicated
    // that the connection is broken.
    EXPECT_TRUE(clientOwner->AwaitDisconnection());
}

TEST_F(NetworkConnectionTests, ReceiveCloseAbruptly) {
    // Set up a connection-oriented socket to receive
    // a connection from the unit under test.
    auto server = socket(AF_INET, SOCK_STREAM, 0);
    std::shared_ptr< SOCKET > serverReference(
        &server,
        [&server](SOCKET* s){
            if (server >= 0) {
                closesocket(server);
            }
        }
    );
    ASSERT_FALSE(server < 0);
    struct sockaddr_in serverAddress;
    (void)memset(&serverAddress, 0, sizeof(serverAddress));
    serverAddress.sin_family = AF_INET;
    serverAddress.IPV4_ADDRESS_IN_SOCKADDR = 0;
    serverAddress.sin_port = 0;
    ASSERT_TRUE(bind(server, (struct sockaddr*)&serverAddress, sizeof(serverAddress)) == 0);
    SOCKADDR_LENGTH_TYPE serverAddressLength = sizeof(serverAddress);
    uint16_t serverPort;
    ASSERT_TRUE(getsockname(server, (struct sockaddr*)&serverAddress, &serverAddressLength) == 0);
    serverPort = ntohs(serverAddress.sin_port);
    ASSERT_TRUE(listen(server, SOMAXCONN) == 0);

    // Have unit under test connect to the server.
    SOCKET serverConnection;
    std::shared_ptr< SOCKET > serverConnectionReference(
        &serverConnection,
        [&serverConnection](SOCKET* s){
            if (serverConnection >= 0) {
                closesocket(serverConnection);
            }
        }
    );
    std::thread serverAccept(
        [server, &serverConnection]{
            struct sockaddr_in clientAddress;
            SOCKADDR_LENGTH_TYPE clientAddressLength = sizeof(clientAddress);
            serverConnection = accept(server, (struct sockaddr*)&clientAddress, &clientAddressLength);
            ASSERT_FALSE(serverConnection < 0);
        }
    );
    ASSERT_TRUE(client.Connect(0x7F000001, serverPort));
    serverAccept.join();
    auto clientOwnerCopy = clientOwner;
    (void)client.Process(
        [clientOwnerCopy](const std::vector< uint8_t >& message){
            clientOwnerCopy->NetworkConnectionMessageReceived(message);
        },
        [clientOwnerCopy](bool graceful){
            clientOwnerCopy->NetworkConnectionBroken(graceful);
        }
    );

    // Queue up to send a large amount of data to the server.
    std::vector< uint8_t > data(10000000, 'X');
    client.SendMessage(data);
    struct linger linger;
    linger.l_onoff = 1;
    linger.l_linger = 0;
    (void)setsockopt(serverConnection, SOL_SOCKET, SO_LINGER, &linger, sizeof(linger));
    (void)closesocket(serverConnection);

    // Verify client receives the abrupt close notification.
    EXPECT_TRUE(clientOwner->AwaitDisconnection());
    EXPECT_FALSE(clientOwner->connectionBrokenGracefully);
    ASSERT_EQ(
        (std::vector< std::string >{
            "NetworkConnection[1]: connection closed abruptly by peer",
            "NetworkConnection[1]: closed connection",
        }),
        diagnosticMessage
    );
}

TEST_F(NetworkConnectionTests, InitiateCloseGracefullyNoDataQueued) {
    // Set up a connection-oriented socket to receive
    // a connection from the unit under test.
    auto server = socket(AF_INET, SOCK_STREAM, 0);
    std::shared_ptr< SOCKET > serverReference(
        &server,
        [&server](SOCKET* s){
            if (server >= 0) {
                closesocket(server);
            }
        }
    );
    ASSERT_FALSE(server < 0);
    struct sockaddr_in serverAddress;
    (void)memset(&serverAddress, 0, sizeof(serverAddress));
    serverAddress.sin_family = AF_INET;
    serverAddress.IPV4_ADDRESS_IN_SOCKADDR = 0;
    serverAddress.sin_port = 0;
    ASSERT_TRUE(bind(server, (struct sockaddr*)&serverAddress, sizeof(serverAddress)) == 0);
    SOCKADDR_LENGTH_TYPE serverAddressLength = sizeof(serverAddress);
    uint16_t serverPort;
    ASSERT_TRUE(getsockname(server, (struct sockaddr*)&serverAddress, &serverAddressLength) == 0);
    serverPort = ntohs(serverAddress.sin_port);
    ASSERT_TRUE(listen(server, SOMAXCONN) == 0);

    // Have unit under test connect to the server.
    SOCKET serverConnection;
    std::shared_ptr< SOCKET > serverConnectionReference(
        &serverConnection,
        [&serverConnection](SOCKET* s){
            if (serverConnection >= 0) {
                closesocket(serverConnection);
            }
        }
    );
    std::thread serverAccept(
        [server, &serverConnection]{
            struct sockaddr_in clientAddress;
            SOCKADDR_LENGTH_TYPE clientAddressLength = sizeof(clientAddress);
            serverConnection = accept(server, (struct sockaddr*)&clientAddress, &clientAddressLength);
            ASSERT_FALSE(serverConnection < 0);
        }
    );
    ASSERT_TRUE(client.Connect(0x7F000001, serverPort));
    serverAccept.join();
    auto clientOwnerCopy = clientOwner;
    (void)client.Process(
        [clientOwnerCopy](const std::vector< uint8_t >& message){
            clientOwnerCopy->NetworkConnectionMessageReceived(message);
        },
        [clientOwnerCopy](bool graceful){
            clientOwnerCopy->NetworkConnectionBroken(graceful);
        }
    );

    // Issue a graceful close on the client.
    client.Close(true);


    int flags = fcntl(serverConnection, F_GETFL, 0);
    flags |= O_NONBLOCK;
    (void)fcntl(serverConnection, F_SETFL, flags);
    bool wouldBlock = true;
    const auto startTime = time(NULL);
    while (wouldBlock) {
        wouldBlock = false;
        ASSERT_FALSE(time(NULL) - startTime > 1);
        std::vector< uint8_t > buffer(100000);
        const auto bytesReceived = recv(
            serverConnection,
            (char*)buffer.data(),
            (int)buffer.size(),
            0
        );
        if (bytesReceived < 0) {
            if (errno == EWOULDBLOCK) {
                wouldBlock = true;
            }
        }
    }

    // Verify unit under test has not yet indicated
    // that the connection is broken.
    ASSERT_FALSE(clientOwner->connectionBroken);

    // Close the server connection and verify the connection
    // is finally broken.
    (void)closesocket(serverConnection);
    serverConnection = -1;
    ASSERT_TRUE(clientOwner->AwaitDisconnection());
}

TEST_F(NetworkConnectionTests, GetAddressOfHost) {
    EXPECT_EQ(0x7f000001, SystemAbstractions::NetworkConnection::GetAddressOfHost("localhost"));
    EXPECT_EQ(0x7f000001, SystemAbstractions::NetworkConnection::GetAddressOfHost("127.0.0.1"));
    EXPECT_EQ(0x08080808, SystemAbstractions::NetworkConnection::GetAddressOfHost("8.8.8.8"));
    EXPECT_EQ(0, SystemAbstractions::NetworkConnection::GetAddressOfHost(".example"));
}

TEST_F(NetworkConnectionTests, ReleaseFromDelegate) {
    SystemAbstractions::NetworkEndpoint server;
    const auto clients = std::make_shared< std::vector< std::shared_ptr< SystemAbstractions::NetworkConnection > > >();
    const auto newConnectionDelegate = [
        &clients
    ](
        std::shared_ptr< SystemAbstractions::NetworkConnection > newConnection
    ){
        clients->push_back(newConnection);
        const std::weak_ptr< std::vector< std::shared_ptr< SystemAbstractions::NetworkConnection > > > weakClients(clients);
        (void)newConnection->Process(
            [weakClients](const std::vector< uint8_t >& message){
                const auto clients = weakClients.lock();
                if (clients == nullptr) {
                    return;
                }
                clients->clear();
            },
            [](bool graceful){
            }
        );
    };
    const auto packetReceivedDelegate = [](
        uint32_t address,
        uint16_t port,
        const std::vector< uint8_t >& body
    ){
    };
    (void)server.Open(
        newConnectionDelegate,
        packetReceivedDelegate,
        SystemAbstractions::NetworkEndpoint::Mode::Connection,
        0x7F000001,
        0,
        0
    );
    (void)client.Connect(0x7F000001, server.GetBoundPort());
    const auto closed = std::make_shared< std::promise< void > >();
    (void)client.Process(
        [](const std::vector< uint8_t >& message){
        },
        [closed](bool graceful){
            closed->set_value();
        }
    );
    const std::string messageAsString("Hello, World!");
    const std::vector< uint8_t > messageAsVector(messageAsString.begin(), messageAsString.end());
    client.SendMessage(messageAsVector);
    auto wasClosed = closed->get_future();
    EXPECT_EQ(
        std::future_status::ready,
        wasClosed.wait_for(std::chrono::milliseconds(1000))
    );
}
