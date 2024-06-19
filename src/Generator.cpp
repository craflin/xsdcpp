
#include "Generator.hpp"

#include <nstd/Console.hpp>
#include <nstd/Error.hpp>
#include <nstd/File.hpp>
#include <nstd/HashMap.hpp>

#include <Resources.hpp>

namespace {

String toCppTypeIdentifier(const String& str)
{
    String result = str;
    for (char* i = result; *i; ++i)
    {
        if (*i == '+')
            *i = 'p';
        else if (!String::isAlphanumeric(*i))
            *i = '_';
    }
    if (String::isDigit(*(char*)result))
        result.prepend("_");
    if (result == "union" || result == "enum" || result == "linux")
        result.append("_");
    return result;
}

String stripNamespacePrefix(const String& str)
{
    const char* x = str.find(':');
    if (!x)
        return str;
    return str.substr(x - (const char*)str + 1);
}

String toCppFieldIdentifier(const String& str)
{
    return toCppTypeIdentifier(stripNamespacePrefix(str));
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
        String cppName = toCppTypeIdentifier(_xsd.name);

        _cppOutput.append("");
        _cppOutput.append(String("#include \"") + cppName + ".hpp\"");
        _cppOutput.append("");

        _cppOutput.append("namespace {");
        _cppOutput.append("");

        _cppOutput.append(String("using namespace ") + cppName + ";");
        _cppOutput.append("");

        _hppOutput.append("");
        _hppOutput.append("#pragma once");
        _hppOutput.append("");
        _hppOutput.append(String("#include \"") + cppName + "_xsd.hpp\"");
        _hppOutput.append("");

        _hppOutput.append(String("namespace ") + cppName + " {");
        _hppOutput.append("");

        String rootTypeCppName;
        if (!processType(_xsd.rootType, rootTypeCppName, 0))
            return false;

        HashMap<String, Xsd::Type>::Iterator it = _xsd.types.find(_xsd.rootType);
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

        _cppOutput.append(String("namespace ") + cppName + " {");
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
            _cppOutput.append("    parse(data.c_str(), elementContext);");
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
    HashMap<String, String> _generatedTypes;
    String _error;

private:
    bool compareXsName(const String& fullName, const String& name) const
    {
        if (_xsd.xmlSchemaNamespacePrefix.isEmpty())
            return fullName == name;

        return fullName.startsWith(_xsd.xmlSchemaNamespacePrefix) && ((const char*)fullName)[_xsd.xmlSchemaNamespacePrefix.length()] &&
            String::compare((const char*)fullName + _xsd.xmlSchemaNamespacePrefix.length() + 1, (const char*)name) == 0;
    }

    usize getChildrenCount(const String& typeName) const
    {
        if (typeName.isEmpty())
            return 0;
        HashMap<String, Xsd::Type>::Iterator it = _xsd.types.find(typeName);
        if (it == _xsd.types.end())
            return 0;
        const Xsd::Type& type = *it;
        return type.elements.size() + getChildrenCount(type.baseType);
    }

    usize getMandatoryChildrenCount(const String& typeName) const
    {
        if (typeName.isEmpty())
            return 0;
        HashMap<String, Xsd::Type>::Iterator it = _xsd.types.find(typeName);
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

    usize getAttributesCount(const String& typeName) const
    {
        if (typeName.isEmpty())
            return 0;
        HashMap<String, Xsd::Type>::Iterator it = _xsd.types.find(typeName);
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

    bool isSkipProcessContentsFlagSet(const String& typeName) const 
    {
        if (typeName.isEmpty())
            return false;
        HashMap<String, Xsd::Type>::Iterator it2 = _xsd.types.find(typeName);
        if (it2 == _xsd.types.end())
            return false;
        if (it2->flags & Xsd::Type::SkipProcessContentsFlag)
            return true;
        const Xsd::Type& type = *it2;
        return isSkipProcessContentsFlagSet(type.baseType);
    }

    bool isBaseTypeString(const String& typeName) const
    {
        if (typeName.isEmpty())
            return false;
        HashMap<String, String>::Iterator it = _generatedTypes.find(typeName);
        if (it != _generatedTypes.end())
        {
            const String& cppTypeName = *it;
            if (cppTypeName == "xsd::string")
                return true;
        }
        HashMap<String, Xsd::Type>::Iterator it2 = _xsd.types.find(typeName);
        if (it2 == _xsd.types.end())
            return false;
        const Xsd::Type& type = *it2;
        return isBaseTypeString(type.baseType);
    }

    ReadTextMode getReadTextMode(const String& typeName) const
    {
        if (typeName.isEmpty() || !isBaseTypeString(typeName))
            return SkipMode;
        if (isSkipProcessContentsFlagSet(typeName) && getChildrenCount(typeName) == 0)
            return SkipProcessingMode;
        return ReadAndProcessMode;
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
            HashMap<String, Xsd::Type>::Iterator it = _xsd.types.find(type.baseType);
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
                    return String("(") + attributeTypeCppName + ")"  + String::fromUInt(value);
        }
        if (finalType.kind == Xsd::Type::StringKind)
            return toCStringLiteral(defaultValue);
        return defaultValue;
    }

    bool processType(const String& typeName, String& cppName, usize level)
    {
        HashMap<String, String>::Iterator it = _generatedTypes.find(typeName);
        if (it != _generatedTypes.end())
        {
            cppName = *it;
            return true;
        }

        HashMap<String, Xsd::Type>::Iterator it2 = _xsd.types.find(typeName);
        if (it2 == _xsd.types.end())
            return _error = String::fromPrintf("Type '%s' not found", (const char*)typeName), false;
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

            return _error = String::fromPrintf("Base type '%s' not supported", (const char*)typeName), false;
        }

        if (type.kind == Xsd::Type::SimpleRefKind)
        {
            cppName = toCppTypeIdentifier(typeName);
            _generatedTypes.append(typeName, cppName);

            String baseCppName;
            if (!processType(type.baseType, baseCppName, level + 1))
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
            if (!processType(type.baseType, baseCppName, level + 1))
                return false;

            _hppOutput.append(String("typedef xsd::vector<") + baseCppName + "> " + cppName + ";");
            _hppOutput.append("");
            return true;
        }

        if (type.kind == Xsd::Type::EnumKind)
        {
            cppName = toCppTypeIdentifier(typeName);
            _generatedTypes.append(typeName, cppName);

            _hppOutput.append(String("enum class ") + cppName);
            _hppOutput.append("{");
            for (List<String>::Iterator i = type.enumEntries.begin(), end = type.enumEntries.end(); i != end; ++i)
                _hppOutput.append(String("    ") + toCppFieldIdentifier(*i) + ",");
            _hppOutput.append("};");
            _hppOutput.append("");

            _cppOutput.append(String("const char* _") + cppName + "_Values[] = {");
            for (List<String>::Iterator i = type.enumEntries.begin(), end = type.enumEntries.end(); i != end; ++i)
                _cppOutput.append(String("    \"") + *i + "\",");
            _cppOutput.append("    nullptr};");
            _cppOutput.append("template<>");
            _cppOutput.append(cppName + " toType<" + cppName + ">(const Position& pos, const std::string& value) {return (" + cppName + ")toType(pos, _" + cppName + "_Values, value);}");
            _cppOutput.append("");
            return true;
        }

        if (type.kind == Xsd::Type::ElementKind)
        {
            cppName = toCppTypeIdentifier(typeName);
            _generatedTypes.append(typeName, cppName);

            String baseCppName;
            if (!type.baseType.isEmpty())
                if (!processType(type.baseType, baseCppName, level + 1))
                    return false;

            List<String> structFields;
            for (List<Xsd::AttributeRef>::Iterator i = type.attributes.begin(), end = type.attributes.end(); i != end; ++i)
            {
                const Xsd::AttributeRef& attributeRef = *i;
                String attributeCppName;
                if (!processType(attributeRef.typeName, attributeCppName, level + 1))
                    return false;
                structFields.append(attributeCppName + " " + toCppFieldIdentifier(attributeRef.name));
            }
            for (List<Xsd::ElementRef>::Iterator i = type.elements.begin(), end = type.elements.end(); i != end; ++i)
            {
                const Xsd::ElementRef& elementRef = *i;
                if (elementRef.groupMembers.isEmpty())
                {
                    String elementCppName;
                    if (!processType(elementRef.typeName, elementCppName, level + 1))
                        return false;
                    if (elementRef.minOccurs == 1 && elementRef.maxOccurs == 1)
                        structFields.append(elementCppName + " " + toCppFieldIdentifier(elementRef.name));
                    else if (elementRef.maxOccurs == 1)
                        structFields.append(String("xsd::optional<") + elementCppName + "> " + toCppFieldIdentifier(elementRef.name));
                    else
                        structFields.append(String("xsd::vector<") + elementCppName + "> " + toCppFieldIdentifier(elementRef.name));
                }
                else for (List<Xsd::GroupMember>::Iterator i = elementRef.groupMembers.begin(), end = elementRef.groupMembers.end(); i != end; ++i)
                {
                    const Xsd::GroupMember& member = *i;
                    String memberCppName;
                    if (!processType(member.typeName, memberCppName, level + 1))
                        return false;
                    if (elementRef.minOccurs == 1 && elementRef.maxOccurs == 1 && elementRef.groupMembers.size() == 1)
                        structFields.append(memberCppName + " " + toCppFieldIdentifier(member.name));
                    else if (elementRef.maxOccurs == 1)
                        structFields.append(String("xsd::optional<") + memberCppName + "> " + toCppFieldIdentifier(member.name));
                    else
                        structFields.append(String("xsd::vector<") + memberCppName + "> " + toCppFieldIdentifier(member.name));
                }
            }

            List<String> structDefintiion;
            if (baseCppName.isEmpty())
                structDefintiion.append(String("struct ") + cppName);
            else
                structDefintiion.append(String("struct ") + cppName + " : " + baseCppName);
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
                if (elementRef.groupMembers.isEmpty())
                {
                    String elementCppName;
                    if (!processType(elementRef.typeName, elementCppName, level + 1))
                        return false;
                    if (elementRef.minOccurs == 1 && elementRef.maxOccurs == 1)
                        _cppOutput.append(elementCppName + "* get_" + cppName + "_" + toCppTypeIdentifier(elementRef.name) + "(" + cppName + "* parent) {return &parent->" + toCppFieldIdentifier(elementRef.name) + ";}");
                    else if (elementRef.maxOccurs == 1)
                        _cppOutput.append(elementCppName + "* get_" + cppName + "_" + toCppTypeIdentifier(elementRef.name) + "(" + cppName + "* parent) {return &*(parent->" + toCppFieldIdentifier(elementRef.name) + " = " + elementCppName + "());}");
                    else
                        _cppOutput.append(elementCppName + "* get_" + cppName + "_" + toCppTypeIdentifier(elementRef.name) + "(" + cppName + "* parent) {return (parent->" + toCppFieldIdentifier(elementRef.name) + ".emplace_back(), &parent->" + toCppFieldIdentifier(elementRef.name) + ".back());}");
                }
                else for (List<Xsd::GroupMember>::Iterator i = elementRef.groupMembers.begin(), end = elementRef.groupMembers.end(); i != end; ++i)
                {
                    const Xsd::GroupMember& member = *i;
                    String memberCppName;
                    if (!processType(member.typeName, memberCppName, level + 1))
                        return false;
                    if (elementRef.minOccurs == 1 && elementRef.maxOccurs == 1 && elementRef.groupMembers.size() == 1)
                        _cppOutput.append(memberCppName + "* get_" + cppName + "_" + toCppTypeIdentifier(member.name) + "(" + cppName + "* parent) {return &parent->" + toCppFieldIdentifier(member.name) + ";}");
                    else if (elementRef.maxOccurs == 1)
                        _cppOutput.append(memberCppName + "* get_" + cppName + "_" + toCppTypeIdentifier(member.name) + "(" + cppName + "* parent) {return &*(parent->" + toCppFieldIdentifier(member.name) + " = " + memberCppName + "());}");
                    else
                        _cppOutput.append(memberCppName + "* get_" + cppName + "_" + toCppTypeIdentifier(member.name) + "(" + cppName + "* parent) {return (parent->" + toCppFieldIdentifier(member.name) + ".emplace_back(), &parent->" + toCppFieldIdentifier(member.name) + ".back());}");
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
                    if (elementRef.groupMembers.isEmpty())
                    {
                        String elementCppName;
                        if (!processType(elementRef.typeName, elementCppName, level + 1))
                            return false;
                        _cppOutput.append(String("    {\"") + stripNamespacePrefix(elementRef.name) + "\", (get_element_field_t)&get_" + cppName + "_" + toCppTypeIdentifier(elementRef.name) + ", &_" + elementCppName + "_Info, " + String::fromUInt(elementRef.minOccurs)  + ", " + String::fromUInt(elementRef.maxOccurs)  + "},");
                    }
                    else for (List<Xsd::GroupMember>::Iterator i = elementRef.groupMembers.begin(), end = elementRef.groupMembers.end(); i != end; ++i)
                    {
                        const Xsd::GroupMember& member = *i;
                        String memberCppName;
                        if (!processType(member.typeName, memberCppName, level + 1))
                            return false;
                        _cppOutput.append(String("    {\"") + stripNamespacePrefix(member.name) + "\", (get_element_field_t)&get_" + cppName + "_" + toCppTypeIdentifier(member.name) + ", &_" + memberCppName + "_Info, 0, " + String::fromUInt(elementRef.maxOccurs)  + "},");
                    }
                }
                _cppOutput.append("    {nullptr}\n};");
            }

            for (List<Xsd::AttributeRef>::Iterator i = type.attributes.begin(), end = type.attributes.end(); i != end; ++i)
            {
                const Xsd::AttributeRef& attributeRef = *i;
                String attributeTypeCppName;
                if (!processType(attributeRef.typeName, attributeTypeCppName, level + 1))
                    return false;

                HashMap<String, Xsd::Type>::Iterator it = _xsd.types.find(attributeRef.typeName);
                if (it == _xsd.types.end())
                    return false;
                Xsd::Type& type = *it;
                const Xsd::Type& finalType = resolveFinalType(type);
                if (finalType.kind == Xsd::Type::StringKind || type.kind == Xsd::Type::UnionKind)
                    _cppOutput.append(String("void set_") + cppName + "_" + toCppTypeIdentifier(attributeRef.name) + "(" + cppName + "* element, const Position&, std::string&& value) { element->" + toCppFieldIdentifier(attributeRef.name) + " = std::move(value); }");
                else
                    _cppOutput.append(String("void set_") + cppName + "_" + toCppTypeIdentifier(attributeRef.name) + "(" + cppName + "* element, const Position& pos, std::string&& value) { element->" + toCppFieldIdentifier(attributeRef.name) + " = toType<" + attributeTypeCppName + ">(pos, value); }");
            
                if (!attributeRef.isMandatory && !attributeRef.defaultValue.isEmpty())
                    _cppOutput.append(String("void default_") + cppName + "_" + toCppTypeIdentifier(attributeRef.name) + "(" + cppName + "* element) { element->" + toCppFieldIdentifier(attributeRef.name) + " = " + resolveDefaultValue(attributeTypeCppName, finalType, attributeRef.defaultValue) + "; }");
            }

            String attributes("nullptr");
            if (!type.attributes.isEmpty())
            {
                attributes = String("_") + cppName + "_Attributes";
                _cppOutput.append(String("AttributeInfo _") + cppName + "_Attributes[] = {");
                for (List<Xsd::AttributeRef>::Iterator i = type.attributes.begin(), end = type.attributes.end(); i != end; ++i)
                {
                    const Xsd::AttributeRef& attributeRef = *i;
                    String attributeCppName;
                    if (!processType(attributeRef.typeName, attributeCppName, level + 1))
                        return false;
                    String setDefault = (attributeRef.isMandatory || attributeRef.defaultValue.isEmpty()) ? String("nullptr") : String("(set_attribute_default_t)&default_") + cppName + "_" + toCppTypeIdentifier(attributeRef.name);
                    _cppOutput.append(String("    {\"") + attributeRef.name + "\", (set_attribute_t)&set_" + cppName + "_" + toCppTypeIdentifier(attributeRef.name) + ", " + (attributeRef.isMandatory ? String("true") : String("false")) +  ", " + setDefault + "},");
                }
                _cppOutput.append("    {nullptr}\n};");
            }

            usize childrenCount = getChildrenCount(typeName);
            usize mandatoryChildrenCount = getMandatoryChildrenCount(typeName);
            uint64 attributesCount = getAttributesCount(typeName);
            String flags = "0";
            if (level == 1)
                flags.append("|ElementInfo::Level1Flag");
            switch (getReadTextMode(typeName))
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

            if (baseCppName == "xsd::string")
                baseCppName.clear();

            _cppOutput.append(String("const ElementInfo _") + cppName + "_Info = { " + flags + ", " + children + ", " + String::fromUInt64(childrenCount) + ", " + String::fromUInt64(mandatoryChildrenCount) 
                + ",  " + attributes + ", " + String::fromUInt64(attributesCount) + ", " + (baseCppName.isEmpty() ? String("nullptr") : String("&_") + baseCppName + "_Info") + " };");
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

    String cppName = toCppTypeIdentifier(xsd.name);

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
