
#include "Generator.hpp"

#include <nstd/Console.hpp>
#include <nstd/Error.hpp>
#include <nstd/File.hpp>
#include <nstd/HashMap.hpp>

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
    return name.xsdNamespace == "http://www.w3.org/2001/XMLSchema" && name.name == rh;
}

class Generator
{
public:
    Generator(const Xsd& xsd, const List<String>& externalNamespacePrefixes, const List<String>& forceTypeProcessing, List<String>& cppOutput, List<String>& hppOutput)
        : _xsd(xsd)
        , _externalNamespacePrefixes(externalNamespacePrefixes)
        , _cppOutputFinal(cppOutput)
        , _hppOutput(hppOutput)
    {
        for (List<String>::Iterator i = forceTypeProcessing.begin(), end = forceTypeProcessing.end(); i != end; ++i)
        {
            Xsd::Name typeName;
            typeName.name = *i;
            typeName.xsdNamespace = _xsd.targetNamespaces.front();
            _requiredTypes.append(typeName);
        }
    }

    const String& getError() const { return _error; }

    bool process()
    {
        _cppNamespace = toCppIdentifier(_xsd.name);

        HashSet<Xsd::Name> localElementTypes;
        HashSet<Xsd::Name> localSubstitutionGroupTypes;
        HashSet<Xsd::Name> localSimpleElementTypes;
        if (!collectLocalTypes(_xsd.rootType, localElementTypes, localSubstitutionGroupTypes))
            return false;
        for (HashSet<Xsd::Name>::Iterator i = _requiredTypes.begin(), end = _requiredTypes.end(); i != end; ++i)
        {
            if (!collectLocalTypes(*i, localElementTypes, localSubstitutionGroupTypes))
                return false;

            if (getType(*i).kind == Xsd::Type::Kind::ElementKind)
                localElementTypes.append(*i);
            else
                localSimpleElementTypes.append(*i);
        }

        HashMap<String, HashSet<Xsd::Name>> externalTypes;
        if (!collectReferencedExternalTypes(localElementTypes, externalTypes))
            return false;

        _cppOutputFinal.append("");
        _cppOutputFinal.append(String("#include \"") + _cppNamespace + ".hpp\"");
        _cppOutputFinal.append("");
        _cppOutputFinal.append("#include <fstream>");
        _cppOutputFinal.append("#include <sstream>");
        _cppOutputFinal.append("");


        for (HashMap<String, HashSet<Xsd::Name>>::Iterator i = externalTypes.begin(), end = externalTypes.end(); i != end; ++i)
        {
            _cppOutputFinal.append(String("namespace ") + i.key() + " {");
            _cppOutputFinal.append("");
            const HashSet<Xsd::Name>& types = *i;
            for (HashSet<Xsd::Name>::Iterator i = types.begin(), end = types.end(); i != end; ++i)
            {
                String cppName = toCppTypeIdentifier2(*i);
                _cppOutputFinal.append(String("extern const xsdcpp::ElementInfo _") + cppName + "_Info;");
                _cppOutputFinal.append(String("void _set_") + cppName + "(" + toCppTypeIdentifierWithNamespace2(*i) + "*, const xsdcpp::Position&, std::string&&);");
            }
            _cppOutputFinal.append("");
            _cppOutputFinal.append("}");
            _cppOutputFinal.append("");
        }


        _cppOutputAnonymousFieldGetter.append("const char* _namespaces[] = {");
        for (HashSet<String>::Iterator i = _xsd.targetNamespaces.begin(), end = _xsd.targetNamespaces.end(); i != end; ++i)
            _cppOutputAnonymousFieldGetter.append(String("    ") + toCStringLiteral(*i) + ",");
        if (_xsd.targetNamespaces.find("http://www.w3.org/2001/XMLSchema-instance") == _xsd.targetNamespaces.end())
            _cppOutputAnonymousFieldGetter.append("    \"http://www.w3.org/2001/XMLSchema-instance\",");
        if (_xsd.targetNamespaces.find("http://www.w3.org/2001/XMLSchema") == _xsd.targetNamespaces.end())
            _cppOutputAnonymousFieldGetter.append("    \"http://www.w3.org/2001/XMLSchema\",");
        _cppOutputAnonymousFieldGetter.append("    nullptr");
        _cppOutputAnonymousFieldGetter.append("};");
        _cppOutputAnonymousFieldGetter.append("");

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

        for (HashSet<Xsd::Name>::Iterator i = localElementTypes.begin(), end = localElementTypes.end(); i != end; ++i)
        {
            if (*i == _xsd.rootType)
                continue;
            String cppName = toCppTypeIdentifier2(*i);
            _cppOutputNamespaceElementInfoExtern.append(String("extern const xsdcpp::ElementInfo _") + cppName + "_Info;");
            _hppOutput.append(String("struct ") + cppName + ";");
        }
        for (HashSet<Xsd::Name>::Iterator i = localSimpleElementTypes.begin(), end = localSimpleElementTypes.end(); i != end; ++i)
            _cppOutputNamespaceElementInfoExtern.append(String("extern const xsdcpp::ElementInfo _") + toCppTypeIdentifier2(*i) + "_Info;");
        for (HashSet<Xsd::Name>::Iterator i = localSubstitutionGroupTypes.begin(), end = localSubstitutionGroupTypes.end(); i != end; ++i)
            _hppOutput.append(String("struct ") + toCppTypeIdentifier2(*i) + ";");
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

        //_cppOutput.append("");
        //_cppOutput.append("#include <fstream>");
        //_cppOutput.append("#include <system_error>");
        //_cppOutput.append("#include <sstream>");
        //_cppOutput.append("");

        _cppOutputFinal.append(String("namespace ") + _cppNamespace + " {");
        _cppOutputFinal.append("");
        _cppOutputFinal.append(_cppOutputNamespaceElementInfoExtern);
        _cppOutputFinal.append("");
        _cppOutputFinal.append("}");
        _cppOutputFinal.append("");

        _cppOutputFinal.append("namespace {");
        _cppOutputFinal.append("");
        _cppOutputFinal.append(_cppOutputAnonymousEnumValues);
        _cppOutputFinal.append("");
        _cppOutputFinal.append("}");
        _cppOutputFinal.append("");

        _cppOutputFinal.append(String("namespace ") + _cppNamespace + " {");
        _cppOutputFinal.append("");
        _cppOutputFinal.append(_cppOutputNamespaceSetValue);
        _cppOutputFinal.append("");
        _cppOutputFinal.append("}");
        _cppOutputFinal.append("");

        _cppOutputFinal.append("namespace {");
        _cppOutputFinal.append("");
        _cppOutputFinal.append(_cppOutputAnonymousFieldGetter);
        _cppOutputFinal.append("");
        _cppOutputFinal.append("}");
        _cppOutputFinal.append("");

        _cppOutputFinal.append(String("namespace ") + _cppNamespace + " {");
        _cppOutputFinal.append("");
        _cppOutputFinal.append(_cppOutputNamespace);
        _cppOutputFinal.append("");

        String rootTypeCppName = toCppTypeIdentifier2(_xsd.rootType);
        for (List<Xsd::ElementRef>::Iterator i = rootType.elements.begin(), end = rootType.elements.end(); i != end; ++i)
        {
            String elementTypeCppName = toCppTypeIdentifier2(i->typeName);
            String elementCppName = toCppFieldIdentifier(i->name);

            _cppOutputFinal.append(String("void load_data(const std::string& data, ") + elementTypeCppName + "& output)");
            _cppOutputFinal.append("{");
            _cppOutputFinal.append(String("    ") + rootTypeCppName + " rootElement;");
            _cppOutputFinal.append("    xsdcpp::ElementContext elementContext;");
            _cppOutputFinal.append(String("    elementContext.info = &_") + rootTypeCppName + "_Info;");
            _cppOutputFinal.append("    elementContext.element = &rootElement;");
            _cppOutputFinal.append("    xsdcpp::parse(data.c_str(), _namespaces, elementContext);");
            _cppOutputFinal.append(String("    output = std::move(rootElement.") + elementCppName + ");");
            _cppOutputFinal.append("}");
            _cppOutputFinal.append("");

            _cppOutputFinal.append(String("void load_file(const std::string& filePath, ") + elementTypeCppName + "& output)");
            _cppOutputFinal.append("{");
            _cppOutputFinal.append("    std::fstream file;");
            _cppOutputFinal.append("    file.exceptions(std::fstream::failbit | std::fstream::badbit);");
            _cppOutputFinal.append("    file.open(filePath, std::fstream::in);");
            _cppOutputFinal.append("    std::stringstream buffer;");
            _cppOutputFinal.append("    buffer << file.rdbuf();");
            _cppOutputFinal.append("    load_data(buffer.str(), output);");
            _cppOutputFinal.append("}");
            _cppOutputFinal.append("");
        }


        _cppOutputFinal.append("}");
        _cppOutputFinal.append("");

        return true;
    }

private:
    const Xsd& _xsd;
    const List<String>& _externalNamespacePrefixes;

    HashMap<String, String> _externalNamespaces;

    List<String>& _cppOutputFinal;
    List<String> _cppOutputNamespaceElementInfoExtern;
    List<String> _cppOutputAnonymousEnumValues;
    List<String> _cppOutputNamespaceSetValue;
    List<String> _cppOutputAnonymousFieldGetter;
    List<String> _cppOutputNamespace;
    List<String>& _hppOutput;
    String _cppNamespace;
    HashSet<Xsd::Name> _generatedTypes2;
    HashSet<Xsd::Name> _generatedElementInfos2;
    HashSet<Xsd::Name> _generatedTypeSetters;
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

        if (typeName.xsdNamespace == _xsd.targetNamespaces.front() || isNamespaceExternal(typeName.xsdNamespace))
            return toCppIdentifier(typeName.name);
        HashMap<String, String>::Iterator it2 = _xsd.namespaceToSuggestedPrefix.find(typeName.xsdNamespace);
        if (it2 != _xsd.namespaceToSuggestedPrefix.end())
            return toCppIdentifier(*it2 + "_" + typeName.name);
        return toCppIdentifier(typeName.name);
    }

    String toCppTypeIdentifierWithNamespace2(const Xsd::Name& typeName)
    {
        String result = toCppTypeIdentifier2(typeName);
        if (result == "_root_t" ||
            result == "xsd::string" || 
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
        if (isNamespaceExternal(typeName.xsdNamespace, namespacePrefix))
            return namespacePrefix + "::" + result;
        return _cppNamespace + "::" + result;
    }

    String toCppNamespacePrefix(const Xsd::Name& typeName)
    {
        String namespacePrefix;
        if (isNamespaceExternal(typeName.xsdNamespace, namespacePrefix))
            return namespacePrefix;
        return _cppNamespace;
    }

    String toSetValueFunctionName(const Xsd::Name& typeName)
    {
        String cppName = toCppTypeIdentifier2(typeName);
        if (cppName == "xsd::string")
            return "xsdcpp::set_string";
        if (cppName == "uint64_t" ||
            cppName == "int64_t" ||
            cppName == "uint32_t" ||
            cppName == "int32_t" ||
            cppName == "uint16_t" ||
            cppName == "int16_t" ||
            cppName == "double" ||
            cppName == "float" ||
            cppName == "bool")
            return String("xsdcpp::set_") + cppName;
        return toCppNamespacePrefix(typeName) + "::_set_" + cppName;
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

    Xsd::Name getRootTypeName(const Xsd::Name& typeName) const // root base type, or the type itself if there is no base type
    {
        HashMap<Xsd::Name, Xsd::Type>::Iterator it = _xsd.types.find(typeName);
        if (it != _xsd.types.end())
        {
            const Xsd::Type& type = *it;
            if ((type.kind == Xsd::Type::Kind::ElementKind || type.kind == Xsd::Type::Kind::SimpleRefKind) && !type.baseType.name.isEmpty())
                return getRootTypeName(type.baseType);
        }
        return typeName;
    }

    Xsd::Name getSimpleBaseTypeName(const Xsd::Name& typeName) const // first base type that is not an element, or nothing if the element has no simple base type
    {
        HashMap<Xsd::Name, Xsd::Type>::Iterator it2 = _xsd.types.find(typeName);
        if (it2 == _xsd.types.end())
            return Xsd::Name();
        const Xsd::Type& type = *it2;
        if (type.kind == Xsd::Type::Kind::ElementKind)
            return getSimpleBaseTypeName(type.baseType);
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

    String resolveDefaultValue(const Xsd::Name& attributeTypeName, const Xsd::Type& rootType, const String& defaultValue)
    {
        if (rootType.kind == Xsd::Type::EnumKind)
        {
            uint value = 0;
            for (List<String>::Iterator i = rootType.enumEntries.begin(), end = rootType.enumEntries.end(); i != end; ++i, ++value)
                if (*i == defaultValue)
                    return String("(") + toCppTypeIdentifierWithNamespace2(attributeTypeName) + ")"  + String::fromUInt(value);
        }
        if (rootType.kind == Xsd::Type::StringKind)
            return toCStringLiteral(defaultValue);
        return defaultValue;
    }

    bool collectLocalTypes(const Xsd::Name& typeName, HashSet<Xsd::Name>& elementTypes, HashSet<Xsd::Name>& substitutionGroupTypes)
    {
        if (isNamespaceExternal(typeName.xsdNamespace))
            return true;

        HashMap<Xsd::Name, Xsd::Type>::Iterator it2 = _xsd.types.find(typeName);
        if (it2 == _xsd.types.end())
            return _error = String::fromPrintf("Type '%s' not found", (const char*)typeName.name), false;
        Xsd::Type& type = *it2;

        if (type.kind == Xsd::Type::SubstitutionGroupKind)
        {
            if (substitutionGroupTypes.contains(typeName))
                return true;

            substitutionGroupTypes.append(typeName);

            for (List<Xsd::ElementRef>::Iterator i = type.elements.begin(), end = type.elements.end(); i != end; ++i)
                if (!collectLocalTypes(i->typeName, elementTypes, substitutionGroupTypes))
                    return false;
        }

        if (type.kind == Xsd::Type::ElementKind)
        {
            if (elementTypes.contains(typeName))
                return true;

            elementTypes.append(typeName);

            if (!type.baseType.name.isEmpty())
                if (!collectLocalTypes(type.baseType, elementTypes, substitutionGroupTypes))
                    return false;

            for (List<Xsd::ElementRef>::Iterator i = type.elements.begin(), end = type.elements.end(); i != end; ++i)
            {
                const Xsd::Name& elementTypeName = i->typeName;
                if (!collectLocalTypes(elementTypeName, elementTypes, substitutionGroupTypes))
                    return false;
            }
        }

        return true;
    }

    bool collectReferencedExternalTypes(const HashSet<Xsd::Name>& localElementTypes, HashMap<String, HashSet<Xsd::Name>>& externalTypes)
    {
        for (HashSet<Xsd::Name>::Iterator i = localElementTypes.begin(), end = localElementTypes.end(); i != end; ++i)
        {
            const Xsd::Name& typeName = *i;
            HashMap<Xsd::Name, Xsd::Type>::Iterator it = _xsd.types.find(typeName);
            if (it == _xsd.types.end())
                return _error = String::fromPrintf("Type '%s' not found", (const char*)typeName.name), false;
            Xsd::Type& type = *it;

            if (!type.baseType.name.isEmpty())
            {
                String namespacePrefix;
                if (isNamespaceExternal(type.baseType.xsdNamespace, namespacePrefix))
                    externalTypes.append(namespacePrefix, HashSet<Xsd::Name>(), false).append(type.baseType);

                Xsd::Name simpleBaseTypeName = getSimpleBaseTypeName(type.baseType);
                if (isNamespaceExternal(simpleBaseTypeName.xsdNamespace, namespacePrefix))
                    externalTypes.append(namespacePrefix, HashSet<Xsd::Name>(), false).append(simpleBaseTypeName);
            }

            for (List<Xsd::ElementRef>::Iterator i = type.elements.begin(), end = type.elements.end(); i != end; ++i)
            {
                const Xsd::Name& elementTypeName = i->typeName;
                String namespacePrefix;
                if (isNamespaceExternal(elementTypeName.xsdNamespace, namespacePrefix))
                    externalTypes.append(namespacePrefix, HashSet<Xsd::Name>(), false).append(elementTypeName);
            }

            for (List<Xsd::AttributeRef>::Iterator i = type.attributes.begin(), end = type.attributes.end(); i != end; ++i)
            {
                const Xsd::Name& elementTypeName = i->typeName;
                String namespacePrefix;
                if (isNamespaceExternal(elementTypeName.xsdNamespace, namespacePrefix))
                    externalTypes.append(namespacePrefix, HashSet<Xsd::Name>(), false).append(elementTypeName);
            }
        }
        return true;
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

        if (isNamespaceExternal(typeName.xsdNamespace))
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

    bool generateTypeSetter(const Xsd::Name& typeName)
    {
        if (_generatedTypeSetters.contains(typeName))
            return true;

        HashMap<Xsd::Name, Xsd::Type>::Iterator it2 = _xsd.types.find(typeName);
        if (it2 == _xsd.types.end())
            return _error = String::fromPrintf("Type '%s' not found", (const char*)typeName.name), false;

        _generatedTypeSetters.append(typeName);

        if (isNamespaceExternal(typeName.xsdNamespace))
            return true;

        String cppName = toCppTypeIdentifier2(typeName);
        if (cppName == "xsd::string" ||
            cppName == "uint64_t" ||
            cppName == "int64_t" ||
            cppName == "uint32_t" ||
            cppName == "int32_t" ||
            cppName == "uint16_t" ||
            cppName == "int16_t" ||
            cppName == "double" ||
            cppName == "float" ||
            cppName == "bool")
            return true;

        Xsd::Type& type = *it2;
        String cppNameWithNamespace = toCppTypeIdentifierWithNamespace2(typeName);
        String functionName = String("_set_") + cppName;

        if (type.kind == Xsd::Type::Kind::ElementKind)
        {
            Xsd::Name baseTypeName = getSimpleBaseTypeName(typeName);
            if (baseTypeName.name.isEmpty())
                return true; // you don't need a setter for such an element
            if (!generateTypeSetter(baseTypeName))
                return false;
            _cppOutputNamespaceSetValue.append(String("void ") + functionName + "(" + cppNameWithNamespace + "* obj, const xsdcpp::Position& pos, std::string&& val) { " + toCppTypeIdentifierWithNamespace2(baseTypeName) + "& base = *obj; " + toSetValueFunctionName(baseTypeName) + "(&base, pos, std::move(val)); }");

        }
        else if (type.kind == Xsd::Type::Kind::StringKind || type.kind == Xsd::Type::Kind::UnionKind)
            _cppOutputNamespaceSetValue.append(String("void ") + functionName + "(" + cppNameWithNamespace + "* obj, const xsdcpp::Position& pos, std::string&& val) { xsdcpp::set_string(obj, pos, std::move(val)); }");
        else if (type.kind == Xsd::Type::Kind::ListKind)
        {
            const Xsd::Name& itemTypeName = type.baseType;
            if (!generateTypeSetter(itemTypeName))
                return false;
            Xsd::Type itemType = getType(itemTypeName);
             if (itemType.kind == Xsd::Type::Kind::StringKind || itemType.kind == Xsd::Type::Kind::UnionKind)
                _cppOutputNamespaceSetValue.append(String("void ") + functionName + "(" + cppNameWithNamespace + "* obj, const xsdcpp::Position& pos, std::string&& val) { const char* s = val.c_str(); std::string item; while (xsdcpp::getListItem(s, item)) { obj->emplace_back(std::move(item)); } }");
            else
                _cppOutputNamespaceSetValue.append(String("void ") + functionName + "(" + cppNameWithNamespace + "* obj, const xsdcpp::Position& pos, std::string&& val) { const char* s = val.c_str(); std::string item; while (xsdcpp::getListItem(s, item)) { obj->emplace_back(); " + toSetValueFunctionName(itemTypeName) + "(&obj->back(), pos, std::move(item)); } }");
        }
        else if (type.kind == Xsd::Type::Kind::EnumKind)
        {
            _cppOutputAnonymousEnumValues.append(String("const char* _") + cppName + "_Values[] = {");
            for (List<String>::Iterator i = type.enumEntries.begin(), end = type.enumEntries.end(); i != end; ++i)
                _cppOutputAnonymousEnumValues.append(String("    \"") + *i + "\",");
            _cppOutputAnonymousEnumValues.append("    nullptr};");
            _cppOutputAnonymousEnumValues.append("");

            _cppOutputNamespaceSetValue.append(String("void ") + functionName + "(" + cppNameWithNamespace + "* obj, const xsdcpp::Position& pos, std::string&& val) { *obj = (" + cppNameWithNamespace + ")xsdcpp::toNumeric(pos, _" + cppName + "_Values, val); }");
        }
        else
        {
            if (!generateTypeSetter(type.baseType))
                return false;
            _cppOutputNamespaceSetValue.append(String("void ") + functionName + "(" + cppNameWithNamespace + "* obj, const xsdcpp::Position& pos, std::string&& val) { " + toSetValueFunctionName(type.baseType)  + "(obj, pos, std::move(val)); }");
        }
        
        return true;
    }

    bool generateAddTextFunction2(const Xsd::Name& typeName, List<String>& flags, String& addTextFunction)
    {
        ReadTextMode readTextMode = getReadTextMode(typeName);
        if (readTextMode == SkipMode)
        {
            addTextFunction = "nullptr";
            return true;
        }
        flags.append("xsdcpp::ElementInfo::ReadTextFlag");
        if (readTextMode == SkipProcessingMode)
            flags.append("xsdcpp::ElementInfo::SkipProcessingFlag");

        if (!generateTypeSetter(typeName))
            return false;
        addTextFunction = String("(xsdcpp::set_value_t)&") + toSetValueFunctionName(typeName);
        return true;
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

        if (isNamespaceExternal(typeName.xsdNamespace))
        {
            _generatedElementInfos2.append(typeName);
            return true;
        }
        
        List<String> flags;

        String addTextFunction;
        if (!generateAddTextFunction2(typeName, flags, addTextFunction))
            return false;

        String flagsStr("0");
        if (!flags.isEmpty())
            flagsStr.join(flags, '|');

        String cppName = toCppTypeIdentifier2(typeName);
        _cppOutputNamespaceSetValue.append(String("const xsdcpp::ElementInfo _") + cppName + "_Info = { " + flagsStr + ", " + addTextFunction + " };");

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
                bool optionalWithoutDefaultValue = !attributeRef.isMandatory && attributeRef.defaultValue.isNull();
                if (!processType2(attributeRef.typeName, level + 1, !optionalWithoutDefaultValue))
                    return false;
                if (optionalWithoutDefaultValue)
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
                _cppOutputAnonymousFieldGetter.append(structDefintiion);
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
                            _cppOutputAnonymousFieldGetter.append(String("void* _get_") + cppName + "_" + toCppFieldIdentifier(elementRef.name) + "_" + toCppFieldIdentifier(subElementRef.name) + "(" + cppNameWithNamespace + "* parent) {return &*(parent->" + toCppFieldIdentifier(elementRef.name) + "." + toCppFieldIdentifier(subElementRef.name) + " = " +  toCppTypeIdentifierWithNamespace2(subElementRef.typeName) + "());}");
                        else if (elementRef.maxOccurs == 1)
                            _cppOutputAnonymousFieldGetter.append(String("void* _get_") + cppName + "_" + toCppFieldIdentifier(elementRef.name) + "_" + toCppFieldIdentifier(subElementRef.name) + "(" + cppNameWithNamespace + "* parent) {return &*((parent->" + toCppFieldIdentifier(elementRef.name) + " = " + toCppTypeIdentifierWithNamespace2(elementRef.typeName) + "())->" + toCppFieldIdentifier(subElementRef.name) + " = " +  toCppTypeIdentifierWithNamespace2(subElementRef.typeName) + "());}");
                        else
                            _cppOutputAnonymousFieldGetter.append(String("void* _get_") + cppName + "_" + toCppFieldIdentifier(elementRef.name) + "_" + toCppFieldIdentifier(subElementRef.name) + "(" + cppNameWithNamespace + "* parent) {return (parent->" + toCppFieldIdentifier(elementRef.name) + ".emplace_back(), &*(parent->" + toCppFieldIdentifier(elementRef.name) + ".back()." + toCppFieldIdentifier(subElementRef.name) + " = " +  toCppTypeIdentifierWithNamespace2(subElementRef.typeName) + "()));}");
                    }
                }
                else
                {
                    if (elementRef.minOccurs == 1 && elementRef.maxOccurs == 1)
                        _cppOutputAnonymousFieldGetter.append(String("void* _get_") + cppName + "_" + toCppFieldIdentifier(elementRef.name) + "(" + cppNameWithNamespace + "* parent) {return &parent->" + toCppFieldIdentifier(elementRef.name) + ";}");
                    else if (elementRef.maxOccurs == 1)
                        _cppOutputAnonymousFieldGetter.append(String("void* _get_") + cppName + "_" + toCppFieldIdentifier(elementRef.name) + "(" + cppNameWithNamespace + "* parent) {return &*(parent->" + toCppFieldIdentifier(elementRef.name) + " = " + toCppTypeIdentifierWithNamespace2(elementRef.typeName) + "());}");
                    else
                        _cppOutputAnonymousFieldGetter.append(String("void* _get_") + cppName + "_" + toCppFieldIdentifier(elementRef.name) + "(" + cppNameWithNamespace + "* parent) {return (parent->" + toCppFieldIdentifier(elementRef.name) + ".emplace_back(), &parent->" + toCppFieldIdentifier(elementRef.name) + ".back());}");
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

                            childElementInfo.append(String("    {\"") + subElementRef.name.name + "\", (xsdcpp::get_field_t)&_get_" + cppName +  "_" + toCppFieldIdentifier(elementRef.name) + "_" + toCppFieldIdentifier(subElementRef.name) + ", &" + toCppNamespacePrefix(subElementRef.typeName) + "::_" + toCppTypeIdentifier2(subElementRef.typeName) + "_Info, 0, " + String::fromUInt(elementRef.maxOccurs)  + "},");
                        }
                    }
                    else
                    {
                        if (!generateElementInfo(elementRef.typeName))
                            return false;

                        childElementInfo.append(String("    {\"") + elementRef.name.name + "\", (xsdcpp::get_field_t)&_get_" + cppName + "_" + toCppFieldIdentifier(elementRef.name) + ", &" + toCppNamespacePrefix(elementRef.typeName) + "::_" + toCppTypeIdentifier2(elementRef.typeName) + "_Info, " + String::fromUInt(elementRef.minOccurs)  + ", " + String::fromUInt(elementRef.maxOccurs)  + "},");
                    }
                }
                childElementInfo.append("    {nullptr}\n};");
                _cppOutputAnonymousFieldGetter.append(childElementInfo);
            }


            HashSet<const Xsd::AttributeRef*> setDefaultValueFunctions;
            for (List<Xsd::AttributeRef>::Iterator i = type.attributes.begin(), end = type.attributes.end(); i != end; ++i)
            {
                const Xsd::AttributeRef& attributeRef = *i;
                if (!processType2(attributeRef.typeName, level + 1, false))
                    return false;

                HashMap<Xsd::Name, Xsd::Type>::Iterator it = _xsd.types.find(attributeRef.typeName);
                if (it == _xsd.types.end())
                    return false;
                const Xsd::Type& attributeType = *it;

                if (!generateTypeSetter(attributeRef.typeName))
                    return false;

                bool optionalWithoutDefaultValue = !attributeRef.isMandatory && attributeRef.defaultValue.isNull();
                if (optionalWithoutDefaultValue)
                    _cppOutputAnonymousFieldGetter.append(String("void* _get_") + cppName + "_" + toCppFieldIdentifier(attributeRef.name) + "(" + toCppTypeIdentifierWithNamespace2(typeName) + "* elem) { return &*(elem->" + toCppFieldIdentifier(attributeRef.name) + " = " + toCppTypeIdentifierWithNamespace2(attributeRef.typeName) + "()); }");
                else
                    _cppOutputAnonymousFieldGetter.append(String("void* _get_") + cppName + "_" + toCppFieldIdentifier(attributeRef.name) + "(" + toCppTypeIdentifierWithNamespace2(typeName) + "* elem) { return &elem->" + toCppFieldIdentifier(attributeRef.name) + "; }");

                if (!attributeRef.isMandatory && !attributeRef.defaultValue.isNull())
                {
                    Xsd::Type rootType = getRootType(attributeRef.typeName);
                    String resolvedDefaultValue = resolveDefaultValue(attributeRef.typeName, rootType, attributeRef.defaultValue.toString());
                    if (resolvedDefaultValue != "\"\"")
                    {
                        setDefaultValueFunctions.append(&attributeRef);
                        _cppOutputAnonymousFieldGetter.append(String("void _default_") + cppName + "_" + toCppFieldIdentifier(attributeRef.name) + "(" + toCppTypeIdentifierWithNamespace2(typeName) + "* element) { element->" + toCppFieldIdentifier(attributeRef.name) + " = " + resolvedDefaultValue + "; }");
                    }
                }
            }
            if (type.flags & Xsd::Type::AnyAttributeFlag)
                _cppOutputAnonymousFieldGetter.append(String("void _any_") + cppName + "(" + toCppTypeIdentifierWithNamespace2(typeName) + "* element, std::string&& name, std::string&& value) { element->other_attributes.emplace_back(xsd::any_attribute{std::move(name), std::move(value)}); }");

            String attributes("nullptr");
            if (!type.attributes.isEmpty())
            {
                attributes = String("_") + cppName + "_Attributes";
                _cppOutputAnonymousFieldGetter.append(String("xsdcpp::AttributeInfo _") + cppName + "_Attributes[] = {");
                for (List<Xsd::AttributeRef>::Iterator i = type.attributes.begin(), end = type.attributes.end(); i != end; ++i)
                {
                    const Xsd::AttributeRef& attributeRef = *i;
                    if (!processType2(attributeRef.typeName, level + 1, false))
                        return false;
                    String setDefault("nullptr");
                    if (setDefaultValueFunctions.contains(&attributeRef))
                        setDefault = String("(xsdcpp::set_default_t)&_default_") + cppName + "_" + toCppFieldIdentifier(attributeRef.name);
                    _cppOutputAnonymousFieldGetter.append(String("    {\"") + attributeRef.name.name + "\", (xsdcpp::get_field_t)&_get_" + cppName + "_" + toCppFieldIdentifier(attributeRef.name) + ", (xsdcpp::set_value_t)&" + toSetValueFunctionName(attributeRef.typeName) + ", " + (attributeRef.isMandatory ? String("true") : String("false")) +  ", " + setDefault + "},");
                }
                _cppOutputAnonymousFieldGetter.append("    {nullptr}\n};");
            }

            usize childrenCount = getChildrenCount(typeName);
            usize mandatoryChildrenCount = getMandatoryChildrenCount(typeName);
            uint64 attributesCount = getAttributesCount(typeName);
            List<String> flags;
            if (level == 1)
                flags.append("xsdcpp::ElementInfo::EntryPointFlag");
            if (type.flags & Xsd::Type::AnyAttributeFlag)
                flags.append("xsdcpp::ElementInfo::AnyAttributeFlag");

            String addTextFunction;
            if (!generateAddTextFunction2(typeName, flags, addTextFunction))
                return false;

            String parentElementCppName;
            if (baseType && baseType->kind == Xsd::Type::Kind::ElementKind)
                parentElementCppName = toCppTypeIdentifier2(type.baseType);

            String flagsStr("0");
            if (!flags.isEmpty())
                flagsStr.join(flags, '|');

            _cppOutputNamespace.append(String("const xsdcpp::ElementInfo _") + cppName + "_Info = { " + flagsStr 
                + ", " + addTextFunction
                + ", " + children + ", " + String::fromUInt64(childrenCount) + ", " + String::fromUInt64(mandatoryChildrenCount) 
                + ", " + attributes + ", " + String::fromUInt64(attributesCount) + ", " + (parentElementCppName.isEmpty() ? String("nullptr") : String("&") + toCppNamespacePrefix(type.baseType) + "::_" + parentElementCppName + "_Info") 
                + ", " + (type.flags & Xsd::Type::AnyAttributeFlag ? String("(xsdcpp::set_any_attribute_t)&_any_") + cppName : String("nullptr"))
                + " };");

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
