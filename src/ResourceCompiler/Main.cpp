
#include <nstd/Console.hpp>
#include <nstd/Error.hpp>
#include <nstd/File.hpp>
#include <nstd/List.hpp>
#include <nstd/Process.hpp>

void usage(const char* argv0)
{
    Console::errorf("Usage: %s <input-file> [...] [-o <output-file>]\n", argv0);
}

bool compileResources(const List<String>& inputFilePaths, const String& outputFilePath, String& error)
{
    File outputFile;

    if (!outputFile.open(outputFilePath, File::writeFlag))
        return (error = Error::getErrorString()), false;

    if (!outputFile.write("\n#pragma once\n\n") ||
        !outputFile.write("#include <nstd/String.hpp>\n\n"))
        return (error = Error::getErrorString()), false;

    for (List<String>::Iterator i = inputFilePaths.begin(), end = inputFilePaths.end(); i != end; ++i)
    {
        const String& filePath = *i;
        File inputFile;
        if (!inputFile.open(filePath))
            return (error = Error::getErrorString()), false;

        String data;
        if (!inputFile.readAll(data))
            return (error = Error::getErrorString()), false;

        String name = File::getBaseName(filePath);
        name.replace(".", "_");

        data.replace("\\", "\\\\");
        data.replace("\"", "\\\"");
        data.replace("\r", "");
#ifdef _WIN32
        data.replace("\n", "\\n\"\n\"");
#else
        data.replace("\n", "\\n\"\r\n\"");
#endif

        if (!outputFile.write(String("static String ") + name + " = \"") || !outputFile.write(data) || !outputFile.write("\";\n"))
            return (error = Error::getErrorString()), false;
    }

    return true;
}

int main(int argc, char* argv[])
{
    List<String> inputFilePaths;
    String outputFilePath;
    {
        Process::Option options[] = {
            { 'o', "output", Process::argumentFlag },
        };
        Process::Arguments arguments(argc, argv, options);
        int character;
        String argument;
        while (arguments.read(character, argument))
            switch (character)
            {
            case 'o':
                outputFilePath = argument;
                break;
            case ':':
                Console::errorf("Option %s required an argument.\n", (const char*)argument);
                return 1;
            case '\0':
                inputFilePaths.append(argument);
                break;
            default:
                usage(argv[0]);
                return 1;
            }
    }
    if (inputFilePaths.isEmpty() || outputFilePath.isEmpty())
    {
        usage(argv[0]);
        return 1;
    }

    String error;
    if (!compileResources(inputFilePaths, outputFilePath, error))
    {
        Console::errorf("%s\n", (const char*)error);
        return 1;
    }

    return 0;
}
