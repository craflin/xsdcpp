

#pragma once

#include <nstd/Document/Xml.hpp>
#include <nstd/HashSet.hpp>

struct Xsd
{
    struct Name
    {
        String name;
        String namespace_;
    };

    struct AttributeRef
    {
        Name name;
        Name typeName;
        bool isMandatory;
        String defaultValue;
    };

    struct GroupMember
    {
        Name name;
        Name typeName;
    };

    struct ElementRef
    {
        Name name;
        uint minOccurs;
        uint maxOccurs;
        Name typeName;
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
        Name baseType;

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
            AnyAttributeFlag = 2,
        };
        uint32 flags;

        // when UnionKind
        List<Name> memberTypes;
    };

    String name;
    HashMap<Name, Type> types;
    Name rootType;

    HashSet<String> targetNamespaces;
    HashMap<String, String> namespaceToSuggestedPrefix;

    //String xmlSchemaNamespacePrefix;
};

inline usize hash(const Xsd::Name& name)
{
    usize result = hash(name.namespace_);
    result *= 16807;
    result ^= hash(name.name);
    return result;
}

inline bool operator==(const Xsd::Name& lh, const Xsd::Name& rh)
{
    return lh.name == rh.name && lh.namespace_ == rh.namespace_;
}

bool readXsd(const String& name, const String& file, Xsd& xsd, String& error);
