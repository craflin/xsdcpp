
#include <nstd/Process.hpp>
#include <nstd/Console.hpp>

#include "Reader.hpp"
#include "Generator.hpp"

void usage(const char* argv0)
{
    Console::errorf("xsdcpp %s, XSD schema to C++ data model and XML parser library converter.\n\
\n\
Usage: xsdcpp [<xsd-file>] [-o <output-dir>]\n\
\n\
Options:\n\
\n\
    <xsd-file>\n\
        The path to the input XSD schema file.\n\
\n\
    -o <output-dir>, --output=<output-dir>\n\
        The folder in which the output files are created.\n\
\n\
    -e <namespace>, --extern=<namespace>\n\
        A namespace that should not be generated in the output files and hence\n\
        must be provided separately. This should be used to avoid code\n\
        duplication if you have a schema that is the base for multiple other\n\
        schemas. It can be set to 'xsdcpp' to omit the generation of the core\n\
        parser library, which is needed if you want to link multiple independent\n\
        generated data models to the same library or executable.\n\
\n\
    -n <namespace>, --name=<namespace>\n\
        The namespace used for the generated data model and base name of the\n\
        output files. The default, is derived from <xsd-file>.\n\
\n\
    -t <type>, --type=<type>\n\
        By default, C++ type definitions are only generated for types that are\n\
        directly in indirectly referenced from an XML element defined at root\n\
        level. However, some additional types might be needed if they are\n\
        referenced from another schema that is based on the input schema. The\n\
        option '-t' can be used to enforce the generation of such types.\n\
\n\
", VERSION);
}

int main(int argc, char* argv[])
{
    String inputFile;
    String outputDir = ".";
    String name;
    List<String> externalNamespacePrefixes;
    List<String> forceTypeProcessing;
    {
        Process::Option options[] = {
            {'o', "output", Process::argumentFlag},
            {'n', "name", Process::argumentFlag},
            {'h', "help", Process::optionFlag},
            {'e', "extern", Process::argumentFlag},
            {'t', "type", Process::argumentFlag},
            {1000, "version", Process::optionFlag},
        };
        Process::Arguments arguments(argc, argv, options);
        int character;
        String argument;
        while (arguments.read(character, argument))
            switch( character)
            {
            case 'o':
                outputDir = argument;
                break;
            case 'n':
                name = argument;
                break;
            case 'e':
                externalNamespacePrefixes.append(argument);
                break;
            case 't':
                forceTypeProcessing.append(argument);
                break;
            case ':':
                Console::errorf("Option %s required an argument.\n", (const char*)argument);
                return 1;
            case 1000:
                Console::errorf("xsdcpp %s\n", VERSION);
                return 0;
            case '\0':
                inputFile = argument;
                break;
            default:
                usage(argv[0]);
                return 1;
            }
    }
    if (inputFile.isEmpty())
    {
        usage(argv[0]);
        return 1;
    }

    String error;
    Xsd xsd;
    if (!readXsd(name, inputFile, forceTypeProcessing, xsd, error) ||
        !generateCpp(xsd, outputDir, externalNamespacePrefixes, forceTypeProcessing, error))
    {
        Console::errorf("error: %s\n", (const char*)error);
        return 1;
    }

    return 0;
}
