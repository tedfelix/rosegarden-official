
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

namespace Rosegarden 
{

enum PropertyType { Int, String, Bool, Tag };

template <PropertyType P>
class PropertyDefn
{
public:
    struct PropertyDefnNotDefined {
	PropertyDefnNotDefined() { assert(0); }
    };
    typedef PropertyDefnNotDefined basic_type;

    static std::string typeName() { return "Undefined"; }
    static basic_type parse(std::string);
    static std::string unparse(basic_type);
};

template <PropertyType P>
PropertyDefn<P>::basic_type
PropertyDefn<P>::parse(std::string)
{
    assert(0);
}

template <PropertyType P>
std::string 
PropertyDefn<P>::unparse(PropertyDefn<P>::basic_type)
{
    assert(0);
}


template <>
class PropertyDefn<Int>
{
public:
    typedef long basic_type;

    static std::string typeName();
    static basic_type parse(std::string s);
    static std::string unparse(basic_type i);
};


template <>
class PropertyDefn<String>
{
public:
    typedef std::string basic_type;

    static std::string typeName();
    static basic_type parse(std::string s);
    static std::string unparse(basic_type i);
};

template <>
class PropertyDefn<Bool>
{
public:
    typedef bool basic_type;

    static std::string typeName();
    static basic_type parse(std::string s);
    static std::string unparse(basic_type i);
};


class PropertyStoreBase {
public:
    virtual ~PropertyStoreBase();

    virtual PropertyType getType() const = 0;
    virtual std::string getTypeName() const = 0;
    virtual PropertyStoreBase *clone() = 0;
    virtual std::string unparse() const = 0;

    virtual bool isPersistent() const = 0;
    virtual void setPersistence(bool) = 0;

    virtual size_t getStorageSize() const = 0; // for debugging

#ifndef NDEBUG
    virtual void dump(std::ostream&) const = 0;
#else
    virtual void dump(std::ostream&) const { }
#endif
};

#ifndef NDEBUG
inline std::ostream& operator<<(std::ostream &out, PropertyStoreBase &e)
{ e.dump(out); return out; }
#endif

template <PropertyType P>
class PropertyStore : public PropertyStoreBase
{
public:
    PropertyStore(PropertyDefn<P>::basic_type d, bool persistent = true) :
        m_data(d), m_persistent(persistent) { }
    PropertyStore(const PropertyStore<P> &p) :
        PropertyStoreBase(p), m_data(p.m_data), m_persistent(p.m_persistent) { }
    PropertyStore &operator=(const PropertyStore<P> &p);

    virtual PropertyType getType() const;
    virtual std::string getTypeName() const;

    virtual PropertyStoreBase* clone();
    
    virtual std::string unparse() const;

    PropertyDefn<P>::basic_type getData() { return m_data; }
    void setData(PropertyDefn<P>::basic_type data) { m_data = data; }

    bool isPersistent() const { return m_persistent; }
    void setPersistence(bool p) { m_persistent = p; }

    virtual size_t getStorageSize() const { return sizeof(*this); }

#ifndef NDEBUG
    void dump(std::ostream&) const;
#endif

private:
    PropertyDefn<P>::basic_type m_data;
    bool m_persistent;
};

template <PropertyType P>
PropertyStore<P>&
PropertyStore<P>::operator=(const PropertyStore<P> &p) {
    if (this != &p) {
        m_data = p.m_data;
        m_persistent = p.m_persistent;
    }
    return *this;
}

template <PropertyType P>
PropertyType
PropertyStore<P>::getType() const
{
    return P;
}

template <PropertyType P>
std::string
PropertyStore<P>::getTypeName() const
{
    return PropertyDefn<P>::typeName();
}

template <PropertyType P>
PropertyStoreBase*
PropertyStore<P>::clone()
{
    return new PropertyStore<P>(*this);
}

template <PropertyType P>
std::string
PropertyStore<P>::unparse() const
{
    return PropertyDefn<P>::unparse(m_data);
}

template <>
size_t
PropertyStore<String>::getStorageSize() const
{
    return sizeof(*this) + m_data.size();
}

#ifndef NDEBUG
template <PropertyType P>
void
PropertyStore<P>::dump(std::ostream &out) const
{
    out << getTypeName() << " - " << unparse();
}
#endif
 
}


#endif
