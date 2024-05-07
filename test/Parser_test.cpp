
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
    struct _
    {
        static std::string unescapeString(const std::string& testStr, const std::string& testSuffix)
        {
            std::string testData = testStr + testSuffix;
            return ::unescapeString(testData.c_str(), testStr.size());
        }
    };

    EXPECT_EQ(_::unescapeString("test", "abc"), "test");
    EXPECT_EQ(_::unescapeString("test&amp;", "abc"), "test&");
    EXPECT_EQ(_::unescapeString("&amp;test", "abc"), "&test");
    EXPECT_EQ(_::unescapeString("&amp;&amp;", "abc"), "&&");
    EXPECT_EQ(_::unescapeString("a&amp;&amp;b", "abc"), "a&&b");
    EXPECT_EQ(_::unescapeString("&#38;", ""), "&");
    EXPECT_EQ(_::unescapeString("&amp", ";"), "&amp");
    EXPECT_EQ(_::unescapeString("a&amp", ";"), "a&amp");
    EXPECT_EQ(_::unescapeString("&#38;&#38;", ""), "&&");
    EXPECT_EQ(_::unescapeString("&#38abc38;", ""), "&#38abc38;");
}

TEST(Parser, stripComments)
{
    struct _
    {
        static std::string stripComments(const std::string& testStr, const std::string& testSuffix)
        {
            std::string testData = testStr + testSuffix;
            return ::stripComments(testData.c_str(), testStr.size());
        }
    };

    EXPECT_EQ(_::stripComments("<!-- abc -->", "abc"), "");
    EXPECT_EQ(_::stripComments("<!-- abc - -->", "abc"), "");
    EXPECT_EQ(_::stripComments("1<!-- abc -->2", "abc"), "12");
    EXPECT_EQ(_::stripComments("1<!-- abc -->2<!-- abc -->3", "abc"), "123");
}
