// -*- c-basic-offset: 4 -*-

/*
    Rosegarden
    A sequencer and musical notation editor.
 
    This program is Copyright 2000-2007
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

#include "Strings.h"
#include "misc/Debug.h"
#include "Debug.h"

#if KDE_VERSION < KDE_MAKE_VERSION(3,2,0)
#include <qdatetime.h>
#include <qpoint.h>
#include <qrect.h>
#include <qregion.h>
#include <qstringlist.h>
#include <qpen.h>
#include <qbrush.h>
#include <qsize.h>
#include <kurl.h>
#endif

#include "base/Event.h"
#include "base/Segment.h"
#include "base/RealTime.h"
#include "base/Colour.h"

#ifndef NDEBUG

kdbgstream&
operator<<(kdbgstream &dbg, const std::string &s)
{
    dbg << strtoqstr(s);
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
    dbg << "Segment for instrument " << t.getTrack()
    << " starting at " << t.getStartTime() << endl;

    for (Rosegarden::Segment::const_iterator i = t.begin();
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

kdbgstream&
operator<<(kdbgstream &dbg, const Rosegarden::RealTime &t)
{
    dbg << t.toString();
    return dbg;
}

kdbgstream&
operator<<(kdbgstream &dbg, const Rosegarden::Colour &c)
{
    dbg << "Colour : rgb = " << c.getRed() << "," << c.getGreen() << "," << c.getBlue();
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

// void DBCheckThrow()
// {
//     using Rosegarden::Int;

//     Rosegarden::Event ev;

//     try {
//         int pitch = ev.get<Int>("BLAH");

//     } catch (Rosegarden::Event::NoData) {
//         RG_DEBUG << "DBCheckThrow()" << endl;
//     }
// }

#if KDE_VERSION < KDE_MAKE_VERSION(3,2,0)
kdbgstream&
operator<<(kdbgstream &dbg, const QDateTime& time)
{
    dbg << time.toString();
    return dbg;
}

kdbgstream&
operator<<(kdbgstream &dbg, const QDate& date)
{
    dbg << date.toString();
    return dbg;
}

kdbgstream&
operator<<(kdbgstream &dbg, const QTime& time )
{
    dbg << time.toString();
    return dbg;
}

kdbgstream&
operator<<(kdbgstream &dbg, const QPoint& p )
{
    dbg << "(" << p.x() << ", " << p.y() << ")";
    return dbg;
}

kdbgstream&
operator<<(kdbgstream &dbg, const QSize& s )
{
    dbg << "[" << s.width() << "x" << s.height() << "]";
    return dbg;
}

kdbgstream&
operator<<(kdbgstream &dbg, const QRect& r )
{
    dbg << "[" << r.x() << "," << r.y() << " - "
    << r.width() << "x" << r.height() << "]";
    return dbg;
}

kdbgstream&
operator<<(kdbgstream &dbg, const QRegion& reg )
{
    dbg << "[ ";

    QMemArray<QRect>rs = reg.rects();
    for (uint i = 0;i < rs.size();++i)
        dbg << QString("[%1,%2 - %3x%4] ").arg(rs[i].x()).arg(rs[i].y())
        .arg(rs[i].width()).arg(rs[i].height() ) ;

    dbg << "]";
    return dbg;
}

kdbgstream&
operator<<(kdbgstream &dbg, const KURL& u )
{
    dbg << u.prettyURL();
    return dbg;
}

kdbgstream&
operator<<(kdbgstream &dbg, const QStringList& l )
{
    dbg << "(";
    dbg << l.join(",");
    dbg << ")";

    return dbg;
}

kdbgstream&
operator<<(kdbgstream &dbg, const QColor& c )
{
    if ( c.isValid() )
        dbg << c.name();
    else
        dbg << "(invalid/default)";
    return dbg;
}

kdbgstream&
operator<<(kdbgstream &dbg, const QPen& p )
{
    static const char* const s_penStyles[] = {
                "NoPen", "SolidLine", "DashLine", "DotLine", "DashDotLine",
                "DashDotDotLine"
            };
    static const char* const s_capStyles[] = {
                "FlatCap", "SquareCap", "RoundCap"
            };
    dbg << "[ style:";
    dbg << s_penStyles[ p.style() ];
    dbg << " width:";
    dbg << p.width();
    dbg << " color:";
    if ( p.color().isValid() )
        dbg << p.color().name();
    else
        dbg << "(invalid/default)";
    if ( p.width() > 0 ) {
        dbg << " capstyle:";
        dbg << s_capStyles[ p.capStyle() >> 4 ];
    }
    dbg << " ]";
    return dbg;
}

kdbgstream&
operator<<(kdbgstream &dbg, const QBrush& b)
{
    static const char* const s_brushStyles[] = {
                "NoBrush", "SolidPattern", "Dense1Pattern", "Dense2Pattern", "Dense3Pattern",
                "Dense4Pattern", "Dense5Pattern", "Dense6Pattern", "Dense7Pattern",
                "HorPattern", "VerPattern", "CrossPattern", "BDiagPattern", "FDiagPattern",
                "DiagCrossPattern"
            };

    dbg << "[ style: ";
    dbg << s_brushStyles[ b.style() ];
    dbg << " color: ";
    if ( b.color().isValid() )
        dbg << b.color().name() ;
    else
        dbg << "(invalid/default)";
    if ( b.pixmap() )
        dbg << " has a pixmap";
    dbg << " ]";
    return dbg;
}
#endif

#endif

