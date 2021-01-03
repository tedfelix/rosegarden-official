/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A sequencer and musical notation editor.
    Copyright 2000-2021 the Rosegarden development team.
    See the AUTHORS file for more details.
 
    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#include "Debug.h"

#include "Strings.h"  // for strtoqstr()

#include <QSettings>


namespace Rosegarden
{


ROSEGARDENPRIVATE_EXPORT QDebug &operator<<(QDebug &dbg, const std::string &s)
{
    dbg << strtoqstr(s);

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
