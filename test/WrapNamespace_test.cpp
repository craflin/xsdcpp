
#include "Example.hpp"

#include <gtest/gtest.h>

// Test wrap namespace option (-w)
// Types should be in testns::inner::Example namespace
TEST(WrapNamespace, LoadData)
{
    testns::inner::Example::List list;
    testns::inner::Example::load_data(R"(<?xml version="1.0" encoding="UTF-8"?>
<List>
    <Person>
        <Name age="40">John Smith</Name>
        <Country>UK</Country>
    </Person>
</List>)", list);

    EXPECT_EQ(list.Person.size(), 1);
    EXPECT_EQ((std::string)list.Person[0].Name, "John Smith");
    EXPECT_EQ(list.Person[0].Name.age, 40);
    EXPECT_TRUE(list.Person[0].Country);
    EXPECT_EQ(*list.Person[0].Country, testns::inner::Example::CountryCode::UK);
}
