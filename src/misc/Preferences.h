/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A sequencer and musical notation editor.
    Copyright 2000-2024 the Rosegarden development team.
    See the AUTHORS file for more details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#pragma once

#include <QString>

// Need this to be seen within main.cpp.
#include <rosegardenprivate_export.h>

namespace Rosegarden
{


/// Preferences
/**
 * Namespace for preferences getters and setters.  Gathering these into
 * a central location has the following benefits:
 *
 *   - Single location for .conf entry names and defaults.
 *   - Avoids confusing calls to QSettings::beginGroup()/endGroup().
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
    // Theme
    constexpr int NativeTheme = 0;  // Thorn off.
    constexpr int ClassicTheme = 1;  // Thorn on.
    constexpr int DarkTheme = 2;  // Thorn on.
    ROSEGARDENPRIVATE_EXPORT void setTheme(int value);
    ROSEGARDENPRIVATE_EXPORT int getTheme();
    ROSEGARDENPRIVATE_EXPORT bool getThorn();

    void setSendProgramChangesWhenLooping(bool value);
    bool getSendProgramChangesWhenLooping();

    void setSendControlChangesWhenLooping(bool value);
    bool getSendControlChangesWhenLooping();

    // ??? Move ChannelManager.cpp:allowReset() and forceChannelSetups() here.

    void setUseNativeFileDialogs(bool value);
    bool getUseNativeFileDialogs();

    void setStopAtSegmentEnd(bool value);
    bool getStopAtSegmentEnd();

    void setJumpToLoop(bool value);
    bool getJumpToLoop();

    void setAdvancedLooping(bool value);
    bool getAdvancedLooping();

    // AudioFileLocationDialog settings

    void setAudioFileLocationDlgDontShow(bool value);
    bool getAudioFileLocationDlgDontShow();

    // See AudioFileLocationDialog::Location enum.
    void setDefaultAudioLocation(int location);
    int getDefaultAudioLocation();

    void setCustomAudioLocation(const QString &location);
    QString getCustomAudioLocation();

    void setJACKLoadCheck(bool value);
    bool getJACKLoadCheck();

    void setShowNoteNames(bool value);
    bool getShowNoteNames();

    // Experimental

    bool getBug1623();

    // Enable/disable LV2 plugin discovery.
    void setLV2(bool value);
    bool getLV2();

    void setDynamicDrag(bool value);
    bool getDynamicDrag();

    void setAutoChannels(bool value);
    bool getAutoChannels();

    void setIncludeAlsaPortNumbersWhenMatching(bool value);
    bool getIncludeAlsaPortNumbersWhenMatching();

    void setSMFExportPPQN(int value);
    int getSMFExportPPQN();
}


}
