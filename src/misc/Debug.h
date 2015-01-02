/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A sequencer and musical notation editor.
    Copyright 2000-2015 the Rosegarden development team.
    See the AUTHORS file for more details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#ifndef ROSEDEBUG_H
#define ROSEDEBUG_H

#include <QDebug>
#include <QTextStream>
#include <string>

namespace Rosegarden {

class Event;
class Segment;
class RealTime;
class Colour;
namespace Guitar {
    class Chord;
}

class RGNoDebug
{
public:
    inline RGNoDebug() {}
    inline ~RGNoDebug(){}

    template <typename T>
    inline RGNoDebug &operator<<(const T &) { return *this; }

    inline RGNoDebug &operator<<(QTextStreamFunction) { return *this; }
};

#if !defined NDEBUG

QDebug &operator<<(QDebug &, const std::string &);
QDebug &operator<<(QDebug &, const Rosegarden::Event &);
QDebug &operator<<(QDebug &, const Rosegarden::Segment &);
QDebug &operator<<(QDebug &, const Rosegarden::RealTime &);
QDebug &operator<<(QDebug &, const Rosegarden::Colour &);
QDebug &operator<<(QDebug &, const Rosegarden::Guitar::Chord &);

#if !defined RG_MODULE_STRING
#define RG_MODULE_STRING "[generic] "
#endif

// Use RG_INFO for startup/shutdown progress messages that will be helpful
// when debugging issues with users.  These will always be written to
// the debug output even with RG_NO_DEBUG_PRINT defined.  Keep them
// to a minimum.
#define RG_INFO QDebug(QtDebugMsg) << RG_MODULE_STRING

// Use RG_WARNING for errors that shouldn't usually occur, but we want
// to know about them when debugging issues with users.  These will always
// be written to the debug output even with RG_NO_DEBUG_PRINT defined.
// For a normal run, there should be no RG_WARNING output.
#define RG_WARNING QDebug(QtDebugMsg) << RG_MODULE_STRING

/*

Given RG_INFO and RG_WARNING, we can define RG_NO_DEBUG_PRINT for a
translation unit and it will silence the RG_DEBUG output but not the info
and warnings.  This can be used to reduce the noise in the debug output.

There's a lot of std::cerr throughout the system.  While it would be nice
to replace it all with either RG_INFO or RG_WARNING, we can't.  The problem
is that RG_INFO and RG_WARNING don't do anything in a release build.  So,
my current thought is that we should upgrade RG_INFO and RG_WARNING to
respond to a "-v" option at the command line.  This will give the users the
ability to turn on some debug output even in a release build.  Maybe -v
would turn on RG_INFO and RG_WARNING, while -vv would also turn on RG_DEBUG.
This would give users complete control over the output.  Without a -v, we
should shoot for complete silence unless we are crashing and have something
to say about it.  RG_FATAL?

*/

#else

#define RG_INFO    RGNoDebug()
#define RG_WARNING RGNoDebug()

#endif

#if !defined NDEBUG && !defined RG_NO_DEBUG_PRINT

#define RG_DEBUG        QDebug(QtDebugMsg) << RG_MODULE_STRING
#define NOTATION_DEBUG  QDebug(QtDebugMsg) << "[notation] "
#define MATRIX_DEBUG    QDebug(QtDebugMsg) << "[matrix] "
#define SEQUENCER_DEBUG QDebug(QtDebugMsg) << "[sequencer] "
#define SEQMAN_DEBUG    QDebug(QtDebugMsg) << "[seqman] "

#else

#define RG_DEBUG        RGNoDebug()
#define NOTATION_DEBUG  RGNoDebug()
#define MATRIX_DEBUG    RGNoDebug()
#define SEQUENCER_DEBUG RGNoDebug()
#define SEQMAN_DEBUG    RGNoDebug()

#endif

#define DEFINE_DUMMY_PRINTER(TYPE)                              \
QDebug &operator<<(QDebug &dbg, const TYPE &) { return dbg; }

}

#endif
