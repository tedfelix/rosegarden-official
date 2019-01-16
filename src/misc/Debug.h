/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A sequencer and musical notation editor.
    Copyright 2000-2018 the Rosegarden development team.
    See the AUTHORS file for more details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#ifndef RG_DEBUG_H
#define RG_DEBUG_H

#include <QDebug>
#include <QTextStream>

#include <string>
#include <rosegardenprivate_export.h>

namespace Rosegarden {


class Event;
class Segment;
class RealTime;
class Colour;
namespace Guitar {
    class Chord;
}

ROSEGARDENPRIVATE_EXPORT QDebug &operator<<(QDebug &, const std::string &);
ROSEGARDENPRIVATE_EXPORT QDebug &operator<<(QDebug &, const Rosegarden::Event &);
ROSEGARDENPRIVATE_EXPORT QDebug &operator<<(QDebug &, const Rosegarden::Segment &);
ROSEGARDENPRIVATE_EXPORT QDebug &operator<<(QDebug &, const Rosegarden::RealTime &);
ROSEGARDENPRIVATE_EXPORT QDebug &operator<<(QDebug &, const Rosegarden::Colour &);
ROSEGARDENPRIVATE_EXPORT QDebug &operator<<(QDebug &, const Rosegarden::Guitar::Chord &);

class RGNoDebug
{
public:
    inline RGNoDebug() {}
    inline ~RGNoDebug(){}

    template <typename T>
    inline RGNoDebug &operator<<(const T &) { return *this; }

    inline RGNoDebug &operator<<(QTextStreamFunction) { return *this; }
};

#if !defined RG_MODULE_STRING
    #define RG_MODULE_STRING "[generic] "
#endif

#if !defined NDEBUG

    // Use RG_INFO for startup/shutdown progress messages that will be helpful
    // when debugging issues with users.  These will always be written to
    // the debug output even with RG_NO_DEBUG_PRINT defined.  Keep them
    // to a minimum.
    #define RG_INFO QDebug(QtDebugMsg) << RG_MODULE_STRING

#else

    #define RG_INFO RGNoDebug() << RG_MODULE_STRING

#endif

#if !defined NDEBUG && !defined RG_NO_DEBUG_PRINT

    // Use RG_DEBUG for general debugging.  Define RG_NO_DEBUG_PRINT at the
    // top of a .cpp to turn off all RG_DEBUG output.
    #define RG_DEBUG        QDebug(QtDebugMsg) << RG_MODULE_STRING

    // !!! The following are deprecated since RG_MODULE_STRING provides more
    //     information.  Define RG_MODULE_STRING and use RG_DEBUG instead.
    #define NOTATION_DEBUG  QDebug(QtDebugMsg) << "[notation] "
    #define MATRIX_DEBUG    QDebug(QtDebugMsg) << "[matrix] "
    #define SEQUENCER_DEBUG QDebug(QtDebugMsg) << "[sequencer] "

#else

    #define RG_DEBUG        RGNoDebug()

    // !!! Deprecated.
    #define NOTATION_DEBUG  RGNoDebug()
    #define MATRIX_DEBUG    RGNoDebug()
    #define SEQUENCER_DEBUG RGNoDebug()

#endif

// Use RG_WARNING for recoverable errors that shouldn't usually occur, but
// we want to know about them when debugging issues with users.  These will
// always be written to the debug output even with RG_NO_DEBUG_PRINT defined.
// For a normal run, there should be no RG_WARNING output.
// This works in both debug and release builds.  Formerly, std::cerr was
// used for this.
#define RG_WARNING QDebug(QtDebugMsg) << RG_MODULE_STRING


}

#endif
