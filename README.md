
# XSDCPP

[![Build Status](http://xaws6t1emwa2m5pr.myfritz.net:8080/buildStatus/icon?job=craflin%2Fxsdcpp%2Fmaster)](http://xaws6t1emwa2m5pr.myfritz.net:8080/job/craflin/job/xsdcpp/job/master/)

XSDCPP is an XSD schema to C++ data model and XML parser converter.

XSDCPP uses an XSD schema as an input file to generate a C++11 data model and an XSD validating XML parser out of it.
The data model (a `.hpp` file) and its XML parser (a `.cpp` file) can be used without linking to any additional libraries.
So, if your XSD is stable, you can just create the data model and parser once and use it in your project.
But, you can also create the data model and parser out of the XSD file within your build system at build time if needed.

## Motivation

When using XML in a C++ project, it is very common to parse the XML into a DOM tree with some third party library and to just extract information relevant to you without considering its XSD schema.
If verification against an XSD schema is desired, you usually use a library like libxml2 that reads the XSD at runtime and does the verification, but you still need to manually write code to convert the DOM tree into your C++ data model.

Loading an XML file this way is unnecessarily slow and manually writing the code to convert the XML data into a C++ data model is time consuming and error prone.
There are toolkits like [CodeSynthesis XSD](https://www.codesynthesis.com/products/xsd/) and others that potentially solve the issue by creating a data model and parser for you.
However, they will probably require you to link against some library.
Since dependency management in C++ projects is not an entirely solved problem, having to depend on a library might be an issue.
It is especially inconvenient if you want to provide a platform independent library that does something with XML based on an XSD without imposing any third party dependencies to your library users.

XSDCPP solves the issue by creating a data model and parser that can easily be added to your library without additional dependencies for the library users.

## Features and Limitations

Since XSD is full of features (and unnecessary complexity), its very hard to support all of them. 
So, XSDCPP does currently just support what was thrown at it so far and there are probably some severe limitations.

Notable supported features:
* elements mapped to a C++ struct,
* optional elements and lists of elements,
* elements derived from a simple type,
* basic attribute data types that are mapped to a `std::string`, an integer type, a floating point type, or a generated C++ enum class,
* attributes with list types,
* default attribute values,
* attribute presence validation,
* optional attributes,
* any attributes,
* substitution groups,
* lax and skip content processing,
* Unicode escape sequence handling,
* `include` processing,
* `import` with namespaces (however, element names in a resulting data model should be unique since the resulting parser will ignore namespaces).

Known missing feature are:
* proper element occurrence validation for choice and substitution groups,
* attribute regex or value range validation,
* consideration of namespaces for processing element or attribute names,
* proper support for union types (they are currently mapped to `std::string`).

Intentionally not supported features:
* codecs other than UTF-8,
* element sequence validation.

## Build Instructions

* Clone the Git repository.
* Initialize submodules.
* Build the project using CMake or Conan.

## Example

An XSD schema *Example.xsd* like this:
```xml
<?xml version="1.0" encoding="UTF-8"?>
<xsd:schema xmlns:example="http://whatever.x/example" xmlns:xsd="http://www.w3.org/2001/XMLSchema" targetNamespace="http://whatever.x/example">

    <xsd:complexType name="List">
        <xsd:sequence>
            <xsd:element name="Person" maxOccurs="unbounded" type="example:Person"/>
        </xsd:sequence>
    </xsd:complexType>

    <xsd:complexType name="Person">
        <xsd:sequence>
            <xsd:element name="Name" type="example:Name"/>
            <xsd:element name="Country" minOccurs="0" type="example:Country"/>
        </xsd:sequence>
    </xsd:complexType>

    <xsd:complexType name="Name">
        <xsd:simpleContent>
            <xsd:extension base="xsd:string">
                <xsd:attribute name="comment" type="xsd:string"/>
                <xsd:attribute name="age" type="xsd:int" use="required"/>
                <xsd:attribute name="hidden" type="xsd:boolean" default="true"/>
            </xsd:extension>
        </xsd:simpleContent>
    </xsd:complexType>

    <xsd:complexType name="Country">
        <xsd:simpleContent>
            <xsd:extension base="example:CountryCode">
                <xsd:attribute name="comment" type="xsd:string"/>
            </xsd:extension>
        </xsd:simpleContent>
    </xsd:complexType>

    <xsd:simpleType name="CountryCode">
        <xsd:restriction base="xsd:string">
            <xsd:enumeration value="DE"/>
            <xsd:enumeration value="FR"/>
            <xsd:enumeration value="ES"/>
            <xsd:enumeration value="UK"/>
        </xsd:restriction>
    </xsd:simpleType>

    <xsd:element name="List" type="example:List"/>

</xsd:schema>
```
Can be converted to C++ using `xsdcpp`:
```
xsdcpp Example.xsd -o /your/output/folder
```
The converter creates three files in */your/output/folder*: *Example.cpp*, *Example.hpp*, and *Example_xsd.hpp*.

*Example.hpp* provides a data model like this:
```cpp
namespace Example {

struct List;
struct Person;
struct Name;
struct Country;

struct List
{
    xsd::vector<Example::Person> Person;
};

struct Name : xsd::string
{
    xsd::optional<xsd::string> comment;
    int32_t age;
    bool hidden;
};

struct Person
{
    Example::Name Name;
    xsd::optional<Example::Country> Country;
};

enum class CountryCode
{
    DE,
    FR,
    ES,
    UK,
};

struct Country : xsd::base<Example::CountryCode>
{
    xsd::optional<xsd::string> comment;
};

}
```
And a functions to load an XML file or XML data from a string:
```cpp
void load_file(const std::string& file, List& List);
void load_data(const std::string& data, List& List);
```
(The implementation of these functions can be found in *Example.cpp* and *Example_xsd.hpp* provides the types of the *xsd* namespace.)

Now, you can add *Example.cpp*, *Example.hpp*, and *Example_xsd.hpp* to your project and write code to load XML data:
```cpp
#include <Example.hpp>
#include <iostream>

int main()
{
    Example::List list;
    Example::load_data(R"(<?xml version="1.0" encoding="UTF-8"?>
<List>
    <Person>
        <Name age="40">John Smith</Name>
        <Country comment="not sure">UK</Country>
    </Person>
    <Person>
        <Name age="54" hidden="false">Mary Jones</Name>
    </Person>
</List>
)", list);

    for (const auto& person : list.Person)
    {
        std::cout << person.Name << " (" << person.Name.age << ")";
        if (person.Country)
            std::cout << " from " << Example::to_string(*person.Country);
        std::cout << std::endl;
    }
    return 0;
}
```
The generated functions will validate the input data to some degree and throw exceptions for missing or unknown elements or attributes etc..