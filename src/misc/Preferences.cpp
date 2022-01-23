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


namespace
{
    // Cached values for performance...
    bool sendProgramChangesWhenLooping = true;
    bool sendControlChangesWhenLooping = true;
    bool useNativeFileDialogs = true;
    bool stopAtEnd = false;
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

void Preferences::setStopAtEnd(bool value)
{
    QSettings settings;
    settings.beginGroup(SequencerOptionsConfigGroup);
    settings.setValue("stopatend", value);
    stopAtEnd = value;
}

bool Preferences::getStopAtEnd()
{
    static bool firstGet = true;

    if (firstGet) {
        firstGet = false;

        QSettings settings;
        settings.beginGroup(SequencerOptionsConfigGroup);
        stopAtEnd =
            settings.value("stopatend", "false").toBool();
        // Write it back out so we can find it if it wasn't there.
        settings.setValue("stopatend", stopAtEnd);
    }

    return stopAtEnd;
}


}
