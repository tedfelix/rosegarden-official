// -*- c-basic-offset: 4 -*-

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

#include "rosedebug.h"

#include "Event.h"
#include "Track.h"


kdbgstream&
operator<<(kdbgstream &dbg, const std::string &s)
{
    dbg << s.c_str();
    return dbg;
}

kdbgstream&
operator<<(kdbgstream &dbg, const Rosegarden::Event &e)
{
    dbg << "Event type : " << e.getType() << endl;

    dbg << "\tDuration : " << e.getDuration()
        << "\n\tAbsolute Time : " << e.getAbsoluteTime()
        << endl;

//     for (Event::PropertyMap::const_iterator i = e.properties().begin();
//          i != e.properties().end(); ++i) {
//         dbg << "\t\t" << (*i).first << " : "
//             << ((*i).second)->unparse() << '\n';
//     }

//     e.dump(std::cerr);
    
    return dbg;
}

kdbgstream&
operator<<(kdbgstream &dbg, const Rosegarden::Segment &t)
{
    dbg << "Segment for instrument " << t.getInstrument()
        << " starting at " << t.getStartIndex() << endl;

    for(Rosegarden::Segment::const_iterator i = t.begin();
        i != t.end(); ++i) {
        if (!(*i)) {
            dbg << "WARNING : skipping null event ptr\n";
            continue;
        }

        dbg << "Dumping Event : \n";
        dbg << *(*i) << endl;
    }

    return dbg;
}


#ifdef NOT_DEFINED

ostream&
kdbgostreamAdapter::operator<<(bool i)
{
    m_kdbgStream << i;
    return *this;
}

ostream&
kdbgostreamAdapter::operator<<(short i)
{
    m_kdbgStream << i;
    return *this;
}

ostream&
kdbgostreamAdapter::operator<<(unsigned short i)
{
    m_kdbgStream << i;
    return *this;
}

ostream&
kdbgostreamAdapter::operator<<(char i)
{
    m_kdbgStream << i;
    return *this;
}

ostream&
kdbgostreamAdapter::operator<<(unsigned char i)
{
    m_kdbgStream << i;
    return *this;
}

ostream&
kdbgostreamAdapter::operator<<(int i)
{
    m_kdbgStream << i;
    return *this;
}

ostream&
kdbgostreamAdapter::operator<<(unsigned int i)
{
    m_kdbgStream << i;
    return *this;
}

ostream&
kdbgostreamAdapter::operator<<(long i)
{
    m_kdbgStream << i;
    return *this;
}

ostream&
kdbgostreamAdapter::operator<<(unsigned long i)
{
    m_kdbgStream << i;
    return *this;
}

ostream&
kdbgostreamAdapter::operator<<(const QString& string)
{
    m_kdbgStream << string;
    return *this;
}

ostream&
kdbgostreamAdapter::operator<<(const char *string)
{
    m_kdbgStream << string;
    return *this;
}

ostream&
kdbgostreamAdapter::operator<<(const QCString& string)
{
    m_kdbgStream << string;
    return *this;
}

ostream&
kdbgostreamAdapter::operator<<(void * p)
{
    m_kdbgStream << p;
    return *this;
}

ostream&
kdbgostreamAdapter::operator<<(KDBGFUNC f)
{
    (*f)(m_kdbgStream);
    return *this;
}

ostream&
kdbgostreamAdapter::operator<<(double d)
{
    m_kdbgStream << d;
    return *this;
}

#endif

// ostream& endl( ostream &s)
// {
//     s << "\n"; return s;
// }

void DBCheckThrow()
{
    using Rosegarden::Int;

    Rosegarden::Event ev;
    
    try {
        int pitch = ev.get<Int>("BLAH");

    } catch (Rosegarden::Event::NoData) {
        kdDebug(KDEBUG_AREA) << "DBCheckThrow()" << endl;
    }
}

