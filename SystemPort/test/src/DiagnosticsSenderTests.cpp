/**
 * @file DiagnosticsSenderTests.cpp
 *
 * This module contains the unit tests of the
 * SystemAbstractions::DiagnosticsSender class.
 *
 * © 2020 by LiuJ
 */

#include <gtest/gtest.h>
#include <string>
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

TEST(DiagnosticsSenderTests, BasicSubscriptionAndTransmission) {
    SystemAbstractions::DiagnosticsSender sender("Joe");
    sender.SendDiagnosticInformationString(100, "Very important message nobody will hear; FeeksBadMan");
    std::vector< ReceivedMessage > receivedMessages;
    const auto unsubscribeDelegate = sender.SubscribeToDiagnostics(
        [&receivedMessages](
            std::string senderName,
            size_t level,
            std::string message
        ){
            receivedMessages.emplace_back(
                senderName,
                level,
                message
            );
        },
        5
    );
    ASSERT_EQ(5, sender.GetMinLevel());
    ASSERT_TRUE(unsubscribeDelegate != nullptr);
    sender.SendDiagnosticInformationString(10, "PogChamp");
    sender.SendDiagnosticInformationString(3, "Did you hear that?");
    sender.PushContext("spam");
    sender.SendDiagnosticInformationString(4, "Level 4 whisper...");
    sender.SendDiagnosticInformationString(5, "Level 5, can you dig it?");
    sender.PopContext();
    sender.SendDiagnosticInformationString(6, "Level 6 FOR THE WIN");
    unsubscribeDelegate();
    sender.SendDiagnosticInformationString(5, "Are you still there?");
    ASSERT_EQ(
        receivedMessages,
        (std::vector< ReceivedMessage >{
            { "Joe", 10, "PogChamp" },
            { "Joe", 5, "spam: Level 5, can you dig it?" },
            { "Joe", 6, "Level 6 FOR THE WIN" },
        })
    );
}

TEST(DiagnosticsSenderTests, FormattedMessage) {
    SystemAbstractions::DiagnosticsSender sender("Joe");
    std::vector< ReceivedMessage > receivedMessages;
    (void)sender.SubscribeToDiagnostics(
        [&receivedMessages](
            std::string senderName,
            size_t level,
            std::string message
        ){
            receivedMessages.emplace_back(
                senderName,
                level,
                message
            );
        }
    );
    sender.SendDiagnosticInformationFormatted(0, "The answer is %d.", 42);
    ASSERT_EQ(
        receivedMessages,
        (std::vector< ReceivedMessage >{
            { "Joe", 0, "The answer is 42." },
        })
    );
}

TEST(DiagnosticsSenderTests, Chaining) {
    SystemAbstractions::DiagnosticsSender outer("outer");
    SystemAbstractions::DiagnosticsSender inner("inner");
    std::vector< ReceivedMessage > receivedMessages;
    (void)outer.SubscribeToDiagnostics(
        [&receivedMessages](
            std::string senderName,
            size_t level,
            std::string message
        ){
            receivedMessages.emplace_back(
                senderName,
                level,
                message
            );
        }
    );
    (void)inner.SubscribeToDiagnostics(outer.Chain());
    inner.SendDiagnosticInformationFormatted(0, "The answer is %d.", 42);
    ASSERT_EQ(
        receivedMessages,
        (std::vector< ReceivedMessage >{
            { "outer", 0, "inner: The answer is 42." },
        })
    );
}

TEST(DiagnosticsSenderTests, UnsubscribeAfterSenderDestroyed) {
    std::unique_ptr< SystemAbstractions::DiagnosticsSender > sender(new SystemAbstractions::DiagnosticsSender("sender"));
    std::vector< ReceivedMessage > receivedMessages;
    const auto unsubscribe = sender->SubscribeToDiagnostics(
        [&receivedMessages](
            std::string senderName,
            size_t level,
            std::string message
        ){
            receivedMessages.emplace_back(
                senderName,
                level,
                message
            );
        }
    );
    sender.reset();
    unsubscribe();
}


TEST(DiagnosticsSenderTests, PublishAfterChainedSenderDestroyed) {
    std::unique_ptr< SystemAbstractions::DiagnosticsSender > outer(new SystemAbstractions::DiagnosticsSender("outer"));
    SystemAbstractions::DiagnosticsSender inner("inner");
    std::vector< ReceivedMessage > receivedMessages;
    (void)outer->SubscribeToDiagnostics(
        [&receivedMessages](
            std::string senderName,
            size_t level,
            std::string message
        ){
            receivedMessages.emplace_back(
                senderName,
                level,
                message
            );
        }
    );
    (void)inner.SubscribeToDiagnostics(outer->Chain());
    outer.reset();
    inner.SendDiagnosticInformationFormatted(0, "The answer is %d.", 42);
    ASSERT_EQ(
        receivedMessages,
        (std::vector< ReceivedMessage >{
        })
    );
}