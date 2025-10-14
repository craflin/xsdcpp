
#include "../src/xsd.hpp"

#include <gtest/gtest.h>

TEST(XsdLib, optional_base)
{
    xsd::optional<int32_t> i32;
    EXPECT_FALSE(i32);
    i32 = (int32_t)23;

}

TEST(XsdLib, optional)
{
    struct A
    {
        int a;
        int b;
        int c;
    };

    xsd::optional<A> a;
    EXPECT_FALSE(a);
    a = A{1,23};
}

TEST(XsdLib, base)
{
    xsd::base<int32_t> a(23);
    EXPECT_EQ(a, 23);
}
