
# XSDCPP

[![Build Status](http://xaws6t1emwa2m5pr.myfritz.net:8080/buildStatus/icon?job=craflin%2Fxsdcpp%2Fmaster)](http://xaws6t1emwa2m5pr.myfritz.net:8080/job/craflin/job/xsdcpp/job/master/)

XSDCPP is an XSD schema to C++ data model and XML parser converter.

XSDCPP uses an XSD schema as an input file to generate a C++11 data model and an XSD validating XML parser out of it.
The data model (a `.hpp` file) and its XML parser (a `.cpp` file) can be used without linking to any additional libraries.
So, if your XSD is stable, you can just create the data model and parser once and use it in your project.
But, you can also create the data model and parser out of the XSD file within your build system at build time if needed.

## Motivation

When using XML in an C++ project, it is very common to parse the XML into a DOM tree with some third party library and to just extract information relevant to you without considering its XSD schema.
If verification against an XSD schema is desired, you usually use a library like libxml2 that reads the XSD at runtime and does the verification, but you still need to manually write code to convert the DOM tree into your C++ data model.

Loading an XML file this way is unnecessarily slow and manually writing the code to convert the XML data into a C++ data model is time consuming and error prone.
There are toolkits like [CodeSynthesis XSD](https://www.codesynthesis.com/products/xsd/) and others that potentially solve the issue by creating a data model and parser for you.
However, they will probably require you to link against some library.
Since dependency management in C++ projects is not an entirely solved problem, having to depend on a library might be an issue.
It is especially inconvenient if you want to provide a platform independent library that does something with XML based on an XSD without imposing any third party dependencies to your library users.

XSDCPP solves the issue by creating a data model and parser that can be added to your library without additional dependencies for the library users.

## Features and Limitations

Since XSD is bloated with weirdness and features, its very hard to support all of them. 
So, XSDCPP does just support what was thrown at it so far and there are probably some severe limitations.

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
