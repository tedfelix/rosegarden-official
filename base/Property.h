
/*
    Rosegarden-4 v0.1
    A sequencer and musical notation editor.

    This program is Copyright 2000-2001
        Guillaume Laurent   <glaurent@telegraph-road.org>,
        Chris Cannam        <cannam@all-day-breakfast.com>,
        Richard Bown        <bownie@bownie.com>

    The moral right of the authors to claim authorship of this work
    has been asserted.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#ifndef _PROPERTY_H_
#define _PROPERTY_H_

#include <string>

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
    static basic_type parse(string);
    static string unparse(basic_type);

};

template <PropertyType P>
PropertyDefn<P>::basic_type
PropertyDefn<P>::parse(string)
{
    assert(0);
}

template <PropertyType P>
string 
PropertyDefn<P>::unparse(PropertyDefn<P>::basic_type)
{
    assert(0);
}


template <>
class PropertyDefn<Int>
{
public:
    static string name();
    typedef long basic_type;

    static basic_type parse(string s);
    static string unparse(basic_type i);
};


template <>
class PropertyDefn<String>
{
public:
    static string name();
    typedef string basic_type;

    static basic_type parse(string s);
    static string unparse(basic_type i);
};

template <>
class PropertyDefn<Bool>
{
public:
    static string name();
    typedef bool basic_type;

    static basic_type parse(string s);
    static string unparse(basic_type i);
};


class PropertyStoreBase {
public:
    virtual ~PropertyStoreBase();

    virtual PropertyType getType() const = 0;
    virtual string getTypeName() const = 0;
    virtual PropertyStoreBase *clone() = 0;
    virtual string unparse() = 0;

#ifndef NDEBUG
    virtual void dump(ostream&) const = 0;
#else
    virtual void dump(ostream&) const {}
#endif
};

#ifndef NDEBUG
inline ostream& operator<<(ostream &out, PropertyStoreBase &e)
{ e.dump(out); return out; }
#endif

template <PropertyType P>
class PropertyStore : public PropertyStoreBase
{
public:
    PropertyStore(PropertyDefn<P>::basic_type d) : m_data(d) { }
    PropertyStore(const PropertyStore<P> &p) : PropertyStoreBase(p), m_data(p.m_data) { }
    PropertyStore &operator=(const PropertyStore<P> &p);

    virtual PropertyType getType() const;
    virtual string getTypeName() const;

    virtual PropertyStoreBase* clone();
    

    virtual string unparse();
    

    PropertyDefn<P>::basic_type getData() { return m_data; }
    void setData(PropertyDefn<P>::basic_type data) { m_data = data; }

#ifndef NDEBUG
    void dump(ostream&) const;
#endif

private:
    PropertyDefn<P>::basic_type m_data;
};

template <PropertyType P>
PropertyStore<P>&
PropertyStore<P>::operator=(const PropertyStore<P> &p) {
    if (this != &p) m_data = p.m_data;
    return *this;
}

template <PropertyType P>
PropertyType
PropertyStore<P>::getType() const
{
    return P;
}

template <PropertyType P>
string
PropertyStore<P>::getTypeName() const
{
    return PropertyDefn<P>::name();
}

template <PropertyType P>
PropertyStoreBase*
PropertyStore<P>::clone()
{
    return new PropertyStore<P>(*this);
}

template <PropertyType P>
string
PropertyStore<P>::unparse()
{
    return PropertyDefn<P>::unparse(m_data);
}

#ifndef NDEBUG
template <PropertyType P>
void
PropertyStore<P>::dump(ostream &out) const
{
    out << getTypeName() << " - " << m_data;
}
#endif

#endif
