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

#include "misc/Strings.h"
#include "sound/Midi.h"
#include "sound/SoundDriver.h"
#include "misc/ConfigGroups.h"
#include "base/MidiProgram.h"
#include "base/Studio.h"
#include "ConfigurationPage.h"
#include "document/RosegardenDocument.h"
#include "gui/dialogs/ShowSequencerStatusDialog.h"
#include "gui/seqmanager/SequenceManager.h"
#include "gui/application/RosegardenApplication.h"
#include "gui/studio/StudioControl.h"
#include "sound/MappedEvent.h"
#include "TabbedConfigurationPage.h"
#include "misc/Debug.h"
#include "gui/widgets/LineEdit.h"
#include "gui/widgets/FileDialog.h"

#include <QComboBox>
#include <QSettings>
#include <QCheckBox>
#include <QComboBox>
#include <QByteArray>
#include <QDataStream>
#include <QFrame>
#include <QLabel>
#include <QObject>
#include <QPushButton>
#include <QLayout>
#include <QSlider>
#include <QSpinBox>
#include <QString>
#include <QStringList>
#include <QTabWidget>
#include <QToolTip>
#include <QWidget>
#include <QHBoxLayout>
#include <QCheckBox>

namespace Rosegarden
{

MIDIConfigurationPage::MIDIConfigurationPage(
    RosegardenDocument *doc,
    QWidget *parent):
        TabbedConfigurationPage(parent),
        m_midiPitchOctave(nullptr)
{
//    RG_DEBUG << "MIDI CONFIGURATION PAGE CTOR";
    // set the document in the super class
    m_doc = doc;

    // ---------------- General tab ------------------
    //
    QFrame *frame = new QFrame(m_tabWidget);
    frame->setContentsMargins(10, 10, 10, 10);
    QGridLayout *layout = new QGridLayout(frame);
    layout->setSpacing(5);

    int row = 0;

    layout->setRowMinimumHeight(row, 15);
    ++row;

    QLabel *label = nullptr;

    QSettings settings;
    settings.beginGroup( GeneralOptionsConfigGroup );

    layout->addWidget(new QLabel(tr("Base octave number for MIDI pitch display"),
                                 frame), row, 0, 1, 2);
    
    m_midiPitchOctave = new QSpinBox(frame);
    connect(m_midiPitchOctave, SIGNAL(valueChanged(int)), this, SLOT(slotModified()));
    m_midiPitchOctave->setMaximum(10);
    m_midiPitchOctave->setMinimum( -10);
    m_midiPitchOctave->setValue( settings.value("midipitchoctave", -2).toInt() );
    layout->addWidget(m_midiPitchOctave, row, 2, row- row+1, 3- 2+1);
    ++row;

    layout->setRowMinimumHeight(row, 20);
    ++row;

    //### settings.beginGroup( GeneralOptionsConfigGroup );
    layout->addWidget(new QLabel(tr("Always use default studio when loading files"),
                                      frame), row, 0, 1, 2);

    m_studio = new QCheckBox(frame);
    connect(m_studio, &QCheckBox::stateChanged, this, &MIDIConfigurationPage::slotModified);
    m_studio->setChecked( qStrToBool( settings.value("alwaysusedefaultstudio", "false" ) ) );
    layout->addWidget(m_studio, row, 2);
    ++row;
    settings.endGroup();

    // Send Controllers
    //
    settings.beginGroup( SequencerOptionsConfigGroup );
    label = new QLabel(tr("Allow Reset All Controllers (CC 121)"), frame);

    QString resetTip = tr("Rosegarden can send a MIDI Reset All Controllers event when setting up a channel.");
    label->setToolTip(resetTip);
    layout->addWidget(label, row, 0, row- row+1, 1- 0+1);

    m_allowResetAllControllers = new QCheckBox(frame);
    connect(m_allowResetAllControllers, &QCheckBox::stateChanged, this, &MIDIConfigurationPage::slotModified);
    const bool sendResetAllControllers = qStrToBool( settings.value("allowresetallcontrollers", "true" ) ) ;
    m_allowResetAllControllers->setChecked(sendResetAllControllers);
    m_allowResetAllControllers->setToolTip(resetTip);
    layout->addWidget(m_allowResetAllControllers, row, 2);
    ++row;

    // Timer selection
    //

    //### settings.beginGroup( SequencerOptionsConfigGroup );
    label = new QLabel(tr("Sequencer timing source"), frame);
    layout->addWidget(label, row, 0, row- row+1, 1- 0+1);

    m_timer = new QComboBox(frame);
    connect(m_timer, SIGNAL(activated(int)), this, SLOT(slotModified()));
    layout->addWidget(m_timer, row, 2, row- row+1, 3-2+1);

    QStringList timers = m_doc->getTimers();
    m_origTimer = m_doc->getCurrentTimer();
    QString currentTimer = settings.value("timer", m_origTimer).toString();

    for (int i = 0; i < timers.size(); ++i) {
        m_timer->addItem(timers[i]);
        if (timers[i] == currentTimer)
            m_timer->setCurrentIndex(i);
    }

    ++row;

    layout->setRowMinimumHeight(row, 20);
    ++row;

    //### settings.beginGroup( SequencerOptionsConfigGroup );

    // SoundFont loading
    //
    QLabel* lbl = new QLabel(tr("Load SoundFont to SoundBlaster card at startup"), frame);
    QString tooltip = tr("Check this box to enable soundfont loading on EMU10K-based cards when Rosegarden is launched");
    lbl->setToolTip(tooltip);
    layout->addWidget(lbl, row, 0, row- row+1, 1- 0+1);

    m_sfxLoadEnabled = new QCheckBox(frame);
    connect(m_sfxLoadEnabled, &QCheckBox::stateChanged, this, &MIDIConfigurationPage::slotModified);
    layout->addWidget(m_sfxLoadEnabled, row, 2);
    m_sfxLoadEnabled->setToolTip(tooltip);
    ++row;

    layout->addWidget(new QLabel(tr("Path to 'asfxload' or 'sfxload' command"), frame), row, 0);
    m_sfxLoadPath = new LineEdit(settings.value("sfxloadpath", "/usr/bin/asfxload").toString() , frame);
    layout->addWidget(m_sfxLoadPath, row, 1, row- row+1, 2);
    m_sfxLoadChoose = new QPushButton(tr("Choose..."), frame);
    layout->addWidget(m_sfxLoadChoose, row, 3);
    ++row;

    layout->addWidget(new QLabel(tr("SoundFont"), frame), row, 0);
    m_soundFontPath = new LineEdit(settings.value("soundfontpath", "").toString() , frame);
    layout->addWidget(m_soundFontPath, row, 1, row- row+1, 2);
    m_soundFontChoose = new QPushButton(tr("Choose..."), frame);
    layout->addWidget(m_soundFontChoose, row, 3);
    ++row;

    bool sfxLoadEnabled = qStrToBool( settings.value("sfxloadenabled", "false" ) ) ;
    m_sfxLoadEnabled->setChecked(sfxLoadEnabled);
    if (!sfxLoadEnabled) {
        m_sfxLoadPath->setEnabled(false);
        m_sfxLoadChoose->setEnabled(false);
        m_soundFontPath->setEnabled(false);
        m_soundFontChoose->setEnabled(false);
    }

    connect(m_sfxLoadEnabled, &QAbstractButton::toggled,
            this, &MIDIConfigurationPage::slotSoundFontToggled);

    connect(m_sfxLoadChoose, &QAbstractButton::clicked,
            this, &MIDIConfigurationPage::slotSfxLoadPathChoose);

    connect(m_soundFontChoose, &QAbstractButton::clicked,
            this, &MIDIConfigurationPage::slotSoundFontChoose);

    layout->setRowStretch(row, 10);

    addTab(frame, tr("General"));

    //### settings.beginGroup( SequencerOptionsConfigGroup );

    //  -------------- Synchronisation tab -----------------
    //
    frame = new QFrame(m_tabWidget);
    frame->setContentsMargins(10, 10, 10, 10);
    layout = new QGridLayout(frame);
    layout->setSpacing(5);

    row = 0;

    layout->setRowMinimumHeight(row, 15);
    ++row;

    // MIDI Clock and System Realtime Messages
    //
    label = new QLabel(tr("MIDI Clock and System messages"), frame);
    layout->addWidget(label, row, 0);
    m_midiSync = new QComboBox(frame);
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
    label = new QLabel(tr("MIDI Machine Control mode"), frame);
    layout->addWidget(label, row, 0);

    m_mmcTransport = new QComboBox(frame);
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
    label = new QLabel(tr("MIDI Time Code mode"), frame);
    layout->addWidget(label, row, 0);

    m_mtcTransport = new QComboBox(frame);
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

    QWidget *hbox = new QWidget(frame);
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

    addTab(frame, tr("MIDI Sync"));

    settings.endGroup();
}


void
MIDIConfigurationPage::slotSoundFontToggled(bool isChecked)
{
    m_sfxLoadPath->setEnabled(isChecked);
    m_sfxLoadChoose->setEnabled(isChecked);
    m_soundFontPath->setEnabled(isChecked);
    m_soundFontChoose->setEnabled(isChecked);
}

void
MIDIConfigurationPage::slotSfxLoadPathChoose()
{
    QString path = FileDialog::getOpenFileName(this, tr("sfxload path"), QDir::currentPath() ); //":SFXLOAD"

    m_sfxLoadPath->setText(path);
}

void
MIDIConfigurationPage::slotSoundFontChoose()
{
    QString path = FileDialog::getOpenFileName(this, tr("Soundfont path"), QDir::currentPath(),
                   tr("Sound fonts") + " (*.sb *.sf2 *.SF2 *.SB)" + ";;" +
                   tr("All files") + " (*)" ); // ":SOUNDFONTS"

    m_soundFontPath->setText(path);
}

void
MIDIConfigurationPage::apply()
{
    RG_DEBUG << "MIDI CONFIGURATION PAGE SETTINGS APPLIED";
    QSettings settings;
    settings.beginGroup( SequencerOptionsConfigGroup );

    settings.setValue("allowresetallcontrollers",
                      m_allowResetAllControllers->isChecked());

    settings.setValue("sfxloadenabled", m_sfxLoadEnabled->isChecked());
    settings.setValue("sfxloadpath", m_sfxLoadPath->text());
    settings.setValue("soundfontpath", m_soundFontPath->text());

    settings.setValue("timer", m_timer->currentText());
    if (m_timer->currentText() != m_origTimer) {
        m_doc->setCurrentTimer(m_timer->currentText());
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

    int octave = m_midiPitchOctave->value();
    settings.setValue("midipitchoctave", octave);

    settings.endGroup();
}

}
