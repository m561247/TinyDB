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
#include <protocol/Greeting.hpp>
#include <protocol/Auth.hpp>

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

struct StreamPacket {

        std::vector< uint8_t > payload;

        std::shared_ptr< SystemAbstractions::NetworkConnection > connection;

        StreamPacket(
            const std::vector< uint8_t >& newPayload,
            std::shared_ptr< SystemAbstractions::NetworkConnection > newConnection
        ) 
            : payload(newPayload)
            , connection(newConnection)
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

    std::vector< StreamPacket > streamPacket;

    std::vector< std::shared_ptr< SystemAbstractions::NetworkConnection > > connections;

    std::vector< uint8_t > GreetingPacket;
   
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

    bool AwaitStreamPacket() {
        std::unique_lock< decltype(mutex) > lock(mutex);
        return condition.wait_for(
            lock,
            std::chrono::seconds(1),
            [this] {
                // std::printf("Check streamPacket... \n");
                return !streamPacket.empty();
            }
        );
    }

    bool AwaitConnection() {
        std::unique_lock< decltype(mutex) > lock(mutex);
        return condition.wait_for(
            lock,
            std::chrono::seconds(1),
            [this]{
                // std::printf("Check is client connected... \n");
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
        std::printf("%d:%d connected\n", newConnection->GetPeerAddress(), newConnection->GetPeerPort());
        newConnection->SendMessage(GreetingPacket);
        condition.notify_all();
        (void)newConnection->Process(
            [this, newConnection](const std::vector< uint8_t >& message) {
                NetworkConnectionMessageReceived(message, newConnection);
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
    void NetworkConnectionMessageReceived(const std::vector< uint8_t >& message, std::shared_ptr< SystemAbstractions::NetworkConnection > newConnection) {
        std::unique_lock< decltype(mutex) > lock(mutex);
        // streamReceived.insert(
        //     streamReceived.end(),
        //     message.begin(),
        //     message.end()
        // );
        streamPacket.emplace_back(message, newConnection);
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
  std::string userInput;
  printf("WELCOME TO TINY DB!\n");
  printf(">> ");
  signal(SIGINT, free_all);

//   while(getline(std::cin, userInput)) {
//       if (userInput == "quit") {
//         break;
//       }
//       std::vector < uint8_t > bytes(userInput.begin(), userInput.end());
//       for (auto c: bytes) {
//         std::cout << std::to_string(c) << ", ";
//       }
//       run_parser(userInput.c_str());
//       printf(">> ");
//   }
  // std::cout << "close" << std::endl;
  // // close db
  // execute_quit();
   // Set up the NetworkEndpoint
  SystemAbstractions::NetworkEndpoint endpoint;
  Owner owner;
  Protocol::GreetingPacket gp(1, "TinyDB-v0.0.1");
  std::vector< uint8_t > outputPacket = gp.Pack();
  owner.GreetingPacket.push_back(outputPacket.size());
  owner.GreetingPacket.push_back(0x00);
  owner.GreetingPacket.push_back(0x00);
  owner.GreetingPacket.push_back(0x00);

  owner.GreetingPacket.insert(
      owner.GreetingPacket.end(),
      outputPacket.begin(),
      outputPacket.end()
  );

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
  
  const std::string messageAsString("+OK\r\n");
  const std::vector< uint8_t > messageAsVector(messageAsString.begin(), messageAsString.end());
  const std::vector< uint8_t > GreetingPacket{74, 0, 0, 0, 10, 70, 97, 107, 101, 68, 66, 0, 1, 0, 0, 0, 102, 116, 41, 51, 66, 52, 110, 64, 0, 13, 162, 33, 2, 0, 9, 1, 21, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 45, 51, 83, 62, 28, 73, 47, 89, 16, 110, 12, 38, 0, 109, 121, 115, 113, 108, 95, 110, 97, 116, 105, 118, 101, 95, 112, 97, 115, 115, 119, 111, 114, 100, 0};
  std::vector< uint8_t > OkPacket = {7, 0, 0, 2, 0, 10, 0, 2, 0, 0, 0};
  int count = 0;
  Protocol::AuthPacket ap;
  while (1) {
    // TCP
    if(owner.AwaitStreamPacket()){
      std::cout << std::endl << "======================================================== GET DATA START SEQ :" << count << std::endl;
      for (auto iter = owner.streamPacket.begin(); iter != owner.streamPacket.end(); iter++)
      {
          if (iter->payload[3] == 1) {
              std::vector< uint8_t >::iterator it = iter->payload.begin();
              auto authPkt = std::vector<uint8_t>(it += 4, iter->payload.end());
              // auth 包
              ap.UnPack(authPkt);
              std::cout << "UserName: " << ap.GetUserName() << std::endl;
              std::cout << "PluginName: " << ap.GetPluginName() << std::endl;
              std::cout << "DataBaseName: " << ap.GetDatabaseName() << std::endl;
              OkPacket[3] = iter->payload[3] + 1;
              iter->connection->SendMessage(OkPacket);
          } else { 
            // print packet
            for (auto it = iter->payload.begin(); it != iter->payload.end(); it++) {
                std::cout << std::to_string(*it) << ", ";
            }
            std::vector< uint8_t >::iterator it = iter->payload.begin();
            std::cout << std::endl;
            // cmd type
            it += 4;
            std::cout << "cmd type: " << std::to_string(*it) << std::endl;
            it ++;
            // cmd param
            size_t SIZE = iter->payload.size();
            std::string cmdParam(it, iter->payload.end());
            std::cout << "cmd param: " << cmdParam << std::endl;
            if(cmdParam == "SELECT PersonID, LastName, FirstName FROM Persons") {
                iter->connection->SendMessage(std::vector<uint8_t>{1, 0, 0, 1, 3});
                iter->connection->SendMessage(std::vector<uint8_t>{54, 0, 0, 2, 3, 100, 101, 102, 2, 100, 98, 7, 80, 101, 114, 115, 111, 110, 115, 7, 80, 101, 114, 115, 111, 110, 115, 8, 80, 101, 114, 115, 111, 110, 73, 68, 8, 80, 101, 114, 115, 111, 110, 73, 68, 12, 63, 0, 11, 0, 0, 0, 3, 3, 0, 0, 0, 0});
                iter->connection->SendMessage(std::vector<uint8_t>{54, 0, 0, 3, 3, 100, 101, 102, 2, 100, 98, 7, 80, 101, 114, 115, 111, 110, 115, 7, 80, 101, 114, 115, 111, 110, 115, 8, 76, 97, 115, 116, 78, 97, 109, 101, 8, 76, 97, 115, 116, 78, 97, 109, 101, 12, 33, 0, 80, 0, 0, 0, 253, 0, 0, 0, 0, 0});
                iter->connection->SendMessage(std::vector<uint8_t>{56, 0, 0, 4, 3, 100, 101, 102, 2, 100, 98, 7, 80, 101, 114, 115, 111, 110, 115, 7, 80, 101, 114, 115, 111, 110, 115, 9, 70, 105, 114, 115, 116, 78, 97, 109, 101, 9, 70, 105, 114, 115, 116, 78, 97, 109, 101, 12, 33, 0, 80, 0, 0, 0, 253, 0, 0, 0, 0, 0});
                iter->connection->SendMessage(std::vector<uint8_t>{5, 0, 0, 5, 254, 0, 0, 2, 0,});
                iter->connection->SendMessage(std::vector<uint8_t>{15, 0, 0, 6, 4, 45, 50, 51, 56, 5, 90, 104, 111, 110, 103, 3, 76, 101, 105});
                iter->connection->SendMessage(std::vector<uint8_t>{23, 0, 0, 7, 4, 49, 48, 48, 48, 11, 87, 97, 115, 115, 101, 114, 115, 116, 101, 105, 110, 5, 90, 104, 97, 110, 103});
                iter->connection->SendMessage(std::vector<uint8_t>{5, 0, 0, 8, 254, 0, 0, 2, 0});
            } else {
                // return
                OkPacket[3] = iter->payload[3] + 1;
                // print ok packet
                // for (auto item : OkPacket) {
                //     std::cout << std::to_string(item) << "| ";
                // }

                iter->connection->SendMessage(OkPacket);
            }
            std::cout << std::endl;
            std::cout << std::flush  << std::endl;
          }
      }
      std::cout << std::endl << "======================================================== GET DATA END SEQ :" << count << std::endl;
      count ++;
      owner.streamPacket.clear();
    }
    // sleep(1);
  }
  return 0;
}
