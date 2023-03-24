
#include "../src/Reader.hpp"
#include "../src/Generator.hpp"

#include <nstd/Console.hpp>
#include <nstd/Directory.hpp>

int main()
{
    String inputFile = FOLDER "/ED247A_ECIC.xsd";
    String error;
    Xsd xsd;
    if (!Directory::create("test_temp") ||
        !readXsd(inputFile, xsd, error) ||
        !generateCpp(xsd, "test_temp", error))
    {
        Console::errorf("%s\n", (const char*)error);
        return 1;
    }

    return 0;
}
