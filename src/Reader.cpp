
#include "Reader.hpp"

#include <nstd/Document/Xml.hpp>
#include <nstd/File.hpp>

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

class Reader
{
public:
    Reader(const Xml::Element& xsd, Xsd& output)
        : _xsd(xsd)
        , _output(output)
    {
    }

    const String& getError() const { return _error; }

    bool process(const String& name)
    {
        List<Xsd::ElementRef> elements;
        if (!processXsAllEtAl(String(), _xsd, elements))
            return false;

        String rootTypeName = name + "_t";

        Xsd::Type& type = _output.types.append(rootTypeName, Xsd::Type());
        type.kind = Xsd::Type::ElementKind;
        //type.attributes.swap(attributes);
        type.elements.swap(elements);


        _output.name = name;
        _output.rootType = rootTypeName;

        /*
        const Xml::Element* entryPoint = findElementByType(_xsd, "xs:element");
        if (!entryPoint)
            return (_error = "Could not find 'xs:element' in root element"), false;

        String typeName;
        if (!processXsElement(String(), *entryPoint, typeName))
            return false;

        _output.rootType = typeName;
        _output.name = getAttribute(*entryPoint, "name");
        */
        return true;
    }

private:
    const Xml::Element& _xsd;
    Xsd& _output;
    String _error;

private:
    bool processXsElement(const String& parentTypeName, const Xml::Element& element, String& typeName)
    {
        typeName = getAttribute(element, "type");

        if (typeName.isEmpty())
        { // inline type
            String name = getAttribute(element, "name");
            typeName = parentTypeName + "_" + name + "_t";

            for (List<Xml::Variant>::Iterator i = element.content.begin(), end = element.content.end(); i != end; ++i)
            {
                const Xml::Variant& variant = *i;
                if (!variant.isElement())
                    continue;
                const Xml::Element& element = variant.toElement();
                if (!processTypeElement(element, typeName))
                    return false;
                break;
            }
        }
        else
        {
            if (!processType(typeName))
                return false;
        }
        return true;
    }

    bool processType(const String& typeName)
    {
        HashMap<String, Xsd::Type>::Iterator it = _output.types.find(typeName);
        if (it != _output.types.end())
            return true;

        if (typeName == "xs:normalizedString" || typeName == "xs:string" || typeName == "xs:nonNegativeInteger" || typeName == "xs:positiveInteger")
        {
            Xsd::Type& type = _output.types.append(typeName, Xsd::Type());
            type.kind = Xsd::Type::BaseKind;
            return true;
        }

        const Xml::Element* element = findElementByName(_xsd, typeName);
        if (!element)
            return (_error = String::fromPrintf("Could not find type '%s'", (const char*)typeName)), false;

        return processTypeElement(*element, typeName);
    }

    bool processTypeElement(const Xml::Element& element, const String& typeName)
    {
        if (element.type == "xs:simpleType")
        {
            const Xml::Element* restriction = findElementByType(element, "xs:restriction");
            if (!restriction)
                return (_error = String::fromPrintf("Could not find 'xs:restriction' in '%s'", (const char*)typeName)), false;
            String base = getAttribute(*restriction, "base");

            if (base == "xs:normalizedString")
            {
                List<String> enumEntries;
                for (List<Xml::Variant>::Iterator i = restriction->content.begin(), end = restriction->content.end(); i != end; ++i)
                {
                    const Xml::Variant& variant = *i;
                    if (!variant.isElement())
                        continue;
                    const Xml::Element& element = variant.toElement();
                    if (element.type != "xs:enumeration")
                        continue;
                    enumEntries.append(getAttribute(element, "value"));
                }

                if (!enumEntries.isEmpty())
                {
                    Xsd::Type& type = _output.types.append(typeName, Xsd::Type());
                    type.kind = Xsd::Type::EnumKind;
                    type.enumEntries.swap(enumEntries);
                    return true;
                }
                else
                {
                    Xsd::Type& type = _output.types.append(typeName, Xsd::Type());
                    type.kind = Xsd::Type::StringKind;
                    // todo: pattern?
                    return true;
                }
            }
            else if (base == "xs:nonNegativeInteger")
            {
                Xsd::Type& type = _output.types.append(typeName, Xsd::Type());
                type.kind = Xsd::Type::SimpleBaseRefKind;
                type.baseType = base;
                return processType(base);
            }
            else
                return (_error = String::fromPrintf("xs:restriction base '%s' not supported", (const char*)base)), false;
        }

        else if (element.type == "xs:complexType")
        {
            List<Xsd::AttributeRef> attributes;
            List<Xsd::ElementRef> elements;
            String baseTypeName;

            for (List<Xml::Variant>::Iterator i = element.content.begin(), end = element.content.end(); i != end; ++i)
            {
                const Xml::Variant& variant = *i;
                if (!variant.isElement())
                    continue;
                const Xml::Element& element = variant.toElement();
                if (element.type == "xs:attribute")
                {
                    Xsd::AttributeRef& attribute = attributes.append(Xsd::AttributeRef());
                    if (!processXsAttribute(element, attribute))
                        return false;
                }
                else if (element.type == "xs:all" || element.type == "xs:choice" || element.type == "xs:sequence")
                {
                    if (!processXsAllEtAl(typeName, element, elements))
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
                            baseTypeName = getAttribute(element, "base");

                            if (!processType(baseTypeName))
                                return false;

                            for (List<Xml::Variant>::Iterator i = element.content.begin(), end = element.content.end(); i != end; ++i)
                            {
                                const Xml::Variant& variant = *i;
                                if (!variant.isElement())
                                    continue;
                                const Xml::Element& element = variant.toElement();
                                if (element.type == "xs:attribute")
                                {
                                    Xsd::AttributeRef& attribute = attributes.append(Xsd::AttributeRef());
                                    if (!processXsAttribute(element, attribute))
                                        return false;
                                }
                                else if (element.type == "xs:all" || element.type == "xs:choice" || element.type == "xs:sequence")
                                {
                                    if (!processXsAllEtAl(typeName, element, elements))
                                        return false;
                                }
                            }
                        }
                    }
                }
            }

            Xsd::Type& type = _output.types.append(typeName, Xsd::Type());
            type.kind = Xsd::Type::ElementKind;
            type.baseType = baseTypeName;
            type.attributes.swap(attributes);
            type.elements.swap(elements);
            return true;

        }
        else
            return (_error = String::fromPrintf("Element type '%s' not supported", (const char*)typeName)), false;
    }

    bool processXsAttribute(const Xml::Element& element, Xsd::AttributeRef& attribute)
    {
        String typeName = getAttribute(element, "type");
        if (!processType(typeName))
            return false;
        attribute.name = getAttribute(element, "name");
        attribute.typeName = typeName;
        // todo: default value
        return true;
    }

    bool processXsAllEtAl(const String& parentTypeName, const Xml::Element& element, List<Xsd::ElementRef>& elements)
    {
        for (List<Xml::Variant>::Iterator i = element.content.begin(), end = element.content.end(); i != end; ++i)
        {
            const Xml::Variant& variant = *i;
            if (!variant.isElement())
                continue;
            const Xml::Element& element = variant.toElement();
            if (element.type == "xs:element")
            {
                String typeName;
                if (!processXsElement(parentTypeName, element, typeName))
                    return false;

                Xsd::ElementRef& elementRef = elements.append(Xsd::ElementRef());
                elementRef.name = getAttribute(element, "name");
                elementRef.typeName = typeName;
                elementRef.minOccurs = getAttribute(element, "minOccurs", "1").toUInt();
                elementRef.maxOccurs = getAttribute(element, "maxOccurs", "1").toUInt();
            }
        }
        return true;
    }
};

}

bool readXsd(const String& name_, const String& file, Xsd& xsd, String& error)
{
    Xml::Element xmlXsd;
    Xml::Parser parser;
    if (!parser.load(file, xmlXsd))
        return (error = String::fromPrintf("Could not load file '%s': %s", (const char*)parser.getErrorString())), false;

    String name = name_;
    if (name.isEmpty())
        name = File::getStem(file);

    Reader reader(xmlXsd, xsd);
    if (!reader.process(name))
        return error = reader.getError(), false;

    return true;
}
