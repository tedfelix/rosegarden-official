
#ifndef _ELEMENT2_H_
#define _ELEMENT2_H_

#include <map>
#include <hash_map>
#include <string>

#include <cstring>
#include <cstdio>

// Need to associate one of those param names (Int, String etc) with a
// storage type and a parser/writer.  The storage-type association
// obviously has to be made at compile-time, with a template.

// This is still a wee bit messy at the moment, but the principle's
// interesting.  Performance is almost identical to that of the more
// static-stylee Element (probably dominated by map lookup/insert; a
// vector might be faster in practice as we expect relatively few
// properties per element)

struct eqstring
{
    bool operator()(const string &s1, const string &s2) const
    {
        return s1 == s2;
    }
};

struct hashstring
{
    static hash<const char*> _H;
    
    size_t operator()(const string &s) const
    {
        return _H(s.c_str());
    }
};

hash<const char*> hashstring::_H;

enum PropertyType { Int, String, Bool, Tag };

template <PropertyType P>
class PropertyDefn
{
public:
    static string name() { return "Undefined"; }
    struct PropertyDefnNotDefined {
	PropertyDefnNotDefined() { assert(0); }
    };
    typedef PropertyDefnNotDefined basic_type;
    static basic_type parse(string) { assert(0); }
    static string unparse(basic_type) { assert(0); }
};

template <>
class PropertyDefn<Int>
{
public:
    static string name() { return "Int"; }
    typedef long basic_type;

    static basic_type parse(string s) { return atoi(s.c_str()); }
    static string unparse(basic_type i) { static char buffer[20]; sprintf(buffer, "%ld", i); return buffer; }
};

template <>
class PropertyDefn<String>
{
public:
    static string name() { return "String"; }
    typedef string basic_type;

    static basic_type parse(string s) { return s; }
    static string unparse(basic_type i) { return i; }
};

template <>
class PropertyDefn<Bool>
{
public:
    static string name() { return "Bool"; }
    typedef bool basic_type;

    static basic_type parse(string s) { return (s == "true"); }
    static string unparse(basic_type i) { return (i ? "true" : "false"); }
};

class PropertyStoreBase {
public:
    virtual PropertyType getType() = 0;
    virtual string getTypeName() = 0;
    virtual PropertyStoreBase *clone() = 0;
    virtual string unparse() = 0;
};

template <PropertyType P>
class PropertyStore : public PropertyStoreBase
{
public:
    PropertyStore(PropertyDefn<P>::basic_type d) : m_data(d) { }
    PropertyStore(const PropertyStore<P> &p) : m_data(p.m_data) { }
    PropertyStore &operator=(const PropertyStore<P> &p);

    virtual PropertyType getType() { return P; }
    virtual string getTypeName() { return PropertyDefn<P>::name(); }

    virtual PropertyStoreBase *clone() {
	return new PropertyStore<P>(*this);
    }

    virtual string unparse() {
	return PropertyDefn<P>::unparse(m_data);
    }

    PropertyDefn<P>::basic_type getData() { return m_data; }
    void setData(PropertyDefn<P>::basic_type data) { m_data = data; }

private:
    PropertyDefn<P>::basic_type m_data;
};

template <PropertyType P>
PropertyStore<P>&
PropertyStore<P>::operator=(const PropertyStore<P> &p) {
    if (this != &p) m_data = p.m_data;
    return *this;
}


class Element2
{
private:

public:
    typedef unsigned int duration;

    struct NoData { };
    struct BadType { };

    Element2();

    Element2(const string &package, const string &type);

    Element2(const Element2 &e);

    virtual ~Element2();

    Element2 &operator=(const Element2 &e);

    // Accessors
    const string &getPackage() const { return m_package; }
    const string &getType() const    { return m_type; }
    void setPackage(const string &p) { m_package = p; }
    void setType(const string &t)    { m_type = t; }

    duration getDuration() const { return m_duration; }
    void setDuration(duration d)      { m_duration = d; }

    bool has(const string &name) const;

    template <PropertyType P>
    PropertyDefn<P>::basic_type get(const string &name) const
        throw (NoData, BadType);

    template <PropertyType P>
    string getAsString(const string &name) const
	throw (NoData, BadType);

    template <PropertyType P>
    void set(const string &name, PropertyDefn<P>::basic_type value)
	throw (BadType);

    template <PropertyType P>
    void setFromString(const string &name, string value)
	throw (BadType);

private:
    void scrapMap();
    void copyFrom(const Element2 &e);

    string m_package;
    string m_type;
    duration m_duration;

    //    typedef map<string, PropertyStoreBase *> PropertyMap;
    typedef hash_map<string, PropertyStoreBase*, hashstring, eqstring> PropertyMap;
    typedef PropertyMap::value_type PropertyPair;
    PropertyMap m_properties;
};


template <PropertyType P>
PropertyDefn<P>::basic_type
Element2::get(const string &name) const
    throw (NoData, BadType)
{
    PropertyMap::const_iterator i = m_properties.find(name);
    if (i != m_properties.end()) { 

        PropertyStoreBase *sb = (*i).second;
        if (sb->getType() == P) return ((PropertyStore<P> *)sb)->getData();
        else {
            cerr << "Error: Element: Attempt to get property \"" << name
                 << "\" as " << PropertyDefn<P>::name() <<", actual type is "
                 << sb->getTypeName() << endl;
            throw BadType();
        }
	    
    } else {
        throw NoData();
    }
}


template <PropertyType P>
string
Element2::getAsString(const string &name) const
    throw (NoData, BadType)
{
    return PropertyDefn<P>::unparse(get<P>(name));
}


template <PropertyType P>
void
Element2::set(const string &name, PropertyDefn<P>::basic_type value)
    throw (BadType)
{
    PropertyMap::const_iterator i = m_properties.find(name);
    if (i != m_properties.end()) {

        PropertyStoreBase *sb = (*i).second;
        if (sb->getType() == P) ((PropertyStore<P> *)sb)->setData(value);
        else {
            cerr << "Error: Element: Attempt to get property \"" << name
                 << "\" as " << PropertyDefn<P>::name() <<", actual type is "
                 << sb->getTypeName() << endl;
            throw BadType();
        }
	    
    } else {
        PropertyStoreBase *p = new PropertyStore<P>(value);
        m_properties.insert(PropertyPair(name, p));
    }
}


template <PropertyType P>
void
Element2::setFromString(const string &name, string value)
    throw (BadType)
{
    set<P>(name, PropertyDefn<P>::parse(value));
}

// Std
#include <list>

typedef list<Element2*> ElementList;


#endif
