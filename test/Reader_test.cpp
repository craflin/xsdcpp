
#include "../src/Reader.hpp"

#include <gtest/gtest.h>

TEST(Reader, readXsd)
{
    {
        String inputFile = FOLDER "/ecic/ED247A_ECIC.xsd";
        String error;
        Xsd xsd;
        EXPECT_TRUE(readXsd(String(), inputFile, xsd, error));
    }
    {
        String inputFile = FOLDER "/SubstitutionGroup.xsd";
        String error;
        Xsd xsd;
        EXPECT_TRUE(readXsd(String(), inputFile, xsd, error));
    }
}
