
#include <nstd/Process.hpp>
#include <nstd/Console.hpp>

#include "Reader.hpp"
#include "Generator.hpp"

void usage(const char* argv0)
{
    Console::errorf("Usage: %s [<xsd-file>] [-o <output-dir>]\n", argv0);
}

int main(int argc, char* argv[])
{
    String inputFile;
    String outputDir = ".";
    {
        Process::Option options[] = {
            {'o', "output", Process::argumentFlag},
            {'h', "help", Process::optionFlag},
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
    if (!readXsd(inputFile, xsd, error) ||
        !generateCpp(xsd, outputDir, error))
    {
        Console::errorf("%s\n", (const char*)error);
        return 1;
    }

    return 0;
}
