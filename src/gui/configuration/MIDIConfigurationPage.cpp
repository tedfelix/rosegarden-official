/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2024 the Rosegarden development team.
 
    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.
 
    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#define RG_MODULE_STRING "[MIDIConfigurationPage]"

#include "MIDIConfigurationPage.h"

#include "misc/ConfigGroups.h"  // For GeneralOptionsConfigGroup...
#include "misc/Debug.h"
#include "gui/widgets/FileDialog.h"
#include "gui/widgets/LineEdit.h"
#include "sound/MappedEvent.h"
#include "misc/Preferences.h"
#include "document/RosegardenDocument.h"
#include "sequencer/RosegardenSequencer.h"
#include "gui/application/RosegardenMainWindow.h"
#include "gui/seqmanager/SequenceManager.h"
#include "base/Studio.h"
#include "gui/studio/StudioControl.h"

#include <QComboBox>
#include <QFrame>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QSettings>
#include <QSpinBox>
#include <QStringList>
#include <QWidget>


namespace Rosegarden
{


MIDIConfigurationPage::MIDIConfigurationPage(QWidget *parent):
    TabbedConfigurationPage(parent),
    m_baseOctaveNumber(nullptr)
{

    // ---------------- General tab ------------------

    QWidget *widget = new QWidget;
    addTab(widget, tr("General"));

    QGridLayout *layout = new QGridLayout(widget);
    layout->setContentsMargins(20, 20, 20, 20);
    layout->setSpacing(5);

    int row = 0;

    QSettings settings;
    settings.beginGroup(GeneralOptionsConfigGroup);

    // Base octave number
    layout->addWidget(
            new QLabel(tr("Base octave number for MIDI pitch display")),
            row, 0, 1, 2);

    m_baseOctaveNumber = new QSpinBox;
    m_baseOctaveNumber->setMinimum(-10);
    m_baseOctaveNumber->setMaximum(10);
    m_baseOctaveNumber->setValue(
            settings.value("midipitchoctave", -2).toInt());
    connect(m_baseOctaveNumber,
                static_cast<void(QSpinBox::*)(int)>(&QSpinBox::valueChanged),
            this, &MIDIConfigurationPage::slotModified);
    layout->addWidget(m_baseOctaveNumber, row, 2, 1, 2);

    ++row;

    // Spacer
    layout->setRowMinimumHeight(row, 20);
    ++row;

    // Always use default studio
    layout->addWidget(
            new QLabel(tr("Always use default studio when loading files")),
            row, 0, 1, 2);

    m_useDefaultStudio = new QCheckBox;
    m_useDefaultStudio->setChecked(
            settings.value("alwaysusedefaultstudio", false).toBool());
    connect(m_useDefaultStudio, &QCheckBox::stateChanged,
            this, &MIDIConfigurationPage::slotModified);
    layout->addWidget(m_useDefaultStudio, row, 2);

    ++row;

    QLabel *label;
    QString toolTip;

    label = new QLabel(tr("Match ALSA port numbers"));
    toolTip = tr("Include ALSA port numbers when trying to match and restore MIDI connections when loading a file." );
    label->setToolTip( toolTip );
    layout->addWidget( label, row, 0, 1, 2 );

    m_includeAlsaPortNumbersWhenMatching = new QCheckBox;
    m_includeAlsaPortNumbersWhenMatching->setToolTip(toolTip);
    m_includeAlsaPortNumbersWhenMatching->setChecked(
            Preferences::getIncludeAlsaPortNumbersWhenMatching());
    connect(m_includeAlsaPortNumbersWhenMatching, &QCheckBox::stateChanged,
            this, &MIDIConfigurationPage::slotModified);
    layout->addWidget(m_includeAlsaPortNumbersWhenMatching, row, 2);

    ++row;


    // External controller port
    label = new QLabel(tr("External controller port"));
    toolTip = tr("Enable the external controller port for control surfaces.");
    label->setToolTip(toolTip);
    layout->addWidget(label, row, 0, 1, 2);

    m_externalControllerPort = new QCheckBox;
    m_externalControllerPort->setToolTip(toolTip);
    m_externalControllerPort->setChecked(
            settings.value("external_controller", false).toBool());
    connect(m_externalControllerPort, &QCheckBox::stateChanged,
            this, &MIDIConfigurationPage::slotModified);
    layout->addWidget(m_externalControllerPort, row, 2);

    ++row;

    label = new QLabel(tr("Controller type"));
    toolTip = tr("Select the type of control surface connected to the external controller port.");
    label->setToolTip(toolTip);
    layout->addWidget(label, row, 0, 1, 2);

    m_controllerType = new QComboBox;
    m_controllerType->addItem(tr("Rosegarden Native"));
    m_controllerType->addItem(tr("Korg nanoKONTROL2"));

    int controllerType = settings.value("controller_type", 0).toInt();
    if (controllerType < 0  ||  controllerType > 1)
        controllerType = 0;
    m_controllerType->setCurrentIndex(controllerType);

    connect(m_controllerType,
                static_cast<void(QComboBox::*)(int)>(&QComboBox::activated),
            this, &MIDIConfigurationPage::slotModified);

    layout->addWidget(m_controllerType, row, 2, 1, 2);

    ++row;

    // Accept transport CCs (116-118)
    label = new QLabel(tr("Accept transport CCs (116-118)"));
    toolTip = tr("Rosegarden will discard these CCs, so disable this if you need CCs in this range for other things.");
    label->setToolTip(toolTip);
    layout->addWidget(label, row, 0, 1, 2);

    m_acceptTransportCCs = new QCheckBox;
    m_acceptTransportCCs->setToolTip(toolTip);
    const bool acceptTransportCCs =
            settings.value("acceptTransportCCs", true).toBool();
    m_acceptTransportCCs->setChecked(acceptTransportCCs);
    connect(m_acceptTransportCCs, &QCheckBox::stateChanged,
            this, &MIDIConfigurationPage::slotModified);
    layout->addWidget(m_acceptTransportCCs, row, 2);

    ++row;

    settings.endGroup();
    settings.beginGroup(SequencerOptionsConfigGroup);

    // Allow Reset All Controllers
    label = new QLabel(tr("Allow Reset All Controllers (CC 121)"));
    toolTip = tr("Rosegarden can send a MIDI Reset All Controllers event when setting up a channel.");
    label->setToolTip(toolTip);
    layout->addWidget(label, row, 0, 1, 2);

    m_allowResetAllControllers = new QCheckBox;
    m_allowResetAllControllers->setToolTip(toolTip);
    const bool allowResetAllControllers =
            settings.value("allowresetallcontrollers", true).toBool();
    m_allowResetAllControllers->setChecked(allowResetAllControllers);
    connect(m_allowResetAllControllers, &QCheckBox::stateChanged,
            this, &MIDIConfigurationPage::slotModified);
    layout->addWidget(m_allowResetAllControllers, row, 2);

    ++row;

    // Send program changes when looping
    label = new QLabel(tr("Send program changes when looping"));
    toolTip = tr("Some synths have trouble with program changes coming in repeatedly.  Use this to turn them off when looping.");
    label->setToolTip(toolTip);
    layout->addWidget(label, row, 0, 1, 2);

    m_sendProgramChangesWhenLooping = new QCheckBox;
    m_sendProgramChangesWhenLooping->setToolTip(toolTip);
    m_sendProgramChangesWhenLooping->setChecked(
            Preferences::getSendProgramChangesWhenLooping());
    connect(m_sendProgramChangesWhenLooping, &QCheckBox::stateChanged,
            this, &MIDIConfigurationPage::slotModified);
    layout->addWidget(m_sendProgramChangesWhenLooping, row, 2);

    ++row;

    // Send control changes when looping
    label = new QLabel(tr("Send control changes when looping"));
    toolTip = tr("Some synths have trouble with control changes coming in repeatedly.  Use this to turn them off when looping.");
    label->setToolTip(toolTip);
    layout->addWidget(label, row, 0, 1, 2);

    m_sendControlChangesWhenLooping = new QCheckBox;
    m_sendControlChangesWhenLooping->setToolTip(toolTip);
    m_sendControlChangesWhenLooping->setChecked(
            Preferences::getSendControlChangesWhenLooping());
    connect(m_sendControlChangesWhenLooping, &QCheckBox::stateChanged,
            this, &MIDIConfigurationPage::slotModified);
    layout->addWidget(m_sendControlChangesWhenLooping, row, 2);

    ++row;

    // Sequencer timing source
    label = new QLabel(tr("Sequencer timing source"));
    layout->addWidget(label, row, 0, 1, 2);

    m_sequencerTimingSource = new QComboBox;

    m_originalTimingSource =
            RosegardenSequencer::getInstance()->getCurrentTimer();

    unsigned timerCount = RosegardenSequencer::getInstance()->getTimers();

    for (unsigned i = 0; i < timerCount; ++i) {
        QString timer = RosegardenSequencer::getInstance()->getTimer(i);

        // Skip the HR timer which causes a hard-lock of the computer.
        if (timer == "HR timer")
            continue;

        m_sequencerTimingSource->addItem(timer);
    }

    m_sequencerTimingSource->setCurrentIndex(
            m_sequencerTimingSource->findText(m_originalTimingSource));

    connect(m_sequencerTimingSource,
                static_cast<void(QComboBox::*)(int)>(&QComboBox::activated),
            this, &MIDIConfigurationPage::slotModified);
    layout->addWidget(m_sequencerTimingSource, row, 2, 1, 2);

    ++row;

    // Spacer
    layout->setRowMinimumHeight(row, 20);
    ++row;

    // Load SoundFont to SoundBlaster
    label = new QLabel(tr("Load SoundFont to SoundBlaster card at startup"));
    toolTip = tr("Check this box to enable soundfont loading on EMU10K-based cards when Rosegarden is launched");
    label->setToolTip(toolTip);
    layout->addWidget(label, row, 0, 1, 2);

    m_loadSoundFont = new QCheckBox;
    m_loadSoundFont->setToolTip(toolTip);
    m_loadSoundFont->setChecked(
            settings.value("sfxloadenabled", false).toBool());
    connect(m_loadSoundFont, &QCheckBox::stateChanged,
            this, &MIDIConfigurationPage::slotModified);
    connect(m_loadSoundFont, &QAbstractButton::clicked,
            this, &MIDIConfigurationPage::slotLoadSoundFontClicked);
    layout->addWidget(m_loadSoundFont, row, 2);

    ++row;

    // Path to 'asfxload'
    layout->addWidget(
            new QLabel(tr("Path to 'asfxload' or 'sfxload' command")),
            row, 0);

    m_pathToLoadCommand = new LineEdit(
            settings.value("sfxloadpath", "/usr/bin/asfxload").toString());
    layout->addWidget(m_pathToLoadCommand, row, 1, 1, 2);

    m_pathToLoadChoose = new QPushButton(tr("Choose..."));
    connect(m_pathToLoadChoose, &QAbstractButton::clicked,
            this, &MIDIConfigurationPage::slotPathToLoadChoose);
    layout->addWidget(m_pathToLoadChoose, row, 3);

    ++row;

    // SoundFont
    layout->addWidget(new QLabel(tr("SoundFont")), row, 0);

    m_soundFont = new LineEdit(
            settings.value("soundfontpath", "").toString());
    layout->addWidget(m_soundFont, row, 1, 1, 2);

    m_soundFontChoose = new QPushButton(tr("Choose..."));
    connect(m_soundFontChoose, &QAbstractButton::clicked,
            this, &MIDIConfigurationPage::slotSoundFontChoose);
    layout->addWidget(m_soundFontChoose, row, 3);

    ++row;

    // PPQN for MIDI File Export
    layout->addWidget(new QLabel(tr("PPQN/Division for MIDI File Export")), row, 0);
    m_ppqnSmfExport = new QSpinBox;
    m_ppqnSmfExport->setMinimum(96);
    m_ppqnSmfExport->setMaximum(960);
    m_ppqnSmfExport->setValue(Preferences::getSMFExportPPQN());
    connect(m_ppqnSmfExport,
            static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged),
            this,
            &MIDIConfigurationPage::slotModified);
    layout->addWidget(m_ppqnSmfExport, row, 2, 1, 2);

    // Fill out the rest of the space so that we do not end up centered.
    layout->setRowStretch(row, 10);


    //  -------------- MIDI Sync tab -----------------

    widget = new QWidget;
    addTab(widget, tr("MIDI Sync"));

    layout = new QGridLayout(widget);
    layout->setContentsMargins(20, 20, 20, 20);
    layout->setSpacing(5);

    row = 0;

    // MIDI Clock and System messages
    label = new QLabel(tr("MIDI Clock and System messages"));
    layout->addWidget(label, row, 0);

    m_midiClock = new QComboBox;
    m_midiClock->addItem(tr("Off"));
    m_midiClock->addItem(tr("Send MIDI Clock, Start and Stop"));
    m_midiClock->addItem(tr("Accept Start, Stop and Continue"));

    int midiClock = settings.value("midiclock", 0).toInt();
    if (midiClock < 0  ||  midiClock > 2)
        midiClock = 0;
    m_midiClock->setCurrentIndex(midiClock);

    connect(m_midiClock,
                static_cast<void(QComboBox::*)(int)>(&QComboBox::activated),
            this, &MIDIConfigurationPage::slotModified);

    layout->addWidget(m_midiClock, row, 1);

    ++row;

    // MIDI Machine Control mode
    label = new QLabel(tr("MIDI Machine Control mode"));
    layout->addWidget(label, row, 0);

    m_midiMachineControlMode = new QComboBox;
    m_midiMachineControlMode->addItem(tr("Off"));
    m_midiMachineControlMode->addItem(tr("MMC Source"));
    m_midiMachineControlMode->addItem(tr("MMC Follower"));

    int mmcMode = settings.value("mmcmode", 0).toInt();
    if (mmcMode < 0  ||  mmcMode > 2)
        mmcMode = 0;
    m_midiMachineControlMode->setCurrentIndex(mmcMode);

    connect(m_midiMachineControlMode,
                static_cast<void(QComboBox::*)(int)>(&QComboBox::activated),
            this, &MIDIConfigurationPage::slotModified);

    layout->addWidget(m_midiMachineControlMode, row, 1);
    
    ++row;

    // MIDI Time Code mode
    label = new QLabel(tr("MIDI Time Code mode"));
    layout->addWidget(label, row, 0);

    m_midiTimeCodeMode = new QComboBox;
    m_midiTimeCodeMode->addItem(tr("Off"));
    m_midiTimeCodeMode->addItem(tr("MTC Source"));
    m_midiTimeCodeMode->addItem(tr("MTC Follower"));

    int mtcMode = settings.value("mtcmode", 0).toInt();
    if (mtcMode < 0  ||  mtcMode > 2)
        mtcMode = 0;
    m_midiTimeCodeMode->setCurrentIndex(mtcMode);

    connect(m_midiTimeCodeMode,
                static_cast<void(QComboBox::*)(int)>(&QComboBox::activated),
            this, &MIDIConfigurationPage::slotModified);

    layout->addWidget(m_midiTimeCodeMode, row, 1);

    ++row;

    label = new QLabel(
            tr("Automatically connect sync output to all devices in use"));
    label->setWordWrap(true);
    layout->addWidget(label, row, 0);

    m_autoConnectSyncOut = new QCheckBox;
    m_autoConnectSyncOut->setChecked(
            settings.value("midisyncautoconnect", false).toBool());

    connect(m_autoConnectSyncOut, &QCheckBox::stateChanged,
            this, &MIDIConfigurationPage::slotModified);

    layout->addWidget(m_autoConnectSyncOut, row, 1);

    ++row;

    // Fill out the rest of the space so that we do not end up centered.
    layout->setRowStretch(row, 10);

    updateWidgets();

}

void
MIDIConfigurationPage::updateWidgets()
{
    const bool soundFontChecked = m_loadSoundFont->isChecked();

    m_pathToLoadCommand->setEnabled(soundFontChecked);
    m_pathToLoadChoose->setEnabled(soundFontChecked);
    m_soundFont->setEnabled(soundFontChecked);
    m_soundFontChoose->setEnabled(soundFontChecked);
}

void
MIDIConfigurationPage::slotLoadSoundFontClicked(bool /*isChecked*/)
{
    updateWidgets();
}

void
MIDIConfigurationPage::slotPathToLoadChoose()
{
    QString path = FileDialog::getOpenFileName(
            this,  // parent
            tr("sfxload path"),  // caption
            QDir::currentPath());  // dir

    // Canceled?  Bail.
    if (path == "")
        return;

    m_pathToLoadCommand->setText(path);
}

void
MIDIConfigurationPage::slotSoundFontChoose()
{
    QString path = FileDialog::getOpenFileName(
            this,  // parent
            tr("Soundfont path"),  // caption
            QDir::currentPath(),  // dir
            tr("Sound fonts") + " (*.sb *.sf2 *.SF2 *.SB)" + ";;" +
                tr("All files") + " (*)");  // filter

    // Canceled?  Bail.
    if (path == "")
        return;

    m_soundFont->setText(path);
}

void
MIDIConfigurationPage::apply()
{
    //RG_DEBUG << "apply()...";

    // Copy from controls to .conf file.  Send update messages as needed.


    // *** General tab

    QSettings settings;
    settings.beginGroup(GeneralOptionsConfigGroup);

    settings.setValue("midipitchoctave", m_baseOctaveNumber->value());
    settings.setValue("alwaysusedefaultstudio",
                      m_useDefaultStudio->isChecked());

    settings.setValue("includeAlsaPortNumbersWhenMatching",
                        m_includeAlsaPortNumbersWhenMatching->isChecked());
    Preferences::setIncludeAlsaPortNumbersWhenMatching(
            m_includeAlsaPortNumbersWhenMatching->isChecked());

    settings.setValue("external_controller",
                      m_externalControllerPort->isChecked());

    // setType() also writes to .conf so no need to do that here.
    ExternalController::self().setType(
            static_cast<ExternalController::ControllerType>(
                    m_controllerType->currentIndex()));

    settings.setValue("acceptTransportCCs",
                      m_acceptTransportCCs->isChecked());

    settings.endGroup();

    settings.beginGroup(SequencerOptionsConfigGroup);

    settings.setValue("allowresetallcontrollers",
                      m_allowResetAllControllers->isChecked());

    Preferences::setSendProgramChangesWhenLooping(
            m_sendProgramChangesWhenLooping->isChecked());
    Preferences::setSendControlChangesWhenLooping(
            m_sendControlChangesWhenLooping->isChecked());

    // If the timer setting has actually changed
    if (m_sequencerTimingSource->currentText() != m_originalTimingSource) {
        RosegardenSequencer::getInstance()->setCurrentTimer(
                m_sequencerTimingSource->currentText());
        // In case this is an Apply without exit, update the cache
        // so that we detect any further changes.
        m_originalTimingSource = m_sequencerTimingSource->currentText();
    }

    settings.setValue("sfxloadenabled", m_loadSoundFont->isChecked());

    settings.setValue("sfxloadpath", m_pathToLoadCommand->text());
    settings.setValue("soundfontpath", m_soundFont->text());

    Preferences::setSMFExportPPQN(m_ppqnSmfExport->value());

    // *** MIDI Sync tab

    // MIDI Clock and System messages
    const int midiClock = m_midiClock->currentIndex();
    settings.setValue("midiclock", midiClock);

    // Now send it (OLD METHOD - to be removed)
    // !!! No, don't remove -- this controls SPP as well doesn't it?
    MappedEvent midiClockEvent;
    midiClockEvent.setInstrumentId(MidiInstrumentBase);  // ??? needed?
    midiClockEvent.setType(MappedEvent::SystemMIDIClock);
    midiClockEvent.setData1(MidiByte(midiClock));
    StudioControl::sendMappedEvent(midiClockEvent);

    settings.setValue("mmcmode", m_midiMachineControlMode->currentIndex());
    MappedEvent mmcModeEvent;
    mmcModeEvent.setInstrumentId(MidiInstrumentBase);  // ??? needed?
    mmcModeEvent.setType(MappedEvent::SystemMMCTransport);
    mmcModeEvent.setData1(MidiByte(m_midiMachineControlMode->currentIndex()));
    StudioControl::sendMappedEvent(mmcModeEvent);

    settings.setValue("mtcmode", m_midiTimeCodeMode->currentIndex());
    MappedEvent mtcModeEvent;
    mtcModeEvent.setInstrumentId(MidiInstrumentBase);  // ??? needed?
    mtcModeEvent.setType(MappedEvent::SystemMTCTransport);
    mtcModeEvent.setData1(MidiByte(m_midiTimeCodeMode->currentIndex()));
    StudioControl::sendMappedEvent(mtcModeEvent);

    settings.setValue("midisyncautoconnect", m_autoConnectSyncOut->isChecked());
    MappedEvent autoConnectSyncOutEvent;
    autoConnectSyncOutEvent.setInstrumentId(MidiInstrumentBase);  // ??? needed?
    autoConnectSyncOutEvent.setType(MappedEvent::SystemMIDISyncAuto);
    autoConnectSyncOutEvent.setData1(MidiByte(m_autoConnectSyncOut->isChecked() ? 1 : 0));
    StudioControl::sendMappedEvent(autoConnectSyncOutEvent);

    settings.endGroup();


    // Update the metronome mapped segment with new clock ticks if needed.

    RosegardenDocument *doc = RosegardenDocument::currentDocument;
    Studio &studio = doc->getStudio();
    const MidiMetronome *metronome = studio.getMetronomeFromDevice(
            studio.getMetronomeDevice());

    if (metronome) {
        InstrumentId instrument = metronome->getInstrument();
        doc->getSequenceManager()->metronomeChanged(instrument, true);
    }

}


}
