/**
 * @file CppStringPlusTests.cpp
 *  
 * 这个文件中包含了对 string 扩展库的测试用例
 *
 * © 2018-2019 by LiuJ
 */

#include <gtest/gtest.h>
#include <inttypes.h>
#include <stdint.h>
#include <vector>
#include <stddef.h>
#include <string>
#include <limits>
#include <protocol/Greeting.hpp>
#include <protocol/Auth.hpp>
#include <protocol/Eof.hpp>
#include <protocol/Field.hpp>
#include <protocol/Common.hpp>
#include <protocol/Ok.hpp>

TEST(ProcotolTests, PackGreetingPack) {
    Protocol::GreetingPacket gp(1, "TinyDB-v0.0.1");
    std::vector< uint8_t > outputPacket = gp.Pack();
    for(auto packet : outputPacket) {
        std::cout << std::to_string(packet) <<  ", ";
    }
    // on example
    // 10, 70, 97, 107, 101, 68, 66, 0, 1, 0, 0, 0, 96, 100, 51, 84, 23, 25, 48, 92, 0, 13, 162, 33, 2, 0, 9, 1, 21, 0, 0, 0, 0, 0, 0, 0, 0, 0, 31, 41, 8, 0, 61, 80, 111, 63, 64, 43, 107, 90, 0, 109, 121, 115, 113, 108, 95, 110, 97, 116, 105, 118, 101, 95, 112, 97, 115, 115, 119, 111, 114, 100, 0
    std::cout << std::endl;
}

TEST(ProcotolTests, PackGreetingUnPack) {
    std::vector< uint8_t > GreetingPacket{10, 70, 97, 107, 101, 68, 66, 0, 1, 0, 0, 0, 33, 17, 13, 38, 7, 117, 99, 0, 0, 13, 162, 33, 2, 0, 9, 1, 21, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 7, 107, 8, 79, 91, 83, 93, 24, 7, 90, 106, 50, 0, 109, 121, 115, 113, 108, 95, 110, 97, 116, 105, 118, 101, 95, 112, 97, 115, 115, 119, 111, 114, 100, 0 };
    Protocol::GreetingPacket gp;
    gp.UnPack(GreetingPacket);
    std::cout << "Server Version: " << gp.GetServerVersion() << std::endl;
    std::cout << "Connection ID: " << gp.GetConnectionID() << std::endl;
    std::cout << "Packet Size: " << GreetingPacket.size() << std::endl;
    std::cout << "AuthPluginName: " << gp.GetAuthPluginName() << std::endl;
    std::cout << "Server Status: " << gp.GetServerStatus() << std::endl;
}

TEST(ProcotolTests, PackAuthUnPack) {
    std::vector< uint8_t > AuthPacket {141, 166, 15, 32, 0, 0, 0, 1, 33, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 109, 111, 99, 107, 0, 0, 116, 101, 115, 116, 0, 109, 121, 115, 113, 108, 95, 110, 97, 116, 105, 118, 101, 95, 112, 97, 115, 115, 119, 111, 114, 100, 0};
    Protocol::AuthPacket ap;
    ap.UnPack(AuthPacket);
    std::cout << "UserName: " << ap.GetUserName() << std::endl;
    std::cout << "PluginName: " << ap.GetPluginName() << std::endl;
    std::cout << "DataBaseName: " << ap.GetDatabaseName() << std::endl;
}

TEST(ProcotolTests, EofPack) {
    Protocol::EofPacket eof(0, 2);
    std::vector< uint8_t > outputPacket = eof.Pack();
    for(auto packet : outputPacket) {
        std::cout << std::to_string(packet) <<  ", ";
    }
    std::cout << std::endl;

    Protocol::EofPacket eofPack;
    std::vector< uint8_t > eofPacket {254, 0, 0, 2, 0};
    eofPack.UnPack(eofPacket);

    std::cout << "GetHeader: " << std::to_string(eofPack.GetHeader()) << std::endl;
    std::cout << "GetStatusFlags: " << eofPack.GetStatusFlags() << std::endl;
    std::cout << "GetWarning: " << eofPack.GetWarning() << std::endl;
}

TEST(ProcotolTests, FieldPack) {
    Protocol::FieldPacket fieldPack;
    std::vector< uint8_t > fieldPacket {3, 100, 101, 102, 2, 100, 98, 7, 80, 101, 114, 115, 111, 110, 115, 7, 80, 101, 114, 115, 111, 110, 115, 8, 76, 97, 115, 116, 78, 97, 109, 101, 8, 76, 97, 115, 116, 78, 97, 109, 101, 12, 46, 0, 80, 0, 0, 0, 253, 0, 0, 0, 0, 0};
    fieldPack.UnPack(fieldPacket);
    std::cout << fieldPack.ToString() << std::endl;

    Protocol::FieldPacket newFieldPack("LastName", static_cast< uint32_t >(TYPE_VARCHAR), "Persons", 
        "Persons", "db", "LastName", 80,
        CHARACTER_SET_UTF8, 0, 0);
    std::vector< uint8_t > outputPacket = newFieldPack.Pack();
    for(auto packet : outputPacket) {
        std::cout << std::to_string(packet) <<  ", ";
    }
}

TEST(ProcotolTests, RowPack) {
    std::vector< std::string > row;
    row.push_back("-238");
    row.push_back("Zhong");
    row.push_back("Lei");
    Protocol::RowPacket rowPack(row);
    std::vector< uint8_t > outputPacket = rowPack.Pack();
    for(auto packet : outputPacket) {
        std::cout << std::to_string(packet) <<  ", ";
    }
}

TEST(ProcotolTests, RowUnPack) {
    Protocol::RowPacket newPack;
    std::vector< uint8_t > rowPacket {4, 45, 50, 51, 56, 5, 90, 104, 111, 110, 103, 3, 76, 101, 105};
    newPack.UnPack(rowPacket, 3);
    std::cout << newPack.ToString() << std::endl;
}

TEST(ProtocolTests, OkPack) {

    Protocol::OkPacket okPack;
    std::vector<uint8_t> outPut = okPack.Pack(10, 0, 2, 0);

    for(auto packet : outPut) {
        std::cout << std::to_string(packet) <<  ", ";
    }
}
