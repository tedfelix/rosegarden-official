// -*- c-basic-offset: 4 -*-


/*
    Rosegarden-4
    A sequencer and musical notation editor.

    This program is Copyright 2000-2003
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

#ifndef _PROPERTY_NAME_H_
#define _PROPERTY_NAME_H_

#include <string>
#include <map>
#include <iostream>

namespace Rosegarden 
{

/**

  A PropertyName is something that can be constructed from a
  string, hashed as a key in a hash map, and streamed out again as
  a string.  It must have accompanying functors PropertyNamesEqual
  and PropertyNameHash which compare and hash PropertyName objects.

  The simplest implementation is a string:

    typedef std::string PropertyName;

    struct PropertyNamesEqual {
      bool operator() (const PropertyName &s1, const PropertyName &s2) const {
        return s1 == s2;
      }
    };

    struct PropertyNameHash {
      static std::hash<const char *> _H;
      size_t operator() (const PropertyName &s) const {
          return _H(s.c_str());
      }
    };

    std::hash<const char *> PropertyNameHash::_H;

  but our implementation is faster in practice: while it behaves
  outwardly like a string, for the Event that makes use of it,
  it performs much like a machine integer.  It also shares
  strings, reducing storage sizes if there are many names in use.

  A big caveat with this class is that it is _not_ safe to persist
  the values of PropertyNames and assume that the original strings
  can be recovered; they can't.  The values are assigned on demand,
  and there's no guarantee that a given string will always map to
  the same value (on separate invocations of the program).  This
  is why there's no PropertyName(int) constructor and no mechanism
  for storing PropertyNames in properties.  (Of course, you can 
  store the string representation of a PropertyName in a property;
  but that's slow.)

*/

class PropertyName
{
public:
    PropertyName() : m_value(-1) { }
    PropertyName(const char *cs) { std::string s(cs); m_value = intern(s); }
    PropertyName(const std::string &s) : m_value(intern(s)) { }
    PropertyName(const PropertyName &p) : m_value(p.m_value) { }
    ~PropertyName() { }

    PropertyName &operator=(const char *cs) {
        std::string s(cs);
        m_value = intern(s);
        return *this;
    }
    PropertyName &operator=(const std::string &s) {
        m_value = intern(s);
        return *this;
    }
    PropertyName &operator=(const PropertyName &p) {
        m_value = p.m_value;
        return *this;
    }

    bool operator==(const PropertyName &p) const {
        return m_value == p.m_value;
    }
    bool operator< (const PropertyName &p) const {
        return m_value <  p.m_value;
    }

    const char *c_str() const {
        return getName().c_str();
    }

    std::string getName() const /* throw (CorruptedValue) */;

    int getValue() const { return m_value; }

    static const PropertyName EmptyPropertyName;
    
private:
    typedef std::map<std::string, int> intern_map;
    typedef intern_map::value_type intern_pair;

    typedef std::map<int, std::string> intern_reverse_map;
    typedef intern_reverse_map::value_type intern_reverse_pair;

    static intern_map *m_interns;
    static intern_reverse_map *m_internsReversed;
    static int m_nextValue;

    int m_value;

    static int intern(const std::string &s);
};

inline std::ostream& operator<<(std::ostream &out, const PropertyName &n) {
    out << n.getName();
    return out;
}

inline std::string operator+(const std::string &s, const PropertyName &n) {
    return s + n.getName();
}

struct PropertyNamesEqual
{
    bool operator() (const PropertyName &s1, const PropertyName &s2) const {
        return s1 == s2;
    }
};

struct PropertyNameHash
{
    size_t operator() (const PropertyName &s) const {
        return static_cast<size_t>(s.getValue());
    }
};

}

#endif
