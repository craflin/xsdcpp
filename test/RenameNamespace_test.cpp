
#include "domain.hpp"

#include <gtest/gtest.h>

// Test wrap namespace + inner namespace rename options (-w -n)
// Types should be in flatns::domain namespace
TEST(RenameNamespace, LoadData)
{
    flatns::domain::List list;
    flatns::domain::load_data(R"(<?xml version="1.0" encoding="UTF-8"?>
<List>
    <Person>
        <Name age="25">Jane Doe</Name>
        <Country>DE</Country>
    </Person>
</List>)", list);

    EXPECT_EQ(list.Person.size(), 1);
    EXPECT_EQ((std::string)list.Person[0].Name, "Jane Doe");
    EXPECT_EQ(list.Person[0].Name.age, 25);
    EXPECT_TRUE(list.Person[0].Country);
    EXPECT_EQ(*list.Person[0].Country, flatns::domain::CountryCode::DE);
}
