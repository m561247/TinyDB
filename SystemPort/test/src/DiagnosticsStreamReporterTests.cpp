/**
 * @file DiagnosticsStreamReporterTests.cpp
 *
 * This module contains the unit tests of the
 * SystemAbstractions::DiagnosticsStreamReporter function object generator.
 *
 * © 2020 by LiuJ
 * 
 */

#include <gtest/gtest.h>
#include <stdio.h>
#include <string>
#include <SystemPort/DiagnosticsStreamReporter.hpp>
#include <SystemPort/DiagnosticsSender.hpp>
#include <SystemPort/File.hpp>
#include <vector>

void CheckLogMessage(FILE* f, const std::string& expected) {
    std::vector< char > lineBuffer(256);
    const auto lineCString = fgets(lineBuffer.data(), (int)lineBuffer.size(), f);
    ASSERT_FALSE(lineCString == NULL);
    const std::string lineCppString(lineCString);
    ASSERT_GE(lineCppString.length(), 2);
    ASSERT_EQ('[', lineCppString[0]);
    const auto firstSpace = lineCppString.find(' ');
    ASSERT_FALSE(firstSpace == std::string::npos);
    ASSERT_EQ(expected, lineCppString.substr(firstSpace + 1));
}

void CheckIsEndOfFile(FILE* f) {
    ASSERT_EQ(EOF, fgetc(f));
}

struct DiagnosticsStreamReporterTests
: public ::testing::Test
{
    std::string testAreaPath;

    virtual void SetUp() {
        testAreaPath = SystemAbstractions::File::GetExeParentDirectory() + "/TestArea";
        ASSERT_TRUE(SystemAbstractions::File::CreateDirectory(testAreaPath));
    }

    virtual void TearDown() {
        ASSERT_TRUE(SystemAbstractions::File::DeleteDirectory(testAreaPath));
    }

};

TEST_F(DiagnosticsStreamReporterTests, SaveDiagnosticMessagesToLogFile) {
    SystemAbstractions::DiagnosticsSender sender("foo");
    auto output = fopen((testAreaPath + "/out.txt").c_str(), "wt");
    auto error = fopen((testAreaPath + "/error.txt").c_str(), "wt");
    const auto unsubscriptionDelegate = sender.SubscribeToDiagnostics(
        SystemAbstractions::DiagnosticsStreamReporter(output, error)
    );

    sender.SendDiagnosticInformationString(0, "hello");
    sender.SendDiagnosticInformationString(10, "world");
    sender.SendDiagnosticInformationString(2, "last message");
    sender.SendDiagnosticInformationString(5, "be careful");
    unsubscriptionDelegate();
    sender.SendDiagnosticInformationString(0, "really the last message");
    (void)fclose(output);
    (void)fclose(error);
    output = fopen((testAreaPath + "/out.txt").c_str(), "rt");
    CheckLogMessage(output, "foo:0] hello\n");
    CheckLogMessage(output, "foo:2] last message\n");
    CheckIsEndOfFile(output);
    (void)fclose(output);
    error = fopen((testAreaPath + "/error.txt").c_str(), "rt");
    CheckLogMessage(error, "foo:10] error: world\n");
    CheckLogMessage(error, "foo:5] warning: be careful\n");
    CheckIsEndOfFile(error);
    (void)fclose(error);
}