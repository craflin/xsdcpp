
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
    Generator(const Xsd& xsd, List<String>& cppOutput, List<String>& hppOutput)
        : _xsd(xsd)
        , _cppOutput(cppOutput)
        , _hppOutput(hppOutput)
    {
    }

    const String& getError() const { return _error; }

    bool process()
    {
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

        _hppOutput.append("");
        _hppOutput.append("#pragma once");
        _hppOutput.append("");
        _hppOutput.append(String("#include \"") + _cppNamespace + "_xsd.hpp\"");
        _hppOutput.append("");

        _hppOutput.append(String("namespace ") + _cppNamespace + " {");
        _hppOutput.append("");

        Map<Xsd::Name, String> elementTypes;
        Map<Xsd::Name, String> substitutionGroupTypes;
        collectElementTypes(_xsd.rootType, elementTypes, substitutionGroupTypes);

        for (Map<Xsd::Name, String>::Iterator i = elementTypes.begin(), end = elementTypes.end(); i != end; ++i)
        {
            const String& cppName = *i;
            if (i.key() == _xsd.rootType)
                continue;
            _cppOutput.append(String("extern const ElementInfo _") + cppName + "_Info;");
            _hppOutput.append(String("struct ") + cppName + ";");
        }
        for (Map<Xsd::Name, String>::Iterator i = substitutionGroupTypes.begin(), end = substitutionGroupTypes.end(); i != end; ++i)
        {
            const String& cppName = *i;
            _hppOutput.append(String("struct ") + cppName + ";");
        }

        _cppOutput.append("");
        _hppOutput.append("");

        String rootTypeCppName;
        if (!processType2(_xsd.rootType, rootTypeCppName, 0, true))
            return false;
        for (HashMap<Xsd::Name, String>::Iterator i = _requiredTypes.begin(), end = _requiredTypes.end(); i != end; ++i)
        {
            String cppName;
            if (!generateType(i.key(), cppName, 1))
                return false;
        }

        HashMap<Xsd::Name, Xsd::Type>::Iterator it = _xsd.types.find(_xsd.rootType);
        Xsd::Type& rootType = *it;

        for (List<Xsd::ElementRef>::Iterator i = rootType.elements.begin(), end = rootType.elements.end(); i != end; ++i)
        {
            String elementTypeCppName = *_generatedTypes.find(i->typeName);
            String elementCppName = toCppFieldIdentifier(i->name);

            _hppOutput.append(String("void load_file(const std::string& file, ") + elementTypeCppName + "& " + elementCppName + ");");
            _hppOutput.append(String("void load_data(const std::string& data, ") + elementTypeCppName + "& " + elementCppName + ");");
            _hppOutput.append("");
        }

        _hppOutput.append("}");

        _cppOutput.append("");
        _cppOutput.append("}");
        _cppOutput.append("");
        _cppOutput.append("#include <fstream>");
        _cppOutput.append("#include <system_error>");
        _cppOutput.append("");

        _cppOutput.append(String("namespace ") + _cppNamespace + " {");
        _cppOutput.append("");

        _cppOutput.append("void fixUnusedToTypeWarning()");
        _cppOutput.append("{");
        _cppOutput.append("    toType(Position(), nullptr, std::string());");
        _cppOutput.append("    toType<uint64_t>(Position(), std::string());");
        _cppOutput.append("    toType<int64_t>(Position(), std::string());");
        _cppOutput.append("    toType<uint32_t>(Position(), std::string());");
        _cppOutput.append("    toType<int32_t>(Position(), std::string());");
        _cppOutput.append("    toType<uint16_t>(Position(), std::string());");
        _cppOutput.append("    toType<int16_t>(Position(), std::string());");
        _cppOutput.append("    toType<double>(Position(), std::string());");
        _cppOutput.append("    toType<float>(Position(), std::string());");
        _cppOutput.append("    toType<bool>(Position(), std::string());");
        _cppOutput.append("}");
        _cppOutput.append("");

        for (List<Xsd::ElementRef>::Iterator i = rootType.elements.begin(), end = rootType.elements.end(); i != end; ++i)
        {
            String elementTypeCppName = *_generatedTypes.find(i->typeName);
            String elementCppName = toCppFieldIdentifier(i->name);

            _cppOutput.append(String("void load_data(const std::string& data, ") + elementTypeCppName + "& output)");
            _cppOutput.append("{");
            _cppOutput.append(String("    ") + rootTypeCppName + " rootElement;");
            _cppOutput.append("    ElementContext elementContext;");
            _cppOutput.append(String("    elementContext.info = &_") + rootTypeCppName + "_Info;");
            _cppOutput.append("    elementContext.element = &rootElement;");
            _cppOutput.append("    parse(data.c_str(), _namespaces, elementContext);");
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
    
    List<String>& _cppOutput;
    List<String>& _hppOutput;
    String _cppNamespace;
    HashMap<Xsd::Name, String> _generatedTypes;
    HashMap<Xsd::Name, String> _requiredTypes;
    String _error;

private:
    String toCppTypeIdentifier(const Xsd::Name& name)
    {
        if (name.namespace_ == _xsd.targetNamespaces.front())
            return toCppIdentifier(name.name);
        HashMap<String, String>::Iterator it = _xsd.namespaceToSuggestedPrefix.find(name.namespace_);
        if (it != _xsd.namespaceToSuggestedPrefix.end())
            return toCppIdentifier(*it + "_" + name.name);
        return toCppIdentifier(name.name);
    }

    String toCppTypeWithNamespace(const String& cppName)
    {
        if (cppName.find("::") ||
            cppName == "_root_t" ||
            cppName == "uint64_t" ||
            cppName == "int64_t" ||
            cppName == "uint32_t" ||
            cppName == "int32_t" ||
            cppName == "uint16_t" ||
            cppName == "int16_t" ||
            cppName == "double" ||
            cppName == "float" ||
            cppName == "bool")
            return cppName;
        return _cppNamespace + "::" + cppName;
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
        ReadAndProcessMode,
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
        if ((type.kind == Xsd::Type::Kind::ElementKind || Xsd::Type::Kind::SimpleRefKind) && !type.baseType.name.isEmpty())
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

    ReadTextMode getReadTextMode(const Xsd::Name& typeName) const
    {
        Xsd::Type rootType = getType(getRootTypeName(typeName));
        if (rootType.kind == Xsd::Type::Kind::StringKind)
        {
            if (isSkipProcessContentsFlagSet(typeName) && getChildrenCount(typeName) == 0)
                return SkipProcessingMode;
            return ReadAndProcessMode;
        }
        if (rootType.kind == Xsd::Type::Kind::BaseKind || rootType.kind == Xsd::Type::Kind::EnumKind)
            return ReadAndProcessMode;
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

    const Xsd::Type& resolveFinalType(const Xsd::Type& type)
    {
        if (type.kind == Xsd::Type::SimpleRefKind)
        {
            HashMap<Xsd::Name, Xsd::Type>::Iterator it = _xsd.types.find(type.baseType);
            if (it != _xsd.types.end())
                return resolveFinalType(*it);
        }
        return type;
    }

    String resolveDefaultValue(const String& attributeTypeCppName, const Xsd::Type& finalType, const String& defaultValue)
    {
        if (finalType.kind == Xsd::Type::EnumKind)
        {
            uint value = 0;
            for (List<String>::Iterator i = finalType.enumEntries.begin(), end = finalType.enumEntries.end(); i != end; ++i, ++value)
                if (*i == defaultValue)
                    return String("(") + toCppTypeWithNamespace(attributeTypeCppName) + ")"  + String::fromUInt(value);
        }
        if (finalType.kind == Xsd::Type::StringKind)
            return toCStringLiteral(defaultValue);
        return defaultValue;
    }

    
    void collectElementTypes(const Xsd::Name& typeName, Map<Xsd::Name, String>& elementTypes, Map<Xsd::Name, String>& substitutionGroupTypes)
    {
        HashMap<Xsd::Name, Xsd::Type>::Iterator it2 = _xsd.types.find(typeName);
        if (it2 == _xsd.types.end())
            return;
        Xsd::Type& type = *it2;

        if (type.kind == Xsd::Type::SubstitutionGroupKind)
        {
            if (substitutionGroupTypes.contains(typeName))
                return;

            String cppName = toCppTypeIdentifier(typeName);
            substitutionGroupTypes.insert(typeName, cppName);

            for (List<Xsd::ElementRef>::Iterator i = type.elements.begin(), end = type.elements.end(); i != end; ++i)
            {
                const Xsd::ElementRef& elementRef = *i;
                collectElementTypes(elementRef.typeName, elementTypes, substitutionGroupTypes);
            }
        }

        if (type.kind == Xsd::Type::ElementKind)
        {
            if (elementTypes.contains(typeName))
                return;

            String cppName = toCppTypeIdentifier(typeName);
            elementTypes.insert(typeName, cppName);

            if (!type.baseType.name.isEmpty())
                collectElementTypes(type.baseType, elementTypes, substitutionGroupTypes);

            for (List<Xsd::ElementRef>::Iterator i = type.elements.begin(), end = type.elements.end(); i != end; ++i)
            {
                const Xsd::ElementRef& elementRef = *i;
                collectElementTypes(elementRef.typeName, elementTypes, substitutionGroupTypes);
            }
        }
    }

    bool processType2(const Xsd::Name& typeName, String& cppName, usize level, bool generate)
    {
        HashMap<Xsd::Name, String>::Iterator it = _generatedTypes.find(typeName);
        if (it != _generatedTypes.end())
        {
            cppName = *it;
            return true;
        }

        HashMap<Xsd::Name, Xsd::Type>::Iterator it2 = _xsd.types.find(typeName);
        if (it2 == _xsd.types.end())
            return _error = String::fromPrintf("Type '%s' not found", (const char*)typeName.name), false;
        Xsd::Type& type = *it2;

        if (generate || (type.kind != Xsd::Type::SubstitutionGroupKind && type.kind != Xsd::Type::ElementKind))
            return generateType(typeName, cppName, level); // create full type definition now

        HashMap<Xsd::Name, String>::Iterator it3 = _requiredTypes.find(typeName);
        if (it3 != _requiredTypes.end())
        {
            cppName = *it3;
            return true;
        }

        cppName = toCppTypeIdentifier(typeName);
        _requiredTypes.append(typeName, cppName); // create full type definition later
        return true;
    }

    bool generateType(const Xsd::Name& typeName, String& cppName, usize level)
    {
        HashMap<Xsd::Name, String>::Iterator it = _generatedTypes.find(typeName);
        if (it != _generatedTypes.end())
        {
            cppName = *it;
            return true;
        }

        HashMap<Xsd::Name, Xsd::Type>::Iterator it2 = _xsd.types.find(typeName);
        if (it2 == _xsd.types.end())
            return _error = String::fromPrintf("Type '%s' not found", (const char*)typeName.name), false;
        Xsd::Type& type = *it2;

        if (type.kind == Xsd::Type::StringKind)
        {
            if (compareXsName(typeName, "normalizedString") || compareXsName(typeName, "string") || compareXsName(typeName, "anyURI"))
            {
                cppName = "xsd::string";
                _generatedTypes.append(typeName, cppName);
                return true;
            }
        }

        if (type.kind == Xsd::Type::BaseKind)
        {
            if (compareXsName(typeName, "nonNegativeInteger") || compareXsName(typeName, "positiveInteger") || compareXsName(typeName, "unsignedLong"))
            {
                cppName = "uint64_t";
                _generatedTypes.append(typeName, cppName);
                return true;
            }

            if (compareXsName(typeName, "integer") || compareXsName(typeName, "long"))
            {
                cppName = "int64_t";
                _generatedTypes.append(typeName, cppName);
                return true;
            }

            if (compareXsName(typeName, "unsignedInt"))
            {
                cppName = "uint32_t";
                _generatedTypes.append(typeName, cppName);
                return true;
            }

            if (compareXsName(typeName, "int"))
            {
                cppName = "int32_t";
                _generatedTypes.append(typeName, cppName);
                return true;
            }

            if (compareXsName(typeName, "unsignedShort"))
            {
                cppName = "uint16_t";
                _generatedTypes.append(typeName, cppName);
                return true;
            }

            if (compareXsName(typeName, "short"))
            {
                cppName = "int16_t";
                _generatedTypes.append(typeName, cppName);
                return true;
            }

            if (compareXsName(typeName, "double") || compareXsName(typeName, "decimal"))
            {
                cppName = "double";
                _generatedTypes.append(typeName, cppName);
                return true;
            }

            if (compareXsName(typeName, "float"))
            {
                cppName = "float";
                _generatedTypes.append(typeName, cppName);
                return true;
            }

            if (compareXsName(typeName, "boolean"))
            {
                cppName = "bool";
                _generatedTypes.append(typeName, cppName);
                return true;
            }

            return _error = String::fromPrintf("Base type '%s' not supported", (const char*)typeName.name), false;
        }

        if (type.kind == Xsd::Type::SimpleRefKind)
        {
            cppName = toCppTypeIdentifier(typeName);
            _generatedTypes.append(typeName, cppName);

            String baseCppName;
            if (!processType2(type.baseType, baseCppName, level + 1, true))
                return false;

            _hppOutput.append(String("typedef ") + baseCppName + " " + cppName + ";");
            _hppOutput.append("");
            return true;
        }

        if (type.kind == Xsd::Type::StringKind || type.kind == Xsd::Type::UnionKind)
        {
            cppName = toCppTypeIdentifier(typeName);
            _generatedTypes.append(typeName, cppName);
            _hppOutput.append(String("typedef xsd::string ") + cppName + ";");
            _hppOutput.append("");
            return true;
        }

        if (type.kind == Xsd::Type::ListKind)
        {
            cppName = toCppTypeIdentifier(typeName);
            _generatedTypes.append(typeName, cppName);

            String baseCppName;
            if (!processType2(type.baseType, baseCppName, level + 1, false))
                return false;

            _hppOutput.append(String("typedef xsd::vector<") + baseCppName + "> " + cppName + ";");
            _hppOutput.append("");
            return true;
        }

        if (type.kind == Xsd::Type::EnumKind)
        {
            cppName = toCppTypeIdentifier(typeName);
            String cppNameWithNamespace = toCppTypeWithNamespace(cppName);
            _generatedTypes.append(typeName, cppName);

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
            _cppOutput.append("template<>");
            _cppOutput.append(cppNameWithNamespace + " toType<" + cppNameWithNamespace + ">(const Position& pos, const std::string& value) {return (" + cppNameWithNamespace + ")toType(pos, _" + cppName + "_Values, value);}");
            _cppOutput.append("");
            return true;
        }

        if (type.kind == Xsd::Type::SubstitutionGroupKind)
        {
            cppName = toCppTypeIdentifier(typeName);
            _generatedTypes.append(typeName, cppName);

            List<String> structFields;
            for (List<Xsd::ElementRef>::Iterator i = type.elements.begin(), end = type.elements.end(); i != end; ++i)
            {
                const Xsd::ElementRef& elementRef = *i;
                String elementCppName;
                if (!processType2(elementRef.typeName, elementCppName, level + 1, false))
                    return false;
                structFields.append(String("xsd::optional<") + toCppTypeWithNamespace(elementCppName) + "> " + toCppFieldIdentifier(elementRef.name));
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
            cppName = toCppTypeIdentifier(typeName);
            String cppNameWithNamespace = toCppTypeWithNamespace(cppName);
            _generatedTypes.append(typeName, cppName);

            String baseCppName;
            Xsd::Type* baseType = nullptr;
            if (!type.baseType.name.isEmpty())
            {
                if (!processType2(type.baseType, baseCppName, level + 1, true))
                    return false;
                baseType = &*_xsd.types.find(type.baseType);
            }

            List<String> structFields;
            for (List<Xsd::AttributeRef>::Iterator i = type.attributes.begin(), end = type.attributes.end(); i != end; ++i)
            {
                const Xsd::AttributeRef& attributeRef = *i;
                String attributeCppName;
                if (!processType2(attributeRef.typeName, attributeCppName, level + 1, true))
                    return false;
                structFields.append(attributeCppName + " " + toCppFieldIdentifier(attributeRef.name));
            }
            if (type.flags & Xsd::Type::AnyAttributeFlag)
                structFields.append("xsd::vector<xsd::any_attribute> other_attributes");
            for (List<Xsd::ElementRef>::Iterator i = type.elements.begin(), end = type.elements.end(); i != end; ++i)
            {
                const Xsd::ElementRef& elementRef = *i;
                String elementCppName;
                bool typeDefinitionRequired = elementRef.minOccurs == 1 && elementRef.maxOccurs == 1;
                if (!processType2(elementRef.typeName, elementCppName, level + 1, typeDefinitionRequired))
                    return false;
                if (elementRef.minOccurs == 1 && elementRef.maxOccurs == 1)
                    structFields.append(toCppTypeWithNamespace(elementCppName) + " " + toCppFieldIdentifier(elementRef.name));
                else if (elementRef.maxOccurs == 1)
                    structFields.append(String("xsd::optional<") + toCppTypeWithNamespace(elementCppName) + "> " + toCppFieldIdentifier(elementRef.name));
                else
                    structFields.append(String("xsd::vector<") + toCppTypeWithNamespace(elementCppName) + "> " + toCppFieldIdentifier(elementRef.name));
            }

            List<String> structDefintiion;
            if (baseType)
            {
                if (baseType->kind == Xsd::Type::BaseKind || baseType->kind == Xsd::Type::EnumKind)
                    structDefintiion.append(String("struct ") + cppName + " : xsd::base<" + baseCppName + ">");
                else
                    structDefintiion.append(String("struct ") + cppName + " : " + baseCppName);
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
                String elementCppName;
                if (!processType2(elementRef.typeName, elementCppName, level + 1, false))
                    return false;
                const Xsd::Type& type = *_xsd.types.find(elementRef.typeName);
                if (type.kind == Xsd::Type::SubstitutionGroupKind)
                {
                    for (List<Xsd::ElementRef>::Iterator i = type.elements.begin(), end = type.elements.end(); i != end; ++i)
                    {
                        const Xsd::ElementRef& subElementRef = *i;
                        String subElementCppName;
                        if (!processType2(subElementRef.typeName, subElementCppName, level + 1, false))
                            return false;

                        if (elementRef.minOccurs == 1 && elementRef.maxOccurs == 1)
                            _cppOutput.append(toCppTypeWithNamespace(subElementCppName) + "* get_" + cppName + "_" + toCppTypeIdentifier(elementRef.name) + "_" + toCppTypeIdentifier(subElementRef.name) + "(" + cppNameWithNamespace + "* parent) {return &*(parent->" + toCppFieldIdentifier(elementRef.name) + "." + toCppFieldIdentifier(subElementRef.name) + " = " +  toCppTypeWithNamespace(subElementCppName) + "());}");
                        else if (elementRef.maxOccurs == 1)
                            _cppOutput.append(toCppTypeWithNamespace(subElementCppName) + "* get_" + cppName + "_" + toCppTypeIdentifier(elementRef.name) + "_" + toCppTypeIdentifier(subElementRef.name) + "(" + cppNameWithNamespace + "* parent) {return &*((parent->" + toCppFieldIdentifier(elementRef.name) + " = " + toCppTypeWithNamespace(elementCppName) + "())->" + toCppFieldIdentifier(subElementRef.name) + " = " +  toCppTypeWithNamespace(subElementCppName) + "());}");
                        else
                            _cppOutput.append(toCppTypeWithNamespace(subElementCppName) + "* get_" + cppName + "_" + toCppTypeIdentifier(elementRef.name) + "_" + toCppTypeIdentifier(subElementRef.name) + "(" + cppNameWithNamespace + "* parent) {return (parent->" + toCppFieldIdentifier(elementRef.name) + ".emplace_back(), &*(parent->" + toCppFieldIdentifier(elementRef.name) + ".back()." + toCppFieldIdentifier(subElementRef.name) + " = " +  toCppTypeWithNamespace(subElementCppName) + "()));}");
                    }
                }
                else
                {
                    if (elementRef.minOccurs == 1 && elementRef.maxOccurs == 1)
                        _cppOutput.append(toCppTypeWithNamespace(elementCppName) + "* get_" + cppName + "_" + toCppTypeIdentifier(elementRef.name) + "(" + cppNameWithNamespace + "* parent) {return &parent->" + toCppFieldIdentifier(elementRef.name) + ";}");
                    else if (elementRef.maxOccurs == 1)
                        _cppOutput.append(toCppTypeWithNamespace(elementCppName) + "* get_" + cppName + "_" + toCppTypeIdentifier(elementRef.name) + "(" + cppNameWithNamespace + "* parent) {return &*(parent->" + toCppFieldIdentifier(elementRef.name) + " = " + toCppTypeWithNamespace(elementCppName) + "());}");
                    else
                        _cppOutput.append(toCppTypeWithNamespace(elementCppName) + "* get_" + cppName + "_" + toCppTypeIdentifier(elementRef.name) + "(" + cppNameWithNamespace + "* parent) {return (parent->" + toCppFieldIdentifier(elementRef.name) + ".emplace_back(), &parent->" + toCppFieldIdentifier(elementRef.name) + ".back());}");
                }
            }

            String children("nullptr");
            if (!type.elements.isEmpty())
            {
                children = String("_") + cppName + "_Children";
                _cppOutput.append(String("ChildElementInfo _") + cppName + "_Children[] = {");
                for (List<Xsd::ElementRef>::Iterator i = type.elements.begin(), end = type.elements.end(); i != end; ++i)
                {
                    const Xsd::ElementRef& elementRef = *i;
                    String elementCppName;
                    if (!processType2(elementRef.typeName, elementCppName, level + 1, false))
                        return false;

                    const Xsd::Type& type = *_xsd.types.find(elementRef.typeName);
                    if (type.kind == Xsd::Type::SubstitutionGroupKind)
                    {
                        for (List<Xsd::ElementRef>::Iterator i = type.elements.begin(), end = type.elements.end(); i != end; ++i)
                        {
                            const Xsd::ElementRef& subElementRef = *i;
                            String subElementCppName;
                            if (!processType2(subElementRef.typeName, subElementCppName, level + 1, false))
                                return false;
                            _cppOutput.append(String("    {\"") + subElementRef.name.name + "\", (get_element_field_t)&get_" + cppName +  "_" + toCppTypeIdentifier(elementRef.name) + "_" + toCppTypeIdentifier(subElementRef.name) + ", &_" + subElementCppName + "_Info, 0, " + String::fromUInt(elementRef.maxOccurs)  + "},");
                        }
                    }
                    else
                        _cppOutput.append(String("    {\"") + elementRef.name.name + "\", (get_element_field_t)&get_" + cppName + "_" + toCppTypeIdentifier(elementRef.name) + ", &_" + elementCppName + "_Info, " + String::fromUInt(elementRef.minOccurs)  + ", " + String::fromUInt(elementRef.maxOccurs)  + "},");
                }
                _cppOutput.append("    {nullptr}\n};");
            }

            for (List<Xsd::AttributeRef>::Iterator i = type.attributes.begin(), end = type.attributes.end(); i != end; ++i)
            {
                const Xsd::AttributeRef& attributeRef = *i;
                String attributeTypeCppName;
                if (!processType2(attributeRef.typeName, attributeTypeCppName, level + 1, false))
                    return false;

                HashMap<Xsd::Name, Xsd::Type>::Iterator it = _xsd.types.find(attributeRef.typeName);
                if (it == _xsd.types.end())
                    return false;
                Xsd::Type& type = *it;
                const Xsd::Type& finalType = resolveFinalType(type);
                if (finalType.kind == Xsd::Type::StringKind || type.kind == Xsd::Type::UnionKind)
                    _cppOutput.append(String("void set_") + cppName + "_" + toCppTypeIdentifier(attributeRef.name) + "(" + toCppTypeWithNamespace(cppName) + "* element, const Position&, std::string&& value) { element->" + toCppFieldIdentifier(attributeRef.name) + " = std::move(value); }");
                else
                    _cppOutput.append(String("void set_") + cppName + "_" + toCppTypeIdentifier(attributeRef.name) + "(" + toCppTypeWithNamespace(cppName) + "* element, const Position& pos, std::string&& value) { element->" + toCppFieldIdentifier(attributeRef.name) + " = toType<" + toCppTypeWithNamespace(attributeTypeCppName) + ">(pos, value); }");
            
                if (!attributeRef.isMandatory && !attributeRef.defaultValue.isEmpty())
                    _cppOutput.append(String("void default_") + cppName + "_" + toCppTypeIdentifier(attributeRef.name) + "(" + toCppTypeWithNamespace(cppName) + "* element) { element->" + toCppFieldIdentifier(attributeRef.name) + " = " + resolveDefaultValue(attributeTypeCppName, finalType, attributeRef.defaultValue) + "; }");
            }
            if (type.flags & Xsd::Type::AnyAttributeFlag)
                _cppOutput.append(String("void any_") + cppName + "(" + toCppTypeWithNamespace(cppName) + "* element, std::string&& name, std::string&& value) { element->other_attributes.emplace_back(xsd::any_attribute{std::move(name), std::move(value)}); }");

            String attributes("nullptr");
            if (!type.attributes.isEmpty())
            {
                attributes = String("_") + cppName + "_Attributes";
                _cppOutput.append(String("AttributeInfo _") + cppName + "_Attributes[] = {");
                for (List<Xsd::AttributeRef>::Iterator i = type.attributes.begin(), end = type.attributes.end(); i != end; ++i)
                {
                    const Xsd::AttributeRef& attributeRef = *i;
                    String attributeCppName;
                    if (!processType2(attributeRef.typeName, attributeCppName, level + 1, false))
                        return false;
                    String setDefault = (attributeRef.isMandatory || attributeRef.defaultValue.isEmpty()) ? String("nullptr") : String("(set_attribute_default_t)&default_") + cppName + "_" + toCppTypeIdentifier(attributeRef.name);
                    _cppOutput.append(String("    {\"") + attributeRef.name.name + "\", (set_attribute_t)&set_" + cppName + "_" + toCppTypeIdentifier(attributeRef.name) + ", " + (attributeRef.isMandatory ? String("true") : String("false")) +  ", " + setDefault + "},");
                }
                _cppOutput.append("    {nullptr}\n};");
            }

            usize childrenCount = getChildrenCount(typeName);
            usize mandatoryChildrenCount = getMandatoryChildrenCount(typeName);
            uint64 attributesCount = getAttributesCount(typeName);
            String flags = "0";
            if (level == 1)
                flags.append("|ElementInfo::Level1Flag");
            if (type.flags & Xsd::Type::AnyAttributeFlag)
                flags.append("|ElementInfo::AnyAttributeFlag");
            ReadTextMode readTextMode = getReadTextMode(typeName);
            switch (readTextMode)
            {
            case SkipMode:
                break;
            case ReadAndProcessMode:
                flags.append("|ElementInfo::ReadTextFlag");
                break;
            case SkipProcessingMode:
                flags.append("|ElementInfo::ReadTextFlag|ElementInfo::SkipProcessingFlag");
                break;
            }
            if (flags.startsWith("0|"))
                flags = flags.substr(2);

            if (baseType && (baseType->kind == Xsd::Type::Kind::StringKind || baseType->kind == Xsd::Type::Kind::EnumKind || baseType->kind == Xsd::Type::Kind::BaseKind))
                baseCppName.clear();

            if (readTextMode != SkipMode)
            {
                Xsd::Name rootTypeName = getRootTypeName(typeName);
                Xsd::Type rootType = getType(rootTypeName);
                if (rootType.kind == Xsd::Type::Kind::StringKind)
                    _cppOutput.append(String("void add_text_") + cppName + "(" + toCppTypeWithNamespace(cppName) + "* element, const Position& pos, std::string&& text) { xsd::string& str = *element; if (str.empty()) str = std::move(text); else str += text; }");
                else
                {
                    String rootTypeCppName = *_generatedTypes.find(rootTypeName);
                    String rootTypeCppNameWithNamespace = toCppTypeWithNamespace(rootTypeCppName);
                    _cppOutput.append(String("void add_text_") + cppName + "(" + toCppTypeWithNamespace(cppName) + "* element, const Position& pos, std::string&& text) { xsd::base<" + rootTypeCppNameWithNamespace+ ">& base = *element; base = toType<" + rootTypeCppNameWithNamespace + ">(pos, text); }");
                }
            }

            _cppOutput.append(String("const ElementInfo _") + cppName + "_Info = { " + flags + ", " + children + ", " + String::fromUInt64(childrenCount) + ", " + String::fromUInt64(mandatoryChildrenCount) 
                + ", " + attributes + ", " + String::fromUInt64(attributesCount) + ", " + (baseCppName.isEmpty() ? String("nullptr") : String("&_") + baseCppName + "_Info") 
                + ", " + (type.flags & Xsd::Type::AnyAttributeFlag ? String("(set_any_attribute_t)&any_") + cppName : String("nullptr"))
                + ", " + (readTextMode != SkipMode ? String("(add_text_t)&add_text_") + cppName : String("nullptr"))
                + " };");
            _cppOutput.append("");

            return true;
        }

        return false; // logic error
    }
};

}

bool generateCpp(const Xsd& xsd, const String& outputDir, String& error)
{
    List<String> cppOutput;
    List<String> hppOutput;
    Generator generator(xsd, cppOutput, hppOutput);
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
