#include "Element2.h"

Element2::Element2(const string &package, const string &type)
    : m_package(package), m_type(type)
{ }

Element2::Element2(const Element2 &e)
{
    copyFrom(e);
}

Element2::~Element2()
{
    scrapMap();
}

Element2&
Element2::operator=(const Element2 &e)
{
    if (&e != this) {
        copyFrom(e);
    }
    return *this;
}    

bool
Element2::has(const string &name) const
{
    PropertyMap::const_iterator i = m_properties.find(name);
    return (i != m_properties.end());
}

void
Element2::scrapMap()
{
    for (PropertyMap::iterator i = m_properties.begin();
         i != m_properties.end(); ++i) {
        delete (*i).second;
    }
}

void
Element2::copyFrom(const Element2 &e)
{
    scrapMap();
    for (PropertyMap::const_iterator i = e.m_properties.begin();
         i != e.m_properties.end(); ++i) {
        m_properties.insert(PropertyPair((*i).first, (*i).second->clone()));
    }
}

