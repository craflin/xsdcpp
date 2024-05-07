
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
    enum ElementFlag
    {
        Level1Flag = 0x01,
        ReadTextFlag = 0x02
    };
    
    size_t flags;
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
                continue;
            }
            return true;
        }
    }
}

const char* _escapeStrings[] = { "apos", "quot", "amp", "lt", "gt" };
const char* _escapeChars = "'\"&<>";

bool appendUnicode(uint32_t ch, std::string& str)
{
    if ((ch & ~(0x80UL - 1)) == 0)
    {
        str.push_back((char)ch);
        return true;
    }
    if ((ch & ~(0x800UL - 1)) == 0)
    {
        str.push_back((ch >> 6) | 0xC0);
        str.push_back((ch & 0x3F) | 0x80);
        return true;
    }
    if ((ch & ~(0x10000UL - 1)) == 0)
    {
        str.push_back((ch >> 12) | 0xE0);
        str.push_back(((ch >> 6) & 0x3F) | 0x80);
        str.push_back((ch & 0x3F) | 0x80);
        return true;
    }
    if (ch < 0x110000UL)
    {
        str.push_back((ch >> 18) | 0xF0);
        str.push_back(((ch >> 12) & 0x3F) | 0x80);
        str.push_back(((ch >> 6) & 0x3F) | 0x80);
        str.push_back((ch & 0x3F) | 0x80);
        return true;
    }
    return false;
}

std::string unescapeString(const char* str, size_t len)
{
    std::string result;
    result.reserve(len);
    for (const char* i = str, * end = str + len;;)
    {
        size_t remainingLen = end - i;
        const char* next = (const char*)memchr(i, '&', remainingLen);
        if (!next)
            return result.append(i, remainingLen);
        else
            result.append(i, next - i);
        i = next + 1;
        remainingLen = end - i;
        const char* sequenceEnd = (const char*)memchr(i, ';', remainingLen);
        if (!sequenceEnd)
        {
            result.push_back('&');
            continue;
        }
        if (*i == '#')
        {
            char* endptr;
            uint32_t unicodeValue = i[1] == 'x' ? strtoul(i + 2, &endptr, 16) : strtoul(i + 1, &endptr, 10);
            if (endptr != sequenceEnd || !appendUnicode(unicodeValue, result))
            {
                result.push_back('&');
                continue;
            }
            i = sequenceEnd + 1;
            continue;
        }
        size_t sequenceLen = sequenceEnd - i;
        for (const char **j = _escapeStrings, **end = _escapeStrings + sizeof(_escapeStrings) / sizeof(*_escapeStrings); j < end; ++j)
            if (strncmp(i, *j, sequenceLen) == 0)
            {
                result.push_back(_escapeChars[j - _escapeStrings]);
                i = sequenceEnd + 1;
                goto sequenceTranslated;
            }
        result.push_back('&');
    sequenceTranslated:;
    }
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

void enterElement(Context& context, ElementContext& parentElementContext, const std::string& name_, ElementContext& elementContext)
{
    std::string nameWithoutNamespace;
    const std::string* name = &name_;
    size_t n = name_.find(':');
    if (n != std::string::npos)
    {
        nameWithoutNamespace = name_.substr(n + 1);
        name = &nameWithoutNamespace;
    }
    for (const ElementInfo* i = parentElementContext.info; i; i = i->base)
        if (const ChildElementInfo* c = i->children)
            for (; c->name; ++c)
                if (*name == c->name)
                {
                    size_t& count = parentElementContext.processedElements[c];
                    if (c->maxOccurs && count >= c->maxOccurs)
                    {
                        std::stringstream s;
                        s << "Maximum occurrence of element '" << *name << "' is " << c->maxOccurs ;
                        throwVerificationException(context.pos,  s.str());
                    }
                    ++count;
                    elementContext.element = c->getElementField(parentElementContext.element);
                    elementContext.info = c->info;
                    elementContext.processedElements.reserve(c->info->childrenCount);
                    return;
                }
    throwVerificationException(context.pos, "Unexpected element '" + name_ + "'");
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
                        s << "Minimum occurrence of element '" << c->name << "' is " << c->minOccurs ;
                        throwVerificationException(context.pos, s.str());
                    }
                }
}

void setAttribute(Context& context, ElementContext& elementContext, const std::string& name, std::string&& value)
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
                    a->setAttribute(elementContext.element, context.pos, std::move(value));
                    return;
                }
    if (elementContext.info->flags & ElementInfo::Level1Flag && (name.find(':') != std::string::npos || name == "xmlns"))
        return;
    throwVerificationException(context.pos, "Unexpected attribute '" + name + "'");
}

void addText(Context& context, ElementContext& elementContext, std::string&& text)
{
    std::string& element = *(std::string*)elementContext.element;
    if (element.empty())
        element = std::move(text);
    else
        element += text;
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
            std::string& attributeValue = context.token.value;
            setAttribute(context, elementContext, attributeName, std::move(attributeValue));
            continue;
        }
    }
    checkAttributes(context, elementContext);
    for (;;)
    {
        const char* start = context.pos.pos;
        skipText(context.pos);
        if (context.pos.pos != start && elementContext.info->flags & ElementInfo::ReadTextFlag)
        {
            std::string text(start, context.pos.pos - start);
            addText(context, elementContext, std::move(text));
        }
        
        readToken(context);
        if (context.token.type == Token::endTagBeginType)
            break;
        if (context.token.type == Token::startTagBeginType)
        {
            parseElement(context, elementContext);
            continue;
        }
        else
            throwSyntaxException(context.token.pos, "Expected '<'");
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
    while (*context.pos.pos == '<' && context.pos.pos[1] == '?')
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
        skipSpace(context.pos);
    }
    readToken(context);
    if (context.token.type != Token::startTagBeginType)
        throwSyntaxException(context.token.pos, "Expected '<'");
    parseElement(context, elementContext);
}

uint32_t toType(const Position& pos, const char* const* values, const std::string& value)
{
    for (const char* const* i = values; *i; ++i)
        if (value == *i)
            return (uint32_t)(i - values);
    throwVerificationException(pos, "Unknown attribute value '" + value + "'");
    return 0;
}

template <typename T>
T toType(const Position& pos, const std::string& value) { throwVerificationException(pos, "Not implemented"); return T(); }

template <typename T>
T toType(const Position& pos, const std::string& value, const char* exceptionMessage)
{
    std::stringstream ss(value);
    T result;
    if (!(ss >> result))
         throwVerificationException(pos, exceptionMessage);
    return result;
}

template <>
uint64_t toType<uint64_t>(const Position& pos, const std::string& value) { return toType<uint64_t>(pos, value,  "Expected unsigned 64-bit integer value"); }
template <>
int64_t toType<int64_t>(const Position& pos, const std::string& value) { return toType<int64_t>(pos, value,  "Expected 64-bit integer value"); }
template <>
uint32_t toType<uint32_t>(const Position& pos, const std::string& value) { return toType<uint32_t>(pos, value,  "Expected unsigned 32-bit integer value"); }
template <>
int32_t toType<int32_t>(const Position& pos, const std::string& value) { return toType<int32_t>(pos, value,  "Expected 32-bit integer value"); }
template <>
uint16_t toType<uint16_t>(const Position& pos, const std::string& value) { return toType<uint16_t>(pos, value,  "Expected unsigned 16-bit integer value"); }
template <>
int16_t toType<int16_t>(const Position& pos, const std::string& value) { return toType<int16_t>(pos, value,  "Expected 16-bit integer value"); }
template <>
double toType<double>(const Position& pos, const std::string& value) { return toType<double>(pos, value,  "Expected single precision floating point value"); }
template <>
float toType<float>(const Position& pos, const std::string& value) { return toType<float>(pos, value,  "Expected double precision floating point value"); }
template <>
bool toType<bool>(const Position& pos, const std::string& value) { return toType<bool>(pos, value,  "Expected boolean value"); }

}
