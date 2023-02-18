
#include <cstring>
#include <sstream>
#include <stdexcept>
#include <string>

namespace {

struct Context;
struct ElementContext;

typedef void (*enter_element_handler_t)(Context& context, ElementContext& parentElementContext, const std::string& name, ElementContext& elementContext);
typedef void (*check_element_handler_t)(Context& context, ElementContext& elementContext);
typedef void (*set_attribute_handler_t)(Context& context, ElementContext& elementContext, const std::string& name, const std::string& value);
typedef void (*check_attribute_handler_t)(Context& context, ElementContext& elementContext);

struct ElementInfo
{
    enter_element_handler_t enterElementHandler;
    check_element_handler_t checkElementHandler;
    set_attribute_handler_t setAttributeHandler;
    check_attribute_handler_t checkAttributesHandler;
    uint32_t mandatoryElements;
    uint32_t mandatoryAttributes;
};

struct ElementContext
{
    const ElementInfo* info;
    void* element;
    //uint32_t expectedElements;
    uint32_t processedElements;
    uint32_t processedAttributes;

    ElementContext() : processedElements(0), processedAttributes(0) {}
};

struct Position
{
    int line;
    const char* pos;
    const char* lineStart;
};

struct Token
{
    enum Type
    {
        startTagBeginType, // <
        tagEndType, // >
        endTagBeginType, // </
        emptyTagEndType, // />
        equalsSignType, // =
        stringType,
        nameType, // attribute or tag name
    };

    Type type;
    std::string value;
    Position pos;
};

struct Context
{
    Position pos;
    Token token;
};

void skipSpace(Position& pos)
{
    for (char c;;)
        switch ((c = *pos.pos))
        {
        case '\r':
            if (*(++pos.pos) == '\n')
                ++pos.pos;
            ++pos.line;
            pos.lineStart = pos.pos;
            continue;
        case '\n':
            ++pos.line;
            ++pos.pos;
            pos.lineStart = pos.pos;
            continue;
        case '<':
            if (strncmp(pos.pos + 1, "!--", 3) == 0)
            {
                pos.pos += 4;
                for (;;)
                {
                    const char* end = strpbrk(pos.pos, "-\n\r");
                    if (!end)
                    {
                        pos.pos = pos.pos + strlen(pos.pos);
                        return;
                    }
                    pos.pos = end;
                    switch (*pos.pos)
                    {
                    case '\r':
                        if (*(++pos.pos) == '\n')
                            ++pos.pos;
                        ++pos.line;
                        pos.lineStart = pos.pos;
                        continue;
                    case '\n':
                        ++pos.line;
                        ++pos.pos;
                        pos.lineStart = pos.pos;
                        continue;
                    default:
                        if (strncmp(pos.pos + 1, "->", 2) == 0)
                        {
                            pos.pos = end + 3;
                            break;
                        }
                        ++pos.pos;
                        continue;
                    }
                    break;
                }
                continue;
            }
            return;
        case ' ':
        case '\t':
        case '\v':
        case '\f':
            ++pos.pos;
            continue;
        default:
            return;
        }
}

void throwSyntaxException(const Position& pos, const std::string& error)
{
    std::stringstream s;
    s << "Syntax error at line '" << pos.line << "': " << error;
    throw std::runtime_error(s.str());
}

void throwVerificationException(const Position& pos, const std::string& error)
{
    std::stringstream s;
    s << "Error at line '" << pos.line << "': " << error;
    throw std::runtime_error(s.str());
}

bool skipText(Position& pos)
{
    const char* start = pos.pos;
    for (;;)
    {
        const char* end = strpbrk(pos.pos, "<\r\n");
        if (!end)
        {
            pos.pos = pos.pos + strlen(pos.pos);
            throwSyntaxException(pos, "Unexpected end of file");
        }
        pos.pos = end;
        switch (*pos.pos)
        {
        case '\r':
            if (*(++pos.pos) == '\n')
                ++pos.pos;
            ++pos.line;
            pos.lineStart = pos.pos;
            continue;
        case '\n':
            ++pos.line;
            ++pos.pos;
            pos.lineStart = pos.pos;
            continue;
        default:
            if (pos.pos[1] == '!')
            {
                skipSpace(pos);
                start = pos.pos;
                continue;
            }
            return true;
        }
    }
}

const char* _escapeStrings[] = { "apos", "quot", "amp", "lt", "gt" };
const char* _escapeChars = "'\"&<>";

bool appendUnicode(uint32_t ch, char*& str)
{
    if ((ch & ~(0x80UL - 1)) == 0)
    {
        *(str++) = (char)ch;
        return true;
    }
    if ((ch & ~(0x800UL - 1)) == 0)
    {
        *(str++) = (ch >> 6) | 0xC0;
        *(str++) = (ch & 0x3F) | 0x80;
        return true;
    }
    if ((ch & ~(0x10000UL - 1)) == 0)
    {
        *(str++) = (ch >> 12) | 0xE0;
        *(str++) = ((ch >> 6) & 0x3F) | 0x80;
        *(str++) = (ch & 0x3F) | 0x80;
        return true;
    }
    if (ch < 0x110000UL)
    {
        *(str++) = (ch >> 18) | 0xF0;
        *(str++) = ((ch >> 12) & 0x3F) | 0x80;
        *(str++) = ((ch >> 6) & 0x3F) | 0x80;
        *(str++) = (ch & 0x3F) | 0x80;
        return true;
    }
    return false;
}

std::string unescapeString(const char* str, size_t len)
{
    const char* src = (const char*)memchr(str, '&', len);
    if (!src)
        return std::string(str, len);
    char* destStart = (char*)alloca(len + 1);
    size_t startLen = src - str;
    memcpy(destStart, str, startLen);
    char* dest = destStart + startLen;
    for (const char* srcEnd = str + len; src < srcEnd;)
        if (*src != '&')
            *(dest++) = *(src++);
        else
        {
            ++src;
            const char* sequenceEnd = (const char*)memchr(src, ';', len - (src - str));
            if (!sequenceEnd)
            {
                *(dest++) = '&';
                continue;
            }
            if (*src == '#')
            {
                uint unicodeValue;
                if (src[1] == 'x')
                    unicodeValue = strtoul(src + 2, nullptr, 16);
                else
                    unicodeValue = strtoul(src + 1, nullptr, 10);
                if (unicodeValue == 0 || !appendUnicode(unicodeValue, dest))
                {
                    *(dest++) = '&';
                    continue;
                }
                src = sequenceEnd + 1;
                continue;
            }
            for (const char **j = _escapeStrings, **end = _escapeStrings + sizeof(_escapeStrings) / sizeof(*_escapeStrings); j < end; ++j)
                if (strcmp(str, *j) == 0)
                {
                    *(dest++) = _escapeChars[j - _escapeStrings];
                    src = sequenceEnd + 1;
                    goto translated;
                }
            *(dest++) = '&';
            continue;
        translated:
            continue;
        }
    return std::string(destStart, dest - destStart);
}

bool readToken(Context& context)
{
    skipSpace(context.pos);
    context.token.pos = context.pos;
    switch (*context.pos.pos)
    {
    case '<':
        if (context.pos.pos[1] == '/')
        {
            context.token.type = Token::endTagBeginType;
            context.pos.pos += 2;
            return true;
        }
        context.token.type = Token::startTagBeginType;
        ++context.pos.pos;
        return true;
    case '>':
        context.token.type = Token::tagEndType;
        ++context.pos.pos;
        return true;
    case '\0':
        throwSyntaxException(context.pos, "Unexpected end of file");
    case '=':
        context.token.type = Token::equalsSignType;
        ++context.pos.pos;
        return true;
    case '"':
    case '\'': {
        char endChars[4] = "x\r\n";
        *endChars = *context.pos.pos;
        const char* end = strpbrk(context.pos.pos + 1, endChars);
        if (!end)
            throwSyntaxException(context.pos, "Unexpected end of file");
        if (*end != *context.pos.pos)
            throwSyntaxException(context.pos, "New line in string");
        context.token.value = unescapeString(context.pos.pos + 1, end - context.pos.pos - 1);
        context.token.type = Token::stringType;
        context.pos.pos = end + 1;
        return true;
    }
    case '/':
        if (context.pos.pos[1] == '>')
        {
            context.token.type = Token::emptyTagEndType;
            context.pos.pos += 2;
            return true;
        }
        // no break
    default: // attribute or tag name
    {
        const char* end = context.pos.pos;
        while (*end && *end != '/' && *end != '>' && *end != '=' && !isspace(*end))
            ++end;
        if (end == context.pos.pos)
            throwSyntaxException(context.pos, "Expected name");
        context.token.value = std::string(context.pos.pos, end - context.pos.pos);
        context.token.type = Token::nameType;
        context.pos.pos = end;
        return true;
    }
    }
}

void parseElement(Context& context, ElementContext& parentElementContext)
{
    // int line = context.token.pos.line;
    // int column = (int)(context.token.pos.pos - context.token.pos.lineStart) + 1;
    readToken(context);
    if (context.token.type != Token::nameType)
        throwSyntaxException(context.token.pos, "Expected tag name");
    std::string elementName = std::move(context.token.value);
    ElementContext elementContext;
    parentElementContext.info->enterElementHandler(context, parentElementContext, elementName, elementContext);
    for (;;)
    {
        readToken(context);
        if (context.token.type == Token::emptyTagEndType)
        {
            elementContext.info->checkAttributesHandler(context, elementContext);
            elementContext.info->checkElementHandler(context, elementContext);
            return;
        }
        if (context.token.type == Token::tagEndType)
            break;
        if (context.token.type == Token::nameType)
        {
            std::string attributeName = std::move(context.token.value);
            readToken(context);
            if (context.token.type != Token::equalsSignType)
                throwSyntaxException(context.token.pos, "Expected '='");
            readToken(context);
            if (context.token.type != Token::stringType)
                throwSyntaxException(context.token.pos, "Expected string");
            const std::string& attributeValue = context.token.value;
            elementContext.info->setAttributeHandler(context, elementContext, attributeName, attributeValue);
            continue;
        }
    }
    elementContext.info->checkAttributesHandler(context, elementContext);
    for (;;)
    {
        skipText(context.pos);
        Position pos = context.pos;
        readToken(context);
        if (context.token.type == Token::endTagBeginType)
            break;
        if (context.token.type == Token::startTagBeginType)
        {
            parseElement(context, elementContext);
            continue;
        }
        else
            throwSyntaxException(context.token.pos, "Expected string");
    }
    readToken(context);
    if (context.token.type != Token::nameType)
        throwSyntaxException(context.token.pos, "Expected tag name");
    if (context.token.value != elementName)
        throwSyntaxException(context.token.pos, "Expected end tag of '" + elementName + "'");
    readToken(context);
    if (context.token.type != Token::tagEndType)
        throwSyntaxException(context.token.pos, "Expected '>'");
    elementContext.info->checkElementHandler(context, elementContext);
}

void parse(const char* data, ElementContext& elementContext)
{
    Context context;
    context.pos.pos = context.pos.lineStart = data;
    context.pos.line = 1;
    
    skipSpace(context.pos);
    if (*context.pos.pos == '<' && context.pos.pos[1] == '?')
    {
        context.pos.pos += 2;
        for (;;)
        {
            const char* end = strpbrk(context.pos.pos, "\r\n?");
            if (!end)
                throwSyntaxException(context.pos, "Unexpected end of file");
            if (*end == '?' && end[1] == '>')
            {
                context.pos.pos = end + 2;
                break;
            }
            context.pos.pos = end + 1;
            skipSpace(context.pos);
        }
    }
    readToken(context);
    if (context.token.type != Token::startTagBeginType)
        throwSyntaxException(context.token.pos, "Expected '<'");
    parseElement(context, elementContext);
}

}
