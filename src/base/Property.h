/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */


/*
    Rosegarden
    A sequencer and musical notation editor.
    Copyright 2000-2018 the Rosegarden development team.
    See the AUTHORS file for more details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#ifndef RG_PROPERTY_H
#define RG_PROPERTY_H

#include "RealTime.h"

#include <QDebug>

#include <string>

#include <rosegardenprivate_export.h>

namespace Rosegarden 
{

enum PropertyType { Int, String, Bool, RealTimeT };

template <PropertyType P>
class ROSEGARDENPRIVATE_EXPORT PropertyDefn
{
public:
    struct PropertyDefnNotDefined {
        PropertyDefnNotDefined() { throw(0); }
    };
    typedef PropertyDefnNotDefined basic_type;

    static std::string typeName() { return "Undefined"; }
    static basic_type parse(std::string);
    static std::string unparse(basic_type);
};

template <PropertyType P>
typename PropertyDefn<P>::basic_type
PropertyDefn<P>::parse(std::string)
{
    throw(0);
    return "";
}

template <PropertyType P>
std::string 
PropertyDefn<P>::unparse(typename PropertyDefn<P>::basic_type)
{
    throw(0);
    return "";
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

template <>
class PropertyDefn<RealTimeT>
{
public:
    typedef RealTime basic_type;

    static std::string typeName();
    static basic_type parse(std::string s);
    static std::string unparse(basic_type i);
};


class ROSEGARDENPRIVATE_EXPORT PropertyStoreBase {
public:
    virtual ~PropertyStoreBase();

    virtual PropertyType getType() const = 0;
    virtual std::string getTypeName() const = 0;
    virtual PropertyStoreBase *clone() = 0;
    virtual std::string unparse() const = 0;

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

inline QDebug &operator<<(QDebug &dbg, const PropertyStoreBase &psb)
{
    dbg << psb.getTypeName().c_str() << "-" << psb.unparse().c_str();
    return dbg;
}

template <PropertyType P>
class ROSEGARDENPRIVATE_EXPORT PropertyStore : public PropertyStoreBase
{
public:
    PropertyStore(typename PropertyDefn<P>::basic_type d) :
        m_data(d) { }
    PropertyStore(const PropertyStore<P> &p) :
        PropertyStoreBase(p), m_data(p.m_data) { }
    PropertyStore &operator=(const PropertyStore<P> &p);

    PropertyType getType() const override;
    std::string getTypeName() const override;

    PropertyStoreBase* clone() override;

    std::string unparse() const override;

    typename PropertyDefn<P>::basic_type getData() { return m_data; }
    void setData(typename PropertyDefn<P>::basic_type data) { m_data = data; }

    size_t getStorageSize() const override;

#ifndef NDEBUG
    void dump(std::ostream&) const override;
#endif

private:
    typename PropertyDefn<P>::basic_type m_data;
};

template <PropertyType P>
PropertyStore<P>&
PropertyStore<P>::operator=(const PropertyStore<P> &p) {
    if (this != &p) {
        m_data = p.m_data;
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
