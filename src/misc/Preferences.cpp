/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A sequencer and musical notation editor.
    Copyright 2000-2022 the Rosegarden development team.
    See the AUTHORS file for more details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#include "Preferences.h"

#include "ConfigGroups.h"
#include "PreferenceBool.h"
#include "PreferenceInt.h"

#include <QSettings>

namespace Rosegarden
{


namespace
{
    // Cached values for performance...

    QString afldCustomLocation;
}

PreferenceBool thorn(
        GeneralOptionsConfigGroup, "use_thorn_style", true);

void Preferences::setThorn(bool value)
{
    thorn.set(value);
}

bool Preferences::getThorn()
{
    return thorn.get();
}

PreferenceBool sendProgramChangesWhenLooping(
        GeneralOptionsConfigGroup, "sendProgramChangesWhenLooping", true);

void Preferences::setSendProgramChangesWhenLooping(bool value)
{
    sendProgramChangesWhenLooping.set(value);
}

bool Preferences::getSendProgramChangesWhenLooping()
{
    return sendProgramChangesWhenLooping.get();
}

PreferenceBool sendControlChangesWhenLooping(
        GeneralOptionsConfigGroup, "sendControlChangesWhenLooping", true);

void Preferences::setSendControlChangesWhenLooping(bool value)
{
    sendControlChangesWhenLooping.set(value);
}

bool Preferences::getSendControlChangesWhenLooping()
{
    return sendControlChangesWhenLooping.get();
}

PreferenceBool useNativeFileDialogs("FileDialog", "useNativeFileDialogs", true);

void Preferences::setUseNativeFileDialogs(bool value)
{
    useNativeFileDialogs.set(value);
}

bool Preferences::getUseNativeFileDialogs()
{
    return useNativeFileDialogs.get();
}

PreferenceBool stopAtSegmentEnd(
        SequencerOptionsConfigGroup, "stopatend", false);

void Preferences::setStopAtSegmentEnd(bool value)
{
    stopAtSegmentEnd.set(value);
}

bool Preferences::getStopAtSegmentEnd()
{
    return stopAtSegmentEnd.get();
}

PreferenceBool jumpToLoop(SequencerOptionsConfigGroup, "jumpToLoop", true);

void Preferences::setJumpToLoop(bool value)
{
    jumpToLoop.set(value);
}

bool Preferences::getJumpToLoop()
{
    return jumpToLoop.get();
}

PreferenceBool advancedLooping(
        SequencerOptionsConfigGroup, "advancedLooping", false);

void Preferences::setAdvancedLooping(bool value)
{
    advancedLooping.set(value);
}

bool Preferences::getAdvancedLooping()
{
    return advancedLooping.get();
}

namespace
{
    const char *AudioFileLocationDialogGroup = "AudioFileLocationDialog";
}

PreferenceBool afldDontShow(AudioFileLocationDialogGroup, "dontShow", false);

void Preferences::setAudioFileLocationDlgDontShow(bool value)
{
    afldDontShow.set(value);
}

bool Preferences::getAudioFileLocationDlgDontShow()
{
    return afldDontShow.get();
}

PreferenceInt afldLocation(AudioFileLocationDialogGroup, "location", 0);

void Preferences::setDefaultAudioLocation(int location)
{
    afldLocation.set(location);
}

int Preferences::getDefaultAudioLocation()
{
    return afldLocation.get();
}

void Preferences::setCustomAudioLocation(QString location)
{
    QSettings settings;
    settings.beginGroup(AudioFileLocationDialogGroup);
    settings.setValue("customLocation", location);
    afldCustomLocation = location;
}

QString Preferences::getCustomAudioLocation()
{
    static bool firstGet = true;

    if (firstGet) {
        firstGet = false;

        QSettings settings;
        settings.beginGroup(AudioFileLocationDialogGroup);
        afldCustomLocation =
                settings.value("customLocation", "./sounds").toString();
        // Write it back out so we can find it if it wasn't there.
        settings.setValue("customLocation", afldCustomLocation);
    }

    return afldCustomLocation;
}

PreferenceBool jackLoadCheck(
        SequencerOptionsConfigGroup, "jackLoadCheck", true);

void Preferences::setJACKLoadCheck(bool value)
{
    jackLoadCheck.set(value);
}

bool Preferences::getJACKLoadCheck()
{
    return jackLoadCheck.get();
}

PreferenceBool bug1623(ExperimentalConfigGroup, "bug1623", false);

bool Preferences::getBug1623()
{
    return bug1623.get();
}

PreferenceBool autoChannels(ExperimentalConfigGroup, "autoChannels", false);

void Preferences::setAutoChannels(bool value)
{
    autoChannels.set(value);
}

bool Preferences::getAutoChannels()
{
    return autoChannels.get();
}


}
