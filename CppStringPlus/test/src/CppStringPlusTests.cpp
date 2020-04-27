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
#include <CppStringPlus/CppStringPlus.hpp>
#include <limits>

TEST(CppStringPlusTests, ToLower) {
    EXPECT_EQ("hello", CppStringPlus::ToLower("Hello"));
    EXPECT_EQ("hello", CppStringPlus::ToLower("hello"));
    EXPECT_EQ("hello", CppStringPlus::ToLower("heLLo"));
    EXPECT_EQ("example", CppStringPlus::ToLower("eXAmplE"));
    EXPECT_EQ("example", CppStringPlus::ToLower("example"));
    EXPECT_EQ("example", CppStringPlus::ToLower("EXAMPLE"));
    EXPECT_EQ("foo1bar", CppStringPlus::ToLower("foo1BAR"));
    EXPECT_EQ("foo1bar", CppStringPlus::ToLower("fOo1bAr"));
    EXPECT_EQ("foo1bar", CppStringPlus::ToLower("foo1bar"));
    EXPECT_EQ("foo1bar", CppStringPlus::ToLower("FOO1BAR"));
}

TEST(CppStringPlusTests, ToInteger) {
    struct TestVector {
        std::string input;
        intmax_t output;
        CppStringPlus::ToIntegerResult expectedResult;
    };
    const auto maxAsString = CppStringPlus::sprintf("%" PRIdMAX, std::numeric_limits< intmax_t >::max());
    const auto minAsString = CppStringPlus::sprintf("%" PRIdMAX, std::numeric_limits< intmax_t >::lowest());
    auto maxPlusOneAsString = maxAsString;
    size_t digit = maxPlusOneAsString.length();
    while (digit > 0) {
        if (maxPlusOneAsString[digit-1] == '9') {
            maxPlusOneAsString[digit-1] = '0';
            --digit;
        } else {
            ++maxPlusOneAsString[digit-1];
            break;
        }
    }
    if (digit == 0) {
        maxPlusOneAsString.insert(maxPlusOneAsString.begin(), '1');
    }
    auto minMinusOneAsString = minAsString;
    digit = minMinusOneAsString.length();
    while (digit > 1) {
        if (minMinusOneAsString[digit-1] == '9') {
            minMinusOneAsString[digit-1] = '0';
            --digit;
        } else {
            ++minMinusOneAsString[digit-1];
            break;
        }
    }
    if (digit == 1) {
        minMinusOneAsString.insert(maxPlusOneAsString.begin() + 1, '1');
    }
    const std::vector< TestVector > testVectors{
        {"0", 0, CppStringPlus::ToIntegerResult::Success},
        {"42", 42, CppStringPlus::ToIntegerResult::Success},
        {"-42", -42, CppStringPlus::ToIntegerResult::Success},
        {
            maxAsString,
            std::numeric_limits< intmax_t >::max(),
            CppStringPlus::ToIntegerResult::Success
        },
        {
            minAsString,
            std::numeric_limits< intmax_t >::lowest(),
            CppStringPlus::ToIntegerResult::Success
        },
        {
            maxPlusOneAsString,
            0,
            CppStringPlus::ToIntegerResult::Overflow
        },
        {
            minMinusOneAsString,
            0,
            CppStringPlus::ToIntegerResult::Overflow
        },
    };
    for (const auto& testVector: testVectors) {
        intmax_t output;
        EXPECT_EQ(
            testVector.expectedResult,
            CppStringPlus::ToInteger(
                testVector.input,
                output
            )
        );
        if (testVector.expectedResult == CppStringPlus::ToIntegerResult::Success) {
            EXPECT_EQ(
                output,
                testVector.output
            );
        }
    }
}