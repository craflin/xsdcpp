

#include "../src/XmlParser.cpp"

#include <gtest/gtest.h>

struct root_type_FileProducer_t
{
    std::string Identifier;
    std::string Comment;
};

enum class standard_revision_type
{
    _,
    A,
};

struct root_type
{
    root_type_FileProducer_t FileProducer;
    std::string Name;
    standard_revision_type StandardRevision;
};

typedef root_type ED247ComponentInstanceConfiguration;

namespace {

template <typename T>
T toEnum(Context& context, const std::string& value);

template <>
standard_revision_type toEnum<standard_revision_type>(Context& context, const std::string& value)
{
    if (value == "")
        return standard_revision_type::_;
    else if (value == "A")
        return standard_revision_type::A;
    else
        throwVerificationException(context.pos, "Invalid enumeration value '" + value + "'");
    return (standard_revision_type)0;
}

void FileProducer_enter_element(Context& context, ElementContext& parentElementContext, const std::string& name, ElementContext& elementContext)
{
    throwVerificationException(context.pos, "Unexpected element '" + name + "'");
}

void FileProducer_check(Context& context, ElementContext& elementContext)
{
}

void FileProducer_set_attribute(Context& context, ElementContext& elementContext, const std::string& name, const std::string& value)
{
    if (name == "Identifier")
    {
        if (elementContext.processedAttributes & 1)
            throwVerificationException(context.pos, "Attribute '" + name + "' was already set");
        elementContext.processedAttributes |= 1;
        ((root_type_FileProducer_t*)elementContext.element)->Identifier = value;
    }
    else if (name == "Comment")
    {
        if (elementContext.processedAttributes & 2)
            throwVerificationException(context.pos, "Attribute '" + name + "' was already set");
        elementContext.processedAttributes |= 2;
        ((root_type_FileProducer_t*)elementContext.element)->Comment = value;
    }
    else
        throwVerificationException(context.pos, "Unexpected attribute '" + name + "'");
}

void FileProducer_check_attributes(Context& context, ElementContext& elementContext)
{
    if ((elementContext.processedAttributes & elementContext.info->mandatoryAttributes) != elementContext.info->mandatoryAttributes)
    { // an attribute was missing
    }
}

const ElementInfo _FileProducer_Info = { &FileProducer_enter_element, &FileProducer_check, &FileProducer_set_attribute, &FileProducer_check_attributes, 0, 0 };

void ED247ComponentInstanceConfiguration_enter_element(Context& context, ElementContext& parentElementContext, const std::string& name, ElementContext& elementContext)
{
    if (name == "FileProducer")
    {
        if (parentElementContext.processedElements & 1)
            throwVerificationException(context.pos, "Element '" + name + "' cannot occur more than once");
        parentElementContext.processedElements |= 1;

        elementContext.info = &_FileProducer_Info;
        elementContext.element = &((ED247ComponentInstanceConfiguration*)parentElementContext.element)->FileProducer;
    }
    else
        throwVerificationException(context.pos, "Unexpected element '" + name + "'");
}

void ED247ComponentInstanceConfiguration_check(Context& context, ElementContext& elementContext)
{
    if ((elementContext.processedElements & elementContext.info->mandatoryElements) != elementContext.info->mandatoryElements)
    { // an element was missing
    }
}

void ED247ComponentInstanceConfiguration_set_attribute(Context& context, ElementContext& elementContext, const std::string& name, const std::string& value)
{
    if (name == "Name")
    {
        if (elementContext.processedAttributes & 1)
            throwVerificationException(context.pos, "Attribute '" + name + "' was already set");
        elementContext.processedAttributes |= 1;
        ((ED247ComponentInstanceConfiguration*)elementContext.element)->Name = value;
    }
    else if (name == "StandardRevision")
    {
        if (elementContext.processedAttributes & 2)
            throwVerificationException(context.pos, "Attribute '" + name + "' was already set");
        elementContext.processedAttributes |= 2;
        ((ED247ComponentInstanceConfiguration*)elementContext.element)->StandardRevision = toEnum<standard_revision_type>(context, value);
    }
    else
        throwVerificationException(context.pos, "Unexpected attribute '" + name + "'");
}

void ED247ComponentInstanceConfiguration_check_attributes(Context& context, ElementContext& elementContext)
{
    if ((elementContext.processedAttributes & elementContext.info->mandatoryAttributes) != elementContext.info->mandatoryAttributes)
    { // an attribute was missing
    }
}

const ElementInfo _ED247ComponentInstanceConfiguration_Info = { &ED247ComponentInstanceConfiguration_enter_element, &ED247ComponentInstanceConfiguration_check, &ED247ComponentInstanceConfiguration_set_attribute, &ED247ComponentInstanceConfiguration_check_attributes, 0, 0 };

void enter_root_element(Context& context, ElementContext& parentElementContext, const std::string& name, ElementContext& elementContext)
{
    if (name == "ED247ComponentInstanceConfiguration")
    {
        if (parentElementContext.processedElements & 1)
            throwVerificationException(context.pos, "Element '" + name + "' cannot occur more than once");
        parentElementContext.processedElements |= 1;

        elementContext.info = &_ED247ComponentInstanceConfiguration_Info;
        elementContext.element = parentElementContext.element;
    }
    else
        throwSyntaxException(context.pos, "Unexpected element '" + name + "'");
}

void leave_root_element(Context& context, ElementContext& elementContext)
{
    if ((elementContext.processedElements & elementContext.info->mandatoryElements) != elementContext.info->mandatoryElements)
    { // an element was missing
    }
}

const ElementInfo _rootInfo = { &enter_root_element, &leave_root_element, nullptr, nullptr, 1, 0 };

}

void load_content(const std::string& content, ED247ComponentInstanceConfiguration& data)
{
    ElementContext elementContext;
    elementContext.info = &_rootInfo;
    elementContext.element = &data;
    parse(content.c_str(), elementContext);
}

TEST(XmlParser, parse)
{
    ED247ComponentInstanceConfiguration config;
    std::string config_file_content = "<?xml version=\"1.0\" encoding=\"UTF-8\"?>"
                                      "<!-- test -->"
                                      "<ED247ComponentInstanceConfiguration Name=\"VirtualComponent\" StandardRevision=\"A\">"
                                      "    <FileProducer Identifier=\"123\"/>"
                                      "</ED247ComponentInstanceConfiguration>";

    load_content(config_file_content, config);

    EXPECT_EQ(config.FileProducer.Identifier, "123");
    EXPECT_EQ(config.Name, "VirtualComponent");
    EXPECT_EQ(config.StandardRevision, standard_revision_type::A);
}
