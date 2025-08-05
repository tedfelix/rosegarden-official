/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A sequencer and musical notation editor.
    Copyright 2000-2025 the Rosegarden development team.
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

#include "gui/configuration/GeneralConfigurationPage.h"

#include <QFileInfo>
#include <QSettings>


namespace Rosegarden
{


static PreferenceInt theme(
        GeneralOptionsConfigGroup, "theme", Preferences::DarkTheme);

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

static PreferenceInt midiPitchOctave(
        GeneralOptionsConfigGroup, "midipitchoctave", -2);

void Preferences::setMIDIPitchOctave(int octave)
{
    midiPitchOctave.set(octave);
}

int Preferences::getMIDIPitchOctave()
{
    return midiPitchOctave.get();
}

static PreferenceBool sendProgramChangesWhenLooping(
        GeneralOptionsConfigGroup, "sendProgramChangesWhenLooping", true);

void Preferences::setSendProgramChangesWhenLooping(bool value)
{
    sendProgramChangesWhenLooping.set(value);
}

bool Preferences::getSendProgramChangesWhenLooping()
{
    return sendProgramChangesWhenLooping.get();
}

static PreferenceBool sendControlChangesWhenLooping(
        GeneralOptionsConfigGroup, "sendControlChangesWhenLooping", true);

void Preferences::setSendControlChangesWhenLooping(bool value)
{
    sendControlChangesWhenLooping.set(value);
}

bool Preferences::getSendControlChangesWhenLooping()
{
    return sendControlChangesWhenLooping.get();
}

static PreferenceBool useNativeFileDialogs(
        "FileDialog", "useNativeFileDialogs", true);

void Preferences::setUseNativeFileDialogs(bool value)
{
    useNativeFileDialogs.set(value);
}

bool Preferences::getUseNativeFileDialogs()
{
    return useNativeFileDialogs.get();
}

static PreferenceBool stopAtSegmentEnd(
        SequencerOptionsConfigGroup, "stopatend", false);

void Preferences::setStopAtSegmentEnd(bool value)
{
    stopAtSegmentEnd.set(value);
}

bool Preferences::getStopAtSegmentEnd()
{
    return stopAtSegmentEnd.get();
}

static PreferenceBool jumpToLoop(
        SequencerOptionsConfigGroup, "jumpToLoop", true);

void Preferences::setJumpToLoop(bool value)
{
    jumpToLoop.set(value);
}

bool Preferences::getJumpToLoop()
{
    return jumpToLoop.get();
}

static PreferenceBool advancedLooping(
        SequencerOptionsConfigGroup, "advancedLooping", false);

void Preferences::setAdvancedLooping(bool value)
{
    advancedLooping.set(value);
}

bool Preferences::getAdvancedLooping()
{
    return advancedLooping.get();
}

static PreferenceBool jackStopAtAutoStop(
        SequencerOptionsConfigGroup, "jackStopAtAutoStop", true);

void Preferences::setJACKStopAtAutoStop(bool value)
{
    jackStopAtAutoStop.set(value);
}

bool Preferences::getJACKStopAtAutoStop()
{
    return jackStopAtAutoStop.get();
}

namespace
{
    const char *AudioFileLocationDialogGroup = "AudioFileLocationDialog";
    PreferenceBool afldDontShow(
            AudioFileLocationDialogGroup, "dontShow", false);
}

void Preferences::setAudioFileLocationDlgDontShow(bool value)
{
    afldDontShow.set(value);
}

bool Preferences::getAudioFileLocationDlgDontShow()
{
    return afldDontShow.get();
}

static PreferenceInt afldLocation(AudioFileLocationDialogGroup, "location", 0);

void Preferences::setDefaultAudioLocation(Location location)
{
    afldLocation.set(location);
}

Preferences::Location Preferences::getDefaultAudioLocation()
{
    return static_cast<Location>(afldLocation.get());
}

QString Preferences::getDefaultAudioLocationString(const QString &absFilePath)
{
    const Location location = Preferences::getDefaultAudioLocation();

    switch (location) {
    case AudioDir:
        return "./audio";
    case DocumentNameDir:
        {
            QFileInfo fileInfo(absFilePath);
            return "./" + fileInfo.completeBaseName();
        }
    case DocumentDir:
        return ".";
    case CentralDir:
        return "~/rosegarden-audio";
    case CustomDir:
        return getCustomAudioLocation();
    }

    return "./audio";
}

static PreferenceString afldCustomLocation(
        AudioFileLocationDialogGroup, "customLocation", "./sounds");

void Preferences::setCustomAudioLocation(const QString &location)
{
    afldCustomLocation.set(location);
}

QString Preferences::getCustomAudioLocation()
{
    return afldCustomLocation.get();
}

static PreferenceBool jackLoadCheck(
        SequencerOptionsConfigGroup, "jackLoadCheck", true);

void Preferences::setJACKLoadCheck(bool value)
{
    jackLoadCheck.set(value);
}

bool Preferences::getJACKLoadCheck()
{
    return jackLoadCheck.get();
}

static PreferenceBool bug1623(ExperimentalConfigGroup, "bug1623", false);

bool Preferences::getBug1623()
{
    return bug1623.get();
}

static PreferenceBool lv2(ExperimentalConfigGroup, "lv2-b", true);

void Preferences::setLV2(bool value)
{
    lv2.set(value);
}

bool Preferences::getLV2()
{
    return lv2.get();
}

// NOTE: "dynamicDrag" is deprecated in the .conf as it had the wrong default.
static PreferenceBool dynamicDrag(
        GeneralOptionsConfigGroup, "dynamicDrag2", true);

void Preferences::setDynamicDrag(bool value)
{
    dynamicDrag.set(value);
}

bool Preferences::getDynamicDrag()
{
    return dynamicDrag.get();
}

static PreferenceBool autoChannels(
        ExperimentalConfigGroup, "autoChannels", false);

void Preferences::setAutoChannels(bool value)
{
    autoChannels.set(value);
}

bool Preferences::getAutoChannels()
{
    return autoChannels.get();
}

static PreferenceBool includeAlsaPortNumbersWhenMatching(
        GeneralOptionsConfigGroup, "includeAlsaPortNumbersWhenMatching", false);

void Preferences::setIncludeAlsaPortNumbersWhenMatching(bool value)
{
    includeAlsaPortNumbersWhenMatching.set(value);
}

bool Preferences::getIncludeAlsaPortNumbersWhenMatching()
{
    return includeAlsaPortNumbersWhenMatching.get();
}

static PreferenceBool showNoteNames(
        MatrixViewConfigGroup, "show_note_names", false);

void Preferences::setShowNoteNames(bool value)
{
    showNoteNames.set(value);
}

bool Preferences::getShowNoteNames()
{
    return showNoteNames.get();
}

static PreferenceInt smfExportPPQN(
        GeneralOptionsConfigGroup, "smfExportPPQN", 480);

void Preferences::setSMFExportPPQN(int value)
{
    smfExportPPQN.set(value);
}

int Preferences::getSMFExportPPQN()
{
    return smfExportPPQN.get();
}

static PreferenceBool matrixConstrainNotes(
        MatrixViewConfigGroup, "constrainNotes", false);

void Preferences::setMatrixConstrainNotes(bool value)
{
    matrixConstrainNotes.set(value);
}

bool Preferences::getMatrixConstrainNotes()
{
    return matrixConstrainNotes.get();
}

static PreferenceInt pdfViewer(
        ExternalApplicationsConfigGroup,
        "pdfviewer",
        GeneralConfigurationPage::getDefaultPDFViewer());

void Preferences::setPDFViewer(int value)
{
    pdfViewer.set(value);
}

int Preferences::getPDFViewer()
{
    return pdfViewer.get();
}

static PreferenceInt filePrinter(
        ExternalApplicationsConfigGroup,
        "fileprinter",
        GeneralConfigurationPage::getDefaultFilePrinter());

void Preferences::setFilePrinter(int value)
{
    filePrinter.set(value);
}

int Preferences::getFilePrinter()
{
    return filePrinter.get();
}


}
