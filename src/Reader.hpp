

#pragma once

#include <nstd/Document/Xml.hpp>

struct Xsd
{
    struct AttributeRef
    {
        String name;
        String typeName;
        bool isMandatory;
        String defaultValue;
    };

    struct GroupMember
    {
        String name;
        String typeName;
    };

    struct ElementRef
    {
        String name;
        uint minOccurs;
        uint maxOccurs;
        String typeName;
        List<GroupMember> groupMembers;
    };

    struct Type
    {
        enum Kind
        {
            BaseKind,
            SimpleRefKind,
            StringKind,
            EnumKind,
            ElementKind,
            UnionKind,
            ListKind,
        };
        Kind kind;

        // when ElementKind, SimpleRefKind, or ListKind
        String baseType;

        // when StringKind
        String pattern;

        // when EnumKind
        List<String> enumEntries;

        // when ElementKind
        List<AttributeRef> attributes;
        List<ElementRef> elements;
        enum Flags
        {
            SkipProcessContentsFlag = 1,
        };
        uint32 flags;

        // when UnionKind
        List<String> memberTypes;
    };

    String name;
    HashMap<String, Type> types;
    String rootType;
    String xmlSchemaNamespacePrefix;
};

bool readXsd(const String& name, const String& file, Xsd& xsd, String& error);
