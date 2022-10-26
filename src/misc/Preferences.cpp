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

#include <QSettings>

namespace Rosegarden
{


// Was thinking about doing a template, but there are finicky little
// differences between the types (QVariant conversions).  Bool is the
// most popular, so I'm trying this one first.  Might take a shot at
// a template later.
class PreferenceBool
{
public:
    PreferenceBool(QString group, QString key, bool defaultValue) :
        m_group(group),
        m_key(key),
        m_defaultValue(defaultValue)
    {
    }

    void set(bool value)
    {
        QSettings settings;
        settings.beginGroup(m_group);
        settings.setValue(m_key, value);
        m_cache = value;
    }

    bool get() const
    {
        if (!m_cacheValid) {
            m_cacheValid = true;

            QSettings settings;
            settings.beginGroup(m_group);
            m_cache = settings.value(
                    m_key, m_defaultValue ? "true" : "false").toBool();
            // Write it back out so we can find it if it wasn't there.
            settings.setValue(m_key, m_cache);
        }

        return m_cache;
    }

private:
    QString m_group;
    QString m_key;

    bool m_defaultValue;

    mutable bool m_cacheValid = false;
    mutable bool m_cache{};
};

namespace
{
    // Cached values for performance...

    int afldLocation = 0;
    QString afldCustomLocation;
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
    const QString AudioFileLocationDialogGroup = "AudioFileLocationDialog";
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

void Preferences::setDefaultAudioLocation(int location)
{
    QSettings settings;
    settings.beginGroup(AudioFileLocationDialogGroup);
    settings.setValue("location", location);
    afldLocation = location;
}

int Preferences::getDefaultAudioLocation()
{
    static bool firstGet = true;

    if (firstGet) {
        firstGet = false;

        QSettings settings;
        settings.beginGroup(AudioFileLocationDialogGroup);
        afldLocation = settings.value("location", "0").toInt();
        // Write it back out so we can find it if it wasn't there.
        settings.setValue("location", afldLocation);
    }

    return afldLocation;
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
