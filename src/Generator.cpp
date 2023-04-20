
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
        if (!String::isAlphanumeric(*i))
            *i = '_';
    if (String::isDigit(*(char*)result))
        result.prepend("_");
    return result;
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
        String cppName = toCppIdentifier(_xsd.name);

        _cppOutput.append("");
        _cppOutput.append(String("#include \"") + cppName + ".hpp\"");
        _cppOutput.append("");

        _cppOutput.append("namespace {");
        _cppOutput.append("");

        _cppOutput.append("uint32_t toType(const Position& pos, const char* const* values, const std::string& value)");
        _cppOutput.append("{");
        _cppOutput.append("    for (const char* const* i = values; *i; ++i)");
        _cppOutput.append("        if (value == *i)");
        _cppOutput.append("            return (uint32_t)(i - values);");
        _cppOutput.append("    throwVerificationException(pos, \"Unknown attribute value '\" + value + \"'\");");
        _cppOutput.append("    return 0;");
        _cppOutput.append("}");
        _cppOutput.append("");

        _cppOutput.append("template <typename T>");
        _cppOutput.append("T toType(const Position& pos, const std::string& value);");
        _cppOutput.append("");
        _cppOutput.append("template <>");
        _cppOutput.append("uint32_t toType<uint32_t>(const Position& pos, const std::string& value)");
        _cppOutput.append("{");
        _cppOutput.append("    std::stringstream ss(value);");
        _cppOutput.append("    uint32_t result;");
        _cppOutput.append("    if (!(ss >> result))");
        _cppOutput.append("         throwVerificationException(pos, \"Expected unsigned integer\");");
        _cppOutput.append("    return result;");
        _cppOutput.append("}");
        _cppOutput.append("");

        _hppOutput.append("");
        _hppOutput.append("#pragma once");
        _hppOutput.append("");
        _hppOutput.append("#include \"xsd.hpp\"");
        _hppOutput.append("");

        String rootTypeCppName;
        if (!processType(_xsd.rootType, rootTypeCppName, 0))
            return false;

        _hppOutput.append(String("typedef ") + rootTypeCppName + " " + cppName + ";");
        _hppOutput.append("");

        _hppOutput.append(String("void load_file(const std::string& file, ") + cppName + "& data);");
        _hppOutput.append(String("void load_content(const std::string& content, ") + cppName + "& data);");
        _hppOutput.append("");

        _cppOutput.append("");
        _cppOutput.append("}");
        _cppOutput.append("");
        _cppOutput.append("#include <fstream>");
        _cppOutput.append("#include <system_error>");
        _cppOutput.append("");

        _cppOutput.append(String("void load_content(const std::string& content, ") + cppName + "& data)");
        _cppOutput.append("{");
        _cppOutput.append("    ElementContext elementContext;");
        _cppOutput.append(String("    elementContext.info = &_") + rootTypeCppName + "_Info;");
        _cppOutput.append("    elementContext.element = &data;");
        _cppOutput.append("    parse(content.c_str(), elementContext);");
        _cppOutput.append("}");
        _cppOutput.append("");

        _cppOutput.append(String("void load_file(const std::string& filePath, ") + cppName + "& data)");
        _cppOutput.append("{");
        _cppOutput.append("    std::fstream file;");
        _cppOutput.append("    file.exceptions(std::fstream::failbit | std::fstream::badbit);");
        _cppOutput.append("    file.open(filePath, std::fstream::in);");
        _cppOutput.append("    std::stringstream buffer;");
        _cppOutput.append("    buffer << file.rdbuf();");
        _cppOutput.append("    load_content(buffer.str(), data);");
        _cppOutput.append("}");
        _cppOutput.append("");

        return true;
    }

private:
    const Xsd& _xsd;
    List<String>& _cppOutput;
    List<String>& _hppOutput;
    HashMap<String, String> _generatedTypes;
    String _error;

private:
    usize getChildrenCount(const String& typeName)
    {
        if (typeName.isEmpty())
            return 0;
        HashMap<String, Xsd::Type>::Iterator it = _xsd.types.find(typeName);
        if (it == _xsd.types.end())
            return 0;
        const Xsd::Type& type = *it;
        return type.elements.size() + getChildrenCount(type.baseType);
    }

    usize getMandatoryChildrenCount(const String& typeName)
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

    usize getAttributesCount(const String& typeName)
    {
        if (typeName.isEmpty())
            return 0;
        HashMap<String, Xsd::Type>::Iterator it = _xsd.types.find(typeName);
        if (it == _xsd.types.end())
            return 0;
        const Xsd::Type& type = *it;
        return type.attributes.size() + getAttributesCount(type.baseType);
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

    String resolveDefaultValue(const String& attributeTypeCppName, const Xsd::Type& type, const String& defaultValue)
    {
        if (type.kind == Xsd::Type::EnumKind)
        {
            uint value = 0;
            for (List<String>::Iterator i = type.enumEntries.begin(), end = type.enumEntries.end(); i != end; ++i)
                if (*i == defaultValue)
                    return String("(") + attributeTypeCppName + ")"  + String::fromUInt(value);
        }
        if (attributeTypeCppName == "xsd::string" || type.kind == Xsd::Type::StringKind)
            return toCStringLiteral(defaultValue);
        if (attributeTypeCppName == "uint32_t" || type.kind == Xsd::Type::SimpleBaseRefKind)
            return defaultValue;
        return "??";
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

        if (type.kind == Xsd::Type::BaseKind)
        {
            if (typeName == "xs:normalizedString" || typeName == "xs:string")
            {
                cppName = "xsd::string";
                _generatedTypes.append(typeName, cppName);
                return true;
            }

            if (typeName == "xs:nonNegativeInteger" || typeName == "xs:positiveInteger")
            {
                cppName = "uint32_t";
                _generatedTypes.append(typeName, cppName);
                return true;
            }

            return _error = String::fromPrintf("Base type '%s' not supported", (const char*)typeName), false;
        }

        if (type.kind == Xsd::Type::SimpleBaseRefKind)
        {
            cppName = toCppIdentifier(typeName);
            _generatedTypes.append(typeName, cppName);

            String baseCppName;
            if (!processType(type.baseType, baseCppName, level + 1))
                return false;

            _hppOutput.append(String("typedef ") + baseCppName + " " + cppName + ";");
            _hppOutput.append("");
            return true;
        }

        if (type.kind == Xsd::Type::StringKind)
        {
            cppName = toCppIdentifier(typeName);
            _generatedTypes.append(typeName, cppName);
            _hppOutput.append(String("typedef xsd::string ") + cppName + ";");
            _hppOutput.append("");
            return true;
        }

        if (type.kind == Xsd::Type::EnumKind)
        {
            cppName = toCppIdentifier(typeName);
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
            _cppOutput.append(cppName + " toType<" + cppName + ">(const Position& pos, const std::string& value) {return (" + cppName + ")toType(pos, _" + cppName + "_Values, value);}");
            _cppOutput.append("");
            return true;
        }

        if (type.kind == Xsd::Type::ElementKind)
        {
            cppName = toCppIdentifier(typeName);
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
                structFields.append(attributeCppName + " " + toCppIdentifier(attributeRef.name));
            }
            for (List<Xsd::ElementRef>::Iterator i = type.elements.begin(), end = type.elements.end(); i != end; ++i)
            {
                const Xsd::ElementRef& elementRef = *i;
                String elementCppName;
                if (!processType(elementRef.typeName, elementCppName, level + 1))
                    return false;
                if (elementRef.minOccurs == 1 && elementRef.maxOccurs == 1)
                    structFields.append(elementCppName + " " + toCppIdentifier(elementRef.name));
                else if (elementRef.minOccurs == 0 && elementRef.maxOccurs == 1)
                    structFields.append(String("xsd::optional<") + elementCppName + "> " + toCppIdentifier(elementRef.name));
                else
                    structFields.append(String("xsd::vector<") + elementCppName + "> " + toCppIdentifier(elementRef.name));
            }

            if (baseCppName.isEmpty())
                _hppOutput.append(String("struct ") + cppName);
            else
                _hppOutput.append(String("struct ") + cppName + " : " + baseCppName);
            _hppOutput.append("{");
            for (List<String>::Iterator i = structFields.begin(), end = structFields.end(); i != end; ++i)
                _hppOutput.append(String("    ") + *i + ";");
            _hppOutput.append("};");
            _hppOutput.append("");

            for (List<Xsd::ElementRef>::Iterator i = type.elements.begin(), end = type.elements.end(); i != end; ++i)
            {
                const Xsd::ElementRef& elementRef = *i;
                String elementCppName;
                if (!processType(elementRef.typeName, elementCppName, level + 1))
                    return false;
                if (elementRef.minOccurs == 1 && elementRef.maxOccurs == 1)
                    _cppOutput.append(elementCppName + "* get_" + cppName + "_" + toCppIdentifier(elementRef.name) + "(" + cppName + "* parent) {return &parent->" + toCppIdentifier(elementRef.name) + ";}");
                else if (elementRef.minOccurs == 0 && elementRef.maxOccurs == 1)
                    _cppOutput.append(elementCppName + "* get_" + cppName + "_" + toCppIdentifier(elementRef.name) + "(" + cppName + "* parent) {return &*(parent->" + toCppIdentifier(elementRef.name) + " = " + elementCppName + "());}");
                else
                    _cppOutput.append(elementCppName + "* get_" + cppName + "_" + toCppIdentifier(elementRef.name) + "(" + cppName + "* parent) {return (parent->" + toCppIdentifier(elementRef.name) + ".emplace_back(), &parent->" + toCppIdentifier(elementRef.name) + ".back());}");
            }

            _cppOutput.append(String("ChildElementInfo _") + cppName + "_Children[] = {");
            for (List<Xsd::ElementRef>::Iterator i = type.elements.begin(), end = type.elements.end(); i != end; ++i)
            {
                const Xsd::ElementRef& elementRef = *i;
                String elementCppName;
                if (!processType(elementRef.typeName, elementCppName, level + 1))
                    return false;
                _cppOutput.append(String("    {\"") + elementRef.name + "\", (get_element_field_t)&get_" + cppName + "_" + toCppIdentifier(elementRef.name) + ", &_" + elementCppName + "_Info, " + String::fromUInt(elementRef.minOccurs)  + ", " + String::fromUInt(elementRef.maxOccurs)  + "},");
            }
            _cppOutput.append("    {nullptr}\n};");

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

                if (attributeTypeCppName == "xsd::string" || type.kind == Xsd::Type::StringKind)
                    _cppOutput.append(String("void set_") + cppName + "_" + toCppIdentifier(attributeRef.name) + "(" + cppName + "* element, const Position&, const std::string& value) { element->" + toCppIdentifier(attributeRef.name) + " = value; }");
                else
                    _cppOutput.append(String("void set_") + cppName + "_" + toCppIdentifier(attributeRef.name) + "(" + cppName + "* element, const Position& pos, const std::string& value) { element->" + toCppIdentifier(attributeRef.name) + " = toType<" + attributeTypeCppName + ">(pos, value); }");
            
                if (!attributeRef.isMandatory && !attributeRef.defaultValue.isEmpty())
                    _cppOutput.append(String("void default_") + cppName + "_" + toCppIdentifier(attributeRef.name) + "(" + cppName + "* element) { element->" + toCppIdentifier(attributeRef.name) + " = " + resolveDefaultValue(attributeTypeCppName, type, attributeRef.defaultValue) + "; }");
            }
            if (level == 1)
            {
                _cppOutput.append("void noop_set_attribute(void* element, const Position& pos, const std::string& value) {}");
            }
            _cppOutput.append(String("AttributeInfo _") + cppName + "_Attributes[] = {");
            for (List<Xsd::AttributeRef>::Iterator i = type.attributes.begin(), end = type.attributes.end(); i != end; ++i)
            {
                const Xsd::AttributeRef& attributeRef = *i;
                String attributeCppName;
                if (!processType(attributeRef.typeName, attributeCppName, level + 1))
                    return false;
                String setDefault = (attributeRef.isMandatory || attributeRef.defaultValue.isEmpty()) ? String("nullptr") : String("(set_attribute_default_t)&default_") + cppName + "_" + toCppIdentifier(attributeRef.name);
                _cppOutput.append(String("    {\"") + attributeRef.name + "\", (set_attribute_t)&set_" + cppName + "_" + toCppIdentifier(attributeRef.name) + ", " + (attributeRef.isMandatory ? String("true") : String("false")) +  ", " + setDefault + "},");
            }
            if (level == 1)
            {
                _cppOutput.append("    {\"xmlns:xsi\", &noop_set_attribute, false, nullptr},");
                _cppOutput.append("    {\"xsi:noNamespaceSchemaLocation\", &noop_set_attribute, false, nullptr},");
            }
            _cppOutput.append("    {nullptr}\n};");

            usize childrenCount = getChildrenCount(typeName);
            usize mandatoryChildrenCount = getMandatoryChildrenCount(typeName);
            uint64 attributesCount = getAttributesCount(typeName);
            _cppOutput.append(String("const ElementInfo _") + cppName + "_Info = { _" + cppName + "_Children, " + String::fromUInt64(childrenCount) + ", " + String::fromUInt64(mandatoryChildrenCount) + ",  _" + cppName + "_Attributes, " + String::fromUInt64(attributesCount) + ", " + (baseCppName.isEmpty() ? String("nullptr") : String("&_") + baseCppName + "_Info") + " };");
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
        String outputFilePath = outputDir + "/xsd.hpp";
        File outputFile;
        if (!outputFile.open(outputFilePath, File::writeFlag))
            return (error = String::fromPrintf("Could not open file '%s': %s", (const char*)outputFilePath, (const char*)Error::getErrorString())), false;
        if (!outputFile.write(xsd_hpp))
            return (error = String::fromPrintf("Could not write to file '%s': %s", (const char*)outputFilePath, (const char*)Error::getErrorString())), false;
    }

    return true;
}
