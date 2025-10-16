
#include <string>
#include <cstdint>
#include <unordered_map>

namespace xsdcpp {

struct Context;
struct ElementContext;
struct Position;
struct ElementInfo;

typedef void (*add_text_t)(void*, const Position&, std::string&& name);
typedef void* (*get_element_field_t)(void*);
typedef void (*set_attribute_t)(void*, const Position&, std::string&& value);
typedef void (*set_attribute_default_t)(void*);
typedef void (*set_any_attribute_t)(void*, std::string&& name, std::string&& value);

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
        EntryPointFlag = 0x01,
        ReadTextFlag = 0x02,
        SkipProcessingFlag = 0x04,
        AnyAttributeFlag = 0x08,
    };
    
    size_t flags;
    add_text_t addText;
    const ChildElementInfo* children;
    size_t childrenCount;
    size_t mandatoryChildrenCount;
    const AttributeInfo* attributes;
    size_t attributesCount;
    const ElementInfo* base;
    set_any_attribute_t setOtherAttribute;
};

struct ElementContext
{
    const ElementInfo* info;
    void* element;
    std::unordered_map<const ChildElementInfo*, size_t> processedElements;
    uint64_t processedAttributes;

    ElementContext() : processedAttributes(0) {}
};

void parse(const char* data, const char** namespaces, ElementContext& elementContext);

bool getListItem(const char*& s, std::string& result);

uint32_t toType(const Position& pos, const char* const* values, const std::string& value);

template <typename T>
T toType(const Position& pos, const std::string& value);

}
