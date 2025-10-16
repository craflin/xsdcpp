

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

    struct AttributeRef // todo: rename to Attribute
    {
        Name name;
        Name typeName;
        bool isMandatory;
        String defaultValue;

        AttributeRef() : isMandatory(false) {}
    };

    struct GroupMember
    {
        Name name;
        Name typeName;
    };

    struct ElementRef // todo: rename to Element
    {
        Name name;
        uint minOccurs;
        uint maxOccurs;
        Name typeName;

        Name refName; // used by the reader for substituion groups

        ElementRef() : minOccurs(1), maxOccurs(1) {}
    };

    struct Type
    {
        enum Kind
        {
            UnknownKind,
            BaseKind,
            SimpleRefKind,
            StringKind,
            EnumKind,
            ElementKind,
            UnionKind,
            ListKind,
            SubstitutionGroupKind,
        };
        Kind kind;

        // when ElementKind, SimpleRefKind, or ListKind
        Name baseType;

        // when StringKind
        String pattern;

        // when EnumKind
        List<String> enumEntries;

        // when ElementKind
        enum Flags
        {
            SkipProcessContentsFlag = 1,
            AnyAttributeFlag = 2,
        };
        uint32 flags;
        List<AttributeRef> attributes;

        // when ElementKind or SubstitutionGroupKind
        List<ElementRef> elements;

        // when UnionKind
        List<Name> memberTypes;

        Type() : kind(UnknownKind), flags(0) {}
    };

    String name;
    HashMap<Name, Type> types;
    Name rootType;

    HashSet<String> targetNamespaces;
    HashMap<String, String> namespaceToSuggestedPrefix;
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

inline bool operator>(const Xsd::Name& lh, const Xsd::Name& rh)
{
    return lh.name > rh.name || (lh.name == rh.name && lh.namespace_ > rh.namespace_);
}

inline bool operator<(const Xsd::Name& lh, const Xsd::Name& rh)
{
    return lh.name < rh.name || (lh.name == rh.name && lh.namespace_ < rh.namespace_);
}

bool readXsd(const String& name, const String& file, const List<String>& forceTypeProcessing, Xsd& xsd, String& error);
