
#include "../src/Reader.hpp"
#include "../src/Generator.hpp"

#include <nstd/Console.hpp>

int main()
{
    String inputFile = FOLDER "/ED247A_ECIC.xsd";
    String error;
    Xml::Element xsd;
    if (!readXsd(inputFile, xsd, error) ||
        !generateCpp(xsd, ".", error))
    {
        Console::errorf("%s\n", (const char*)error);
        return 1;
    }

    return 0;
}
