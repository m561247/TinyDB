#include <string.h>
#include <iostream>
#include <string>
#include <stdio.h>
#include <signal.h>
#include <condition_variable>
#include <mutex>
#include <SystemPort/NetworkEndpoint.hpp>
#include <iostream>
#include <unistd.h>
#include <netinet/ip.h>
#include <sys/socket.h>
#include <algorithm>
#include "parser/execute.h"
#define IPV4_ADDRESS_IN_SOCKADDR sin_addr.s_addr
#define SOCKADDR_LENGTH_TYPE socklen_t

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


extern "C" char run_parser(const char *input);

void free_all(int num) {
  execute_quit();
  printf("exit successful!");
  exit(0);
}

int main(int argc, char *argv[])
{
  // std::string userInput;
  // printf("WELCOME TO TINY DB!\n");
  // printf(">> ");
  // signal(SIGINT, free_all);

  // while(getline(std::cin, userInput)) {
  //     if (userInput == "quit") {
  //       break;
  //     }
  //     run_parser(userInput.c_str());
  //     printf(">> ");
  // }
  // std::cout << "close" << std::endl;
  // // close db
  // execute_quit();
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
      SystemAbstractions::NetworkEndpoint::Mode::Connection,
      0,
      0,
      12306        
  );

  int count = 0;
  owner.AwaitConnection();
  while (1) {
    if(owner.AwaitStream(2)){
      for_each(owner.streamReceived.begin(), owner.streamReceived.end(), [](const uint8_t& val)->void{ std::printf("%c ", val); });
      owner.streamReceived.clear();
    }
    count ++;
    std::printf("count: %d \n", count);
    sleep(1);
  }
  return 0;
}
