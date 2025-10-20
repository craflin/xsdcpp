
#include "../src/Reader.hpp"
#include "../src/Generator.hpp"

#include <nstd/Directory.hpp>

#include <gtest/gtest.h>

TEST(Generator, generateCpp)
{
    {
        String inputFile = FOLDER "/ecic/ED247A_ECIC.xsd";
        String error;
        Xsd xsd;
        EXPECT_TRUE(Directory::create("test_temp"));
        EXPECT_TRUE(readXsd(String(), inputFile, List<String>(), xsd, error));
        EXPECT_TRUE(generateCpp(xsd, "test_temp", List<String>(), List<String>(), error));
    }
    {
        String inputFile = FOLDER "/SubstitutionGroup.xsd";
        String error;
        Xsd xsd;
        EXPECT_TRUE(Directory::create("test_temp"));
        EXPECT_TRUE(readXsd(String(), inputFile, List<String>(), xsd, error));
        EXPECT_TRUE(generateCpp(xsd, "test_temp", List<String>(), List<String>(), error));
    }
}
