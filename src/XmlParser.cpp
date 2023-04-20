
#include <cstring>
#include <sstream>
#include <stdexcept>
#include <string>
#include <cstdint>
#include <unordered_map>

namespace {

struct Context;
struct ElementContext;
struct Position;
struct ElementInfo;

typedef void* (*get_element_field_t)(void*);
typedef void (*set_attribute_t)(void*, const Position&, const std::string& value);
typedef void (*set_attribute_default_t)(void*);

struct ChildElementInfo
{
    const char* name;
    get_element_field_t getElementField;
    const ElementInfo* info;
    size_t minOccurs;
    size_t maxOccurs;
};

struct AttributeInfo
{
    const char* name;
    set_attribute_t setAttribute;
    bool isMandatory;
    set_attribute_default_t setDefaultValue;
};

struct ElementInfo
{
    const ChildElementInfo* children;
    size_t childrenCount;
    size_t mandatoryChildrenCount;
    const AttributeInfo* attributes;
    size_t attributesCount;
    const ElementInfo* base;
};

struct ElementContext
{
    const ElementInfo* info;
    void* element;
    std::unordered_map<const ChildElementInfo*, size_t> processedElements;
    uint64_t processedAttributes;

    ElementContext() : processedAttributes(0) {}
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
                uint32_t unicodeValue;
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

void enterElement(Context& context, ElementContext& parentElementContext, const std::string& name, ElementContext& elementContext)
{
    for (const ElementInfo* i = parentElementContext.info; i; i = i->base)
        if (const ChildElementInfo* c = i->children)
            for (; c->name; ++c)
                if (name == c->name)
                {
                    size_t& count = parentElementContext.processedElements[c];
                    if (c->maxOccurs && count >= c->maxOccurs)
                    {
                        std::stringstream s;
                        s << "Maximum occurence of element '" << name << "' is " << c->maxOccurs ;
                        throwVerificationException(context.pos,  s.str());
                    }
                    ++count;
                    elementContext.element = c->getElementField(parentElementContext.element);
                    elementContext.info = c->info;
                    elementContext.processedElements.reserve(c->info->childrenCount);
                    return;
                }
    throwVerificationException(context.pos, "Unexpected element '" + name + "'");
}

void checkElement(Context& context, const ElementContext& elementContext)
{
    if (elementContext.info->mandatoryChildrenCount)
        for (const ElementInfo* i = elementContext.info; i; i = i->base)
            if (const ChildElementInfo* c = i->children)
                for (; c->name; ++c)
                {
                    std::unordered_map<const ChildElementInfo*, size_t>::const_iterator it = elementContext.processedElements.find(c);
                    size_t count = it == elementContext.processedElements.end() ? 0 : it->second;
                    if (count < c->minOccurs)
                    {
                        std::stringstream s;
                        s << "Minimum occurence of element '" << c->name << "' is " << c->minOccurs ;
                        throwVerificationException(context.pos, s.str());
                    }
                }
}

void setAttribute(Context& context, ElementContext& elementContext, const std::string& name, const std::string& value)
{
    uint64_t attribute = 1;
    for (const ElementInfo* i = elementContext.info; i; i = i->base)
        if (const AttributeInfo* a = i->attributes)
            for (; a->name; ++a, attribute <<= 1)
                if (name == a->name)
                {
                    if (elementContext.processedAttributes & attribute)
                        throwVerificationException(context.pos, "Repeated attribute '" + name + "'");
                    elementContext.processedAttributes |= attribute;
                    a->setAttribute(elementContext.element, context.pos, value);
                    return;
                }
    throwVerificationException(context.pos, "Unexpected attribute '" + name + "'");
}

void checkAttributes(Context& context, ElementContext& elementContext)
{
    uint64_t attributes = (uint64_t)-1 >> (64 - elementContext.info->attributesCount);
    uint64_t missingAttributes = attributes & ~elementContext.processedAttributes;
    if (missingAttributes)
    {
        uint64_t attribute = 1;
        for (const ElementInfo* i = elementContext.info; i; i = i->base)
            if (const AttributeInfo* a = i->attributes)
                for (; a->name; ++a, attribute <<= 1)
                    if (missingAttributes & attribute)
                    {
                        if (a->isMandatory)
                            throwVerificationException(context.pos, "Missing attribute '" + std::string(a->name) + "'");
                        if (a->setDefaultValue)
                            a->setDefaultValue(elementContext.element);
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
    enterElement(context, parentElementContext, elementName, elementContext);
    for (;;)
    {
        readToken(context);
        if (context.token.type == Token::emptyTagEndType)
        {
            checkAttributes(context, elementContext);
            checkElement(context, elementContext);
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
            setAttribute(context, elementContext, attributeName, attributeValue);
            continue;
        }
    }
    checkAttributes(context, elementContext);
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
    checkElement(context, elementContext);
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
