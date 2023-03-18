
#include "Generator.hpp"

#include <nstd/Console.hpp>
#include <nstd/Error.hpp>
#include <nstd/File.hpp>
#include <nstd/HashMap.hpp>

#include <Resources.hpp>

namespace {

const Xml::Element* findElementByType(const Xml::Element& parent, const String& type)
{
    for (List<Xml::Variant>::Iterator i = parent.content.begin(), end = parent.content.end(); i != end; ++i)
    {
        const Xml::Variant& variant = *i;
        if (!variant.isElement())
            continue;

        const Xml::Element& element = variant.toElement();
        if (element.type == type)
            return &element;
    }
    return nullptr;
}

String getAttribute(const Xml::Element& element, const String& name, const String& defaultValue = String())
{
    HashMap<String, String>::Iterator it = element.attributes.find(name);
    if (it == element.attributes.end())
        return defaultValue;
    return *it;
}

const Xml::Element* findElementByName(const Xml::Element& parent, const String& name)
{
    for (List<Xml::Variant>::Iterator i = parent.content.begin(), end = parent.content.end(); i != end; ++i)
    {
        const Xml::Variant& variant = *i;
        if (!variant.isElement())
            continue;

        const Xml::Element& element = variant.toElement();

        if (getAttribute(element, "name") == name)
            return &element;
    }
    return nullptr;
}

String toCppIdentifier(const String& str)
{
    String result = str;
    for (char* i = result; *i; ++i)
        if (!String::isAlphanumeric(*i))
            *i = '_';
    if (String::isDigit(*(char*)result))
        result.prepend("_");
    return result;
}

class Generator
{
public:
    struct TypeInfo
    {
        String cppName;
    };

public:
    Generator(const Xml::Element& xsd, List<String>& output)
        : _xsd(xsd)
        , _output(output)
    {
    }

    const String& getError() const { return _error; }

    bool processXsElement(const Xml::Element& element, const TypeInfo*& typeInfo)
    {
        return processXsElement(String("root_t"), element, typeInfo);
    }

private:
    const Xml::Element& _xsd;
    List<String>& _output;
    HashMap<String, TypeInfo> _generatedTypes;
    String _error;

private:
    bool processXsElement(const String& parentTypeName, const Xml::Element& element, const TypeInfo*& typeInfo)
    {
        String typeName = getAttribute(element, "type");
        String name = getAttribute(element, "name");

        if (typeName.isEmpty())
        { // inline type
            typeName = parentTypeName + "_" + name + "_t";

            for (List<Xml::Variant>::Iterator i = element.content.begin(), end = element.content.end(); i != end; ++i)
            {
                const Xml::Variant& variant = *i;
                if (!variant.isElement())
                    continue;
                const Xml::Element& element = variant.toElement();
                if (!processTypeElement(element, typeName, typeInfo))
                    return false;
                break;
            }
        }
        else
        {
            const TypeInfo* _;
            if (!generateType(typeName, typeInfo))
                return false;
        }
        return true;
    }

    bool generateType(const String& typeName, const TypeInfo*& typeInfo_)
    {
        HashMap<String, TypeInfo>::Iterator it = _generatedTypes.find(typeName);
        if (it != _generatedTypes.end())
        {
            typeInfo_ = &*it;
            return true;
        }

        if (typeName == "xs:normalizedString" || typeName == "xs:string")
        {
            TypeInfo& typeInfo = _generatedTypes.append(typeName, TypeInfo());
            typeInfo.cppName = "xsd::string";
            typeInfo_ = &typeInfo;
            return true;
        }

        if (typeName == "xs:nonNegativeInteger" || typeName == "xs:positiveInteger")
        {
            TypeInfo& typeInfo = _generatedTypes.append(typeName, TypeInfo());
            typeInfo.cppName = "uint32_t";
            typeInfo_ = &typeInfo;
            return true;
        }

        const Xml::Element* element = findElementByName(_xsd, typeName);
        if (!element)
            return (_error = String::fromPrintf("Could not find type '%s'", (const char*)typeName)), false;

        return processTypeElement(*element, typeName, typeInfo_);
    }

    bool processTypeElement(const Xml::Element& element, const String& typeName, const TypeInfo*& typeInfo_)
    {
        if (element.type == "xs:simpleType")
        {
            const Xml::Element* restriction = findElementByType(element, "xs:restriction");
            if (!restriction)
                return (_error = String::fromPrintf("Could not find 'xs:restriction' in '%s'", (const char*)typeName)), false;
            String base = getAttribute(*restriction, "base");

            if (base == "xs:normalizedString")
            {
                List<String> enumValues;
                for (List<Xml::Variant>::Iterator i = restriction->content.begin(), end = restriction->content.end(); i != end; ++i)
                {
                    const Xml::Variant& variant = *i;
                    if (!variant.isElement())
                        continue;
                    const Xml::Element& element = variant.toElement();
                    if (element.type != "xs:enumeration")
                        continue;
                    enumValues.append(toCppIdentifier(getAttribute(element, "value")));
                }

                if (!enumValues.isEmpty())
                {
                    _output.append(String("enum class ") + typeName);
                    _output.append("{");
                    for (List<String>::Iterator i = enumValues.begin(), end = enumValues.end(); i != end; ++i)
                        _output.append(String("    ") + *i + ",");
                    _output.append("};");
                    _output.append("");
                }
                else
                {
                    _output.append(String("typedef xsd::string ") + typeName + ";");
                    _output.append("");
                }
            }
            else if (base == "xs:nonNegativeInteger")
            {
                _output.append(String("typedef uint32_t ") + typeName + ";");
                _output.append("");
            }
            else
                return (_error = String::fromPrintf("xs:restriction base '%s' not supported", (const char*)base)), false;
        }

        else if (element.type == "xs:complexType")
        {
            List<String> attributes;
            const TypeInfo* baseTypeInfo = nullptr;

            for (List<Xml::Variant>::Iterator i = element.content.begin(), end = element.content.end(); i != end; ++i)
            {
                const Xml::Variant& variant = *i;
                if (!variant.isElement())
                    continue;
                const Xml::Element& element = variant.toElement();
                if (element.type == "xs:attribute")
                {
                    if (!processXsAttribute(element, attributes))
                        return false;
                }
                else if (element.type == "xs:all" || element.type == "xs:choice" || element.type == "xs:sequence")
                {
                    if (!processXsAllEtAl(typeName, element, attributes))
                        return false;
                }
                else if (element.type == "xs:complexContent")
                {
                    for (List<Xml::Variant>::Iterator i = element.content.begin(), end = element.content.end(); i != end; ++i)
                    {
                        const Xml::Variant& variant = *i;
                        if (!variant.isElement())
                            continue;
                        const Xml::Element& element = variant.toElement();
                        if (element.type == "xs:extension")
                        {
                            String baseTypeName = getAttribute(element, "base");

                            if (!generateType(baseTypeName, baseTypeInfo))
                                return false;

                            for (List<Xml::Variant>::Iterator i = element.content.begin(), end = element.content.end(); i != end; ++i)
                            {
                                const Xml::Variant& variant = *i;
                                if (!variant.isElement())
                                    continue;
                                const Xml::Element& element = variant.toElement();
                                if (element.type == "xs:attribute")
                                {
                                    if (!processXsAttribute(element, attributes))
                                        return false;
                                }
                                else if (element.type == "xs:all" || element.type == "xs:choice" || element.type == "xs:sequence")
                                {
                                    if (!processXsAllEtAl(typeName, element, attributes))
                                        return false;
                                }
                            }
                        }
                    }
                }
            }

            if (baseTypeInfo)
                _output.append(String("struct ") + typeName + " : " + baseTypeInfo->cppName);
            else
                _output.append(String("struct ") + typeName);
            _output.append("{");
            for (List<String>::Iterator i = attributes.begin(), end = attributes.end(); i != end; ++i)
                _output.append(String("    ") + *i + ";");
            _output.append("};");
            _output.append("");
        }
        else
            return (_error = String::fromPrintf("Element type '%s' not supported", (const char*)typeName)), false;

        TypeInfo& typeInfo = _generatedTypes.append(typeName, TypeInfo());
        typeInfo.cppName = toCppIdentifier(typeName);
        typeInfo_ = &typeInfo;
        return true;
    }

    bool processXsAttribute(const Xml::Element& element, List<String>& attributes)
    {
        String typeName = getAttribute(element, "type");
        const TypeInfo* typeInfo = nullptr;
        if (!generateType(typeName, typeInfo))
            return false;
        attributes.append(typeInfo->cppName + " " + toCppIdentifier(getAttribute(element, "name")));
        return true;
    }

    bool processXsAllEtAl(const String& typeName, const Xml::Element& element, List<String>& attributes)
    {
        for (List<Xml::Variant>::Iterator i = element.content.begin(), end = element.content.end(); i != end; ++i)
        {
            const Xml::Variant& variant = *i;
            if (!variant.isElement())
                continue;
            const Xml::Element& element = variant.toElement();
            if (element.type == "xs:element")
            {
                const TypeInfo* typeInfo;
                if (!processXsElement(typeName, element, typeInfo))
                    return false;

                String name = getAttribute(element, "name");
                uint minOccurs = getAttribute(element, "minOccurs", "1").toUInt();
                uint maxOccurs = getAttribute(element, "maxOccurs", "1").toUInt();

                if (minOccurs == 1 && maxOccurs == 1)
                    attributes.append(typeInfo->cppName + " " + toCppIdentifier(name));
                else if (minOccurs == 0 && maxOccurs == 1)
                    attributes.append(String("xsd::optional<") + typeInfo->cppName + "> " + toCppIdentifier(name));
                else
                    attributes.append(String("xsd::vector<") + typeInfo->cppName + "> " + toCppIdentifier(name));
            }
        }
        return true;
    }
};

}

bool generateCpp(const Xml::Element& xsd, const String& outputDir, String& error)
{
    const Xml::Element* entryPoint = findElementByType(xsd, "xs:element");
    if (!entryPoint)
        return (error = "Could not find 'xs:element' in root element"), false;

    List<String> output;
    Generator generator(xsd, output);
    const Generator::TypeInfo* typeInfo = nullptr;
    if (!generator.processXsElement(*entryPoint, typeInfo))
        return (error = generator.getError()), false;

    String rootName = getAttribute(*entryPoint, "name");

    output.append(String("typedef ") + typeInfo->cppName + " " + toCppIdentifier(rootName) + ";");
    output.append("");
    output.append(String("void load_xml(const std::string& file, ") + toCppIdentifier(rootName) + "& data);");
    output.append("");

    List<String> header;
    header.append("");
    header.append("#pragma once");
    header.append("");
    header.append("#include \"xsd.hpp\"");
    header.append("");
    output.prepend(header);

    {
        String outputFilePath = outputDir + "/" + rootName + ".hpp";
        File outputFile;
        if (!outputFile.open(outputFilePath, File::writeFlag))
            return (error = String::fromPrintf("Could not open file '%s': %s", (const char*)outputFilePath, (const char*)Error::getErrorString())), false;

        for (List<String>::Iterator i = output.begin(), end = output.end(); i != end; ++i)
            outputFile.write(*i + "\n");
    }
    {
        String outputFilePath = outputDir + "/" + rootName + ".cpp";
        File outputFile;
        if (!outputFile.open(outputFilePath, File::writeFlag))
            return (error = String::fromPrintf("Could not open file '%s': %s", (const char*)outputFilePath, (const char*)Error::getErrorString())), false;

        List<String> output;
        output.append("");
        output.append(String("#include \"") + rootName + ".hpp\"");
        output.append("");
        output.append(String("void load_xml(const std::string& file, ") + toCppIdentifier(rootName) + "& data)");
        output.append("{");
        output.append("}");
        output.append("");

        for (List<String>::Iterator i = output.begin(), end = output.end(); i != end; ++i)
            outputFile.write(*i + "\n");
    }

    {
        String outputFilePath = outputDir + "/xsd.hpp";
        File outputFile;
        if (!outputFile.open(outputFilePath, File::writeFlag))
            return (error = String::fromPrintf("Could not open file '%s': %s", (const char*)outputFilePath, (const char*)Error::getErrorString())), false;
        outputFile.write(xsd_hpp);
    }

    return true;
}
