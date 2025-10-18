
#include "Generator.hpp"

#include <nstd/Console.hpp>
#include <nstd/Error.hpp>
#include <nstd/File.hpp>
#include <nstd/HashMap.hpp>
#include <nstd/Map.hpp>

#include <Resources.hpp>

namespace {

String toCppIdentifier(const String& str)
{
    String result = str;
    for (char* i = result; *i; ++i)
    {
        if (*i == '+')
            *i = 'p';
        else if (!String::isAlphanumeric(*i))
            *i = '_';
    }
    if (result.isEmpty() || String::isDigit(*(const char*)result))
        result.prepend("_");
    if (result == "union" || result == "enum" || result == "linux")
        result.append("_");
    return result;
}


String toCppFieldIdentifier(const Xsd::Name& name)
{
    return toCppIdentifier(name.name);
}

String toCStringLiteral(const String& str)
{
    String result;
    result.reserve(str.length() + 4);
    result.append("\"");
    // todo: replace stuff
    result.append(str);
    result.append("\"");
    return result;
}

bool compareXsName(const Xsd::Name& name, const String& rh)
{
    return name.namespace_ == "http://www.w3.org/2001/XMLSchema" && name.name == rh;
}

class Generator
{
public:
    Generator(const Xsd& xsd, const List<String>& externalNamespacePrefixes, const List<String>& forceTypeProcessing, List<String>& cppOutput, List<String>& hppOutput)
        : _xsd(xsd)
        , _externalNamespacePrefixes(externalNamespacePrefixes)
        , _cppOutput(cppOutput)
        , _hppOutput(hppOutput)
    {
        for (List<String>::Iterator i = forceTypeProcessing.begin(), end = forceTypeProcessing.end(); i != end; ++i)
        {
            Xsd::Name typeName;
            typeName.name = *i;
            typeName.namespace_ = _xsd.targetNamespaces.front();
            _requiredTypes.append(typeName);
        }
    }

    const String& getError() const { return _error; }

    bool process()
    {
        Map<Xsd::Name, String> elementTypes;
        Map<Xsd::Name, String> substitutionGroupTypes;
        Map<Xsd::Name, String> enumTypes;
        collectTypes(_xsd.rootType, elementTypes, substitutionGroupTypes, enumTypes);
        for (HashSet<Xsd::Name>::Iterator i = _requiredTypes.begin(), end = _requiredTypes.end(); i != end; ++i)
        {
            collectTypes(*i, elementTypes, substitutionGroupTypes, enumTypes);
            elementTypes.insert(*i, toCppTypeIdentifier2(*i));
        }

        Map<String, List<String>> externalElementTypes;
        for (Map<Xsd::Name, String>::Iterator i = elementTypes.begin(), end = elementTypes.end(); i != end; ++i)
        {
            String namespacePrefix;
            if (isNamespaceExternal(i.key().namespace_, namespacePrefix))
            {
                if (!externalElementTypes.contains(namespacePrefix))
                    externalElementTypes.insert(namespacePrefix, List<String>());
                externalElementTypes.find(namespacePrefix)->append(*i);
            }
        }
        Map<String, List<String>> externalEnumTypes;
        for (Map<Xsd::Name, String>::Iterator i = enumTypes.begin(), end = enumTypes.end(); i != end; ++i)
        {
            String namespacePrefix;
            if (isNamespaceExternal(i.key().namespace_, namespacePrefix))
            {
                if (!externalEnumTypes.contains(namespacePrefix))
                    externalEnumTypes.insert(namespacePrefix, List<String>());
                externalEnumTypes.find(namespacePrefix)->append(*i);
            }
        }


        _cppNamespace = toCppIdentifier(_xsd.name);

        _cppOutput.append("");
        _cppOutput.append(String("#include \"") + _cppNamespace + ".hpp\"");
        _cppOutput.append("");

        _cppOutput.append("namespace {");
        _cppOutput.append("");

        _cppOutput.append("const char* _namespaces[] = {");
        for (HashSet<String>::Iterator i = _xsd.targetNamespaces.begin(), end = _xsd.targetNamespaces.end(); i != end; ++i)
            _cppOutput.append(String("    ") + toCStringLiteral(*i) + ",");
        if (_xsd.targetNamespaces.find("http://www.w3.org/2001/XMLSchema-instance") == _xsd.targetNamespaces.end())
            _cppOutput.append("    \"http://www.w3.org/2001/XMLSchema-instance\",");
        if (_xsd.targetNamespaces.find("http://www.w3.org/2001/XMLSchema") == _xsd.targetNamespaces.end())
            _cppOutput.append("    \"http://www.w3.org/2001/XMLSchema\",");
        _cppOutput.append("    nullptr");
        _cppOutput.append("};");
        _cppOutput.append("");

        _cppOutput.append("}");
        _cppOutput.append("");

        for (Map<String, List<String>>::Iterator i = externalElementTypes.begin(), end = externalElementTypes.end(); i != end; ++i)
        {
            _cppOutput.append(String("namespace ") + i.key() + " {");
            _cppOutput.append("");
            const List<String>& elements = *i;
            for (List<String>::Iterator i = elements.begin(), end = elements.end(); i != end; ++i)
                _cppOutput.append(String("extern const xsdcpp::ElementInfo _") + *i + "_Info;");
            _cppOutput.append("");
            _cppOutput.append("}");
            _cppOutput.append("");
        }

        for (Map<String, List<String>>::Iterator i = externalEnumTypes.begin(), end = externalEnumTypes.end(); i != end; ++i)
        {
            _cppOutput.append(String("namespace ") + i.key() + " {");
            _cppOutput.append("");
            const List<String>& elements = *i;
            for (List<String>::Iterator i = elements.begin(), end = elements.end(); i != end; ++i)
                _cppOutput.append(String("extern const char* _") + *i + "_Values[];");
            _cppOutput.append("");
            _cppOutput.append("}");
            _cppOutput.append("");
        }

        _cppOutput.append(String("namespace ") + _cppNamespace + " {");
        _cppOutput.append("");

        _hppOutput.append("");
        _hppOutput.append("#pragma once");
        _hppOutput.append("");
        bool includeXsdHeader = true;
        for (List<String>::Iterator i = _externalNamespacePrefixes.begin(), end = _externalNamespacePrefixes.end(); i != end; ++i)
            if (*i != "xsdcpp")
            {
                _hppOutput.append(String("#include \"") + *i + ".hpp\"");
                includeXsdHeader = false;
            }

        if (includeXsdHeader)
            _hppOutput.append(String("#include \"") + _cppNamespace + "_xsd.hpp\"");
        _hppOutput.append("");

        _hppOutput.append(String("namespace ") + _cppNamespace + " {");
        _hppOutput.append("");

        for (Map<Xsd::Name, String>::Iterator i = elementTypes.begin(), end = elementTypes.end(); i != end; ++i)
        {
            const String& cppName = *i;
            if (i.key() == _xsd.rootType)
                continue;
            if (!isNamespaceExternal(i.key().namespace_))
            {
                _cppOutput.append(String("extern const xsdcpp::ElementInfo _") + cppName + "_Info;");
                if (_xsd.types.find(i.key())->kind == Xsd::Type::Kind::ElementKind)
                    _hppOutput.append(String("struct ") + cppName + ";");
            }
        }
        for (Map<Xsd::Name, String>::Iterator i = substitutionGroupTypes.begin(), end = substitutionGroupTypes.end(); i != end; ++i)
        {
            const String& cppName = *i;
            if (!isNamespaceExternal(i.key().namespace_))
                _hppOutput.append(String("struct ") + cppName + ";");
        }
        _cppOutput.append("");
        _hppOutput.append("");

        if (!processType2(_xsd.rootType, 0, true))
            return false;
        for (HashSet<Xsd::Name>::Iterator i = _requiredTypes.begin(), end = _requiredTypes.end(); i != end; ++i)
        {
            if (!generateType(*i, 2))
                return false;
            if (!generateElementInfo(*i))
                return false;
        }

        HashMap<Xsd::Name, Xsd::Type>::Iterator it = _xsd.types.find(_xsd.rootType);
        Xsd::Type& rootType = *it;

        for (List<Xsd::ElementRef>::Iterator i = rootType.elements.begin(), end = rootType.elements.end(); i != end; ++i)
        {
            String elementTypeCppName = toCppTypeIdentifier2(i->typeName);
            String elementCppName = toCppFieldIdentifier(i->name);

            _hppOutput.append(String("void load_file(const std::string& file, ") + elementTypeCppName + "& " + elementCppName + ");");
            _hppOutput.append(String("void load_data(const std::string& data, ") + elementTypeCppName + "& " + elementCppName + ");");
            _hppOutput.append("");
        }

        _hppOutput.append("}");

        _cppOutput.append("}");
        _cppOutput.append("");
        _cppOutput.append("#include <fstream>");
        _cppOutput.append("#include <system_error>");
        _cppOutput.append("#include <sstream>");
        _cppOutput.append("");

        _cppOutput.append(String("namespace ") + _cppNamespace + " {");
        _cppOutput.append("");

        String rootTypeCppName = toCppTypeIdentifier2(_xsd.rootType);
        for (List<Xsd::ElementRef>::Iterator i = rootType.elements.begin(), end = rootType.elements.end(); i != end; ++i)
        {
            String elementTypeCppName = toCppTypeIdentifier2(i->typeName);
            String elementCppName = toCppFieldIdentifier(i->name);

            _cppOutput.append(String("void load_data(const std::string& data, ") + elementTypeCppName + "& output)");
            _cppOutput.append("{");
            _cppOutput.append(String("    ") + rootTypeCppName + " rootElement;");
            _cppOutput.append("    xsdcpp::ElementContext elementContext;");
            _cppOutput.append(String("    elementContext.info = &_") + rootTypeCppName + "_Info;");
            _cppOutput.append("    elementContext.element = &rootElement;");
            _cppOutput.append("    xsdcpp::parse(data.c_str(), _namespaces, elementContext);");
            _cppOutput.append(String("    output = std::move(rootElement.") + elementCppName + ");");
            _cppOutput.append("}");
            _cppOutput.append("");

            _cppOutput.append(String("void load_file(const std::string& filePath, ") + elementTypeCppName + "& output)");
            _cppOutput.append("{");
            _cppOutput.append("    std::fstream file;");
            _cppOutput.append("    file.exceptions(std::fstream::failbit | std::fstream::badbit);");
            _cppOutput.append("    file.open(filePath, std::fstream::in);");
            _cppOutput.append("    std::stringstream buffer;");
            _cppOutput.append("    buffer << file.rdbuf();");
            _cppOutput.append("    load_data(buffer.str(), output);");
            _cppOutput.append("}");
            _cppOutput.append("");
        }

        _cppOutput.append("}");

        return true;
    }

private:
    const Xsd& _xsd;
    const List<String>& _externalNamespacePrefixes;

    HashMap<String, String> _externalNamespaces;

    List<String>& _cppOutput;
    List<String>& _hppOutput;
    String _cppNamespace;
    HashSet<Xsd::Name> _generatedTypes2;
    HashSet<Xsd::Name> _generatedElementInfos2;
    HashSet<Xsd::Name> _requiredTypes;
    String _error;

private:
    String toCppTypeIdentifier2(const Xsd::Name& typeName)
    {
        HashMap<Xsd::Name, Xsd::Type>::Iterator it = _xsd.types.find(typeName);
        if (it != _xsd.types.end())
        {
            const Xsd::Type& type = *it;
            if (type.kind == Xsd::Type::StringKind)
            {
                if (compareXsName(typeName, "normalizedString") || compareXsName(typeName, "string") || compareXsName(typeName, "anyURI"))
                    return "xsd::string";
            }

            if (type.kind == Xsd::Type::BaseKind)
            {
                if (compareXsName(typeName, "nonNegativeInteger") || compareXsName(typeName, "positiveInteger") || compareXsName(typeName, "unsignedLong"))
                    return "uint64_t";

                if (compareXsName(typeName, "integer") || compareXsName(typeName, "long"))
                    return "int64_t";

                if (compareXsName(typeName, "unsignedInt"))
                    return "uint32_t";

                if (compareXsName(typeName, "int"))
                    return "int32_t";

                if (compareXsName(typeName, "unsignedShort"))
                    return "uint16_t";

                if (compareXsName(typeName, "short"))
                    return "int16_t";

                if (compareXsName(typeName, "double") || compareXsName(typeName, "decimal"))
                    return "double" ;

                if (compareXsName(typeName, "float"))
                    return "float";

                if (compareXsName(typeName, "boolean"))
                    return "bool";

                return "??not_supported";
            }
        }

        if (typeName.namespace_ == _xsd.targetNamespaces.front() || isNamespaceExternal(typeName.namespace_))
            return toCppIdentifier(typeName.name);
        HashMap<String, String>::Iterator it2 = _xsd.namespaceToSuggestedPrefix.find(typeName.namespace_);
        if (it2 != _xsd.namespaceToSuggestedPrefix.end())
            return toCppIdentifier(*it2 + "_" + typeName.name);
        return toCppIdentifier(typeName.name);
    }

    String toCppTypeIdentifierWithNamespace2(const Xsd::Name& typeName)
    {
        String result = toCppTypeIdentifier2(typeName);
        if (result == "xsd::string" ||
            result == "uint64_t" ||
            result == "int64_t" ||
            result == "uint32_t" ||
            result == "int32_t" ||
            result == "uint16_t" ||
            result == "int16_t" ||
            result == "double" ||
            result == "float" ||
            result == "bool")
            return result;
        String namespacePrefix;
        if (isNamespaceExternal(typeName.namespace_, namespacePrefix))
            return namespacePrefix + "::" + result;
        return _cppNamespace + "::" + result;
    }

    String toCppNamespacePrefix(const Xsd::Name& typeName)
    {
        String namespacePrefix;
        if (isNamespaceExternal(typeName.namespace_, namespacePrefix))
            return namespacePrefix;
        return _cppNamespace;
    }

    usize getChildrenCount(const Xsd::Name& typeName) const
    {
        if (typeName.name.isEmpty())
            return 0;
        HashMap<Xsd::Name, Xsd::Type>::Iterator it = _xsd.types.find(typeName);
        if (it == _xsd.types.end())
            return 0;
        const Xsd::Type& type = *it;
        return type.elements.size() + getChildrenCount(type.baseType);
    }

    usize getMandatoryChildrenCount(const Xsd::Name& typeName) const
    {
        if (typeName.name.isEmpty())
            return 0;
        HashMap<Xsd::Name, Xsd::Type>::Iterator it = _xsd.types.find(typeName);
        if (it == _xsd.types.end())
            return 0;
        const Xsd::Type& type = *it;
        usize count = 0;
        for (List<Xsd::ElementRef>::Iterator i = type.elements.begin(), end = type.elements.end(); i != end; ++i)
        {
            const Xsd::ElementRef& elementRef = *i;
            if (elementRef.minOccurs)
                ++count;
        }
        return count + getMandatoryChildrenCount(type.baseType);
    }

    usize getAttributesCount(const Xsd::Name& typeName) const
    {
        if (typeName.name.isEmpty())
            return 0;
        HashMap<Xsd::Name, Xsd::Type>::Iterator it = _xsd.types.find(typeName);
        if (it == _xsd.types.end())
            return 0;
        const Xsd::Type& type = *it;
        return type.attributes.size() + getAttributesCount(type.baseType);
    }

    enum ReadTextMode
    {
        SkipMode,
        ReadAndProcessTextMode,
        SkipProcessingMode,
    };

    bool isSkipProcessContentsFlagSet(const Xsd::Name& typeName) const 
    {
        if (typeName.name.isEmpty())
            return false;
        HashMap<Xsd::Name, Xsd::Type>::Iterator it2 = _xsd.types.find(typeName);
        if (it2 == _xsd.types.end())
            return false;
        if (it2->flags & Xsd::Type::SkipProcessContentsFlag)
            return true;
        const Xsd::Type& type = *it2;
        return isSkipProcessContentsFlagSet(type.baseType);
    }

    Xsd::Name getRootTypeName(const Xsd::Name& typeName) const
    {
        HashMap<Xsd::Name, Xsd::Type>::Iterator it2 = _xsd.types.find(typeName);
        if (it2 == _xsd.types.end())
            return typeName;
        const Xsd::Type& type = *it2;
        if ((type.kind == Xsd::Type::Kind::ElementKind || type.kind == Xsd::Type::Kind::SimpleRefKind) && !type.baseType.name.isEmpty())
            return getRootTypeName(type.baseType);
        return typeName;
    }

    Xsd::Type getType(const Xsd::Name& typeName) const
    {
        HashMap<Xsd::Name, Xsd::Type>::Iterator it2 = _xsd.types.find(typeName);
        if (it2 == _xsd.types.end())
            return Xsd::Type();
        return *it2;
    }

    Xsd::Type getRootType(const Xsd::Name& typeName) const
    {
        return getType(getRootTypeName(typeName));
    }

    ReadTextMode getReadTextMode(const Xsd::Name& typeName) const
    {
        Xsd::Type rootType = getType(getRootTypeName(typeName));
        if (rootType.kind == Xsd::Type::Kind::StringKind)
        {
            if (isSkipProcessContentsFlagSet(typeName) && getChildrenCount(typeName) == 0)
                return SkipProcessingMode;
            return ReadAndProcessTextMode;
        }
        if (rootType.kind == Xsd::Type::Kind::BaseKind || rootType.kind == Xsd::Type::Kind::EnumKind || rootType.kind == Xsd::Type::Kind::ListKind)
            return ReadAndProcessTextMode;
        return SkipMode;
    }

    static String toCStringLiteral(const String& str)
    {
        String result;
        result.reserve(str.length() + 4);
        result.append("\"");
        // todo: replace stuff
        result.append(str);
        result.append("\"");
        return result;
    }

    String resolveDefaultValue(const Xsd::Name& attributeTypeName, const Xsd::Type& finalType, const String& defaultValue)
    {
        if (finalType.kind == Xsd::Type::EnumKind)
        {
            uint value = 0;
            for (List<String>::Iterator i = finalType.enumEntries.begin(), end = finalType.enumEntries.end(); i != end; ++i, ++value)
                if (*i == defaultValue)
                    return String("(") + toCppTypeIdentifierWithNamespace2(attributeTypeName) + ")"  + String::fromUInt(value);
        }
        if (finalType.kind == Xsd::Type::StringKind)
            return toCStringLiteral(defaultValue);
        return defaultValue;
    }

    void collectTypes(const Xsd::Name& typeName, Map<Xsd::Name, String>& elementTypes, Map<Xsd::Name, String>& substitutionGroupTypes, Map<Xsd::Name, String>& enumTypes)
    {
        HashMap<Xsd::Name, Xsd::Type>::Iterator it2 = _xsd.types.find(typeName);
        if (it2 == _xsd.types.end())
            return;
        Xsd::Type& type = *it2;

        if (type.kind == Xsd::Type::SubstitutionGroupKind)
        {
            if (substitutionGroupTypes.contains(typeName))
                return;

            String cppName = toCppTypeIdentifier2(typeName);
            substitutionGroupTypes.insert(typeName, cppName);

            for (List<Xsd::ElementRef>::Iterator i = type.elements.begin(), end = type.elements.end(); i != end; ++i)
                collectTypes(i->typeName, elementTypes, substitutionGroupTypes, enumTypes);
        }

        if (type.kind == Xsd::Type::ElementKind)
        {
            if (elementTypes.contains(typeName))
                return;

            String cppName = toCppTypeIdentifier2(typeName);
            elementTypes.insert(typeName, cppName);

            if (!type.baseType.name.isEmpty())
                collectTypes(type.baseType, elementTypes, substitutionGroupTypes, enumTypes);

            for (List<Xsd::ElementRef>::Iterator i = type.elements.begin(), end = type.elements.end(); i != end; ++i)
            {
                const Xsd::Name& elementTypeName = i->typeName;

                collectTypes(elementTypeName, elementTypes, substitutionGroupTypes, enumTypes);

                HashMap<Xsd::Name, Xsd::Type>::Iterator it2 = _xsd.types.find(elementTypeName);
                if (it2 != _xsd.types.end() && it2->kind != Xsd::Type::ElementKind)
                    elementTypes.insert(elementTypeName, toCppTypeIdentifier2(elementTypeName));
            }
            for (List<Xsd::AttributeRef>::Iterator i = type.attributes.begin(), end = type.attributes.end(); i != end; ++i)
                collectTypes(i->typeName, elementTypes, substitutionGroupTypes, enumTypes);
        }

        if (type.kind == Xsd::Type::EnumKind)
        {
            if (enumTypes.contains(typeName))
                return;

            String cppName = toCppTypeIdentifier2(typeName);
            enumTypes.insert(typeName, cppName);
        }

        if (type.kind == Xsd::Type::SimpleRefKind || type.kind == Xsd::Type::ListKind)
            collectTypes(type.baseType, elementTypes, substitutionGroupTypes, enumTypes);
    }

    bool isNamespaceExternal(const String& xsdNamespace, String& namespacePrefix)
    {
        HashMap<String, String>::Iterator it = _externalNamespaces.find(xsdNamespace);
        if (it != _externalNamespaces.end())
        {
            if (it->isEmpty())
                return false;
            namespacePrefix = *it;
            return true;
        }

        HashMap<String, String>::Iterator it2 = _xsd.namespaceToSuggestedPrefix.find(xsdNamespace);
        if (it2 != _xsd.namespaceToSuggestedPrefix.end())
        {
            const String& xsdNamespacePrefix = *it2;
            for (List<String>::Iterator i = _externalNamespacePrefixes.begin(), end = _externalNamespacePrefixes.end(); i != end; ++i)
            {
                if (*i == xsdNamespacePrefix)
                {
                    _externalNamespaces.append(xsdNamespace, xsdNamespacePrefix);
                    namespacePrefix = xsdNamespacePrefix;
                    return true;
                }
            }
        }

        _externalNamespaces.append(xsdNamespace, String());
        return false;
    }

    bool isNamespaceExternal(const String& xsdNamespace)
    {
        String _;
        return isNamespaceExternal(xsdNamespace, _);
    }

    bool processType2(const Xsd::Name& typeName, usize level, bool generate)
    {
        if (_generatedTypes2.contains(typeName))
            return true;

        if (isNamespaceExternal(typeName.namespace_))
        {
            _generatedTypes2.append(typeName);
            return true;
        }

        HashMap<Xsd::Name, Xsd::Type>::Iterator it2 = _xsd.types.find(typeName);
        if (it2 == _xsd.types.end())
            return _error = String::fromPrintf("Type '%s' not found", (const char*)typeName.name), false;
        Xsd::Type& type = *it2;

        if (generate || (type.kind != Xsd::Type::SubstitutionGroupKind && type.kind != Xsd::Type::ElementKind))
            return generateType(typeName, level); // create full type definition now

        _requiredTypes.append(typeName); // create full type definition later
        return true;
    }

    String generateAddTextFunction(const Xsd::Name& typeName, List<String>& flags)
    {
        ReadTextMode readTextMode = getReadTextMode(typeName);
        if (readTextMode == SkipMode)
            return "nullptr";
        flags.append("xsdcpp::ElementInfo::ReadTextFlag");
        if (readTextMode == SkipProcessingMode)
            flags.append("xsdcpp::ElementInfo::SkipProcessingFlag");

        String cppName = toCppTypeIdentifier2(typeName);
        String cppNameWithNamespace = toCppTypeIdentifierWithNamespace2(typeName);
        String addTextFunctionName = String("add_text_") + cppName;

        Xsd::Name rootTypeName = getRootTypeName(typeName);
        Xsd::Type rootType = getType(rootTypeName);
        if (rootType.kind == Xsd::Type::Kind::StringKind)
            _cppOutput.append(String("void ") + addTextFunctionName + "(" + cppNameWithNamespace + "* element, const xsdcpp::Position& pos, std::string&& text) { xsd::string& str = *element; if (str.empty()) str = std::move(text); else str += text; }");
        else if (rootType.kind == Xsd::Type::Kind::ListKind)
        {
            Xsd::Type itemType = getType(rootType.baseType);
            if (itemType.kind == Xsd::Type::Kind::StringKind)
                _cppOutput.append(String("void ") + addTextFunctionName + "(" + cppNameWithNamespace + "* element, const xsdcpp::Position& pos, std::string&& text) { const char* s = text.c_str(); std::string item; while (xsdcpp::getListItem(s, item)) element->emplace_back(std::move(item)); }");
            else
            {
                String itemTypeCppName = toCppTypeIdentifier2(rootType.baseType);
                String itemTypeCppNameWithNamespace = toCppTypeIdentifierWithNamespace2(rootType.baseType);
                if (itemType.kind == Xsd::Type::Kind::EnumKind)
                    _cppOutput.append(String("void ") + addTextFunctionName + "(" + cppNameWithNamespace + "* element, const xsdcpp::Position& pos, std::string&& text) { const char* s = text.c_str(); std::string item; while (xsdcpp::getListItem(s, item)) element->emplace_back((" + itemTypeCppNameWithNamespace + ")xsdcpp::toType(pos, " + toCppNamespacePrefix(rootType.baseType) +  "::_" + itemTypeCppName + "_Values, item)); }");
                else
                    _cppOutput.append(String("void ") + addTextFunctionName + "(" + cppNameWithNamespace + "* element, const xsdcpp::Position& pos, std::string&& text) { const char* s = text.c_str(); std::string item; while (xsdcpp::getListItem(s, item)) element->emplace_back(xsdcpp::toType<" + itemTypeCppNameWithNamespace + ">(pos, item)); }");
            }
        }
        else
        {
            String rootTypeCppName = toCppTypeIdentifier2(rootTypeName);
            String rootTypeCppNameWithNamespace = toCppTypeIdentifierWithNamespace2(rootTypeName);
            if (rootType.kind == Xsd::Type::Kind::EnumKind)
                _cppOutput.append(String("void ") + addTextFunctionName + "(" + cppNameWithNamespace + "* element, const xsdcpp::Position& pos, std::string&& text) { xsd::base<" + rootTypeCppNameWithNamespace+ ">& base = *element; base = (" + rootTypeCppNameWithNamespace + ")xsdcpp::toType(pos, " + toCppNamespacePrefix(rootTypeName) + "::_" + rootTypeCppName + "_Values, text); }");
            else
                _cppOutput.append(String("void ") + addTextFunctionName + "(" + cppNameWithNamespace + "* element, const xsdcpp::Position& pos, std::string&& text) { xsd::base<" + rootTypeCppNameWithNamespace+ ">& base = *element; base = xsdcpp::toType<" + rootTypeCppNameWithNamespace + ">(pos, text); }");
        }
        return String("(xsdcpp::add_text_t)&") + addTextFunctionName;
    }


    bool generateElementInfo(const Xsd::Name& typeName)
    {
        if (_generatedElementInfos2.contains(typeName))
            return true;

        HashMap<Xsd::Name, Xsd::Type>::Iterator it2 = _xsd.types.find(typeName);
        if (it2 == _xsd.types.end())
            return _error = String::fromPrintf("Type '%s' not found", (const char*)typeName.name), false;
        Xsd::Type& type = *it2;

        if (type.kind == Xsd::Type::ElementKind)
            return true;

        if (isNamespaceExternal(typeName.namespace_))
        {
            _generatedElementInfos2.append(typeName);
            return true;
        }
        
        List<String> flags;

        String addTextFunction = generateAddTextFunction(typeName, flags);

        String flagsStr("0");
        if (!flags.isEmpty())
            flagsStr.join(flags, '|');

        String cppName = toCppTypeIdentifier2(typeName);
        _cppOutput.append(String("const xsdcpp::ElementInfo _") + cppName + "_Info = { " + flagsStr + ", " + addTextFunction + " };");

        _generatedElementInfos2.append(typeName);
        return true;
    }

    bool generateType(const Xsd::Name& typeName, usize level)
    {
        if (_generatedTypes2.contains(typeName))
            return true;

        HashMap<Xsd::Name, Xsd::Type>::Iterator it2 = _xsd.types.find(typeName);
        if (it2 == _xsd.types.end())
            return _error = String::fromPrintf("Type '%s' not found", (const char*)typeName.name), false;
        Xsd::Type& type = *it2;

        if (type.kind == Xsd::Type::StringKind)
        {
            if (compareXsName(typeName, "normalizedString") || compareXsName(typeName, "string") || compareXsName(typeName, "anyURI"))
            {
                _generatedTypes2.append(typeName);
                return true;
            }
        }

        if (type.kind == Xsd::Type::BaseKind)
        {
            if (compareXsName(typeName, "nonNegativeInteger") || compareXsName(typeName, "positiveInteger") || compareXsName(typeName, "unsignedLong"))
            {
                _generatedTypes2.append(typeName);
                return true;
            }

            if (compareXsName(typeName, "integer") || compareXsName(typeName, "long"))
            {
                _generatedTypes2.append(typeName);
                return true;
            }

            if (compareXsName(typeName, "unsignedInt"))
            {
                _generatedTypes2.append(typeName);
                return true;
            }

            if (compareXsName(typeName, "int"))
            {
                _generatedTypes2.append(typeName);
                return true;
            }

            if (compareXsName(typeName, "unsignedShort"))
            {
                _generatedTypes2.append(typeName);
                return true;
            }

            if (compareXsName(typeName, "short"))
            {
                _generatedTypes2.append(typeName);
                return true;
            }

            if (compareXsName(typeName, "double") || compareXsName(typeName, "decimal"))
            {
                _generatedTypes2.append(typeName);
                return true;
            }

            if (compareXsName(typeName, "float"))
            {
                _generatedTypes2.append(typeName);
                return true;
            }

            if (compareXsName(typeName, "boolean"))
            {
                _generatedTypes2.append(typeName);
                return true;
            }

            return _error = String::fromPrintf("Base type '%s' not supported", (const char*)typeName.name), false;
        }

        if (type.kind == Xsd::Type::SimpleRefKind)
        {
            _generatedTypes2.append(typeName);

            if (!processType2(type.baseType, level + 1, true))
                return false;

            String cppName = toCppTypeIdentifier2(typeName);
            String baseCppName = toCppTypeIdentifierWithNamespace2(type.baseType);

            _hppOutput.append(String("typedef ") + baseCppName + " " + cppName + ";");
            _hppOutput.append("");
            return true;
        }

        if (type.kind == Xsd::Type::StringKind || type.kind == Xsd::Type::UnionKind)
        {
            _generatedTypes2.append(typeName);
            String cppName = toCppTypeIdentifier2(typeName);
            _hppOutput.append(String("typedef xsd::string ") + cppName + ";");
            _hppOutput.append("");
            return true;
        }

        if (type.kind == Xsd::Type::ListKind)
        {
            _generatedTypes2.append(typeName);

            String baseCppNae;
            if (!processType2(type.baseType, level + 1, false))
                return false;

            String cppName = toCppTypeIdentifier2(typeName);
            String baseCppName = toCppTypeIdentifierWithNamespace2(type.baseType);

            _hppOutput.append(String("typedef xsd::vector<") + baseCppName + "> " + cppName + ";");
            _hppOutput.append("");
            return true;
        }

        if (type.kind == Xsd::Type::EnumKind)
        {
            _generatedTypes2.append(typeName);

            String cppName = toCppTypeIdentifier2(typeName);
            _hppOutput.append(String("enum class ") + cppName);
            _hppOutput.append("{");
            for (List<String>::Iterator i = type.enumEntries.begin(), end = type.enumEntries.end(); i != end; ++i)
                _hppOutput.append(String("    ") + toCppIdentifier(*i) + ",");
            _hppOutput.append("};");
            _hppOutput.append("");

            _cppOutput.append(String("const char* _") + cppName + "_Values[] = {");
            for (List<String>::Iterator i = type.enumEntries.begin(), end = type.enumEntries.end(); i != end; ++i)
                _cppOutput.append(String("    \"") + *i + "\",");
            _cppOutput.append("    nullptr};");
            _cppOutput.append("");
            return true;
        }

        if (type.kind == Xsd::Type::SubstitutionGroupKind)
        {
            _generatedTypes2.append(typeName);
            String cppName = toCppTypeIdentifier2(typeName);

            List<String> structFields;
            for (List<Xsd::ElementRef>::Iterator i = type.elements.begin(), end = type.elements.end(); i != end; ++i)
            {
                const Xsd::ElementRef& elementRef = *i;
                if (!processType2(elementRef.typeName, level + 1, false))
                    return false;
                structFields.append(String("xsd::optional<") + toCppTypeIdentifierWithNamespace2(elementRef.typeName) + "> " + toCppFieldIdentifier(elementRef.name));
            }

            List<String> structDefintiion;
            structDefintiion.append(String("struct ") + cppName);
            structDefintiion.append("{");
            for (List<String>::Iterator i = structFields.begin(), end = structFields.end(); i != end; ++i)
                structDefintiion.append(String("    ") + *i + ";");
            structDefintiion.append("};");
            structDefintiion.append("");

            _hppOutput.append(structDefintiion);
            return true;
        }

        if (type.kind == Xsd::Type::ElementKind)
        {
            _generatedTypes2.append(typeName);
            String cppName = toCppTypeIdentifier2(typeName);
            String cppNameWithNamespace = toCppTypeIdentifierWithNamespace2(typeName);

            Xsd::Type* baseType = nullptr;
            if (!type.baseType.name.isEmpty())
            {
                if (!processType2(type.baseType, level + 1, true))
                    return false;
                baseType = &*_xsd.types.find(type.baseType);
            }

            List<String> structFields;
            for (List<Xsd::AttributeRef>::Iterator i = type.attributes.begin(), end = type.attributes.end(); i != end; ++i)
            {
                const Xsd::AttributeRef& attributeRef = *i;
                bool isOptionalWithoutDefaultValue = !attributeRef.isMandatory && attributeRef.defaultValue.isNull();
                if (!processType2(attributeRef.typeName, level + 1, !isOptionalWithoutDefaultValue))
                    return false;
                if (isOptionalWithoutDefaultValue)
                    structFields.append(String("xsd::optional<") + toCppTypeIdentifierWithNamespace2(attributeRef.typeName) + "> " + toCppFieldIdentifier(attributeRef.name));
                else
                    structFields.append(toCppTypeIdentifierWithNamespace2(attributeRef.typeName) + " " + toCppFieldIdentifier(attributeRef.name));
            }
            if (type.flags & Xsd::Type::AnyAttributeFlag)
                structFields.append("xsd::vector<xsd::any_attribute> other_attributes");
            for (List<Xsd::ElementRef>::Iterator i = type.elements.begin(), end = type.elements.end(); i != end; ++i)
            {
                const Xsd::ElementRef& elementRef = *i;
                bool typeDefinitionRequired = elementRef.minOccurs == 1 && elementRef.maxOccurs == 1;
                if (!processType2(elementRef.typeName, level + 1, typeDefinitionRequired))
                    return false;
                if (elementRef.minOccurs == 1 && elementRef.maxOccurs == 1)
                    structFields.append(toCppTypeIdentifierWithNamespace2(elementRef.typeName) + " " + toCppFieldIdentifier(elementRef.name));
                else if (elementRef.maxOccurs == 1)
                    structFields.append(String("xsd::optional<") + toCppTypeIdentifierWithNamespace2(elementRef.typeName) + "> " + toCppFieldIdentifier(elementRef.name));
                else
                    structFields.append(String("xsd::vector<") + toCppTypeIdentifierWithNamespace2(elementRef.typeName) + "> " + toCppFieldIdentifier(elementRef.name));
            }

            List<String> structDefintiion;
            if (baseType)
            {
                if (baseType->kind == Xsd::Type::BaseKind || baseType->kind == Xsd::Type::EnumKind)
                    structDefintiion.append(String("struct ") + cppName + " : xsd::base<" + toCppTypeIdentifierWithNamespace2(type.baseType) + ">");
                else
                    structDefintiion.append(String("struct ") + cppName + " : " + toCppTypeIdentifierWithNamespace2(type.baseType));
            }
            else
                structDefintiion.append(String("struct ") + cppName);

            structDefintiion.append("{");
            for (List<String>::Iterator i = structFields.begin(), end = structFields.end(); i != end; ++i)
                structDefintiion.append(String("    ") + *i + ";");
            structDefintiion.append("};");
            structDefintiion.append("");

            if (level == 0)
                _cppOutput.append(structDefintiion);
            else
                _hppOutput.append(structDefintiion);

            for (List<Xsd::ElementRef>::Iterator i = type.elements.begin(), end = type.elements.end(); i != end; ++i)
            {
                const Xsd::ElementRef& elementRef = *i;
                if (!processType2(elementRef.typeName, level + 1, false))
                    return false;
                const Xsd::Type& type = *_xsd.types.find(elementRef.typeName);
                if (type.kind == Xsd::Type::SubstitutionGroupKind)
                {
                    for (List<Xsd::ElementRef>::Iterator i = type.elements.begin(), end = type.elements.end(); i != end; ++i)
                    {
                        const Xsd::ElementRef& subElementRef = *i;
                        if (!processType2(subElementRef.typeName, level + 1, false))
                            return false;

                        if (elementRef.minOccurs == 1 && elementRef.maxOccurs == 1)
                            _cppOutput.append(toCppTypeIdentifierWithNamespace2(subElementRef.typeName) + "* get_" + cppName + "_" + toCppFieldIdentifier(elementRef.name) + "_" + toCppFieldIdentifier(subElementRef.name) + "(" + cppNameWithNamespace + "* parent) {return &*(parent->" + toCppFieldIdentifier(elementRef.name) + "." + toCppFieldIdentifier(subElementRef.name) + " = " +  toCppTypeIdentifierWithNamespace2(subElementRef.typeName) + "());}");
                        else if (elementRef.maxOccurs == 1)
                            _cppOutput.append(toCppTypeIdentifierWithNamespace2(subElementRef.typeName) + "* get_" + cppName + "_" + toCppFieldIdentifier(elementRef.name) + "_" + toCppFieldIdentifier(subElementRef.name) + "(" + cppNameWithNamespace + "* parent) {return &*((parent->" + toCppFieldIdentifier(elementRef.name) + " = " + toCppTypeIdentifierWithNamespace2(elementRef.typeName) + "())->" + toCppFieldIdentifier(subElementRef.name) + " = " +  toCppTypeIdentifierWithNamespace2(subElementRef.typeName) + "());}");
                        else
                            _cppOutput.append(toCppTypeIdentifierWithNamespace2(subElementRef.typeName) + "* get_" + cppName + "_" + toCppFieldIdentifier(elementRef.name) + "_" + toCppFieldIdentifier(subElementRef.name) + "(" + cppNameWithNamespace + "* parent) {return (parent->" + toCppFieldIdentifier(elementRef.name) + ".emplace_back(), &*(parent->" + toCppFieldIdentifier(elementRef.name) + ".back()." + toCppFieldIdentifier(subElementRef.name) + " = " +  toCppTypeIdentifierWithNamespace2(subElementRef.typeName) + "()));}");
                    }
                }
                else
                {
                    if (elementRef.minOccurs == 1 && elementRef.maxOccurs == 1)
                        _cppOutput.append(toCppTypeIdentifierWithNamespace2(elementRef.typeName) + "* get_" + cppName + "_" + toCppFieldIdentifier(elementRef.name) + "(" + cppNameWithNamespace + "* parent) {return &parent->" + toCppFieldIdentifier(elementRef.name) + ";}");
                    else if (elementRef.maxOccurs == 1)
                        _cppOutput.append(toCppTypeIdentifierWithNamespace2(elementRef.typeName) + "* get_" + cppName + "_" + toCppFieldIdentifier(elementRef.name) + "(" + cppNameWithNamespace + "* parent) {return &*(parent->" + toCppFieldIdentifier(elementRef.name) + " = " + toCppTypeIdentifierWithNamespace2(elementRef.typeName) + "());}");
                    else
                        _cppOutput.append(toCppTypeIdentifierWithNamespace2(elementRef.typeName) + "* get_" + cppName + "_" + toCppFieldIdentifier(elementRef.name) + "(" + cppNameWithNamespace + "* parent) {return (parent->" + toCppFieldIdentifier(elementRef.name) + ".emplace_back(), &parent->" + toCppFieldIdentifier(elementRef.name) + ".back());}");
                }
            }

            String children("nullptr");
            if (!type.elements.isEmpty())
            {
                children = String("_") + cppName + "_Children";
                List<String> childElementInfo;
                childElementInfo.append(String("xsdcpp::ChildElementInfo _") + cppName + "_Children[] = {");
                for (List<Xsd::ElementRef>::Iterator i = type.elements.begin(), end = type.elements.end(); i != end; ++i)
                {
                    const Xsd::ElementRef& elementRef = *i;
                    if (!processType2(elementRef.typeName, level + 1, false))
                        return false;

                    const Xsd::Type& type = *_xsd.types.find(elementRef.typeName);
                    if (type.kind == Xsd::Type::SubstitutionGroupKind)
                    {
                        for (List<Xsd::ElementRef>::Iterator i = type.elements.begin(), end = type.elements.end(); i != end; ++i)
                        {
                            const Xsd::ElementRef& subElementRef = *i;
                            if (!processType2(subElementRef.typeName, level + 1, false))
                                return false;

                            if (!generateElementInfo(subElementRef.typeName))
                                return false;

                            childElementInfo.append(String("    {\"") + subElementRef.name.name + "\", (xsdcpp::get_element_field_t)&get_" + cppName +  "_" + toCppFieldIdentifier(elementRef.name) + "_" + toCppFieldIdentifier(subElementRef.name) + ", &" + toCppNamespacePrefix(subElementRef.typeName) + "::_" + toCppTypeIdentifier2(subElementRef.typeName) + "_Info, 0, " + String::fromUInt(elementRef.maxOccurs)  + "},");
                        }
                    }
                    else
                    {
                        if (!generateElementInfo(elementRef.typeName))
                            return false;

                        childElementInfo.append(String("    {\"") + elementRef.name.name + "\", (xsdcpp::get_element_field_t)&get_" + cppName + "_" + toCppFieldIdentifier(elementRef.name) + ", &" + toCppNamespacePrefix(elementRef.typeName) + "::_" + toCppTypeIdentifier2(elementRef.typeName) + "_Info, " + String::fromUInt(elementRef.minOccurs)  + ", " + String::fromUInt(elementRef.maxOccurs)  + "},");
                    }
                }
                childElementInfo.append("    {nullptr}\n};");
                _cppOutput.append(childElementInfo);
            }


            for (List<Xsd::AttributeRef>::Iterator i = type.attributes.begin(), end = type.attributes.end(); i != end; ++i)
            {
                const Xsd::AttributeRef& attributeRef = *i;
                if (!processType2(attributeRef.typeName, level + 1, false))
                    return false;

                HashMap<Xsd::Name, Xsd::Type>::Iterator it = _xsd.types.find(attributeRef.typeName);
                if (it == _xsd.types.end())
                    return false;
                const Xsd::Type& attributeType = *it;
                Xsd::Type rootTye = getRootType(attributeRef.typeName);
                if (rootTye.kind == Xsd::Type::StringKind || rootTye.kind == Xsd::Type::UnionKind)
                    _cppOutput.append(String("void set_") + cppName + "_" + toCppFieldIdentifier(attributeRef.name) + "(" + toCppTypeIdentifierWithNamespace2(typeName) + "* element, const xsdcpp::Position&, std::string&& value) { element->" + toCppFieldIdentifier(attributeRef.name) + " = std::move(value); }");
                else if (rootTye.kind == Xsd::Type::EnumKind)
                    _cppOutput.append(String("void set_") + cppName + "_" + toCppFieldIdentifier(attributeRef.name) + "(" + toCppTypeIdentifierWithNamespace2(typeName) + "* element, const xsdcpp::Position& pos, std::string&& value) { element->" + toCppFieldIdentifier(attributeRef.name) + " = (" + toCppTypeIdentifierWithNamespace2(attributeRef.typeName) + ")xsdcpp::toType(pos, " + toCppNamespacePrefix(attributeRef.typeName) + "::_" + toCppTypeIdentifier2(attributeRef.typeName) + "_Values, value); }");
                else if (rootTye.kind == Xsd::Type::ListKind)
                {
                    Xsd::Type itemType = getType(rootTye.baseType);
                    String itemTypeCppName = toCppTypeIdentifier2(rootTye.baseType);
                    String itemTypeCppNameWithNamespace = toCppTypeIdentifierWithNamespace2(rootTye.baseType);

                    bool optionalWithoutDefaultValue = !attributeRef.isMandatory && attributeRef.defaultValue.isNull();
                    if (optionalWithoutDefaultValue)
                    {
                        if (itemType.kind == Xsd::Type::Kind::EnumKind)
                            _cppOutput.append(String("void set_") + cppName + "_" + toCppFieldIdentifier(attributeRef.name) + "(" + toCppTypeIdentifierWithNamespace2(typeName) + "* element, const xsdcpp::Position& pos, std::string&& value) { element->" + toCppFieldIdentifier(attributeRef.name) + " = " + toCppTypeIdentifierWithNamespace2(attributeRef.typeName) +"(); const char* s = value.c_str(); std::string item; while (xsdcpp::getListItem(s, item)) element->" + toCppFieldIdentifier(attributeRef.name) + "->emplace_back((" + itemTypeCppNameWithNamespace + ")xsdcpp::toType(pos, " + toCppNamespacePrefix(rootTye.baseType) + "::_" + itemTypeCppName + "_Values, item)); }");
                        else if (itemType.kind == Xsd::Type::Kind::StringKind)
                            _cppOutput.append(String("void set_") + cppName + "_" + toCppFieldIdentifier(attributeRef.name) + "(" + toCppTypeIdentifierWithNamespace2(typeName) + "* element, const xsdcpp::Position& pos, std::string&& value) { element->" + toCppFieldIdentifier(attributeRef.name) + " = " + toCppTypeIdentifierWithNamespace2(attributeRef.typeName) +"(); const char* s = value.c_str(); std::string item; while (xsdcpp::getListItem(s, item)) element->" + toCppFieldIdentifier(attributeRef.name) + "->emplace_back(std::move(item)); }");
                        else
                            _cppOutput.append(String("void set_") + cppName + "_" + toCppFieldIdentifier(attributeRef.name) + "(" + toCppTypeIdentifierWithNamespace2(typeName) + "* element, const xsdcpp::Position& pos, std::string&& value) { element->" + toCppFieldIdentifier(attributeRef.name) + " = " + toCppTypeIdentifierWithNamespace2(attributeRef.typeName) +"(); const char* s = value.c_str(); std::string item; while (xsdcpp::getListItem(s, item)) element->" + toCppFieldIdentifier(attributeRef.name) + "->emplace_back(xsdcpp::toType<" + itemTypeCppNameWithNamespace + ">(pos, item)); }");
                    }
                    else
                    {
                        if (itemType.kind == Xsd::Type::Kind::EnumKind)
                            _cppOutput.append(String("void set_") + cppName + "_" + toCppFieldIdentifier(attributeRef.name) + "(" + toCppTypeIdentifierWithNamespace2(typeName) + "* element, const xsdcpp::Position& pos, std::string&& value) { const char* s = value.c_str(); std::string item; while (xsdcpp::getListItem(s, item)) element->" + toCppFieldIdentifier(attributeRef.name) + ".emplace_back((" + itemTypeCppNameWithNamespace + ")xsdcpp::toType(pos, " + toCppNamespacePrefix(rootTye.baseType) + "::_" + itemTypeCppName + "_Values, item)); }");
                        else if (itemType.kind == Xsd::Type::Kind::StringKind)
                            _cppOutput.append(String("void set_") + cppName + "_" + toCppFieldIdentifier(attributeRef.name) + "(" + toCppTypeIdentifierWithNamespace2(typeName) + "* element, const xsdcpp::Position& pos, std::string&& value) { const char* s = value.c_str(); std::string item; while (xsdcpp::getListItem(s, item)) element->" + toCppFieldIdentifier(attributeRef.name) + ".emplace_back(std::move(item)); }");
                        else
                            _cppOutput.append(String("void set_") + cppName + "_" + toCppFieldIdentifier(attributeRef.name) + "(" + toCppTypeIdentifierWithNamespace2(typeName) + "* element, const xsdcpp::Position& pos, std::string&& value) { const char* s = value.c_str(); std::string item; while (xsdcpp::getListItem(s, item)) element->" + toCppFieldIdentifier(attributeRef.name) + ".emplace_back(xsdcpp::toType<" + itemTypeCppNameWithNamespace + ">(pos, item)); }");
                    }
                }
                else
                    _cppOutput.append(String("void set_") + cppName + "_" + toCppFieldIdentifier(attributeRef.name) + "(" + toCppTypeIdentifierWithNamespace2(typeName) + "* element, const xsdcpp::Position& pos, std::string&& value) { element->" + toCppFieldIdentifier(attributeRef.name) + " = xsdcpp::toType<" + toCppTypeIdentifierWithNamespace2(attributeRef.typeName) + ">(pos, value); }");
            
                if (!attributeRef.isMandatory && !attributeRef.defaultValue.isNull())
                    _cppOutput.append(String("void default_") + cppName + "_" + toCppFieldIdentifier(attributeRef.name) + "(" + toCppTypeIdentifierWithNamespace2(typeName) + "* element) { element->" + toCppFieldIdentifier(attributeRef.name) + " = " + resolveDefaultValue(attributeRef.typeName, rootTye, attributeRef.defaultValue.toString()) + "; }");
            }
            if (type.flags & Xsd::Type::AnyAttributeFlag)
                _cppOutput.append(String("void any_") + cppName + "(" + toCppTypeIdentifierWithNamespace2(typeName) + "* element, std::string&& name, std::string&& value) { element->other_attributes.emplace_back(xsd::any_attribute{std::move(name), std::move(value)}); }");

            String attributes("nullptr");
            if (!type.attributes.isEmpty())
            {
                attributes = String("_") + cppName + "_Attributes";
                _cppOutput.append(String("xsdcpp::AttributeInfo _") + cppName + "_Attributes[] = {");
                for (List<Xsd::AttributeRef>::Iterator i = type.attributes.begin(), end = type.attributes.end(); i != end; ++i)
                {
                    const Xsd::AttributeRef& attributeRef = *i;
                    if (!processType2(attributeRef.typeName, level + 1, false))
                        return false;
                    String setDefault = (attributeRef.isMandatory || attributeRef.defaultValue.isNull()) ? String("nullptr") : String("(xsdcpp::set_attribute_default_t)&default_") + cppName + "_" + toCppFieldIdentifier(attributeRef.name);
                    _cppOutput.append(String("    {\"") + attributeRef.name.name + "\", (xsdcpp::set_attribute_t)&set_" + cppName + "_" + toCppFieldIdentifier(attributeRef.name) + ", " + (attributeRef.isMandatory ? String("true") : String("false")) +  ", " + setDefault + "},");
                }
                _cppOutput.append("    {nullptr}\n};");
            }

            usize childrenCount = getChildrenCount(typeName);
            usize mandatoryChildrenCount = getMandatoryChildrenCount(typeName);
            uint64 attributesCount = getAttributesCount(typeName);
            List<String> flags;
            if (level == 1)
                flags.append("xsdcpp::ElementInfo::EntryPointFlag");
            if (type.flags & Xsd::Type::AnyAttributeFlag)
                flags.append("xsdcpp::ElementInfo::AnyAttributeFlag");

            String addTextFunction = generateAddTextFunction(typeName, flags);

            String parentElementCppName;
            if (baseType && baseType->kind == Xsd::Type::Kind::ElementKind)
                parentElementCppName = toCppTypeIdentifier2(type.baseType);

            String flagsStr("0");
            if (!flags.isEmpty())
                flagsStr.join(flags, '|');

            _cppOutput.append(String("const xsdcpp::ElementInfo _") + cppName + "_Info = { " + flagsStr 
                + ", " + addTextFunction
                + ", " + children + ", " + String::fromUInt64(childrenCount) + ", " + String::fromUInt64(mandatoryChildrenCount) 
                + ", " + attributes + ", " + String::fromUInt64(attributesCount) + ", " + (parentElementCppName.isEmpty() ? String("nullptr") : String("&") + toCppNamespacePrefix(type.baseType) + "::_" + parentElementCppName + "_Info") 
                + ", " + (type.flags & Xsd::Type::AnyAttributeFlag ? String("(xsdcpp::set_any_attribute_t)&any_") + cppName : String("nullptr"))
                + " };");
            _cppOutput.append("");

            _generatedElementInfos2.append(typeName);

            return true;
        }

        return false; // logic error
    }
};

}

bool generateCpp(const Xsd& xsd, const String& outputDir, const List<String>& excludedNamespacePrefixes, const List<String>& forceTypeProcessing, String& error)
{
    List<String> cppOutput;
    List<String> hppOutput;
    Generator generator(xsd, excludedNamespacePrefixes, forceTypeProcessing, cppOutput, hppOutput);
    if (!generator.process())
        return (error = generator.getError()), false;

    String cppName = toCppIdentifier(xsd.name);

    {
        String outputFilePath = outputDir + "/" + cppName + ".hpp";
        File outputFile;
        if (!outputFile.open(outputFilePath, File::writeFlag))
            return (error = String::fromPrintf("Could not open file '%s': %s", (const char*)outputFilePath, (const char*)Error::getErrorString())), false;

        for (List<String>::Iterator i = hppOutput.begin(), end = hppOutput.end(); i != end; ++i)
            if (!outputFile.write(*i + "\n"))
                return (error = String::fromPrintf("Could not write to file '%s': %s", (const char*)outputFilePath, (const char*)Error::getErrorString())), false;
    }

    {
        String outputFilePath = outputDir + "/" + cppName + ".cpp";
        File outputFile;
        if (!outputFile.open(outputFilePath, File::writeFlag))
            return (error = String::fromPrintf("Could not open file '%s': %s", (const char*)outputFilePath, (const char*)Error::getErrorString())), false;

        if (!outputFile.write(XmlParser_hpp))
            return (error = String::fromPrintf("Could not write to file '%s': %s", (const char*)outputFilePath, (const char*)Error::getErrorString())), false;
        if (excludedNamespacePrefixes.isEmpty())
            if (!outputFile.write(XmlParser_cpp))
                return (error = String::fromPrintf("Could not write to file '%s': %s", (const char*)outputFilePath, (const char*)Error::getErrorString())), false;

        for (List<String>::Iterator i = cppOutput.begin(), end = cppOutput.end(); i != end; ++i)
            if (!outputFile.write(*i + "\n"))
                return (error = String::fromPrintf("Could not write to file '%s': %s", (const char*)outputFilePath, (const char*)Error::getErrorString())), false;
    }

    {
        String outputFilePath = outputDir + "/" + cppName + "_xsd.hpp";
        File outputFile;
        if (!outputFile.open(outputFilePath, File::writeFlag))
            return (error = String::fromPrintf("Could not open file '%s': %s", (const char*)outputFilePath, (const char*)Error::getErrorString())), false;
        if (!outputFile.write(xsd_hpp))
            return (error = String::fromPrintf("Could not write to file '%s': %s", (const char*)outputFilePath, (const char*)Error::getErrorString())), false;
    }

    return true;
}
