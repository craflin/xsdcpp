
#include "Reader.hpp"

#include <nstd/Document/Xml.hpp>
#include <nstd/File.hpp>
#include <nstd/Console.hpp>
#include <nstd/HashMap.hpp>

namespace {

const Xml::Element* findElementByType2(const Xml::Element& parent, const String& type)
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

bool compareElementName(const HashMap<String, String>& namespaceToPrefixMap, const String& elementName, const String& namespace_, const String& name)
{
    HashMap<String, String>::Iterator it = namespaceToPrefixMap.find(namespace_);
    if (it == namespaceToPrefixMap.end())
        return false;
    const String& namespacePrefix = *it;
    if (namespacePrefix.isEmpty())
        return elementName == name;
    return elementName.length() == namespacePrefix.length() + 1 + name.length() && elementName.startsWith(namespacePrefix) && ((const char*)elementName)[namespacePrefix.length()] == ':' && elementName.endsWith(name);
}

class Reader
{
public:
    Reader(Xsd& output)
        : _output(output)
    {
    }

    const String& getError() const { return _error; }

    bool read(const String& file, const String& name)
    {
        _path = file;

        if (!loadXsdFile(file))
            return false;

        List<Xsd::ElementRef> elements;
        if (!process(elements))
            return false;

        if (elements.isEmpty())
            return (_error = "Root element not found"), false;

        String rootTypeName = "_root_t";

        Xsd::Type& type = _output.types.append(rootTypeName, Xsd::Type());
        type.kind = Xsd::Type::ElementKind;
        type.elements.swap(elements);

        _output.name = name;
        _output.rootType = rootTypeName;

        if (!resolveElementRefs())
            return false;

        return true;
    }

private:
    struct XsdFileData
    {
        Xml::Element xsd;
        HashMap<String, String> prefixToNamespaceMap;
        HashMap<String, String> namespaceToPrefixMap;
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

    struct Group
    {
        List<Xsd::GroupMember> members;
    };

private:
    Xsd& _output;
    String _path;
    HashMap<Namespace, NamespaceData> _namespaces;
    String _error;
    HashMap<String, Group> _groups;

private:

    bool resolveElementRefs()
    {
        for (HashMap<String, Xsd::Type>::Iterator i = _output.types.begin(), end = _output.types.end(); i != end; ++i)
        {
            Xsd::Type& type = *i;

            for (List<Xsd::ElementRef>::Iterator i = type.elements.begin(), end = type.elements.end(); i != end; ++i)
            {
                Xsd::ElementRef& elementRef = *i;

                if (!elementRef.typeName.startsWith("ref "))
                    continue;

                String refName = elementRef.typeName.substr(4);
                const char* n = refName.find(' ');
                String typeName = refName.substr(n + 1 - (const char*)refName);
                refName = refName.substr(0, n - (const char*)refName);

                elementRef.name = refName;
                elementRef.typeName = typeName;

                HashMap<String, Group>::Iterator it = _groups.find(refName);
                if (it == _groups.end())
                    continue;

                Group& group = *it;
                elementRef.groupMembers = group.members;
            }
        }
        return true;
    }

    bool loadXsdFile(const String& file)
    {
        Console::printf("Importing '%s'...\n", (const char*)file);

        Xml::Element xsd;
        Xml::Parser parser;
        if (!parser.load(file, xsd))
            return (_error = String::fromPrintf("Could not load file '%s': %s", (const char*)file, (const char*) parser.getErrorString())), false;

        String targetNamespace = getAttribute(xsd, "targetNamespace");

        _namespaces.append(targetNamespace, NamespaceData());
        NamespaceData& namespaceData = _namespaces.back();
        namespaceData.files.append(file, XsdFileData());
        XsdFileData& xsdFileData = namespaceData.files.back();
        xsdFileData.xsd = xsd; // todo: swap
        if (!loadXsdFile(namespaceData, xsdFileData))
            return false;

        HashMap<String, String>::Iterator it = xsdFileData.namespaceToPrefixMap.find("http://www.w3.org/2001/XMLSchema");
        if (it != xsdFileData.namespaceToPrefixMap.end())
            _output.xmlSchemaNamespacePrefix = *it;

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
                xsdFileData.prefixToNamespaceMap.append(prefix, *i);
                xsdFileData.namespaceToPrefixMap.append(*i, prefix);
            }
            else if (i.key() == "xmlns")
            {
                xsdFileData.prefixToNamespaceMap.append(String(), *i);
                xsdFileData.namespaceToPrefixMap.append(*i, String());
            }

        for (List<Xml::Variant>::Iterator i = position.element->content.begin(); i != position.element->content.end(); ++i)
        {
            const Xml::Variant& variant = *i;
            if (!variant.isElement())
                continue;

            const Xml::Element& element = variant.toElement();
            if (compareXsName(position, element.type, "include"))
            {
                String schemaLocation = getAttribute(element, "schemaLocation");

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
                XsdFileData& xsdFileData = namespaceData.files.back();
                xsdFileData.xsd = xsd; // todo: swap

                if (!loadXsdFile(namespaceData, xsdFileData))
                    return false;
            }
            else if (compareXsName(position, element.type, "import"))
            {
                String namespace_ = getAttribute(element, "namespace");

                if (namespace_ == "http://www.w3.org/XML/1998/namespace")
                    continue;

                if (_namespaces.contains(namespace_))
                    continue;

                String schemaLocation = getAttribute(element, "schemaLocation");

                if (!File::isAbsolutePath(schemaLocation))
                    schemaLocation = File::getDirectoryName(_path) + "/" + schemaLocation;

                if (!loadXsdFile(schemaLocation))
                    return false;
            }
        }

        return true;
    }

    bool compareXsName(const Position& position, const String& fullName, const String& name) const
    {
        return compareElementName(position.xsdFileData->namespaceToPrefixMap, fullName, "http://www.w3.org/2001/XMLSchema", name);
    }

    bool isXsStringName(const Position& position, const String& fullName)
    {
        return compareXsName(position, fullName, "normalizedString") || 
            compareXsName(position, fullName, "string") ||
            compareXsName(position, fullName, "anyURI") ||
            compareXsName(position, fullName, "NCName") ||
            compareXsName(position, fullName, "QName") ||
            fullName == "xml:lang";
    }

    Position findGlobalElementByName(const Position& position, const String& name)
    {
        String namespace_;
        String nameWithoutPrefix;
        if (const char* x = name.find(':'))
        {
            usize n = x - (const char*)name;
            String namespacePrefix = name.substr(0, n);
            nameWithoutPrefix = name.substr(n + 1);
            HashMap<String, String>::Iterator it = position.xsdFileData->prefixToNamespaceMap.find(namespacePrefix);
            if (it == position.xsdFileData->prefixToNamespaceMap.end())
                return Position();
            namespace_ = *it;
        }
        else
        {
            HashMap<String, String>::Iterator it = position.xsdFileData->prefixToNamespaceMap.find(String());
            if (it == position.xsdFileData->prefixToNamespaceMap.end())
            {
                Position result;
                result.element = findElementByName(position.xsdFileData->xsd, name);
                if (result.element)
                {
                    result.xsdFileData = position.xsdFileData;
                    return result;
                }
                return Position();
            }
            namespace_ = *it;
            nameWithoutPrefix = name;
        }
        HashMap<String, NamespaceData>::Iterator it2 = _namespaces.find(namespace_);
        if (it2 == _namespaces.end())
            return Position();
        NamespaceData& namespaceData = *it2;
        for (HashMap<String, XsdFileData>::Iterator i = namespaceData.files.begin(), end = namespaceData.files.end(); i != end; ++i)
        {
            const XsdFileData& fileData = *i;
            Position result;
            result.element = findElementByName(fileData.xsd, nameWithoutPrefix);
            if (result.element)
            {
                result.xsdFileData = &fileData;
                return result;
            }
        }
        return Position();
    }

    Position findElementByType(const Position& position, const String& namespace_, const String& type)
    {
        HashMap<String, String>::Iterator it = position.xsdFileData->namespaceToPrefixMap.find(namespace_);
        if (it == position.xsdFileData->namespaceToPrefixMap.end())
            return Position();
        const String& prefix = *it;
        Position result;
        result.element = findElementByType2(*position.element, prefix.isEmpty() ? type : prefix + ":" + type);
        if (!result.element)
            return Position();
        result.xsdFileData = position.xsdFileData;
        return result;
    }

    Position findXsElementByType(const Position& position, const String& type)
    {
        return findElementByType(position, "http://www.w3.org/2001/XMLSchema", type);
    }

    bool processXsElement(const Position& position, const String& parentTypeName, String& typeName, bool atRoot = false)
    {
        String substitutionGroup = getAttribute(*position.element, "substitutionGroup");
        if (!substitutionGroup.isEmpty())
        {
            HashMap<String, Group>::Iterator it = _groups.find(substitutionGroup);
            Group* group;
            if (it == _groups.end())
                group = &_groups.append(substitutionGroup, Group());
            else
                group = &*it;

            Xsd::GroupMember& member = group->members.append(Xsd::GroupMember());
            member.name = getAttribute(*position.element, "name");
            member.typeName = getAttribute(*position.element, "type");;

             if (!processType(position, member.typeName))
                return false;
        }

        // reference to another element
        String ref = getAttribute(*position.element, "ref");
        if (!ref.isEmpty())
        {
            if (atRoot)
                return true;
            Position refPos = findGlobalElementByName(position, ref);
            if (!refPos)
                return (_error = String::fromPrintf("Could not find type '%s'", (const char*)ref)), false;
            if (!processXsElement(refPos, String(), typeName))
                return false;
            typeName = String("ref ") + ref + " " + typeName;
            return true;
        }

        // typed element
        typeName = getAttribute(*position.element, "type");
        if (!typeName.isEmpty())
        {
            if (!processType(position, typeName))
                return false;

            if (isXsStringName(position, typeName))
            {
                String name = getAttribute(*position.element, "name");

                typeName = parentTypeName + "_" + name + "_t";

                Xsd::Type& type = _output.types.append(typeName, Xsd::Type());
                type.kind = Xsd::Type::ElementKind;
                type.baseType = "string";
                if (!_output.xmlSchemaNamespacePrefix.isEmpty())
                    type.baseType.prepend(_output.xmlSchemaNamespacePrefix + ":");
            }

            return true;
        }

        if (atRoot)
            return true;

        // inline element
        String name = getAttribute(*position.element, "name");
        if (!name.isEmpty())
        {
            typeName = parentTypeName + "_" + name + "_t";
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
                    if (!processTypeElement(childPosition, typeName))
                        return false;
                    return true;
                }
            }
            return (_error = String::fromPrintf("Could not find 'xs:element', 'xs:complexType' or 'xs:simpleType' in '%s'", (const char*)position.element->type)), false;
        }

        return (_error = String::fromPrintf("Missing element 'ref', 'type' or 'name' attribute in '%s'", (const char*)position.element->type)), false;
    }

    bool processType(const Position& position, const String& typeName)
    {
        HashMap<String, Xsd::Type>::Iterator it = _output.types.find(typeName);
        if (it != _output.types.end())
            return true;

        if (isXsStringName(position, typeName))
        {
            Xsd::Type& type = _output.types.append(typeName, Xsd::Type());
            type.kind = Xsd::Type::StringKind;
            return true;
        }

        if (compareXsName(position, typeName, "nonNegativeInteger") || compareXsName(position, typeName, "positiveInteger") || compareXsName(position, typeName, "integer") ||
            compareXsName(position, typeName, "int") || compareXsName(position, typeName, "long") || compareXsName(position, typeName, "short") ||
            compareXsName(position, typeName, "unsignedInt") || compareXsName(position, typeName, "unsignedLong") || compareXsName(position, typeName, "unsignedShort") ||
            compareXsName(position, typeName, "decimal") ||
            compareXsName(position, typeName, "double") || compareXsName(position, typeName, "float") || 
            compareXsName(position, typeName, "boolean"))
        {
            Xsd::Type& type = _output.types.append(typeName, Xsd::Type());
            type.kind = Xsd::Type::BaseKind;
            return true;
        }

        Position elementPos = findGlobalElementByName(position, typeName);
        if (!elementPos)
        {
            elementPos = findGlobalElementByName(position, typeName);
            return (_error = String::fromPrintf("Could not find type '%s'", (const char*)typeName)), false;
        }

        return processTypeElement(elementPos, typeName);
    }

    bool processTypeElement(const Position& position, const String& typeName)
    {
        const Xml::Element& element = *position.element;

        if (compareXsName(position, element.type, "simpleType"))
        {
            const Position restriction = findXsElementByType(position, "restriction");
            if (restriction)
            {
                String base = getAttribute(*restriction.element, "base");

                if (compareXsName(position, base, "normalizedString") || compareXsName(position, base, "string"))
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
                else
                {
                    Xsd::Type& type = _output.types.append(typeName, Xsd::Type());
                    type.kind = Xsd::Type::SimpleRefKind;
                    type.baseType = base;
                    return processType(position, base);
                }
                return true;
            }
            const Position union_ = findXsElementByType(position, "union");
            if (union_)
            {
                String memberTypesStr = getAttribute(*union_.element, "memberTypes");
                List<String> memberTypes;
                memberTypesStr.split(memberTypes, " ");

                Xsd::Type& type = _output.types.append(typeName, Xsd::Type());
                type.kind = Xsd::Type::UnionKind;
                type.memberTypes = memberTypes;

                for (List<String>::Iterator i = memberTypes.begin(), end = memberTypes.end(); i != end; ++i)
                    if (!processType(position, *i))
                        return false;
                return true;
            }
            const Position list = findXsElementByType(position, "list");
            if (list)
            {
                String itemTypeStr = getAttribute(*list.element, "itemType");

                Xsd::Type& type = _output.types.append(typeName, Xsd::Type());
                type.kind = Xsd::Type::ListKind;
                type.baseType = itemTypeStr;

                if (!processType(position, itemTypeStr))
                    return false;
                return true;
            }
            return (_error = String::fromPrintf("Could not find 'xs:restriction', 'xs:union', or 'xs:list' in '%s'", (const char*)typeName)), false;
        }

        else if (compareXsName(position, element.type, "complexType"))
        {
            List<Xsd::AttributeRef> attributes;
            List<Xsd::ElementRef> elements;
            String baseTypeName;

            String mixed = getAttribute(*position.element, "mixed");
            if (mixed == "true")
            {
                baseTypeName = "string";
                if (!_output.xmlSchemaNamespacePrefix.isEmpty())
                    baseTypeName.prepend(_output.xmlSchemaNamespacePrefix + ":");
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
                    if (!processXsAllEtAl(childPosition, typeName, elements))
                        return false;
                }
                else if (compareXsName(position, element.type, "choice"))
                {
                    List<Xsd::ElementRef> choiceElements;
                    if (!processXsAllEtAl(childPosition, typeName, choiceElements))
                        return false;

                    if (!choiceElements.isEmpty())
                    {
                        Xsd::ElementRef& elementRef = elements.append(Xsd::ElementRef());
                        elementRef.minOccurs = getAttribute(element, "minOccurs", "1").toUInt();
                        elementRef.maxOccurs = getAttribute(element, "maxOccurs", "1").toUInt();

                        for (List<Xsd::ElementRef>::Iterator i = choiceElements.begin(), end = choiceElements.end(); i != end; ++i)
                        {
                            const Xsd::ElementRef& choiceElement = *i;
                            Xsd::GroupMember& groupMember = elementRef.groupMembers.append(Xsd::GroupMember());
                            groupMember.name = choiceElement.name;
                            groupMember.typeName = choiceElement.typeName;
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
                            baseTypeName = getAttribute(element, "base");

                            if (!processType(position, baseTypeName))
                                return false;

                            if (compareXsName(position, element.type, "restriction"))
                                baseTypeName.clear(); // todo: I don't see what we should inherit from the baseType in this case.

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
                                    if (!processXsAllEtAl(childPosition, typeName, elements))
                                        return false;
                                }
                                else if (compareXsName(position, element.type, "choice"))
                                {
                                    List<Xsd::ElementRef> choiceElements;
                                    if (!processXsAllEtAl(childPosition, typeName, choiceElements))
                                        return false;

                                    if (!choiceElements.isEmpty())
                                    {
                                        Xsd::ElementRef& elementRef = elements.append(Xsd::ElementRef());
                                        elementRef.minOccurs = getAttribute(element, "minOccurs", "1").toUInt();
                                        elementRef.maxOccurs = getAttribute(element, "maxOccurs", "1").toUInt();

                                        for (List<Xsd::ElementRef>::Iterator i = choiceElements.begin(), end = choiceElements.end(); i != end; ++i)
                                        {
                                            const Xsd::ElementRef& choiceElement = *i;
                                            Xsd::GroupMember& groupMember = elementRef.groupMembers.append(Xsd::GroupMember());
                                            groupMember.name = choiceElement.name;
                                            groupMember.typeName = choiceElement.typeName;
                                        }
                                    }
                                }
                                else
                                    Console::printf("skipped %s\n", (const char*)element.type);
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
                    ;
                }
                else
                    Console::printf("skipped %s\n", (const char*)element.type);
            }

            Xsd::Type& type = _output.types.append(typeName, Xsd::Type());
            type.kind = Xsd::Type::ElementKind;
            type.baseType = baseTypeName;
            type.attributes.swap(attributes);
            type.elements.swap(elements);
            return true;

        }
        else if (compareXsName(position, element.type, "element"))
        {
            const Position complexType = findXsElementByType(position, "complexType");
            if (complexType)
                return processTypeElement(complexType, typeName);
            else
            {
                String typeStr = getAttribute(element, "type");
                if (!typeStr.isEmpty())
                {
                    if (!processType(position, typeStr))
                        return false;
                
                    Xsd::Type& type = _output.types.append(typeName, Xsd::Type());
                    type.kind = Xsd::Type::ElementKind;
                    type.baseType = typeStr;
                
                    return true;
                }
                return (_error = String::fromPrintf("Could not find 'xs:complexType' in '%s'", (const char*)element.type)), false;
            }
        }
        else
            return (_error = String::fromPrintf("Element type '%s' not supported", (const char*)element.type)), false;
    }

    bool processXsAttribute(const Position& position, Xsd::AttributeRef& attribute)
    {
        String ref = getAttribute(*position.element, "ref");
        if (!ref.isEmpty())
        {
            if (ref == "xml:lang")
            {
                if (!processType(position, ref))
                    return false;

                attribute.name = "lang";
                attribute.typeName = ref;
                attribute.isMandatory = false;
                return true;
            }
            const Position refAttribute = findGlobalElementByName(position, ref);
            if (!refAttribute)
                return (_error = String::fromPrintf("Could not find attribute '%s'", (const char*)ref)), false;
            return processXsAttribute(refAttribute, attribute);
        }

        String typeName = getAttribute(*position.element, "type");
        if (!typeName.isEmpty())
        {
            if (!processType(position, typeName))
                return false;
            attribute.typeName = typeName;
            attribute.name = getAttribute(*position.element, "name");
            String use = getAttribute(*position.element, "use");
            attribute.isMandatory = use == "required";
            attribute.defaultValue = getAttribute(*position.element, "default");
            return true;
        }

        String name = getAttribute(*position.element, "name");
        if (!name.isEmpty())
        {
            attribute.name = name;
            attribute.typeName = name + "_t";
            String use = getAttribute(*position.element, "use");
            attribute.isMandatory = use == "required";
            attribute.defaultValue = getAttribute(*position.element, "default");

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

            return (_error = String::fromPrintf("Could not find 'xs:complexType' or 'xs:simpleType' in '%s'", (const char*)position.element->type)), false;
        }

        return (_error = String::fromPrintf("Missing element 'ref', 'type' or 'name' attribute in '%s'", (const char*)position.element->type)), false;
    }

    bool processXsAllEtAl(const Position& position, const String& parentTypeName, List<Xsd::ElementRef>& elements)
    {
        for (List<Xml::Variant>::Iterator i = position.element->content.begin(), end = position.element->content.end(); i != end; ++i)
        {
            const Xml::Variant& variant = *i;
            if (!variant.isElement())
                continue;
            const Xml::Element& element = variant.toElement();
            if (compareXsName(position, element.type, "element"))
            {
                String typeName;
                Position elementPosition;
                elementPosition.element = &element;
                elementPosition.xsdFileData = position.xsdFileData;
                if (!processXsElement(elementPosition, parentTypeName, typeName))
                    return false;

                if (typeName.isEmpty())
                    continue;

                Xsd::ElementRef& elementRef = elements.append(Xsd::ElementRef());
                elementRef.name = getAttribute(element, "name");
                elementRef.typeName = typeName;
                // todo: get minOccurs and maxOccurs from referenced element in isRef case?
                elementRef.minOccurs = getAttribute(element, "minOccurs", "1").toUInt();
                elementRef.maxOccurs = getAttribute(element, "maxOccurs", "1").toUInt();
            }
            else if (compareXsName(position, element.type, "choice"))
            {
                Position choicePosition;
                choicePosition.element = &element;
                choicePosition.xsdFileData = position.xsdFileData;
            
                List<Xsd::ElementRef> choiceElements;
                if (!processXsAllEtAl(choicePosition, parentTypeName,  choiceElements))
                    return false;

                if (!choiceElements.isEmpty())
                {
                    Xsd::ElementRef& elementRef = elements.append(Xsd::ElementRef());
                    elementRef.minOccurs = getAttribute(element, "minOccurs", "1").toUInt();
                    elementRef.maxOccurs = getAttribute(element, "maxOccurs", "1").toUInt();

                    for (List<Xsd::ElementRef>::Iterator i = choiceElements.begin(), end = choiceElements.end(); i != end; ++i)
                    {
                        const Xsd::ElementRef& choiceElement = *i;
                        Xsd::GroupMember& groupMember = elementRef.groupMembers.append(Xsd::GroupMember());
                        groupMember.name = choiceElement.name;
                        groupMember.typeName = choiceElement.typeName;
                    }
                }
            }
            else if (compareXsName(position, element.type, "sequence"))
            {
                Position choicePosition;
                choicePosition.element = &element;
                choicePosition.xsdFileData = position.xsdFileData;
            
                if (!processXsAllEtAl(choicePosition, parentTypeName,  elements))
                    return false;
            }
            else if (compareXsName(position, element.type, "any"))
            {
                ;
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

                        String typeName;
                        if (!processXsElement(elementPosition, String(), typeName, true))
                            return false;

                        if (typeName.isEmpty())
                            continue;

                        if (&namespaceData == &*_namespaces.begin())
                        {
                            Xsd::ElementRef& elementRef = elements.append(Xsd::ElementRef());
                            elementRef.name = getAttribute(element, "name");
                            elementRef.typeName = typeName;
                            elementRef.minOccurs = 1;
                            elementRef.maxOccurs = 1;
                        }
                    }
                }
            }
        }

        return true;
    }
};

}

bool readXsd(const String& name_, const String& file, Xsd& xsd, String& error)
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
    if (!reader.read(file, name))
        return error = reader.getError(), false;

    return true;
}
