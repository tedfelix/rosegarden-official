/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2018 the Rosegarden development team.
 
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
#include "document/RosegardenDocument.h"
#include "sequencer/RosegardenSequencer.h"
#include "gui/seqmanager/SequenceManager.h"
#include "misc/Strings.h"   // For qStrToBool()...
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


MIDIConfigurationPage::MIDIConfigurationPage(RosegardenDocument *doc,
                                             QWidget *parent):
    TabbedConfigurationPage(parent),
    m_baseOctaveNumber(nullptr)
{
    // ??? Get the document directly instead.  Need to do that in
    //     TabbedConfigurationPage first, then in the derivers.
    m_doc = doc;


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
    m_useDefaultStudio->setChecked(qStrToBool(
            settings.value("alwaysusedefaultstudio", "false")));
    connect(m_useDefaultStudio, &QCheckBox::stateChanged,
            this, &MIDIConfigurationPage::slotModified);
    layout->addWidget(m_useDefaultStudio, row, 2);

    ++row;

    settings.endGroup();
    settings.beginGroup(SequencerOptionsConfigGroup);

    // Allow Reset All Controllers
    QLabel *label = new QLabel(tr("Allow Reset All Controllers (CC 121)"));
    QString toolTip = tr("Rosegarden can send a MIDI Reset All Controllers event when setting up a channel.");
    label->setToolTip(toolTip);
    layout->addWidget(label, row, 0, 1, 2);

    m_allowResetAllControllers = new QCheckBox;
    m_allowResetAllControllers->setToolTip(toolTip);
    const bool sendResetAllControllers =
            qStrToBool(settings.value("allowresetallcontrollers", "true"));
    m_allowResetAllControllers->setChecked(sendResetAllControllers);
    connect(m_allowResetAllControllers, &QCheckBox::stateChanged,
            this, &MIDIConfigurationPage::slotModified);
    layout->addWidget(m_allowResetAllControllers, row, 2);

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
        if (timer == m_originalTimingSource)
            m_sequencerTimingSource->setCurrentIndex(i);
    }

    connect(m_sequencerTimingSource, SIGNAL(activated(int)),
            this, SLOT(slotModified()));
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
    const bool sfxLoadEnabled =
            qStrToBool(settings.value("sfxloadenabled", "false"));
    m_loadSoundFont->setChecked(sfxLoadEnabled);
    connect(m_loadSoundFont, &QCheckBox::stateChanged,
            this, &MIDIConfigurationPage::slotModified);
    connect(m_loadSoundFont, &QAbstractButton::clicked,
            this, &MIDIConfigurationPage::slotSoundFontToggled);
    layout->addWidget(m_loadSoundFont, row, 2);

    ++row;

    // Path to 'asfxload'
    layout->addWidget(
            new QLabel(tr("Path to 'asfxload' or 'sfxload' command")),
            row, 0);

    m_pathToLoadCommand = new LineEdit(
            settings.value("sfxloadpath", "/usr/bin/asfxload").toString());
    m_pathToLoadCommand->setEnabled(sfxLoadEnabled);
    layout->addWidget(m_pathToLoadCommand, row, 1, 1, 2);

    m_pathToLoadChoose = new QPushButton(tr("Choose..."));
    m_pathToLoadChoose->setEnabled(sfxLoadEnabled);
    connect(m_pathToLoadChoose, &QAbstractButton::clicked,
            this, &MIDIConfigurationPage::slotSfxLoadPathChoose);
    layout->addWidget(m_pathToLoadChoose, row, 3);

    ++row;

    // SoundFont
    layout->addWidget(new QLabel(tr("SoundFont")), row, 0);

    m_soundFont = new LineEdit(
            settings.value("soundfontpath", "").toString());
    m_soundFont->setEnabled(sfxLoadEnabled);
    layout->addWidget(m_soundFont, row, 1, 1, 2);

    m_soundFontChoose = new QPushButton(tr("Choose..."));
    m_soundFontChoose->setEnabled(sfxLoadEnabled);
    connect(m_soundFontChoose, &QAbstractButton::clicked,
            this, &MIDIConfigurationPage::slotSoundFontChoose);
    layout->addWidget(m_soundFontChoose, row, 3);

    ++row;

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
    label = new QLabel(tr("MIDI Clock and System messages"), widget);
    layout->addWidget(label, row, 0);
    m_midiSync = new QComboBox(widget);
    connect(m_midiSync, SIGNAL(activated(int)), this, SLOT(slotModified()));
    layout->addWidget(m_midiSync, row, 1);

    m_midiSync->addItem(tr("Off"));
    m_midiSync->addItem(tr("Send MIDI Clock, Start and Stop"));
    m_midiSync->addItem(tr("Accept Start, Stop and Continue"));

    int midiClock = settings.value("midiclock", 0).toInt() ;
    if (midiClock < 0 || midiClock > 2)
        midiClock = 0;
    m_midiSync->setCurrentIndex(midiClock);

    ++row;

    // MMC Transport
    //
    label = new QLabel(tr("MIDI Machine Control mode"), widget);
    layout->addWidget(label, row, 0);

    m_mmcTransport = new QComboBox(widget);
    connect(m_mmcTransport, SIGNAL(activated(int)), this, SLOT(slotModified()));
    layout->addWidget(m_mmcTransport, row, 1); //, Qt::AlignHCenter);

    m_mmcTransport->addItem(tr("Off"));
    m_mmcTransport->addItem(tr("MMC Source"));
    m_mmcTransport->addItem(tr("MMC Follower"));

    int mmcMode = settings.value("mmcmode", 0).toInt() ;
    if (mmcMode < 0 || mmcMode > 2)
        mmcMode = 0;
    m_mmcTransport->setCurrentIndex(mmcMode);
    
    ++row;

    // MTC transport
    //
    label = new QLabel(tr("MIDI Time Code mode"), widget);
    layout->addWidget(label, row, 0);

    m_mtcTransport = new QComboBox(widget);
    connect(m_mtcTransport, SIGNAL(activated(int)), this, SLOT(slotModified()));
    layout->addWidget(m_mtcTransport, row, 1);

    m_mtcTransport->addItem(tr("Off"));
    m_mtcTransport->addItem(tr("MTC Source"));
    m_mtcTransport->addItem(tr("MTC Follower"));

    int mtcMode = settings.value("mtcmode", 0).toInt() ;
    if (mtcMode < 0 || mtcMode > 2)
        mtcMode = 0;
    m_mtcTransport->setCurrentIndex(mtcMode);

    ++row;

    QWidget *hbox = new QWidget(widget);
    QHBoxLayout *hboxLayout = new QHBoxLayout;
    hboxLayout->setSpacing(5);
    layout->addWidget(hbox, row, 0, row- row+1, 1- 0+1);

    label = new QLabel(tr("Automatically connect sync output to all devices in use"), hbox );
    hboxLayout->addWidget(label);
//    layout->addWidget(label, row, 0);
    m_midiSyncAuto = new QCheckBox( hbox );
    connect(m_midiSyncAuto, &QCheckBox::stateChanged, this, &MIDIConfigurationPage::slotModified);
    hboxLayout->addWidget(m_midiSyncAuto);
    hbox->setLayout(hboxLayout);
//    layout->addWidget(m_midiSyncAuto, row, 1);

    m_midiSyncAuto->setChecked( qStrToBool( settings.value("midisyncautoconnect", "false" ) ) );

    ++row;

    layout->setRowStretch(row, 10);

}

void
MIDIConfigurationPage::slotSoundFontToggled(bool isChecked)
{
    m_pathToLoadCommand->setEnabled(isChecked);
    m_pathToLoadChoose->setEnabled(isChecked);
    m_soundFont->setEnabled(isChecked);
    m_soundFontChoose->setEnabled(isChecked);
}

void
MIDIConfigurationPage::slotSfxLoadPathChoose()
{
    QString path = FileDialog::getOpenFileName(this, tr("sfxload path"), QDir::currentPath() ); //":SFXLOAD"

    // ??? Need to handle Cancel properly!

    m_pathToLoadCommand->setText(path);
}

void
MIDIConfigurationPage::slotSoundFontChoose()
{
    QString path = FileDialog::getOpenFileName(this, tr("Soundfont path"), QDir::currentPath(),
                   tr("Sound fonts") + " (*.sb *.sf2 *.SF2 *.SB)" + ";;" +
                   tr("All files") + " (*)" ); // ":SOUNDFONTS"

    // ??? Need to handle Cancel properly!

    m_soundFont->setText(path);
}

void
MIDIConfigurationPage::apply()
{
    RG_DEBUG << "MIDI CONFIGURATION PAGE SETTINGS APPLIED";

    QSettings settings;
    settings.beginGroup( SequencerOptionsConfigGroup );

    settings.setValue("allowresetallcontrollers",
                      m_allowResetAllControllers->isChecked());

    settings.setValue("sfxloadenabled", m_loadSoundFont->isChecked());
    settings.setValue("sfxloadpath", m_pathToLoadCommand->text());
    settings.setValue("soundfontpath", m_soundFont->text());

    // If the timer setting has actually changed
    if (m_sequencerTimingSource->currentText() != m_originalTimingSource) {
        RosegardenSequencer::getInstance()->setCurrentTimer(
                m_sequencerTimingSource->currentText());
        // In case this is an Apply without exit.
        m_originalTimingSource = m_sequencerTimingSource->currentText();
    }

    // Write the entries
    //
    settings.setValue("mmcmode", m_mmcTransport->currentIndex());
    settings.setValue("mtcmode", m_mtcTransport->currentIndex());
    settings.setValue("midisyncautoconnect", m_midiSyncAuto->isChecked());

    // Now send
    //
    MappedEvent mEmccValue(MidiInstrumentBase,  // InstrumentId
                           MappedEvent::SystemMMCTransport,
                           MidiByte(m_mmcTransport->currentIndex()));

    StudioControl::sendMappedEvent(mEmccValue);

    MappedEvent mEmtcValue(MidiInstrumentBase,  // InstrumentId
                           MappedEvent::SystemMTCTransport,
                           MidiByte(m_mtcTransport->currentIndex()));

    StudioControl::sendMappedEvent(mEmtcValue);

    MappedEvent mEmsaValue(MidiInstrumentBase,  // InstrumentId
                           MappedEvent::SystemMIDISyncAuto,
                           MidiByte(m_midiSyncAuto->isChecked() ? 1 : 0));

    StudioControl::sendMappedEvent(mEmsaValue);


    // ------------- MIDI Clock and System messages ------------
    //
    int midiClock = m_midiSync->currentIndex();
    settings.setValue("midiclock", midiClock);

    // Now send it (OLD METHOD - to be removed)
    //!!! No, don't remove -- this controls SPP as well doesn't it?
    //
    MappedEvent mEMIDIClock(MidiInstrumentBase,  // InstrumentId
                            MappedEvent::SystemMIDIClock,
                            MidiByte(midiClock));

    StudioControl::sendMappedEvent(mEMIDIClock);


    // Now update the metronome mapped segment with new clock ticks
    // if needed.
    //
    Studio &studio = m_doc->getStudio();
    const MidiMetronome *metronome = studio.
                                     getMetronomeFromDevice(studio.getMetronomeDevice());

    if (metronome) {
        InstrumentId instrument = metronome->getInstrument();
        m_doc->getSequenceManager()->metronomeChanged(instrument, true);
    }
    settings.endGroup();
    settings.beginGroup( GeneralOptionsConfigGroup );

    bool deftstudio = getUseDefaultStudio();
    settings.setValue("alwaysusedefaultstudio", deftstudio);

    int octave = m_baseOctaveNumber->value();
    settings.setValue("midipitchoctave", octave);

    settings.endGroup();
}

}
