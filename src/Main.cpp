
#include <nstd/Process.hpp>
#include <nstd/Console.hpp>

#include "Reader.hpp"
#include "Generator.hpp"

void usage(const char* argv0)
{
    Console::errorf("Usage: %s [<xsd-file>] [-o <output-dir>] [-n <schema-name>]\n", argv0);
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
