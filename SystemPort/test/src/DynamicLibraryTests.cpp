/**
 * @file DynamicLibraryTests.cpp
 *
 * This module contains the unit tests of the
 * SystemAbstractions::DynamicLibrary class.
 *
 * © 2020 by LiuJ
 */

#include <gtest/gtest.h>
#include <SystemPort/DynamicLibrary.hpp>
#include <SystemPort/File.hpp>

TEST(DynamicLibraryTests, LoadAndGetProcedure) {
    SystemAbstractions::DynamicLibrary lib;
    ASSERT_TRUE(
        lib.Load(
            SystemAbstractions::File::GetExeParentDirectory(),
            "MockDynamicLibrary"
        )
    );
    auto procedureAddress = lib.GetProcedure("Foo");
    ASSERT_FALSE(procedureAddress == nullptr);
    int(*procedure)(int) = (int(*)(int))procedureAddress;
    ASSERT_EQ(49, procedure(7));
}

TEST(DynamicLibraryTests, Unload) {
    SystemAbstractions::DynamicLibrary lib;
    ASSERT_TRUE(
        lib.Load(
            SystemAbstractions::File::GetExeParentDirectory(),
            "MockDynamicLibrary"
        )
    );
    auto procedureAddress = lib.GetProcedure("Foo");
    ASSERT_FALSE(procedureAddress == nullptr);
    int(*procedure)(int) = (int(*)(int))procedureAddress;
    lib.Unload();
    ASSERT_DEATH(procedure(7), "");
}
