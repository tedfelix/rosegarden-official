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

#include "Debug.h"

#include "Strings.h"  // for strtoqstr()

#include "base/Event.h"
//#include "base/Segment.h"
#include "base/RealTime.h"
#include "base/Colour.h"
#include "gui/editors/guitar/Chord.h"
#include "gui/editors/guitar/Fingering.h"

#include <QSettings>

namespace Rosegarden
{


ROSEGARDENPRIVATE_EXPORT QDebug &operator<<(QDebug &dbg, const std::string &s)
{
    dbg << strtoqstr(s);

    return dbg;
}

ROSEGARDENPRIVATE_EXPORT QDebug &operator<<(QDebug &dbg, const Rosegarden::RealTime &t)
{
    dbg << t.toString();
    return dbg;
}

ROSEGARDENPRIVATE_EXPORT QDebug &operator<<(QDebug &dbg, const Rosegarden::Colour &c)
{
    dbg << "Colour : rgb = " << c.getRed() << "," << c.getGreen() << "," << c.getBlue();
    return dbg;
}

ROSEGARDENPRIVATE_EXPORT QDebug &operator<<(QDebug &dbg, const Rosegarden::Guitar::Chord &c)
{
    dbg << "Chord root = " << c.getRoot() << ", ext = '" << c.getExt() << "'";

//    for(unsigned int i = 0; i < c.getNbFingerings(); ++i) {
//        dbg << "\nFingering " << i << " : " << c.getFingering(i).toString().c_str();
//    }
    
     Rosegarden::Guitar::Fingering f = c.getFingering();

     dbg << ", fingering : ";

     for(unsigned int j = 0; j < 6; ++j) {
         int pos = f[j];
         if (pos >= 0)
             dbg << pos << ' ';
         else
             dbg << "x ";
    }        
    return dbg;
}

#if 0
// Handy logging switcher.  Repurpose when needed in the future.
bool bug1560Logging()
{
    // Only check on the first call.
    static bool checked = false;
    static bool enabled = false;

    // If this is the first call, check...
    if (!checked) {
        checked = true;

        QSettings settings;
        QString key = "Logging/bug1560";
        enabled = settings.value(key, "false").toBool();
        // Write back out so it is easy to find.
        settings.setValue(key, enabled);
    }

    return enabled;
}
#endif


}
