

#include "SubstitutionGroup.hpp"
#include "Choice.hpp"
#include "Recursion.hpp"
#include "SimpleTypeExtension.hpp"
#include "Import.hpp"
#include "Attributes.hpp"
#include "Occurrence.hpp"

#include <gtest/gtest.h>

TEST(Features, Enums)
{
    EXPECT_EQ(SimpleTypeExtension::XEnumZZ::A, (SimpleTypeExtension::XEnumZZ)0);
    EXPECT_EQ(SimpleTypeExtension::XEnumZZ::B, (SimpleTypeExtension::XEnumZZ)1);
    EXPECT_EQ(SimpleTypeExtension::XEnumZZ::C, (SimpleTypeExtension::XEnumZZ)2);
    EXPECT_EQ(to_string(SimpleTypeExtension::XEnumZZ::A), "A");
    EXPECT_EQ(to_string(SimpleTypeExtension::XEnumZZ::B), "B");
    EXPECT_EQ(to_string(SimpleTypeExtension::XEnumZZ::C), "C");
}

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

TEST(Features, SimpleTypeExtension)
{
    {
        SimpleTypeExtension::MainEnum main;
        SimpleTypeExtension::load_data(R"(<?xml version="1.0" encoding="UTF-8"?>
    <MainEnum>B</MainEnum>)", main);
        EXPECT_EQ(main, SimpleTypeExtension::XEnumZZ::B);
    }

    {
        SimpleTypeExtension::MainInt main;
        SimpleTypeExtension::load_data(R"(<?xml version="1.0" encoding="UTF-8"?>
    <MainInt>42</MainInt>)", main);
        EXPECT_EQ(main, 42);
    }

    {
        SimpleTypeExtension::MainStr main;
        SimpleTypeExtension::load_data(R"(<?xml version="1.0" encoding="UTF-8"?>
    <MainStr>hello  space</MainStr>)", main);
        EXPECT_EQ(main, "hello  space");
    }

    {
        SimpleTypeExtension::MainEnumList main;
        SimpleTypeExtension::load_data(R"(<?xml version="1.0" encoding="UTF-8"?>
    <MainEnumList>C   B A</MainEnumList>)", main);
        EXPECT_EQ(main.size(), 3);
        EXPECT_EQ(main[0], SimpleTypeExtension::MyList_item_t::C);
        EXPECT_EQ(main[1], SimpleTypeExtension::MyList_item_t::B);
        EXPECT_EQ(main[2], SimpleTypeExtension::MyList_item_t::A);
    }

    {
        SimpleTypeExtension::MainWithListElement main;
        SimpleTypeExtension::load_data(R"(<?xml version="1.0" encoding="UTF-8"?>
    <MainWithListElement><MyList>C   B A</MyList></MainWithListElement>)", main);
        EXPECT_EQ(main.MyList[0].size(), 3);
        EXPECT_EQ(main.MyList[0][0], SimpleTypeExtension::MyList_item_t::C);
        EXPECT_EQ(main.MyList[0][1], SimpleTypeExtension::MyList_item_t::B);
        EXPECT_EQ(main.MyList[0][2], SimpleTypeExtension::MyList_item_t::A);
    }
}

TEST(Features, Import)
{
    Import::MainType1 main;
    Import::load_data(R"(<?xml version="1.0" encoding="UTF-8"?>
    <Main1 myList="C   B A" entity="test">B<MainInt>42</MainInt><Version>2.0</Version></Main1>)", main);
    EXPECT_EQ(main, SimpleTypeExtension::XEnumZZ::B);
    EXPECT_EQ(main.entity, "test");
    EXPECT_EQ(main.myList.size(), 3);
    EXPECT_EQ(main.myList[0], SimpleTypeExtension::MyList_item_t::C);
    EXPECT_EQ(main.myList[1], SimpleTypeExtension::MyList_item_t::B);
    EXPECT_EQ(main.myList[2], SimpleTypeExtension::MyList_item_t::A);
    EXPECT_EQ(main.MainInt, 42);
    EXPECT_EQ(main.Version, "2.0");
}

TEST(Features, Attribute_DefaultValue)
{
    Attributes::MainType1 main;
    Attributes::load_data(R"(<?xml version="1.0" encoding="UTF-8"?>
    <Main1 required="test"/>)", main);
    EXPECT_EQ(main.required, "test");
    EXPECT_EQ(main.optional_with_default, "No");
    EXPECT_FALSE(main.optional_without_default);
    EXPECT_FALSE(main.optional_without_default_list);
}

TEST(Features, Attribute_ListType)
{
    Attributes::MainType1 main;
    Attributes::load_data(R"(<?xml version="1.0" encoding="UTF-8"?>
    <Main1 required="test" optional_without_default="abc" optional_without_default_list="item1  item2 item3"/>)", main);
    EXPECT_EQ(main.required, "test");
    EXPECT_EQ(main.optional_with_default, "No");
    EXPECT_EQ(main.optional_without_default, "abc");
    EXPECT_TRUE(main.optional_without_default_list);
    EXPECT_EQ(main.optional_without_default_list->size(), 3);
    EXPECT_EQ(main.optional_without_default_list->at(0), "item1");
    EXPECT_EQ(main.optional_without_default_list->at(1), "item2");
    EXPECT_EQ(main.optional_without_default_list->at(2), "item3");
}

TEST(Features, Attribute_Optional)
{
    {
        Attributes::MainType1 main;
        Attributes::load_data(R"(<?xml version="1.0" encoding="UTF-8"?>
        <Main1 required="test"/>)", main);
        EXPECT_EQ(main.required, "test");
        EXPECT_EQ(main.optional_with_default, "No");
        EXPECT_FALSE(main.optional_without_default);
        EXPECT_FALSE(main.optional_without_default_list);
    }
    {
        Attributes::MainType1 main;
        Attributes::load_data(R"(<?xml version="1.0" encoding="UTF-8"?>
        <Main1 required="test" optional_without_default="abc"/>)", main);
        EXPECT_EQ(main.required, "test");
        EXPECT_EQ(main.optional_with_default, "No");
        EXPECT_EQ(main.optional_without_default, "abc");
        EXPECT_FALSE(main.optional_without_default_list);
    }
}

TEST(Features, Attribute_Duplicated)
{
    Attributes::MainType1 main;
    try
    {
        Attributes::load_data(R"(<?xml version="1.0" encoding="UTF-8"?>
<Main1 required="test" required="test"/>)", main);
        FAIL();
    }
    catch(const std::exception& e)
    {
        EXPECT_EQ(std::string(e.what()), "Error at line '2': Repeated attribute 'required'");
    }
}

TEST(Features, Attribute_Missing)
{
    Attributes::MainType1 main;
    try
    {
        Attributes::load_data(R"(<?xml version="1.0" encoding="UTF-8"?>
<Main1/>)", main);
        FAIL();
    }
    catch(const std::exception& e)
    {
        EXPECT_EQ(std::string(e.what()), "Error at line '2': Missing attribute 'required'");
    }
}


TEST(Features, Attribute_Unexpected)
{
    Attributes::MainType1 main;
    try
    {
        Attributes::load_data(R"(<?xml version="1.0" encoding="UTF-8"?>
<Main1 not_defined_in_xsd="test"/>)", main);
        FAIL();
    }
    catch(const std::exception& e)
    {
        EXPECT_EQ(std::string(e.what()), "Error at line '2': Unexpected attribute 'not_defined_in_xsd'");
    }
}

TEST(Features, Element_Occurrence)
{
    {
        Occurrence::Main main;
        Occurrence::load_data(R"(<?xml version="1.0" encoding="UTF-8"?>
    <Main><MyElement name="name1"/></Main>)", main);
        EXPECT_EQ(main.MyElement.size(), 1);
    }
    {
        Occurrence::Main main;
        Occurrence::load_data(R"(<?xml version="1.0" encoding="UTF-8"?>
    <Main><MyElement name="name1"/><MyElement name="name2"/><MyElement name="name3"/></Main>)", main);
        EXPECT_EQ(main.MyElement.size(), 3);
    }
    {
        try
        {
            Occurrence::Main main;
            Occurrence::load_data(R"(<?xml version="1.0" encoding="UTF-8"?>
        <Main></Main>)", main);
            EXPECT_EQ(main.MyElement.size(), 1);
        }
        catch(const std::exception& e)
        {
            EXPECT_EQ(std::string(e.what()), "Error at line '2': Minimum occurrence of element 'MyElement' is 1");
        }
    }
    {
        try
        {
            Occurrence::Main main;
            Occurrence::load_data(R"(<?xml version="1.0" encoding="UTF-8"?>
    <Main><MyElement name="name1"/><MyElement name="name2"/><MyElement name="name3"/><MyElement name="name4"/></Main>)", main);
            EXPECT_EQ(main.MyElement.size(), 1);
        }
        catch(const std::exception& e)
        {
            std::string xx(e.what());
            EXPECT_EQ(std::string(e.what()), "Error at line '2': Maximum occurrence of element 'MyElement' is 3");
        }
    }

}

// todo:

// Int Attribute out of range
// Invalid Enum Attribute
// Attribute not matching pattern