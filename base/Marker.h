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

#ifndef _MARKER_H_
#define _MARKER_H_

#include <string>

#include "Event.h"
#include "XmlExportable.h"

// A Marker is a user defined point in a Composition that can be
// used to define looping points - jump to, make notes at etc.
//
// Not to be confused with Composition or Segment start and 
// end markers.  Which they probably could be quite easily.
// I probably should've thought the name through a bit better
// really..
//

namespace Rosegarden
{

class Marker : public XmlExportable
{
public:
    Marker():m_time(0), m_name(std::string("<unnamed>")), 
             m_description(std::string("<none>")) { }

    Marker(Rosegarden::timeT time, const std::string &name,
           const std::string &description):
        m_time(time), m_name(name), m_description(description) { }

    Rosegarden::timeT getTime() const { return m_time; }
    std::string getName() const { return m_name; }
    std::string getDescription() const { return m_description; }

    void setTime(Rosegarden::timeT time) { m_time = time; }
    void setName(const std::string &name) { m_name = name; }
    void setDescription(const std::string &des) { m_description = des; }

    // export as XML
    virtual std::string toXmlString();

protected:

    Rosegarden::timeT    m_time;
    std::string          m_name;
    std::string          m_description;

};

}

#endif // _CONTROLPARAMETER_H_
