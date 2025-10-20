
#include "Reader.hpp"

#include <nstd/Document/Xml.hpp>
#include <nstd/File.hpp>
#include <nstd/Console.hpp>
#include <nstd/HashMap.hpp>
#include <nstd/Variant.hpp>

namespace {

const Xml::Element* findXmlElementByXmlType(const Xml::Element& parent, const String& type)
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

String getXmlAttribute(const Xml::Element& element, const String& name, const String& defaultValue = String())
{
    HashMap<String, String>::Iterator it = element.attributes.find(name);
    if (it == element.attributes.end())
        return defaultValue;
    return *it;
}


Variant getXmlAttributeVariant(const Xml::Element& element, const String& name, const Variant& defaultValue = Variant())
{
    HashMap<String, String>::Iterator it = element.attributes.find(name);
    if (it == element.attributes.end())
        return defaultValue;
    return *it;
}


class Reader
{
public:
    Reader(Xsd& output)
        : _output(output)
    {
    }

    const String& getError() const { return _error; }

    bool read(const String& file, const List<String>& forceTypeProcessing, const String& name)
    {
        _path = file;

        if (!loadXsdFile(file))
            return false;

        List<Xsd::ElementRef> elements;
        if (!process(elements))
            return false;

        if (!_namespaces.isEmpty())
            for (List<String>::Iterator i = forceTypeProcessing.begin(), end = forceTypeProcessing.end(); i != end; ++i)
            {
                Xsd::Name typeName;
                typeName.name = *i;
                typeName.xsdNamespace = _namespaces.begin().key();
                if (!processType(typeName))
                    return false;
            }

        if (elements.isEmpty())
            return (_error = "Root element not found"), false;

        Xsd::Name rootTypeName;
        rootTypeName.name = "_root_t";

        Xsd::Type& type = _output.types.append(rootTypeName, Xsd::Type());
        type.kind = Xsd::Type::ElementKind;
        type.elements.swap(elements);

        _output.name = name;
        _output.rootType = rootTypeName;

        resolveElementRefs();

        return true;
    }

private:
    struct XsdFileData
    {
        Xml::Element xsd;
        HashMap<String, String> prefixToNamespaceMap;
        HashMap<String, String> namespaceToPrefixMap;
        String targetNamespace;
    };

    struct NamespaceData
    {
        HashMap<String, XsdFileData> files;
    };

    typedef String Namespace;

    struct Position
    {
        const XsdFileData* xsdFileData;
        const Xml::Element* element;

        Position()
            : xsdFileData(nullptr)
            , element(nullptr)
        {
        }

        operator bool() const
        {
            return element != nullptr;
        }
    };

private:
    Xsd& _output;
    String _path;
    HashMap<Namespace, NamespaceData> _namespaces;
    String _error;

private:

    void resolveElementRefs()
    {
        for (HashMap<Xsd::Name, Xsd::Type>::Iterator i = _output.types.begin(), end = _output.types.end(); i != end; ++i)
        {
            Xsd::Type& type = *i;

            for (List<Xsd::ElementRef>::Iterator i = type.elements.begin(), end = type.elements.end(); i != end; ++i)
            {
                Xsd::ElementRef& elementRef = *i;

                if (elementRef.refName.name.isEmpty())
                    continue;

                Xsd::Name substitutionGroupTypeName = elementRef.refName;
                substitutionGroupTypeName.name.append("_group_t");

                if (_output.types.contains(substitutionGroupTypeName))
                    elementRef.typeName = substitutionGroupTypeName;
                else
                    elementRef.refName = Xsd::Name();
            }
        }
    }

    bool loadXsdFile(const String& file)
    {
        Console::printf("Importing '%s'...\n", (const char*)file);

        Xml::Element xsd;
        Xml::Parser parser;
        if (!parser.load(file, xsd))
            return (_error = String::fromPrintf("Could not load file '%s': %s", (const char*)file, (const char*) parser.getErrorString())), false;

        String targetNamespace = getXmlAttribute(xsd, "targetNamespace");
        _output.targetNamespaces.append(targetNamespace);

        _namespaces.append(targetNamespace, NamespaceData());
        NamespaceData& namespaceData = _namespaces.back();
        namespaceData.files.append(file, XsdFileData());
        XsdFileData& xsdFileData = namespaceData.files.back();
        xsdFileData.xsd = xsd;
        xsdFileData.targetNamespace = targetNamespace;
        if (!loadXsdFile(namespaceData, xsdFileData))
            return false;

        return true;
    }

    bool loadXsdFile(NamespaceData& namespaceData, XsdFileData& xsdFileData)
    {
        Position position;
        position.element = &xsdFileData.xsd;
        position.xsdFileData = &xsdFileData;

        for (HashMap<String, String>::Iterator i = position.element->attributes.begin(), end = position.element->attributes.end(); i != end; ++i)
            if (i.key().startsWith("xmlns:"))
            {
                String prefix = i.key().substr(6);
                const String& namespace_ = *i;
                xsdFileData.prefixToNamespaceMap.append(prefix, namespace_);
                xsdFileData.namespaceToPrefixMap.append(namespace_, prefix);
                if (_output.namespaceToSuggestedPrefix.find(namespace_) == _output.namespaceToSuggestedPrefix.end())
                    _output.namespaceToSuggestedPrefix.append(namespace_, prefix);
            }
            else if (i.key() == "xmlns")
            {
                const String& namespace_ = *i;
                xsdFileData.prefixToNamespaceMap.append(String(), namespace_);
                xsdFileData.namespaceToPrefixMap.append(namespace_, String());
            }

        for (List<Xml::Variant>::Iterator i = position.element->content.begin(); i != position.element->content.end(); ++i)
        {
            const Xml::Variant& variant = *i;
            if (!variant.isElement())
                continue;

            const Xml::Element& element = variant.toElement();
            if (compareXsName(position, element.type, "include"))
            {
                String schemaLocation = getXmlAttribute(element, "schemaLocation");

                if (!File::isAbsolutePath(schemaLocation))
                    schemaLocation = File::getDirectoryName(_path) + "/" + schemaLocation;

                schemaLocation = File::simplifyPath(schemaLocation);
                if (namespaceData.files.find(schemaLocation) != namespaceData.files.end())
                    continue; // already included

                Console::printf("Including '%s'...\n", (const char*)schemaLocation);

                Xml::Element xsd;
                Xml::Parser parser;
                if (!parser.load(schemaLocation, xsd))
                    return (_error = String::fromPrintf("Could not load file '%s': %s", (const char*)schemaLocation, (const char*) parser.getErrorString())), false;

                namespaceData.files.append(schemaLocation, XsdFileData());
                XsdFileData& includedXsdFileData = namespaceData.files.back();
                includedXsdFileData.namespaceToPrefixMap = xsdFileData.namespaceToPrefixMap;
                includedXsdFileData.prefixToNamespaceMap = xsdFileData.prefixToNamespaceMap;
                includedXsdFileData.targetNamespace = includedXsdFileData.targetNamespace;
                includedXsdFileData.xsd = xsd;

                if (!loadXsdFile(namespaceData, includedXsdFileData))
                    return false;
            }
            else if (compareXsName(position, element.type, "import"))
            {
                String namespace_ = getXmlAttribute(element, "namespace");

                if (namespace_ == "http://www.w3.org/XML/1998/namespace")
                {
                    xsdFileData.prefixToNamespaceMap.append("xml", namespace_);
                    xsdFileData.namespaceToPrefixMap.append(namespace_, "xml");
                    _output.namespaceToSuggestedPrefix.append(namespace_, "xml");
                    continue;
                }

                if (_namespaces.contains(namespace_))
                    continue;

                String schemaLocation = getXmlAttribute(element, "schemaLocation");
                if (schemaLocation.startsWith("platform:"))
                    schemaLocation = File::getBaseName(schemaLocation); // assume all XSD files can be found in the same folder

                if (!File::isAbsolutePath(schemaLocation))
                    schemaLocation = File::getDirectoryName(_path) + "/" + schemaLocation;

                if (!loadXsdFile(schemaLocation))
                    return false;
            }
        }

        return true;
    }

    static bool compareXsName(const Position& position, const String& nameWithNamespacePrefix, const String& rh)
    {
        const char* n = nameWithNamespacePrefix.find(':');
        if (n)
        {
            String namespacePrefix = nameWithNamespacePrefix.substr(0, n - (const char*)nameWithNamespacePrefix);
            HashMap<String, String>::Iterator it = position.xsdFileData->prefixToNamespaceMap.find(namespacePrefix);
            if (it == position.xsdFileData->prefixToNamespaceMap.end())
                return false;
            return *it == "http://www.w3.org/2001/XMLSchema" && String::compare(n + 1, (const char*)rh) == 0;
        }
        else
        {
            HashMap<String, String>::Iterator it = position.xsdFileData->prefixToNamespaceMap.find(String());
            if (it == position.xsdFileData->prefixToNamespaceMap.end())
                return false;
            return *it == "http://www.w3.org/2001/XMLSchema" && nameWithNamespacePrefix == rh;
        }
    }

    static bool compareXsName(const Xsd::Name& name, const String& rh)
    {
        return name.xsdNamespace == "http://www.w3.org/2001/XMLSchema" && name.name == rh;
    }

    static bool isXsStringBaseType(const Xsd::Name& typeName)
    {
        if (typeName.xsdNamespace == "http://www.w3.org/2001/XMLSchema")
            return !isXsNumericBaseType(typeName);
        else if(typeName.xsdNamespace == "http://www.w3.org/XML/1998/namespace")
            return typeName.name == "lang";
        return false;
    }

    static bool isXsNumericBaseType(const Xsd::Name& typeName)
    {
        if (typeName.xsdNamespace == "http://www.w3.org/2001/XMLSchema")
            return typeName.name == "nonNegativeInteger" || typeName.name == "positiveInteger" || typeName.name == "integer" ||
                typeName.name == "int" || typeName.name == "long" || typeName.name == "short" ||
                typeName.name == "unsignedInt" || typeName.name == "unsignedLong" || typeName.name == "unsignedShort" ||
                typeName.name == "decimal" ||
                typeName.name == "double" || typeName.name == "float" || 
                typeName.name == "boolean";
        return false;
    }

    Position findGlobalTypeByName(const Xsd::Name& name)
    {
        HashMap<String, NamespaceData>::Iterator it2 = _namespaces.find(name.xsdNamespace);
        if (it2 == _namespaces.end())
            return Position();
        NamespaceData& namespaceData = *it2;
        for (HashMap<String, XsdFileData>::Iterator i = namespaceData.files.begin(), end = namespaceData.files.end(); i != end; ++i)
        {
            const XsdFileData& fileData = *i;

            for (List<Xml::Variant>::Iterator i = fileData.xsd.content.begin(), end = fileData.xsd.content.end(); i != end; ++i)
            {
                const Xml::Variant& variant = *i;
                if (!variant.isElement())
                    continue;
                const Xml::Element& element = variant.toElement();
                if (getXmlAttribute(element, "name") == name.name)
                {
                    Position result;
                    result.element = &element;
                    result.xsdFileData = &fileData;
                    if (!compareXsName(result, element.type, "element"))
                        return result;
                }
            }
        }
        return Position();
    }

    Position findGlobalRefByName(const String& xsElementNameWithoutPrefix, const Xsd::Name& name)
    {
        HashMap<String, NamespaceData>::Iterator it2 = _namespaces.find(name.xsdNamespace);
        if (it2 == _namespaces.end())
            return Position();
        NamespaceData& namespaceData = *it2;
        for (HashMap<String, XsdFileData>::Iterator i = namespaceData.files.begin(), end = namespaceData.files.end(); i != end; ++i)
        {
            const XsdFileData& fileData = *i;

            for (List<Xml::Variant>::Iterator i = fileData.xsd.content.begin(), end = fileData.xsd.content.end(); i != end; ++i)
            {
                const Xml::Variant& variant = *i;
                if (!variant.isElement())
                    continue;
                const Xml::Element& element = variant.toElement();
                if (getXmlAttribute(element, "name") == name.name)
                {
                    Position result;
                    result.element = &element;
                    result.xsdFileData = &fileData;
                    if (compareXsName(result, element.type, xsElementNameWithoutPrefix))
                        return result;
                }
            }
        }
        return Position();
    }

    Position findXmlElementByNamespaceAndXmlType(const Position& position, const String& namespace_, const String& type)
    {
        HashMap<String, String>::Iterator it = position.xsdFileData->namespaceToPrefixMap.find(namespace_);
        if (it == position.xsdFileData->namespaceToPrefixMap.end())
            return Position();
        const String& prefix = *it;
        Position result;
        result.element = findXmlElementByXmlType(*position.element, prefix.isEmpty() ? type : prefix + ":" + type);
        if (!result.element)
            return Position();
        result.xsdFileData = position.xsdFileData;
        return result;
    }

    Position findXsElementByXmlType(const Position& position, const String& type)
    {
        return findXmlElementByNamespaceAndXmlType(position, "http://www.w3.org/2001/XMLSchema", type);
    }

    bool resolveNamespacePrefix(const Position& position, const String& typeNameWithNamespacePrefix, Xsd::Name& result)
    {
        const char* n = typeNameWithNamespacePrefix.find(':');
        if (n)
        {
            String namespacePrefix = typeNameWithNamespacePrefix.substr(0, n - (const char*)typeNameWithNamespacePrefix);
            HashMap<String, String>::Iterator it = position.xsdFileData->prefixToNamespaceMap.find(namespacePrefix);
            if (it == position.xsdFileData->prefixToNamespaceMap.end())
                return (_error = String::fromPrintf("Could not resolve namespace prefix '%s'", (const char*)namespacePrefix)), false;
            result.xsdNamespace = *it;
            result.name = typeNameWithNamespacePrefix.substr(namespacePrefix.length() + 1);
        }
        else
        {
            HashMap<String, String>::Iterator it = position.xsdFileData->prefixToNamespaceMap.find(String());
            if (it == position.xsdFileData->prefixToNamespaceMap.end())
                result.xsdNamespace.clear();
            else
                result.xsdNamespace = *it;
            result.name = typeNameWithNamespacePrefix;
        }
        return true;
    }

    bool processXsElement(const Position& position, const Xsd::Name& parentTypeName, Xsd::ElementRef& elementRef, bool atRoot = false)
    {
        // reference to another element
        String refWithNamespacePrefix = getXmlAttribute(*position.element, "ref");
        if (!refWithNamespacePrefix.isEmpty())
        {
            if (atRoot)
                return true;

            Xsd::Name refName;
            if (!resolveNamespacePrefix(position, refWithNamespacePrefix, refName))
                return false;

            Position refPos = findGlobalRefByName("element", refName);
            if (!refPos)
                return (_error = String::fromPrintf("Could not find ref '%s'", (const char*)refName.name)), false;

            if (!processXsElement(refPos, Xsd::Name(), elementRef, atRoot))
                return false;

            elementRef.minOccurs = getXmlAttribute(*position.element, "minOccurs", "1").toUInt();
            elementRef.maxOccurs = getXmlAttribute(*position.element, "maxOccurs", "1").toUInt();
            elementRef.refName = refName;
            return true;
        }

        // typed element
        String typeNameWithNamespacePrefix = getXmlAttribute(*position.element, "type");
        if (!typeNameWithNamespacePrefix.isEmpty())
        {
            if (!resolveNamespacePrefix(position, typeNameWithNamespacePrefix, elementRef.typeName))
                return false;

            if (!processType(elementRef.typeName))
                return false;

            if (isXsStringBaseType(elementRef.typeName))
            {
                String name = getXmlAttribute(*position.element, "name");

                elementRef.typeName.name = parentTypeName.name + "_" + name + "_t";
                elementRef.typeName.xsdNamespace = position.xsdFileData->targetNamespace;

                Xsd::Type& type = _output.types.append(elementRef.typeName, Xsd::Type());
                type.kind = Xsd::Type::ElementKind;
                type.baseType.name = "string";
                type.baseType.xsdNamespace = "http://www.w3.org/2001/XMLSchema";
            }

            elementRef.name.name = getXmlAttribute(*position.element, "name");
            elementRef.name.xsdNamespace = position.xsdFileData->targetNamespace;
            elementRef.minOccurs = getXmlAttribute(*position.element, "minOccurs", "1").toUInt();
            elementRef.maxOccurs = getXmlAttribute(*position.element, "maxOccurs", "1").toUInt();

            // add the type to its substitution group
            String substitutionGroupWithNamespacePrefix = getXmlAttribute(*position.element, "substitutionGroup");
            if (!substitutionGroupWithNamespacePrefix.isEmpty())
            {
                Xsd::Name substitutionGroup;
                if (!resolveNamespacePrefix(position, substitutionGroupWithNamespacePrefix, substitutionGroup))
                    return false;

                Xsd::Name substitutionGroupTypeName = substitutionGroup;
                substitutionGroupTypeName.name.append("_group_t");

                Xsd::Type& type  = _output.types.append(substitutionGroupTypeName, Xsd::Type(), false);
                

                bool addNewGroupMember = true;
                for (List<Xsd::ElementRef>::Iterator i = type.elements.begin(), end = type.elements.end(); i != end; ++i)
                {
                    const Xsd::ElementRef& elementRefInGroup = *i;
                    if (elementRefInGroup.name == elementRef.name)
                    {
                        addNewGroupMember = false;
                        break;
                    }
                }

                if (addNewGroupMember)
                {
                    type.kind = Xsd::Type::Kind::SubstitutionGroupKind;
                    Xsd::ElementRef& elementRefInGroup = type.elements.append(Xsd::ElementRef());
                    elementRefInGroup.name = elementRef.name;
                    elementRefInGroup.typeName = elementRef.typeName;
                    elementRefInGroup.minOccurs = 0;
                }
            }

            return true;
        }

        if (atRoot)
            return true;

        // inline element
        String name = getXmlAttribute(*position.element, "name");
        if (!name.isEmpty())
        {
            elementRef.typeName.name = parentTypeName.name + "_" + name + "_t";
            elementRef.typeName.xsdNamespace = position.xsdFileData->targetNamespace;

            for (List<Xml::Variant>::Iterator i = position.element->content.begin(), end = position.element->content.end(); i != end; ++i)
            {
                const Xml::Variant& variant = *i;
                if (!variant.isElement())
                    continue;

                const Xml::Element& element = variant.toElement();
                Position childPosition;
                childPosition.element = &element;
                childPosition.xsdFileData = position.xsdFileData;
                if (compareXsName(position, element.type, "element") || compareXsName(position, element.type, "complexType") || compareXsName(position, element.type, "simpleType"))
                {
                    if (!processTypeElement(childPosition, elementRef.typeName))
                        return false;

                    elementRef.name.name = getXmlAttribute(*position.element, "name");
                    elementRef.name.xsdNamespace = position.xsdFileData->targetNamespace;
                    elementRef.minOccurs = getXmlAttribute(*position.element, "minOccurs", "1").toUInt();
                    elementRef.maxOccurs = getXmlAttribute(*position.element, "maxOccurs", "1").toUInt();
                    return true;
                }
            }
            return (_error = String::fromPrintf("Could not find 'element', 'complexType' or 'simpleType' in '%s'", (const char*)position.element->type)), false;
        }

        return (_error = String::fromPrintf("Missing element 'ref', 'type' or 'name' attribute in '%s'", (const char*)position.element->type)), false;
    }

    bool processType(const Xsd::Name& typeName)
    {
        HashMap<Xsd::Name, Xsd::Type>::Iterator it = _output.types.find(typeName);
        if (it != _output.types.end())
            return true;

        if (isXsStringBaseType(typeName))
        {
            Xsd::Type& type = _output.types.append(typeName, Xsd::Type());
            type.kind = Xsd::Type::StringKind;
            return true;
        }

        if (isXsNumericBaseType(typeName))
        {
            Xsd::Type& type = _output.types.append(typeName, Xsd::Type());
            type.kind = Xsd::Type::BaseKind;
            return true;
        }

        Position elementPos = findGlobalTypeByName(typeName);
        if (!elementPos)
            return (_error = String::fromPrintf("Could not find type '%s'", (const char*)typeName.name)), false;

        return processTypeElement(elementPos, typeName);
    }

    bool processTypeElement(const Position& position, const Xsd::Name& typeName)
    {
        const Xml::Element& element = *position.element;

        if (compareXsName(position, element.type, "simpleType"))
        {
            const Position restriction = findXsElementByXmlType(position, "restriction");
            if (restriction)
            {
                String baseWithNamespacePrefix = getXmlAttribute(*restriction.element, "base");
                Xsd::Name base;

                if (!resolveNamespacePrefix(position, baseWithNamespacePrefix, base))
                    return false;

                if (compareXsName(base, "normalizedString") || compareXsName(base, "string"))
                {
                    List<String> enumEntries;
                    for (List<Xml::Variant>::Iterator i = restriction.element->content.begin(), end = restriction.element->content.end(); i != end; ++i)
                    {
                        const Xml::Variant& variant = *i;
                        if (!variant.isElement())
                            continue;
                        const Xml::Element& element = variant.toElement();
                        if (!compareXsName(position, element.type, "enumeration"))
                            continue;
                        enumEntries.append(getXmlAttribute(element, "value"));
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
                else
                {
                    Xsd::Type& type = _output.types.append(typeName, Xsd::Type());
                    type.kind = Xsd::Type::SimpleRefKind;
                    type.baseType = base;
                    return processType(base);
                }
                return true;
            }
            const Position union_ = findXsElementByXmlType(position, "union");
            if (union_)
            {
                String memberTypesStr = getXmlAttribute(*union_.element, "memberTypes");
                List<String> memberTypesStrList;
                memberTypesStr.split(memberTypesStrList, " ");
                List<Xsd::Name> memberTypes;
                for (List<String>::Iterator i = memberTypesStrList.begin(), end = memberTypesStrList.end(); i != end; ++i)
                {
                    memberTypes.append(Xsd::Name());
                    if (!resolveNamespacePrefix(position, *i, memberTypes.back()))
                        return false;
                }

                Xsd::Type& type = _output.types.append(typeName, Xsd::Type());
                type.kind = Xsd::Type::UnionKind;
                type.memberTypes = memberTypes;

                for (List<Xsd::Name>::Iterator i = memberTypes.begin(), end = memberTypes.end(); i != end; ++i)
                    if (!processType(*i))
                        return false;
                return true;
            }
            const Position list = findXsElementByXmlType(position, "list");
            if (list)
            {
                Xsd::Type& type = _output.types.append(typeName, Xsd::Type());
                type.kind = Xsd::Type::ListKind;

                String itemTypeStr = getXmlAttribute(*list.element, "itemType");
                if (itemTypeStr.isEmpty())
                {
                    const Position simpleType = findXsElementByXmlType(list, "simpleType");
                    if (simpleType)
                    {
                        type.baseType = typeName;
                        type.baseType.name += "_item_t";
                        if (!processTypeElement(simpleType, type.baseType))
                            return false;
                    }
                    else
                    {
                        type.baseType.name  = "anySimpleType";
                        type.baseType.xsdNamespace = "http://www.w3.org/2001/XMLSchema";
                    }
                }
                else if (!resolveNamespacePrefix(position, itemTypeStr, type.baseType))
                    return false;

                if (!processType(type.baseType))
                    return false;
                return true;
            }
            return (_error = String::fromPrintf("Could not find 'restriction', 'union', or 'list' in '%s'", (const char*)typeName.name)), false;
        }

        else if (compareXsName(position, element.type, "complexType"))
        {
            Xsd::Type& type = _output.types.append(typeName, Xsd::Type());
            type.kind = Xsd::Type::ElementKind;

            List<Xsd::AttributeRef> attributes;
            List<Xsd::ElementRef> elements;
            Xsd::Name baseTypeName;
            uint32 flags = 0;

            String mixed = getXmlAttribute(*position.element, "mixed");
            if (mixed == "true")
            {
                baseTypeName.name = "string";
                baseTypeName.xsdNamespace = "http://www.w3.org/2001/XMLSchema";
            }

            for (List<Xml::Variant>::Iterator i = element.content.begin(), end = element.content.end(); i != end; ++i)
            {
                const Xml::Variant& variant = *i;
                if (!variant.isElement())
                    continue;
                const Xml::Element& element = variant.toElement();
                Position childPosition;
                childPosition.element = &element;
                childPosition.xsdFileData = position.xsdFileData;
                if (compareXsName(position, element.type, "attribute"))
                {
                    Xsd::AttributeRef& attribute = attributes.append(Xsd::AttributeRef());
                    if (!processXsAttribute(childPosition, attribute))
                        return false;
                }
                else if (compareXsName(position, element.type, "all") || compareXsName(position, element.type, "sequence"))
                {
                    if (!processXsAllEtAl(childPosition, typeName, elements, flags))
                        return false;
                }
                else if (compareXsName(position, element.type, "choice"))
                {
                    List<Xsd::ElementRef> choiceElements;
                    uint32 _;
                    if (!processXsAllEtAl(childPosition, typeName, choiceElements, _))
                        return false;

                    if (!choiceElements.isEmpty())
                    {
                        uint minOccurs = getXmlAttribute(element, "minOccurs", "1").toUInt();
                        uint maxOccurs = getXmlAttribute(element, "maxOccurs", "1").toUInt();

                        for (List<Xsd::ElementRef>::Iterator i = choiceElements.begin(), end = choiceElements.end(); i != end; ++i)
                        {
                            const Xsd::ElementRef& choiceElement = *i;
                            Xsd::ElementRef& elementRef = elements.append(choiceElement);
                            //elementRef.minOccurs = minOccurs; // todo: skip this if min/max was not actually set in choiceElement?
                            elementRef.minOccurs = 0;
                            elementRef.maxOccurs = maxOccurs;
                        }
                    }
                }
                else if (compareXsName(position, element.type, "complexContent") || compareXsName(position, element.type, "simpleContent"))
                {
                    for (List<Xml::Variant>::Iterator i = element.content.begin(), end = element.content.end(); i != end; ++i)
                    {
                        const Xml::Variant& variant = *i;
                        if (!variant.isElement())
                            continue;
                        const Xml::Element& element = variant.toElement();
                        if (compareXsName(position, element.type, "extension") || compareXsName(position, element.type, "restriction"))
                        {
                            if (!resolveNamespacePrefix(position, getXmlAttribute(element, "base"), baseTypeName))
                                return false;

                            if (!processType(baseTypeName))
                                return false;

                            if (!compareXsName(position, element.type, "restriction")) // todo: proper handling of restrictions
                            {
                                for (List<Xml::Variant>::Iterator i = element.content.begin(), end = element.content.end(); i != end; ++i)
                                {
                                    const Xml::Variant& variant = *i;
                                    if (!variant.isElement())
                                        continue;
                                    const Xml::Element& element = variant.toElement();
                                    Position childPosition;
                                    childPosition.element = &element;
                                    childPosition.xsdFileData = position.xsdFileData;

                                    if (compareXsName(position, element.type, "attribute"))
                                    {
                                        Xsd::AttributeRef& attribute = attributes.append(Xsd::AttributeRef());
                                        if (!processXsAttribute(childPosition, attribute))
                                            return false;
                                    }
                                    else if (compareXsName(position, element.type, "all") || compareXsName(position, element.type, "sequence"))
                                    {
                                        if (!processXsAllEtAl(childPosition, typeName, elements, flags))
                                            return false;
                                    }
                                    else if (compareXsName(position, element.type, "choice"))
                                    {
                                        List<Xsd::ElementRef> choiceElements;
                                        uint32 _;
                                        if (!processXsAllEtAl(childPosition, typeName, choiceElements, _))
                                            return false;

                                        if (!choiceElements.isEmpty())
                                        {
                                            uint minOccurs = getXmlAttribute(element, "minOccurs", "1").toUInt();
                                            uint maxOccurs = getXmlAttribute(element, "maxOccurs", "1").toUInt();

                                            for (List<Xsd::ElementRef>::Iterator i = choiceElements.begin(), end = choiceElements.end(); i != end; ++i)
                                            {
                                                const Xsd::ElementRef& choiceElement = *i;
                                                Xsd::ElementRef& elementRef = elements.append(choiceElement);
                                                //elementRef.minOccurs = minOccurs;  // todo: skip this if min/max was set actually set in choiceElement?
                                                elementRef.minOccurs  = 0;
                                                elementRef.maxOccurs = maxOccurs;
                                            }
                                        }
                                    }
                                    else
                                        Console::printf("skipped %s\n", (const char*)element.type);
                                }
                            }
                        }
                    }
                }
                else if (compareXsName(position, element.type, "annotation"))
                {
                    ;
                }
                else if (compareXsName(position, element.type, "anyAttribute"))
                {
                    flags |= Xsd::Type::AnyAttributeFlag;
                }
                else
                    Console::printf("skipped %s\n", (const char*)element.type);
            }

            type.baseType = baseTypeName;
            type.attributes.swap(attributes);
            type.elements.swap(elements);
            type.flags = flags;
            return true;

        }
        else if (compareXsName(position, element.type, "element"))
        {
            const Position complexType = findXsElementByXmlType(position, "complexType");
            if (complexType)
                return processTypeElement(complexType, typeName);
            else
            {
                String typeStr = getXmlAttribute(element, "type");
                if (!typeStr.isEmpty())
                {
                    Xsd::Name baseType;
                    if (!resolveNamespacePrefix(position, typeStr, baseType))
                        return false;

                    Xsd::Type& type = _output.types.append(typeName, Xsd::Type());
                    type.kind = Xsd::Type::ElementKind;

                    if (!processType(baseType))
                        return false;
                
                    type.baseType = baseType;
                
                    return true;
                }
                return (_error = String::fromPrintf("Could not find 'complexType' in '%s'", (const char*)element.type)), false;
            }
        }
        else
            return (_error = String::fromPrintf("Element type '%s' not supported", (const char*)element.type)), false;
    }

    bool processXsAttribute(const Position& position, Xsd::AttributeRef& attribute)
    {
        String ref = getXmlAttribute(*position.element, "ref");
        if (!ref.isEmpty())
        {
            Xsd::Name refName;
            if (!resolveNamespacePrefix(position, ref, refName))
                return false;

            if (refName.name == "lang" && refName.xsdNamespace == "http://www.w3.org/XML/1998/namespace")
            {
                if (!processType(refName))
                    return false;

                attribute.name.name = "lang";
                attribute.name.xsdNamespace = position.xsdFileData->targetNamespace;
                attribute.typeName = refName;
                attribute.isMandatory = false;
                return true;
            }

            const Position refAttribute = findGlobalRefByName("attribute", refName);
            if (!refAttribute)
                return (_error = String::fromPrintf("Could not find attribute '%s'", (const char*)refName.name)), false;
            return processXsAttribute(refAttribute, attribute);
        }

        String typeName = getXmlAttribute(*position.element, "type");
        if (!typeName.isEmpty())
        {
            Xsd::Name typeNameResolved;
            if (!resolveNamespacePrefix(position, typeName, typeNameResolved))
                return false;

            if (!processType(typeNameResolved))
                return false;
            attribute.typeName = typeNameResolved;
            attribute.name.name = getXmlAttribute(*position.element, "name");
            attribute.name.xsdNamespace = position.xsdFileData->targetNamespace;
            String use = getXmlAttribute(*position.element, "use");
            attribute.isMandatory = use == "required";
            attribute.defaultValue = getXmlAttributeVariant(*position.element, "default");
            return true;
        }

        String name = getXmlAttribute(*position.element, "name");
        if (!name.isEmpty())
        {
            attribute.name.name = name;
            attribute.name.xsdNamespace = position.xsdFileData->targetNamespace;
            attribute.typeName.name = name + "_t";
            attribute.typeName.xsdNamespace = position.xsdFileData->targetNamespace;
            String use = getXmlAttribute(*position.element, "use");
            attribute.isMandatory = use == "required";
            attribute.defaultValue = getXmlAttributeVariant(*position.element, "default");

            for (List<Xml::Variant>::Iterator i = position.element->content.begin(), end = position.element->content.end(); i != end; ++i)
            {
                const Xml::Variant& variant = *i;
                if (!variant.isElement())
                    continue;

                const Xml::Element& element = variant.toElement();
                if (compareXsName(position, element.type, "complexType") || compareXsName(position, element.type, "simpleType"))
                {
                    Position elementPosition;
                    elementPosition.element = &element;
                    elementPosition.xsdFileData = position.xsdFileData;
                    if (!processTypeElement(elementPosition, attribute.typeName))
                        return false;
                    return true;
                }
            }

            return (_error = String::fromPrintf("Could not find 'complexType' or 'simpleType' in '%s'", (const char*)position.element->type)), false;
        }

        return (_error = String::fromPrintf("Missing element 'ref', 'type' or 'name' attribute in '%s'", (const char*)position.element->type)), false;
    }

    bool processXsAllEtAl(const Position& position, const Xsd::Name& parentTypeName, List<Xsd::ElementRef>& elements, uint32& flags)
    {
        for (List<Xml::Variant>::Iterator i = position.element->content.begin(), end = position.element->content.end(); i != end; ++i)
        {
            const Xml::Variant& variant = *i;
            if (!variant.isElement())
                continue;
            const Xml::Element& element = variant.toElement();
            if (compareXsName(position, element.type, "element"))
            {
                Position elementPosition;
                elementPosition.element = &element;
                elementPosition.xsdFileData = position.xsdFileData;
                Xsd::ElementRef elementRef;
                if (!processXsElement(elementPosition, parentTypeName, elementRef))
                    return false;

                if (elementRef.name.name.isEmpty() || elementRef.typeName.name.isEmpty())
                    continue;

                elements.append(elementRef);
            }
            else if (compareXsName(position, element.type, "choice"))
            {
                Position choicePosition;
                choicePosition.element = &element;
                choicePosition.xsdFileData = position.xsdFileData;
            
                List<Xsd::ElementRef> choiceElements;
                uint32 _;
                if (!processXsAllEtAl(choicePosition, parentTypeName,  choiceElements, _))
                    return false;

                if (!choiceElements.isEmpty())
                {
                    uint minOccurs = getXmlAttribute(element, "minOccurs", getXmlAttribute(*position.element, "minOccurs", "1")).toUInt();
                    uint maxOccurs = getXmlAttribute(element, "maxOccurs", getXmlAttribute(*position.element, "maxOccurs", "1")).toUInt();
                    
                    for (List<Xsd::ElementRef>::Iterator i = choiceElements.begin(), end = choiceElements.end(); i != end; ++i)
                    {
                        const Xsd::ElementRef& choiceElement = *i;
                        Xsd::ElementRef& elementRef = elements.append(choiceElement);
                        //elementRef.minOccurs = minOccurs; // todo: skip this if min/max was set actually set in choiceElement?
                        elementRef.minOccurs = 0;
                        elementRef.maxOccurs = maxOccurs;
                    }
                }
            }
            else if (compareXsName(position, element.type, "sequence"))
            {
                Position choicePosition;
                choicePosition.element = &element;
                choicePosition.xsdFileData = position.xsdFileData;
            
                if (!processXsAllEtAl(choicePosition, parentTypeName,  elements, flags))
                    return false;
            }
            else if (compareXsName(position, element.type, "any"))
            {
                 String processContents = getXmlAttribute(element, "processContents");
                 if (processContents == "skip" || processContents == "lax")
                     flags |= Xsd::Type::SkipProcessContentsFlag;
            }
            else
                Console::printf("skipped %s\n", (const char*)element.type);
        }
        return true;
    }

    bool process(List<Xsd::ElementRef>& elements)
    {
        for (HashMap<String, NamespaceData>::Iterator i = _namespaces.begin(), end = _namespaces.end(); i != end; ++i)
        {
            NamespaceData& namespaceData = *i;
            for (HashMap<String, XsdFileData>::Iterator i = namespaceData.files.begin(), end = namespaceData.files.end(); i != end; ++i)
            {
                const XsdFileData& xsdFileData = *i;
                Position position;
                position.element = &xsdFileData.xsd;
                position.xsdFileData = &xsdFileData;

                for (List<Xml::Variant>::Iterator i = position.element->content.begin(), end = position.element->content.end(); i != end; ++i)
                {
                    const Xml::Variant& variant = *i;
                    if (!variant.isElement())
                        continue;
                    const Xml::Element& element = variant.toElement();
                    if (compareXsName(position, element.type, "element"))
                    {
                        Position elementPosition;
                        elementPosition.element = &element;
                        elementPosition.xsdFileData = position.xsdFileData;

                        Xsd::ElementRef elementRef;
                        if (!processXsElement(elementPosition, Xsd::Name(), elementRef, true))
                            return false;

                        if (elementRef.name.name.isEmpty() || elementRef.typeName.name.isEmpty())
                            continue;

                        if (&namespaceData != &*_namespaces.begin() ||
                            getXmlAttribute(element, "abstract", "false").toBool() ||
                            !getXmlAttribute(element, "substitutionGroup").isEmpty())
                            continue;

                        HashMap<Xsd::Name, Xsd::Type>::Iterator it = _output.types.find(elementRef.typeName);
                        if (it == _output.types.end())
                            continue;
                        const Xsd::Type& type = *it;
                        if (type.kind != Xsd::Type::Kind::ElementKind)
                            continue;

                        // todo: filter out elements that have been referenced from other elements?

                        elements.append(elementRef);
                    }
                }
            }
        }

        return true;
    }
};

}

bool readXsd(const String& name_, const String& file, const List<String>& forceTypeProcessing, Xsd& xsd, String& error)
{
    String name = name_;
    if (name.isEmpty())
    {
        name = File::getBaseName(file);
        const char* x = name.findLast('.');
        if (x)
            name = name.substr(0, x - (const char*)name);
    }

    Reader reader(xsd);
    if (!reader.read(file, forceTypeProcessing, name))
        return error = reader.getError(), false;

    return true;
}
