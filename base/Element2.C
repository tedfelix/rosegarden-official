#include <cstdio>

#include "Element2.h"

string
PropertyDefn<Int>::name()
{
    return "Int";
}    

PropertyDefn<Int>::basic_type
PropertyDefn<Int>::parse(string s)
{
    return atoi(s.c_str());
}

string
PropertyDefn<Int>::unparse(PropertyDefn<Int>::basic_type i)
{
    static char buffer[20]; sprintf(buffer, "%ld", i);
    return buffer;
}

string
PropertyDefn<String>::name()
{
    return "String";
}    

PropertyDefn<String>::basic_type
PropertyDefn<String>::parse(string s)
{
    return s;
}

string
PropertyDefn<String>::unparse(PropertyDefn<String>::basic_type i)
{
    return i;
}

string
PropertyDefn<Bool>::name()
{
    return "Bool";
}    

PropertyDefn<Bool>::basic_type
PropertyDefn<Bool>::parse(string s)
{
    return s == "true";
}

string
PropertyDefn<Bool>::unparse(PropertyDefn<Bool>::basic_type i)
{
    return (i ? "true" : "false");
}



PropertyStoreBase::~PropertyStoreBase()
{
}


Element2::Element2()
    : m_duration(0),
      m_absoluteTime(0),
      m_viewElements(0)
{
}

Element2::Element2(const string &package, const string &type)
    : m_package(package),
      m_type(type),
      m_duration(0),
      m_absoluteTime(0),
      m_viewElements(0)
{
}

Element2::Element2(const Element2 &e)
    : m_duration(0),
      m_absoluteTime(0),
      m_viewElements(0)
{
    copyFrom(e);
}

Element2::~Element2()
{
    scrapMap();
    delete m_viewElements;
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
    delete m_viewElements;
    scrapMap();

    for (PropertyMap::const_iterator i = e.m_properties.begin();
         i != e.m_properties.end(); ++i) {
        m_properties.insert(PropertyPair((*i).first, (*i).second->clone()));
    }

    if (e.viewElements())
        m_viewElements = new ViewElements(*(e.viewElements()));

}


void
Element2::setViewElements(ViewElements *ve)
{
    if (m_viewElements) {
        // this will also delete all elements in m_group
        delete m_viewElements;
    }
    
    m_viewElements = ve;
}

void
Element2::dump(ostream& out) const
{
#ifndef NDEBUG
    if (m_package.length()) {
        out << "Event type : " << m_type << " - package : "
            << m_package << '\n';
    } else {    
        out << "Event type : " << m_type.c_str() << '\n';
    }

    out << "\tDuration : " << m_duration
        << "\n\tAbsolute Time : " << m_absoluteTime
        << "\n\tProperties : \n";

    for (PropertyMap::const_iterator i = m_properties.begin();
         i != m_properties.end(); ++i) {
        out << "\t\t" << (*i).first << '\t'
            << *((*i).second) << '\n';
    }
#endif
}

bool
operator<(const Element2 &a, const Element2 &b)
{
    return a.absoluteTime() < b.absoluteTime();
}

//////////////////////////////////////////////////////////////////////

ViewElement::ViewElement(Event *e)
    : m_event(e)
{
}

ViewElement::~ViewElement()
{
}

//////////////////////////////////////////////////////////////////////

EventList::~EventList()
{
    // delete content
    for(iterator it = begin(); it != end(); ++it)
        delete (*it);
}

ViewElements::~ViewElements()
{
    // delete content
    for(iterator it = begin(); it != end(); ++it) {
        cerr << "ViewElements delete" << endl;
        delete (*it);
    }

}


bool
operator<(const ViewElement &a, const ViewElement &b)
{
    return a.event()->absoluteTime() < b.event()->absoluteTime();
}
