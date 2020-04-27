/**
 * @file DiagnosticsContextTests.cpp
 *
 * This module contains the unit tests of the
 * SystemAbstractions::DiagnosticsContext class.
 *
 * © 2020 by LiuJ
 */

#include <gtest/gtest.h>
#include <SystemPort/DiagnosticsContext.hpp>
#include <SystemPort/DiagnosticsSender.hpp>
#include <vector>

struct ReceivedMessage {

    std::string senderName;

    size_t level;

    std::string message;

    ReceivedMessage(
        std::string newSenderName,
        size_t newLevel,
        std::string newMessage
    ) 
        : senderName(newSenderName)
        , level(newLevel)
        , message(newMessage)
    {
    }

    // 重载对象比较运算符
    bool operator==(const ReceivedMessage& other) const noexcept {
        return (
            (senderName == other.senderName)
            && (level == other.level)
            && (message == other.message)
        );
    };

};

TEST(DiagnosticsContextTests, PushAndPopContext) {
    SystemAbstractions::DiagnosticsSender sender("foo");
    std::vector< ReceivedMessage > receviedMessages;
    sender.SubscribeToDiagnostics(
        [&receviedMessages](
            std::string senderName,
            size_t level,
            std::string message
        ) {
            receviedMessages.emplace_back(
                senderName,
                level,
                message
            );
        }
    );
    sender.SendDiagnosticInformationString(0, "hello");
    {
        SystemAbstractions::DiagnosticsContext testContext(sender, "bar");
        sender.SendDiagnosticInformationString(0, "world");
    }
    sender.SendDiagnosticInformationString(0, "last message");
    ASSERT_EQ(
        (std::vector< ReceivedMessage >{
            {"foo", 0, "hello"},
            {"foo", 0, "bar: world"},
            {"foo", 0, "last message"},
        }),
        receviedMessages
    );
}
