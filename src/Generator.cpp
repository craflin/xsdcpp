
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
        String cppName;
        if (!processType(_xsd.rootType, cppName))
            return false;

        _hppOutput.append(String("typedef ") + cppName + " " + toCppIdentifier(_xsd.name) + ";");
        _hppOutput.append("");
        _hppOutput.append(String("void load_xml(const std::string& file, ") + toCppIdentifier(_xsd.name) + "& data);");
        _hppOutput.append("");
        return true;
    }

private:
    const Xsd& _xsd;
    List<String>& _cppOutput;
    List<String>& _hppOutput;
    HashMap<String, String> _generatedTypes;
    String _error;

private:
    bool processType(const String& typeName, String& cppName)
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
            if (!processType(type.baseType, baseCppName))
                return false;

            _hppOutput.append(String("typedef ") + baseCppName + " "  + cppName + ";");
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
            return true;
        }

        if (type.kind == Xsd::Type::ElementKind)
        {
            cppName = toCppIdentifier(typeName);
            _generatedTypes.append(typeName, cppName);

            String baseCppName;
            if (!type.baseType.isEmpty())
                if (!processType(type.baseType, baseCppName))
                    return false;

            List<String> structFields;
            for (List<Xsd::AttributeRef>::Iterator i = type.attributes.begin(), end = type.attributes.end(); i != end; ++i)
            {
                const Xsd::AttributeRef& attributeRef = *i;
                String attributeCppName;
                if (!processType(attributeRef.typeName, attributeCppName))
                    return false;
                structFields.append(attributeCppName + " " + toCppIdentifier(attributeRef.name));
            }
            for (List<Xsd::ElementRef>::Iterator i = type.elements.begin(), end = type.elements.end(); i != end; ++i)
            {
                const Xsd::ElementRef& elementRef = *i;
                String elementCppName;
                if (!processType(elementRef.typeName, elementCppName))
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

    List<String> header;
    header.append("");
    header.append("#pragma once");
    header.append("");
    header.append("#include \"xsd.hpp\"");
    header.append("");
    hppOutput.prepend(header);

    {
        String outputFilePath = outputDir + "/" + xsd.name + ".hpp";
        File outputFile;
        if (!outputFile.open(outputFilePath, File::writeFlag))
            return (error = String::fromPrintf("Could not open file '%s': %s", (const char*)outputFilePath, (const char*)Error::getErrorString())), false;

        for (List<String>::Iterator i = hppOutput.begin(), end = hppOutput.end(); i != end; ++i)
            if (!outputFile.write(*i + "\n"))
                return (error = String::fromPrintf("Could not write to file '%s': %s", (const char*)outputFilePath, (const char*)Error::getErrorString())), false;
    }
    {
        String outputFilePath = outputDir + "/" + xsd.name + ".cpp";
        File outputFile;
        if (!outputFile.open(outputFilePath, File::writeFlag))
            return (error = String::fromPrintf("Could not open file '%s': %s", (const char*)outputFilePath, (const char*)Error::getErrorString())), false;

        List<String> output;
        output.append("");
        output.append(String("#include \"") + xsd.name + ".hpp\"");
        output.append("");
        output.append(String("void load_xml(const std::string& file, ") + toCppIdentifier(xsd.name) + "& data)");
        output.append("{");
        output.append("}");
        output.append("");

        if (!outputFile.write(XmlParser_cpp))
            return (error = String::fromPrintf("Could not write to file '%s': %s", (const char*)outputFilePath, (const char*)Error::getErrorString())), false;

        for (List<String>::Iterator i = output.begin(), end = output.end(); i != end; ++i)
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
