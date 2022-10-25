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
// differences between the types.  Bool is the most popular, so I'm
// trying this one first.
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

    bool sendProgramChangesWhenLooping = true;
    bool sendControlChangesWhenLooping = true;
    bool useNativeFileDialogs = true;
    bool stopAtSegmentEnd = false;
    //bool jumpToLoop = true;
    bool advancedLooping = false;

    bool afldDontShow = false;
    int afldLocation = 0;
    QString afldCustomLocation;

    bool bug1623 = false;
    bool autoChannels = false;
}

void Preferences::setSendProgramChangesWhenLooping(bool value)
{
    QSettings settings;
    settings.beginGroup(GeneralOptionsConfigGroup);
    settings.setValue("sendProgramChangesWhenLooping", value);
    sendProgramChangesWhenLooping = value;
}

bool Preferences::getSendProgramChangesWhenLooping()
{
    static bool firstGet = true;

    if (firstGet) {
        firstGet = false;

        QSettings settings;
        settings.beginGroup(GeneralOptionsConfigGroup);
        sendProgramChangesWhenLooping =
                settings.value("sendProgramChangesWhenLooping", "true").toBool();
        // Write it back out so we can find it if it wasn't there.
        settings.setValue("sendProgramChangesWhenLooping",
                          sendProgramChangesWhenLooping);
    }

    return sendProgramChangesWhenLooping;
}

void Preferences::setSendControlChangesWhenLooping(bool value)
{
    QSettings settings;
    settings.beginGroup(GeneralOptionsConfigGroup);
    settings.setValue("sendControlChangesWhenLooping", value);
    sendControlChangesWhenLooping = value;
}

bool Preferences::getSendControlChangesWhenLooping()
{
    static bool firstGet = true;

    if (firstGet) {
        firstGet = false;

        QSettings settings;
        settings.beginGroup(GeneralOptionsConfigGroup);
        sendControlChangesWhenLooping =
                settings.value("sendControlChangesWhenLooping", "true").toBool();
        // Write it back out so we can find it if it wasn't there.
        settings.setValue("sendControlChangesWhenLooping",
                          sendControlChangesWhenLooping);
    }

    return sendControlChangesWhenLooping;
}

void Preferences::setUseNativeFileDialogs(bool value)
{
    QSettings settings;
    settings.beginGroup("FileDialog");
    settings.setValue("useNativeFileDialogs", value);
    useNativeFileDialogs = value;
}

bool Preferences::getUseNativeFileDialogs()
{
    static bool firstGet = true;

    if (firstGet) {
        firstGet = false;

        QSettings settings;
        settings.beginGroup("FileDialog");
        useNativeFileDialogs =
                settings.value("useNativeFileDialogs", "true").toBool();
        // Write it back out so we can find it if it wasn't there.
        settings.setValue("useNativeFileDialogs",
                useNativeFileDialogs);
    }

    return useNativeFileDialogs;
}

void Preferences::setStopAtSegmentEnd(bool value)
{
    QSettings settings;
    settings.beginGroup(SequencerOptionsConfigGroup);
    settings.setValue("stopatend", value);
    stopAtSegmentEnd = value;
}

bool Preferences::getStopAtSegmentEnd()
{
    static bool firstGet = true;

    if (firstGet) {
        firstGet = false;

        QSettings settings;
        settings.beginGroup(SequencerOptionsConfigGroup);
        stopAtSegmentEnd =
            settings.value("stopatend", "false").toBool();
        // Write it back out so we can find it if it wasn't there.
        settings.setValue("stopatend", stopAtSegmentEnd);
    }

    return stopAtSegmentEnd;
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

void Preferences::setAdvancedLooping(bool value)
{
    QSettings settings;
    settings.beginGroup(SequencerOptionsConfigGroup);
    settings.setValue("advancedLooping", value);
    advancedLooping = value;
}

bool Preferences::getAdvancedLooping()
{
    static bool firstGet = true;

    if (firstGet) {
        firstGet = false;

        QSettings settings;
        settings.beginGroup(SequencerOptionsConfigGroup);
        advancedLooping =
            settings.value("advancedLooping", "false").toBool();
        // Write it back out so we can find it if it wasn't there.
        settings.setValue("advancedLooping", advancedLooping);
    }

    return advancedLooping;
}

namespace
{
    const QString AudioFileLocationDialogGroup = "AudioFileLocationDialog";
}

void Preferences::setAudioFileLocationDlgDontShow(bool value)
{
    QSettings settings;
    settings.beginGroup(AudioFileLocationDialogGroup);
    settings.setValue("dontShow", value);
    afldDontShow = value;
}

bool Preferences::getAudioFileLocationDlgDontShow()
{
    static bool firstGet = true;

    if (firstGet) {
        firstGet = false;

        QSettings settings;
        settings.beginGroup(AudioFileLocationDialogGroup);
        afldDontShow = settings.value("dontShow", "false").toBool();
        // Write it back out so we can find it if it wasn't there.
        settings.setValue("dontShow", afldDontShow);
    }

    return afldDontShow;
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

bool Preferences::getBug1623()
{
    static bool firstGet = true;

    if (firstGet) {
        firstGet = false;

        QSettings settings;
        settings.beginGroup(ExperimentalConfigGroup);
        bug1623 = settings.value("bug1623", "false").toBool();
        // Write it back out so we can find it if it wasn't there.
        settings.setValue("bug1623", bug1623);
    }

    return bug1623;
}

void Preferences::setAutoChannels(bool value)
{
    QSettings settings;
    settings.beginGroup(ExperimentalConfigGroup);
    settings.setValue("autoChannels", value);
    autoChannels = value;
}

bool Preferences::getAutoChannels()
{
    static bool firstGet = true;

    if (firstGet) {
        firstGet = false;

        QSettings settings;
        settings.beginGroup(ExperimentalConfigGroup);
        autoChannels = settings.value("autoChannels", "false").toBool();
        // Write it back out so we can find it if it wasn't there.
        settings.setValue("autoChannels", autoChannels);
    }

    return autoChannels;
}


}
