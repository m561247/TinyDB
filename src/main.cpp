/**
 *  @file main.cpp
 *  
 *  tinydb 主程序 v0.0.1
 *      
 *  make it work! 一个有很多潜藏 bug 的版本。
 * 
 *  Copyright © 2020 by LiuJ
 */

#include <string.h>
#include <iostream>
#include <string>
#include <stdio.h>
#include <signal.h>
#include <condition_variable>
#include <mutex>
#include <SystemPort/NetworkEndpoint.hpp>
#include <SystemPort/File.hpp>
#include <iostream>
#include <unistd.h>
#include <netinet/ip.h>
#include <sys/socket.h>
#include <algorithm>
#include <protocol/Greeting.hpp>
#include <protocol/Auth.hpp>
#include <CppStringPlus/CppStringPlus.hpp>
#include "parser/parser.h"
#include "database/dbms.h"

#define IPV4_ADDRESS_IN_SOCKADDR sin_addr.s_addr
#define SOCKADDR_LENGTH_TYPE socklen_t

template<typename T, typename DataDeleter>
void free_linked_list(linked_list_t *linked_list, DataDeleter data_deleter)
{
	for(linked_list_t *l_ptr = linked_list; l_ptr; )
	{
		T* data = (T*)l_ptr->data;
		data_deleter(data);
		linked_list_t *tmp = l_ptr;
		l_ptr = l_ptr->next;
		free(tmp);
	}
}

void expression::free_exprnode(expr_node_t *expr)
{
	if(!expr) return;
	if(expr->op == OPERATOR_NONE)
	{
		switch(expr->term_type)
		{
			case TERM_STRING:
				free(expr->val_s);
				break;
			case TERM_COLUMN_REF:
				free(expr->column_ref->table);
				free(expr->column_ref->column);
				free(expr->column_ref);
				break;
			case TERM_LITERAL_LIST:
				free_linked_list<expr_node_t>(
					expr->literal_list,
					expression::free_exprnode
				);
				break;
			default:
				break;
		}
	} else {
		free_exprnode(expr->left);
		free_exprnode(expr->right);
	}

	free(expr);
}

void free_column_ref(column_ref_t *cref)
{
	if(!cref) return;
	free(cref->column);
	free(cref->table);
	free(cref);
}

bool fill_table_header(table_header_t *header, const table_def_t *table);

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

    // https://www.cnblogs.com/haippy/p/3252041.html
    std::condition_variable_any condition; 

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
  dbms::get_instance()->close_database();
  printf("exit successful!");
  exit(0);
}

int main(int argc, char *argv[])
{

  printf("WELCOME TO TINY DB!\n");
  signal(SIGINT, free_all);
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

  // 创建数据目录
  std::string dataDir =  SystemAbstractions::File::GetExeParentDirectory() + "/data";
  SystemAbstractions::File::CreateDirectory(dataDir);
  std::vector< uint8_t > OkPacket = {7, 0, 0, 2, 0, 0, 0, 2, 0, 0, 0};
  // 客户端认证包
  Protocol::AuthPacket ap;
  while (1) {
    // TCP
    if(owner.AwaitStreamPacket()){
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
            uint8_t cmdType = *it;
            it ++;
            // cmd param
            size_t SIZE = iter->payload.size();
            std::string cmdParam(it, iter->payload.end());
            cmdParam.push_back(';');
            std::cout << "cmd param: " << cmdParam << std::endl;
            // parser packet
            /**
             *  不支持的语句
             */
            if(cmdParam != "select @@version_comment limit 1;" && cmdParam != "SELECT DATABASE();" && cmdParam != "show databases;" && cmdParam != "show tables;") {
                switch (cmdType)
                {
                case 2:
                    // switch db
                    run_parser(std::string("USE " + cmdParam).c_str());
                    switch (result.type)
                    {
                    case SQL_USE_DATABASE: {
                        printf("execute_use_database\n");
                        std::string dbName((char*)result.param);
                        std::cout << dbName << std::endl;
                        dbms::get_instance()->switch_database(dbName.c_str(), iter->connection);
                        result.type = SQL_RESET;
                        free((char*)result.param);
                    } break;
                    default:
                        // return
                        OkPacket[3] = iter->payload[3] + 1;
                        iter->connection->SendMessage(OkPacket);
                        result.type = SQL_RESET;
                        break;
                    }
                    break;
                case 3:
                    // query
                    {
                        run_parser(cmdParam.c_str());
                        switch (result.type)
                        {
                        case SQL_CREATE_DATABASE: {
                            printf("execute_create_database\t");
                            std::string dbName((char*)result.param);
                            std::cout << dbName << std::endl;
                            dbms::get_instance()->create_database(dbName.c_str(), iter->connection);
                            result.type = SQL_RESET;
                            free((char*)result.param);
                        } break;
                        case SQL_DROP_DATABASE: {
                            printf("execute_drop_database\t");
                            std::string dbName((char*)result.param);
                            std::cout << dbName << std::endl;
                            dbms::get_instance()->drop_database(dbName.c_str(), iter->connection);
                            free((char*)result.param);
                        } break;
                        case SQL_SHOW_DATABASE: {
                            printf("execute_show_database\n");
                            std::string dbName((char*)result.param);
                            std::cout << dbName << std::endl;
                            dbms::get_instance()->show_database(dbName.c_str(), iter->connection);
                            result.type = SQL_RESET;
                            free((char*)result.param);                            
                        } break;
                        case SQL_CREATE_TABLE: {
                            printf("execute_create_table\n");
                            table_def_t *table = (table_def_t*)result.param;
                            table_header_t *header = new table_header_t;
                            std::cout << "T name:" << table->name << std::endl;
                            if(fill_table_header(header, table)) {
                                dbms::get_instance()->create_table(header, iter->connection);
                            } else {
                                printf("[Error] Fail to create table!\t");
                            }
                            // free resources
                            delete header;
                            free(table->name);
                            free_linked_list<table_constraint_t>(table->constraints, [](table_constraint_t *data) {
                            	expression::free_exprnode(data->check_cond);
                            	free_column_ref(data->column_ref);
                            	free_column_ref(data->foreign_column_ref);
                            	free(data);
                            } );
                            for(field_item_t *it = table->fields; it; )
                            {
                            	field_item_t *tmp = it;
                            	free(it->name);
                            	expression::free_exprnode(it->default_value);
                            	it = it->next;
                            	free(tmp);
                            }
                            free((void*)table);
                            result.type = SQL_RESET;
                        } break;
                        case SQL_INSERT: {
                            printf("execute_insert\n");
                            insert_info_t *info = (insert_info_t*)result.param;
                            dbms::get_instance()->insert_rows(info, iter->connection);
                            // free resources
                            free(info->table);
                            free_linked_list<column_ref_t>(info->columns, free_column_ref);
                            free_linked_list<linked_list_t>(info->values, [](linked_list_t *expr_list) {
                                free_linked_list<expr_node_t>(expr_list, expression::free_exprnode);
                            } );
                            free((void*)info);
                            result.type = SQL_RESET;
                        } break;
                        case SQL_SELECT: {
                            printf("execute_select\n");
                            select_info_t *select_info = (select_info_t*)result.param;
                            dbms::get_instance()->select_rows(select_info, iter->connection);
                            expression::free_exprnode(select_info->where);
                            free_linked_list<expr_node_t>(select_info->exprs, expression::free_exprnode);
                            free_linked_list<table_join_info_t>(select_info->tables, [](table_join_info_t *data) {
                                free(data->table);
                                if(data->join_table)
                                    free(data->join_table);
                                if(data->alias)
                                    free(data->alias);
                                expression::free_exprnode(data->cond);
                                free(data);
                            });	
                            free((void*)select_info);
                            result.type = SQL_RESET;
                        } break;
                        case SQL_UPDATE: {
                            printf("execute_update\n");
                            update_info_t *update_info = (update_info_t*)result.param;
                            dbms::get_instance()->update_rows(update_info, iter->connection);
                            free(update_info->table);
                            free_column_ref(update_info->column_ref);
                            expression::free_exprnode(update_info->where);
                            expression::free_exprnode(update_info->value);
                            free((void*)update_info);
                            result.type = SQL_RESET;
                        } break;
                        case SQL_DELETE: {
                            printf("execute_delete\n");
                            delete_info_t *delete_info = (delete_info_t*)result.param;
                            dbms::get_instance()->delete_rows(delete_info, iter->connection);
                            free(delete_info->table);
                            expression::free_exprnode(delete_info->where);
                            free((void*)delete_info); 
                            result.type = SQL_RESET;
                        } break;
                        case SQL_CREATE_INDEX: {
                            printf("execute_create_index\n");
                            std::string tAc((char*)result.param);
                            std::cout << tAc << std::endl;
                            std::vector< std::string > items = CppStringPlus::Split(tAc, '$');
                            std::string table_name = items.front();
                            items.erase(items.begin());
                            std::string col_name = items.front();
                            dbms::get_instance()->create_index(table_name.c_str(), col_name.c_str(), iter->connection);
                            result.type = SQL_RESET;
                        } break;
                        default:
                            // return
                            OkPacket[3] = iter->payload[3] + 1;
                            iter->connection->SendMessage(OkPacket);
                            result.type = SQL_RESET;
                        } break;
                    }
                    break;     
                default:
                    break;
                }
            } else {
                    OkPacket[3] = iter->payload[3] + 1;
                    iter->connection->SendMessage(OkPacket);
            }
            std::cout << std::endl;
            std::cout << std::flush  << std::endl;
          }
      }
      owner.streamPacket.clear();
    }
  }
  return 0;
}
