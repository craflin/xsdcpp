
#include "Generator.hpp"

#include <nstd/Console.hpp>
#include <nstd/HashSet.hpp>
#include <nstd/File.hpp>
#include <nstd/Error.hpp>

namespace {

const Xml::Element* findElementByType(const Xml::Element& parent, const String& type)
{
    for (List<Xml::Variant>::Iterator i = parent.content.begin(), end = parent.content.end(); i != end; ++i)
    {
        const Xml::Variant& variant = *i;
        if (!variant.isElement())
            continue;

        const Xml::Element& element = variant.toElement();
        if (element.type ==  type)
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

bool generateType(const Xml::Element& xsd, const String& type, HashSet<String>& types, List<String>& output_, String& error)
{
    if (types.find(type) != types.end())
        return false;

    const Xml::Element* element = findElementByName(xsd, type);
    if (!element)
        return (error = String::fromPrintf("Could not find type '%s'", (const char*)type)), false;

    if (element->type == "xs:simpleType")
    {
        const Xml::Element* restriction = findElementByType(*element, "xs:restriction");
        if (!restriction)
            return (error = String::fromPrintf("Could not find 'xs:restriction' in '%s'", (const char*)type)), false;
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
                enumValues.append(getAttribute(element,"value"));
            }

            if (!enumValues.isEmpty())
            {
                List<String> output;
                output.append(String("enum class ") + type);
                output.append("{");
                for (List<String>::Iterator i = enumValues.begin(), end = enumValues.end(); i != end; ++i)
                    output.append(String("    ") + *i);
                output.append("};");
                output.append("");
                output_.prepend(output);
            }
        }
        else
            return (error = String::fromPrintf("xs:restriction base '%s' not supported", (const char*)base)), false;
    }

    else if (element->type == "xs:complexType")
    {
        for (List<Xml::Variant>::Iterator i = element->content.begin(), end = element->content.end(); i != end; ++i)
        {
            const Xml::Variant& variant = *i;
            if (!variant.isElement())
                continue;
            const Xml::Element& element = variant.toElement();
            // todo ?
        }
        
        List<String> output;
        output.append(String("struct ") + type);
        output.append("{");
        output.append("};");
        output.append("");
        output_.prepend(output);
    }
    else
        return (error = String::fromPrintf("Element type '%s' not supported", (const char*)type)), false;

    types.append(type);
    return true;
}

}

bool generateCpp(const Xml::Element& xsd, const String& outputDir, String& error)
{
    List<String> output;

    const Xml::Element* entryPoint = findElementByType(xsd, "xs:element");
    if (!entryPoint)
        return (error = "Could not find 'xs:element' in root element"), false;

    String rootType = getAttribute(*entryPoint, "type");
    String rootName = getAttribute(*entryPoint, "name");
    output.append(String("typedef ") + rootType + " " + rootName + ";");
    output.append("");

    HashSet<String> types;
    if (!generateType(xsd, rootType, types, output, error))
        return false;

    String outputFilePath = outputDir + "/" + rootName + ".hpp";
    File outputFile;
    if (!outputFile.open(outputFilePath, File::writeFlag))
        return (error = String::fromPrintf("Could not open file '%s': %s", (const char*)outputFilePath, (const char*)Error::getErrorString())), false;

    for (List<String>::Iterator i = output.begin(), end = output.end(); i != end; ++i)
        outputFile.write(*i + "\n");

    return true;
}
