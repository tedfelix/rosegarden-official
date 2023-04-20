/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A sequencer and musical notation editor.
    Copyright 2000-2023 the Rosegarden development team.
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
#include "PreferenceString.h"

#include <QSettings>

namespace Rosegarden
{


PreferenceInt theme(GeneralOptionsConfigGroup, "theme", Preferences::DarkTheme);

void Preferences::setTheme(int value)
{
    theme.set(value);
}

int Preferences::getTheme()
{
    return theme.get();
}

bool Preferences::getThorn()
{
    return (theme.get() != Preferences::NativeTheme);
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

PreferenceString afldCustomLocation(
        AudioFileLocationDialogGroup, "customLocation", "./sounds");

void Preferences::setCustomAudioLocation(const QString &location)
{
    afldCustomLocation.set(location);
}

QString Preferences::getCustomAudioLocation()
{
    return afldCustomLocation.get();
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
