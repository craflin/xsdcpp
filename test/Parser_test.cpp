
#include "../src/XmlParser.cpp"

#include <gtest/gtest.h>

void fixUnusedToTypeWarning()
{
    toType(Position(), nullptr, std::string());
    toType<uint64_t>(Position(), std::string());
    toType<int64_t>(Position(), std::string());
    toType<uint32_t>(Position(), std::string());
    toType<int32_t>(Position(), std::string());
    toType<uint16_t>(Position(), std::string());
    toType<int16_t>(Position(), std::string());
    toType<double>(Position(), std::string());
    toType<float>(Position(), std::string());
    toType<bool>(Position(), std::string());
    ElementContext elementContext;
    parse(nullptr, elementContext);
}

TEST(Parser, unescapeString)
{
    EXPECT_EQ(unescapeString("testabc", 4), "test");
    EXPECT_EQ(unescapeString("test&amp;abc", 9), "test&");
    EXPECT_EQ(unescapeString("&amp;testabc", 9), "&test");
    EXPECT_EQ(unescapeString("&amp;&amp;abc", 10), "&&");
    EXPECT_EQ(unescapeString("a&amp;&amp;babc", 12), "a&&b");
    EXPECT_EQ(unescapeString("&#38;", 5), "&");
    EXPECT_EQ(unescapeString("&amp;", 4), "&amp");
    EXPECT_EQ(unescapeString("a&amp;", 5), "a&amp");
    EXPECT_EQ(unescapeString("&#38;&#38;", 10), "&&");
    EXPECT_EQ(unescapeString("&#38abc38;", 10), "&#38abc38;");
}
