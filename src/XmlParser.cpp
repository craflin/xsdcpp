
#include <cstring>
#include <sstream>
#include <stdexcept>

namespace xsdcpp {

ElementContext::ElementContext(const ElementInfo* info, void* element)
    : info(info)
    , element(element)
    , processedAttributes2(0)
{
    memset(processedElements2, 0, sizeof(size_t) * info->childrenCount);
}

struct Position
{
    int line;
    const char* pos;
    const char* lineStart;
};

}


namespace {

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
    xsdcpp::Position pos;
};

struct Context
{
    xsdcpp::Position pos;
    Token token;
    const char** namespaces;
    std::string xmlSchemaNamespacePrefix;
};

void skipSpace(xsdcpp::Position& pos)
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

void throwSyntaxException(const xsdcpp::Position& pos, const std::string& error)
{
    std::stringstream s;
    s << "Syntax error at line '" << pos.line << "': " << error;
    throw std::runtime_error(s.str());
}

void throwVerificationException(const xsdcpp::Position& pos, const std::string& error)
{
    std::stringstream s;
    s << "Error at line '" << pos.line << "': " << error;
    throw std::runtime_error(s.str());
}

void skipText(xsdcpp::Position& pos)
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
            return;
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

std::string stripComments(const char* str, size_t len)
{
    std::string result;
    result.reserve(len);
    for (const char* i = str, * end = str + len;;)
    {
        size_t remainingLen = end - i;
        const char* next = (const char*)memchr(i, '<', remainingLen);
        if (!next)
            return result.append(i, remainingLen);
        else
            result.append(i, next - i);
        i = next;
        if (strncmp(i + 1, "!--", 3) != 0)
            return result.append(i, end - i);
        i += 4;
        for (;;)
        {
            const char* commentEnd = strpbrk(i, "-");
            if (!commentEnd)
            {
                i = end;
                continue;
            }
            i = commentEnd;
            if (strncmp(i + 1, "->", 2) == 0)
            {
                i += 3;
                break;
            }
            ++i;
        }
    }
}

void readToken(Context& context)
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
            return;
        }
        context.token.type = Token::startTagBeginType;
        ++context.pos.pos;
        return;
    case '>':
        context.token.type = Token::tagEndType;
        ++context.pos.pos;
        return;
    case '\0':
        throwSyntaxException(context.pos, "Unexpected end of file");
    case '=':
        context.token.type = Token::equalsSignType;
        ++context.pos.pos;
        return;
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
        return;
    }
    case '/':
        if (context.pos.pos[1] == '>')
        {
            context.token.type = Token::emptyTagEndType;
            context.pos.pos += 2;
            return;
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
            return;
        }
    }
}

void skipTextAndSubElements(Context& context, const std::string& elementName)
{
    for (;;)
    {
        skipText(context.pos);
        xsdcpp::Position posBackup = context.pos;
        readToken(context);
        switch (context.token.type)
        {
        case Token::startTagBeginType:
            readToken(context);
            if (context.token.type == Token::nameType)
            {
                std::string elementName = std::move(context.token.value);
                skipTextAndSubElements(context, elementName);
                readToken(context);
            }
            break;
        case Token::endTagBeginType:
            readToken(context);
            if (context.token.type == Token::nameType && context.token.value == elementName)
            {
                context.pos = posBackup;
                return;
            }
            break;
        default:
            break;
        }
    }
}

xsdcpp::ElementContext enterElement(Context& context, xsdcpp::ElementContext& parentElementContext, const xsdcpp::ChildElementInfo& childInfo)
{
    size_t& count = parentElementContext.processedElements2[childInfo.trackIndex];
    if (childInfo.maxOccurs && count >= childInfo.maxOccurs)
    {
        std::stringstream s;
        s << "Maximum occurrence of element '" << childInfo.name << "' is " << childInfo.maxOccurs ;
        throwVerificationException(context.pos,  s.str());
    }
    ++count;
    return xsdcpp::ElementContext(childInfo.info, childInfo.getElementField(parentElementContext.element));
}

xsdcpp::ElementContext enterElement(Context& context, xsdcpp::ElementContext& parentElementContext, const std::string& name)
{
    for (const xsdcpp::ElementInfo* i = parentElementContext.info; i; i = i->base)
        if (const xsdcpp::ChildElementInfo* c = i->children)
            for (; c->name; ++c)
                if (name == c->name)
                    return enterElement(context, parentElementContext, *c);
    size_t n = name.find(':');
    if (n != std::string::npos)
    {
        std::string nameWithoutNamespace = name.substr(n + 1);
        for (const xsdcpp::ElementInfo* i = parentElementContext.info; i; i = i->base)
            if (const xsdcpp::ChildElementInfo* c = i->children)
                for (; c->name; ++c)
                    if (nameWithoutNamespace == c->name)
                        return enterElement(context, parentElementContext, *c);
    }
    throwVerificationException(context.pos, "Unexpected element '" + name + "'");
    return xsdcpp::ElementContext(nullptr, nullptr);
}

void checkElement(Context& context, const xsdcpp::ElementContext& elementContext)
{
    if (elementContext.info->flags & xsdcpp::ElementInfo::CheckChildrenFlag)
        for (const xsdcpp::ElementInfo* i = elementContext.info; i; i = i->base)
            if (const xsdcpp::ChildElementInfo* c = i->children)
                for (; c->name; ++c)
                    if (elementContext.processedElements2[c->trackIndex] < c->minOccurs)
                    {
                        std::stringstream s;
                        s << "Minimum occurrence of element '" << c->name << "' is " << c->minOccurs;
                        throwVerificationException(context.pos, s.str());
                    }
}

void setAttribute(Context& context, xsdcpp::ElementContext& elementContext, std::string&& name, std::string&& value)
{
    for (const xsdcpp::ElementInfo* i = elementContext.info; i; i = i->base)
        if (const xsdcpp::AttributeInfo* a = i->attributes)
            for (; a->name; ++a)
                if (name == a->name)
                {
                    if (elementContext.processedAttributes2 & a->trackBit)
                        throwVerificationException(context.pos, "Repeated attribute '" + name + "'");
                    elementContext.processedAttributes2 |= a->trackBit;
                    a->setValue(a->getAttribute(elementContext.element), context.pos, std::move(value));
                    return;
                }
    if (elementContext.info->flags & xsdcpp::ElementInfo::EntryPointFlag)
    {
        if (name.compare(0, 5, "xmlns") == 0 && (name.size() == 5 || name.c_str()[5] == ':'))
        {
            std::string namespacePrefix;
            if (name.size() > 6)
                namespacePrefix = name.substr(6);
            size_t namespaceIndex = 0;
            for (const char** ns = context.namespaces; *ns; ++ns, ++namespaceIndex)
                if (value == *ns)
                {
                    if (value == "http://www.w3.org/2001/XMLSchema-instance")
                        context.xmlSchemaNamespacePrefix = std::move(namespacePrefix);
                    return;
                }
            throwVerificationException(context.pos, "Unknown namespace '" + value + "'");
        }
        size_t n = name.find(':');
        if (n != std::string::npos && name.compare(n + 1, std::string::npos, "noNamespaceSchemaLocation") == 0)
        {
            std::string namespacePrefix = name.substr(0, n);
            if (namespacePrefix == context.xmlSchemaNamespacePrefix)
                return;
        }
    }
    for (const xsdcpp::ElementInfo* i = elementContext.info; i; i = i->base)
        if (i->flags & xsdcpp::ElementInfo::AnyAttributeFlag)
        {
            i->setOtherAttribute(elementContext.element, std::move(name), std::move(value));
            return;
        }

    throwVerificationException(context.pos, "Unexpected attribute '" + name + "'");
}

void checkAttributes(Context& context, xsdcpp::ElementContext& elementContext)
{
    uint64_t missingAttributes = elementContext.info->checkAttributeMask & ~elementContext.processedAttributes2;
    if (missingAttributes)
    {
        for (const xsdcpp::ElementInfo* i = elementContext.info; i; i = i->base)
            if (const xsdcpp::AttributeInfo* a = i->attributes)
                for (; a->name; ++a)
                    if (missingAttributes & a->trackBit)
                    {
                        if (a->isMandatory)
                            throwVerificationException(context.pos, "Missing attribute '" + std::string(a->name) + "'");
                        a->setDefaultValue(elementContext.element);
                    }
    }
}

void parseElement(Context& context, xsdcpp::ElementContext& parentElementContext)
{
    readToken(context);
    if (context.token.type != Token::nameType)
        throwSyntaxException(context.token.pos, "Expected tag name");
    std::string elementName = std::move(context.token.value);
    xsdcpp::ElementContext elementContext = enterElement(context, parentElementContext, elementName);
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
            setAttribute(context, elementContext, std::move(attributeName), std::move(attributeValue));
            continue;
        }
    }
    checkAttributes(context, elementContext);
    for (;;)
    {
        if (elementContext.info->flags & xsdcpp::ElementInfo::ReadTextFlag)
        {
            const char* start = context.pos.pos;
            if (elementContext.info->flags & xsdcpp::ElementInfo::SkipProcessingFlag)
                skipTextAndSubElements(context, elementName);
            else
                skipText(context.pos);
            if (context.pos.pos != start)
            {
                std::string text = stripComments(start, context.pos.pos - start);
                elementContext.info->addText(elementContext.element, context.pos, std::move(text));
            }
        }
        else
            skipText(context.pos);
        
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

}

namespace xsdcpp {

bool getListItem(const char*& s, std::string& result)
{
    while (isspace(*s))
        ++s;
    if (!*s)
        return false;
    const char* end = strpbrk(s, " \t\n\r");
    if (end)
    {
        result = std::string(s, end - s);
        s = end;
    }
    else
    {
        result = s;
        s += result.size();
    }
    return true;
}

void parse(const char* data, const char** namespaces, ElementContext& elementContext)
{
    Context context;
    context.pos.pos = context.pos.lineStart = data;
    context.pos.line = 1;
    context.namespaces = namespaces;
    
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

uint32_t toNumeric(const Position& pos, const char* const* values, const std::string& value)
{
    for (const char* const* i = values; *i; ++i)
        if (value == *i)
            return (uint32_t)(i - values);
    throwVerificationException(pos, "Unknown attribute value '" + value + "'");
    return 0;
}

void set_string(std::string* obj, const Position&, std::string&& val) { if (obj->empty()) *obj = std::move(val); else *obj += val; }
void set_uint64_t(uint64_t* obj,  const Position& pos, std::string&& val) { std::stringstream ss(val); if (!(ss >> *obj)) throwVerificationException(pos, "Expected unsigned 64-bit integer value"); }
void set_int64_t(int64_t* obj,  const Position& pos, std::string&& val) { std::stringstream ss(val); if (!(ss >> *obj)) throwVerificationException(pos, "Expected 64-bit integer value"); }
void set_uint32_t(uint32_t* obj,  const Position& pos, std::string&& val) { std::stringstream ss(val); if (!(ss >> *obj)) throwVerificationException(pos, "Expected unsigned 32-bit integer value"); }
void set_int32_t(int32_t* obj,  const Position& pos, std::string&& val) { std::stringstream ss(val); if (!(ss >> *obj)) throwVerificationException(pos, "Expected 32-bit integer value"); }
void set_uint16_t(uint16_t* obj,  const Position& pos, std::string&& val) { std::stringstream ss(val); if (!(ss >> *obj)) throwVerificationException(pos, "Expected unsigned 16-bit integer value"); }
void set_int16_t(int16_t* obj,  const Position& pos, std::string&& val) { std::stringstream ss(val); if (!(ss >> *obj)) throwVerificationException(pos, "Expected 16-bit integer value"); }
void set_float(float* obj,  const Position& pos, std::string&& val) { std::stringstream ss(val); if (!(ss >> *obj)) throwVerificationException(pos, "Expected single precision floating point value"); }
void set_double(double* obj,  const Position& pos, std::string&& val) { std::stringstream ss(val); if (!(ss >> *obj)) throwVerificationException(pos, "Expected double precision floating point value"); }
void set_bool(bool* obj, const Position& pos, std::string&& val) { std::stringstream ss(val); if (!(ss >> std::boolalpha >> *obj)) throwVerificationException(pos, "Expected boolean value"); }

}
