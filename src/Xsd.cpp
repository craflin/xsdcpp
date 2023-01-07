
#include "Xsd.hpp"

#include <nstd/Document/Xml.hpp>

bool readXsd(const String& file, Xml::Element& xsd,  String& error)
{
    Xml::Parser parser;
    if (!parser.load(file, xsd))
        return (error = String::fromPrintf("Could not load file '%s': %s", (const char*)parser.getErrorString())), false;
    return true;
}
