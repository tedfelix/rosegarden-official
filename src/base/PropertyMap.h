/*
    Rosegarden
    A sequencer and musical notation editor.
    Copyright 2000-2025 the Rosegarden development team.
    See the AUTHORS file for more details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#ifndef RG_PROPERTY_MAP_H
#define RG_PROPERTY_MAP_H

#include "Property.h"
#include "base/PropertyName.h"

#include <rosegardenprivate_export.h>

#include <map>

namespace Rosegarden {

/// Holds name/value pairs in an effort to reduce memory usage.
/**
 * Event has a PropertyMap (Event::EventData::m_properties) which allows Event
 * objects to be flexible about the fields that they contain.  All Event objects
 * need a time (Event::m_absoluteTime) and (usually) a duration
 * (Event::m_duration) but not all Event objects need a pitch
 * (BaseProperties::PITCH).  E.g a control change Event does not need a pitch.
 */
class ROSEGARDENPRIVATE_EXPORT PropertyMap : public std::map<PropertyName, PropertyStoreBase *>
{
public:
    PropertyMap() { }
    PropertyMap(const PropertyMap &pm);

    ~PropertyMap();
    
    void clear();
    
    std::string toXmlString() const;

    bool operator==(const PropertyMap &other) const;
    bool operator!=(const PropertyMap &other) const { return !operator==(other); }

private:
    PropertyMap &operator=(const PropertyMap &); // not provided
};

typedef PropertyMap::value_type PropertyPair;

}

#endif
