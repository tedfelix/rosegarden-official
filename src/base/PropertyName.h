/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A sequencer and musical notation editor.
    Copyright 2000-2023 the Rosegarden development team.
    See the AUTHORS file for more details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#ifndef RG_PROPERTY_NAME_H
#define RG_PROPERTY_NAME_H

#include <string>

#include <rosegardenprivate_export.h>

namespace Rosegarden
{


/// PropertyName ID class optimized for speed and space.
/**

  Maps a property name (e.g. "pitch", BaseProperties::PITCH) to a temporary
  serial ID (PropertyName::m_id) for use *only* at runtime.
  The actual property names (e.g. "pitch") are stored in the .rg file.

  This class is an optimization intended to speed up the code by avoiding
  string compares in favor of int compares.  It also saves space by storing
  only an int instead of an entire string for each property.

  @see PropertyMap which holds PropertyName / value pairs.

  @see BaseProperties which defines the PropertyName constants used with Event.

  A PropertyName is something that can be constructed from a string,
  compared quickly as an int, hashed as a key in a hash map, and
  streamed out again as a string.  It must have accompanying functors
  PropertyNamesEqual and PropertyNameHash which compare and hash
  PropertyName objects.

  The simplest implementation is a string:

@code
    typedef std::string PropertyName;

    struct PropertyNamesEqual {
      bool operator() (const PropertyName &s1, const PropertyName &s2) const {
        return s1 == s2;
      }
    };

    struct PropertyNameHash {
      static std::hash<const char *> hash;
      size_t operator() (const PropertyName &s) const {
          return hash(s.c_str());
      }
    };

    std::hash<const char *> PropertyNameHash::hash;
@endcode

  but our implementation is faster in practice: while it behaves
  outwardly like a string, for the Event that makes use of it,
  it performs much like a machine integer.  It also shares
  strings, reducing storage sizes if there are many names in use.

  A big caveat with this class is that it is _not_ safe to persist
  the ID of a PropertyName and assume that the original string
  can be recovered; it cannot.  The IDs are assigned on demand,
  and there's no guarantee that a given string will always map to
  the same ID (on separate invocations of the program).  This
  is why there's no PropertyName(int) constructor and no mechanism
  for storing PropertyName objects in properties.  (Of course, you can
  store the string representation of a PropertyName in a property;
  but that's slow.)

*/

class ROSEGARDENPRIVATE_EXPORT PropertyName
{
public:
    PropertyName() : m_id(-1)  { }
    explicit PropertyName(const char *name);
    explicit PropertyName(const std::string &name);

    PropertyName &operator=(const char *name);
    PropertyName &operator=(const std::string &name);

    bool operator==(const PropertyName &p) const
            { return m_id == p.m_id; }
    bool operator<(const PropertyName &p) const
            { return m_id < p.m_id; }

    std::string getName() const;

    int getId() const  { return m_id; }

    /// Returns the empty string PropertyName ("").
    static const PropertyName &Empty()
    {
        // Create on first use to avoid static init order fiasco.
        static const PropertyName empty("");
        return empty;
    }

private:

    // The name's ID.
    int m_id;

};

inline std::ostream& operator<<(std::ostream &out, const PropertyName &n)
{
    out << n.getName();
    return out;
}

inline std::string operator+(const std::string &s, const PropertyName &n)
{
    return s + n.getName();
}

struct PropertyNamesEqual
{
    bool operator() (const PropertyName &s1, const PropertyName &s2) const
            { return s1 == s2; }
};

// We just use the ID for the "hash".
struct PropertyNameHash
{
    size_t operator() (const PropertyName &s) const
            { return static_cast<size_t>(s.getId()); }
};


}

#endif
