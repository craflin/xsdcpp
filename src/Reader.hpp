

#pragma once

#include <nstd/Document/Xml.hpp>

struct Xsd
{
    struct AttributeRef
    {
        String name;
        String typeName;
        String defaultValue;
    };

    struct ElementRef
    {
        String name;
        uint minOccurs;
        uint maxOccurs;
        String typeName;
    };

    struct Type
    {
        enum Kind
        {
            BaseKind,
            SimpleBaseRefKind,
            StringKind,
            EnumKind,
            ElementKind,
        };
        Kind kind;

        // when ElementKind or SimpleBaseRefKind
        String baseType;

        // when StringKind
        String pattern;

        // when EnumKind
        List<String> enumEntries;

        // when ElementKind
        List<AttributeRef> attributes;
        List<ElementRef> elements;
    };

    String name;
    HashMap<String, Type> types;
    String rootType;
};

bool readXsd(const String& name, const String& file, Xsd& xsd, String& error);
