
# XSDCPP

[![Build Status](http://xaws6t1emwa2m5pr.myfritz.net:8080/buildStatus/icon?job=craflin%2Fxsdcpp%2Fmaster)](http://xaws6t1emwa2m5pr.myfritz.net:8080/job/craflin/job/xsdcpp/job/master/)

## Work in progress

XSDCPP has not yet reached a state in which it is usable.

<!-- 

XSDCPP is an XSD schema to C++ data model and parser converter.

XSDCPP uses an XSD schema as an input file to generate a C++ data model and an XSD validating XML parser out of it.
The data model (a C++11 `.hpp` file) and its XML parser (a C++11 `.cpp` file) can be used without linking to any additional libraries.
So, if your XSD is stable, you can just create the data model and parser once and use it in your project.
But, you can also create the data model and parser out of the XSD file within your build system at build time if needed.

## Motivation

When using XML in an C++ project, it is pretty common to parse the XML into a DOM three with some third party library and to just extract relevant information out of it without considering its XSD schema.
If verification against an XSD schema is desired, you usually use a library like libxml2 that reads the XSD at runtime and does the verification, but you still need to manually write code to convert the DOM tree into your C++ data model.

Loading an XML file this way is unnecessarily slow and manually writing the code is time consuming and error prone.
There are toolkits like ??? and others that potentially solve the issue by creating a data model and parser for you.
However, they are commercial and/or require you to link against some library.
Since dependency management in C++ projects is not an entirely solved issue, having to depend on a library might be an issue.
It's especially inconvenient if you want to provide a platform independent library that does something with XML based on and XSD without imposing any third party dependencies to your library users.

XSDCPP solves the issue by creating a data model conform to C++11 and parser that can be added to your library without additional dependencies for library users.

-->