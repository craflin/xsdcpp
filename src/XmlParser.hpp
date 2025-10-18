
#include <string>
#include <cstdint>
#include <unordered_map>

namespace xsdcpp {

struct Context;
struct ElementContext;
struct Position;
struct ElementInfo;

typedef void* (*get_field_t)(void*);
typedef void (*set_value_t)(void* obj, const Position&, std::string&&);
typedef void (*set_default_t)(void*);
typedef void (*set_any_attribute_t)(void*, std::string&& name, std::string&& value);

struct ChildElementInfo
{
    const char* name;
    get_field_t getElementField;
    const ElementInfo* info;
    size_t minOccurs;
    size_t maxOccurs;
};

struct AttributeInfo
{
    const char* name;
    get_field_t getAttribute;
    set_value_t setValue;
    bool isMandatory;
    set_default_t setDefaultValue;
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
    set_value_t addText;
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

uint32_t toNumeric(const Position& pos, const char* const* values, const std::string& value);

void set_string(std::string* obj,const Position&, std::string&& val);
void set_uint64_t(uint64_t* obj, const Position& pos, std::string&& val);
void set_int64_t(int64_t* obj, const Position& pos, std::string&& val);
void set_uint32_t(uint32_t* obj, const Position& pos, std::string&& val);
void set_int32_t(int32_t* obj, const Position& pos, std::string&& val);
void set_uint16_t(uint16_t* obj, const Position& pos, std::string&& val);
void set_int16_t(int16_t* obj, const Position& pos, std::string&& val);
void set_float(float* obj, const Position& pos, std::string&& val);
void set_double(double* obj, const Position& pos, std::string&& val);
void set_bool(bool* obj, const Position& pos, std::string&& val);


}
