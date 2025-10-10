
#include "../src/Reader.hpp"
#include "../src/Generator.hpp"

#include <nstd/Directory.hpp>

#include <gtest/gtest.h>

TEST(Generator, generateCpp)
{/*
    {
        String inputFile = FOLDER "/ED247A_ECIC.xsd";
        String error;
        Xsd xsd;
        EXPECT_TRUE(Directory::create("test_temp"));
        EXPECT_TRUE(readXsd(String(), inputFile, xsd, error));
        EXPECT_TRUE(generateCpp(xsd, "test_temp", error));
    }
    */
    {
        String inputFile = FOLDER "/SubstitutionGroup.xsd";
        String error;
        Xsd xsd;
        EXPECT_TRUE(Directory::create("test_temp"));
        EXPECT_TRUE(readXsd(String(), inputFile, xsd, error));
        EXPECT_TRUE(generateCpp(xsd, "test_temp", error));
    }
}
