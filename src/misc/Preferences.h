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

#pragma once

namespace Rosegarden
{


/// Preferences
/**
 * Namespace for preferences getters and setters.  Gathering these into
 * a central location has the following benefits:
 *
 *   - Single location for .conf entry names and defaults.
 *   - Performance.  Caching to avoid hitting the .conf file.
 *   - Avoiding restarts.  By calling the setters, the
 *     Preferences dialogs can have a direct effect on cached
 *     values.  The user need not restart Rosegarden to see
 *     these changes take effect.
 *   - Consistent high-performance "write on first read" behavior to
 *     make it easier to find new .conf values during development.
 *
 */
namespace Preferences
{
    void setSendProgramChangesWhenLooping(bool value);
    bool getSendProgramChangesWhenLooping();

    void setSendControlChangesWhenLooping(bool value);
    bool getSendControlChangesWhenLooping();

    // ??? Move ChannelManager.cpp:allowReset() and forceChannelSetups() here.
}


}
