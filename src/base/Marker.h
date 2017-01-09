/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A sequencer and musical notation editor.
    Copyright 2000-2017 the Rosegarden development team.
    See the AUTHORS file for more details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#ifndef RG_MARKER_H
#define RG_MARKER_H

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
             m_description(std::string("<none>")) { m_id = nextSeqVal(); }

    Marker(timeT time, const std::string &name,
           const std::string &description):
        m_time(time), m_name(name), m_description(description) { m_id = nextSeqVal(); }
    Marker(const Marker &other, timeT time) :
        m_id(other.m_id),
        m_time(time),
        m_name(other.m_name),
        m_description(other.m_description)
            {}
        
    int getID() const { return m_id; }
    timeT getTime() const { return m_time; }
    std::string getName() const { return m_name; }
    std::string getDescription() const { return m_description; }

    void setTime(timeT time) { m_time = time; }
    void setName(const std::string &name) { m_name = name; }
    void setDescription(const std::string &des) { m_description = des; }

    // export as XML
    virtual std::string toXmlString() const;

protected:

	int      m_id;
    timeT    m_time;
    std::string          m_name;
    std::string          m_description;

private:
	static int nextSeqVal() { return ++m_sequence; } // assume there won't be concurrency problem
	static int m_sequence;
};

}

#endif // RG_MARKER_H
