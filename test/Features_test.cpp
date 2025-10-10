

#include "SubstitutionGroup.hpp"
#include "Choice.hpp"
#include "Recursion.hpp"

#include <gtest/gtest.h>

TEST(Features, SubstitutionGroup)
{
    SubstitutionGroup::Main main;
    SubstitutionGroup::load_data(R"(<?xml version="1.0" encoding="UTF-8"?>
<Main>
  <BooleanProperty name="a" value="true"/>
  <FloatingPointProperty name="b" value="1.0"/>
  <BooleanProperty name="c" value="false"/>
</Main>)", main);

    EXPECT_EQ(main.Property.size(), 3);

    EXPECT_TRUE(main.Property[0].BooleanProperty);
    EXPECT_FALSE(main.Property[0].FloatingPointProperty);
    EXPECT_EQ(main.Property[0].BooleanProperty->name, "a");
    EXPECT_EQ(main.Property[0].BooleanProperty->value, true);

    EXPECT_TRUE(main.Property[1].FloatingPointProperty);
    EXPECT_FALSE(main.Property[1].BooleanProperty);
    EXPECT_EQ(main.Property[1].FloatingPointProperty->name, "b");
    EXPECT_EQ(main.Property[1].FloatingPointProperty->value, 1.0);

    EXPECT_TRUE(main.Property[2].BooleanProperty);
    EXPECT_FALSE(main.Property[2].FloatingPointProperty);
    EXPECT_EQ(main.Property[2].BooleanProperty->name, "c");
    EXPECT_EQ(main.Property[2].BooleanProperty->value, false);
}

TEST(Features, Choice)
{
    Choice::MainType main;
    Choice::load_data(R"(<?xml version="1.0" encoding="UTF-8"?>
<Main attr="sdas">
  <ChoiceA name="a" value="xyz"/>
  <ChoiceB name="b" value="123"/>
  <ChoiceA name="c" value="457"/>
</Main>)", main);

    EXPECT_EQ(main.ChoiceA.size(), 2);
    EXPECT_EQ(main.ChoiceB.size(), 1);
    EXPECT_EQ(main.ChoiceA[0].name, "a");
    EXPECT_EQ(main.ChoiceA[1].name, "c");
    EXPECT_EQ(main.ChoiceB[0].name, "b");
}

TEST(Features, Recursion)
{
    {
        Recursion::MainType1 main1;
        Recursion::load_data(R"(<?xml version="1.0" encoding="UTF-8"?>
    <Main1 channel="sdas">
      <SubMain channel="xyz">
        <SubMain channel="xsd"/>
      </SubMain>
    </Main1>)", main1);

        EXPECT_EQ(main1.SubMain.size(), 1);
        EXPECT_EQ(main1.SubMain[0].SubMain.size(), 1);
    }

    {
        Recursion::MainType2 main2;
        Recursion::load_data(R"(<?xml version="1.0" encoding="UTF-8"?>
    <Main2 channel="sdas">
      <SubMain channel="xyz">
        <SubMain channel="xsd"/>
      </SubMain>
    </Main2>)", main2);

        EXPECT_TRUE(main2.SubMain);
        EXPECT_TRUE(main2.SubMain->SubMain);
    }
}
