/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
 
    This program is Copyright 2000-2007
        Guillaume Laurent   <glaurent@telegraph-road.org>,
        Chris Cannam        <cannam@all-day-breakfast.com>,
        Richard Bown        <richard.bown@ferventsoftware.com>
 
    The moral rights of Guillaume Laurent, Chris Cannam, and Richard
    Bown to claim authorship of this work have been asserted.
 
    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.
 
    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/


#include "MIDIConfigurationPage.h"

#include "sound/Midi.h"
#include "sound/SoundDriver.h"
#include "document/ConfigGroups.h"
#include "base/MidiProgram.h"
#include "base/Studio.h"
#include "ConfigurationPage.h"
#include "document/RosegardenGUIDoc.h"
#include "gui/dialogs/ShowSequencerStatusDialog.h"
#include "gui/seqmanager/SequenceManager.h"
#include "gui/application/RosegardenApplication.h"
#include "gui/studio/StudioControl.h"
#include "sound/MappedEvent.h"
#include "TabbedConfigurationPage.h"
#include <kcombobox.h>
#include <kconfig.h>
#include <kfiledialog.h>
#include <qcheckbox.h>
#include <qcombobox.h>
#include <qcstring.h>
#include <qdatastream.h>
#include <qframe.h>
#include <qlabel.h>
#include <qlineedit.h>
#include <qobject.h>
#include <qpushbutton.h>
#include <qlayout.h>
#include <qslider.h>
#include <qspinbox.h>
#include <qstring.h>
#include <qstringlist.h>
#include <qtabwidget.h>
#include <qtooltip.h>
#include <qwidget.h>
#include <qcheckbox.h>
#include <qhbox.h>

namespace Rosegarden
{

MIDIConfigurationPage::MIDIConfigurationPage(
    RosegardenGUIDoc *doc,
    KConfig *cfg,
    QWidget *parent,
    const char *name):
        TabbedConfigurationPage(cfg, parent, name),
        m_midiPitchOctave(0)
{
    // set the document in the super class
    m_doc = doc;

    // ---------------- General tab ------------------
    //
    QFrame *frame = new QFrame(m_tabWidget);
    QGridLayout *layout = new QGridLayout(frame, 9, 4, 10, 5);

    int row = 0;

    layout->setRowSpacing(row, 15);
    ++row;

    QLabel *label = 0;

    m_cfg->setGroup(GeneralOptionsConfigGroup);

    layout->addMultiCellWidget(new QLabel(i18n("Base octave number for MIDI pitch display"),
					  frame), row, row, 0, 1);
    
    m_midiPitchOctave = new QSpinBox(frame);
    m_midiPitchOctave->setMaxValue(10);
    m_midiPitchOctave->setMinValue( -10);
    m_midiPitchOctave->setValue(m_cfg->readNumEntry("midipitchoctave", -2));
    layout->addMultiCellWidget(m_midiPitchOctave, row, row, 2, 3);
    ++row;

    layout->setRowSpacing(row, 20);
    ++row;

    m_cfg->setGroup(GeneralOptionsConfigGroup);

    layout->addMultiCellWidget(new QLabel(i18n("Always use default studio when loading files"),
					  frame), row, row, 0, 1);

    m_studio = new QCheckBox(frame);
    m_studio->setChecked(m_cfg->readBoolEntry("alwaysusedefaultstudio", false));
    layout->addWidget(m_studio, row, 2);
    ++row;

    // Send Controllers
    //
    m_cfg->setGroup(SequencerOptionsConfigGroup);

    label = new QLabel(i18n("Send all MIDI Controllers at start of each playback"), frame);

    QString controllerTip = i18n("Rosegarden can send all MIDI Controllers (Pan, Reverb etc) to all MIDI devices every\ntime you hit play if you so wish.  Please note that this option will usually incur a\ndelay at the start of playback due to the amount of data being transmitted.");
    QToolTip::add
        (label, controllerTip);
    layout->addMultiCellWidget(label, row, row, 0, 1);

    m_sendControllersAtPlay = new QCheckBox(frame);
    bool sendControllers = m_cfg->readBoolEntry("alwayssendcontrollers", false);
    m_sendControllersAtPlay->setChecked(sendControllers);
    QToolTip::add
        (m_sendControllersAtPlay, controllerTip);
    layout->addWidget(m_sendControllersAtPlay, row, 2);
    ++row;

    // Timer selection
    //
    m_cfg->setGroup(SequencerOptionsConfigGroup);

    label = new QLabel(i18n("Sequencer timing source"), frame);
    layout->addMultiCellWidget(label, row, row, 0, 1);

    m_timer = new KComboBox(frame);
    layout->addMultiCellWidget(m_timer, row, row, 2, 3);

    QStringList timers = m_doc->getTimers();
    m_origTimer = m_doc->getCurrentTimer();
    QString currentTimer = m_cfg->readEntry("timer", m_origTimer);

    for (unsigned int i = 0; i < timers.size(); ++i) {
        m_timer->insertItem(timers[i]);
        if (timers[i] == currentTimer)
            m_timer->setCurrentItem(i);
    }

    ++row;

    layout->setRowSpacing(row, 20);
    ++row;

    m_cfg->setGroup(SequencerOptionsConfigGroup);

    // SoundFont loading
    //
    QLabel* lbl = new QLabel(i18n("Load SoundFont to SoundBlaster card at startup"), frame);
    QString tooltip = i18n("Check this box to enable soundfont loading on EMU10K-based cards when Rosegarden is launched");
    QToolTip::add(lbl, tooltip);
    layout->addMultiCellWidget(lbl, row, row, 0, 1);

    m_sfxLoadEnabled = new QCheckBox(frame);
    layout->addWidget(m_sfxLoadEnabled, row, 2);
    QToolTip::add(m_sfxLoadEnabled, tooltip);
    ++row;

    layout->addWidget(new QLabel(i18n("Path to 'asfxload' or 'sfxload' command"), frame), row, 0);
    m_sfxLoadPath = new QLineEdit(m_cfg->readEntry("sfxloadpath", "/bin/sfxload"), frame);
    layout->addMultiCellWidget(m_sfxLoadPath, row, row, 1, 2);
    m_sfxLoadChoose = new QPushButton("Choose...", frame);
    layout->addWidget(m_sfxLoadChoose, row, 3);
    ++row;

    layout->addWidget(new QLabel(i18n("SoundFont"), frame), row, 0);
    m_soundFontPath = new QLineEdit(m_cfg->readEntry("soundfontpath", ""), frame);
    layout->addMultiCellWidget(m_soundFontPath, row, row, 1, 2);
    m_soundFontChoose = new QPushButton("Choose...", frame);
    layout->addWidget(m_soundFontChoose, row, 3);
    ++row;

    bool sfxLoadEnabled = m_cfg->readBoolEntry("sfxloadenabled", false);
    m_sfxLoadEnabled->setChecked(sfxLoadEnabled);
    if (!sfxLoadEnabled) {
        m_sfxLoadPath->setEnabled(false);
        m_sfxLoadChoose->setEnabled(false);
        m_soundFontPath->setEnabled(false);
        m_soundFontChoose->setEnabled(false);
    }

    connect(m_sfxLoadEnabled, SIGNAL(toggled(bool)),
            this, SLOT(slotSoundFontToggled(bool)));

    connect(m_sfxLoadChoose, SIGNAL(clicked()),
            this, SLOT(slotSfxLoadPathChoose()));

    connect(m_soundFontChoose, SIGNAL(clicked()),
            this, SLOT(slotSoundFontChoose()));

    layout->setRowStretch(row, 10);

    addTab(frame, i18n("General"));

    m_cfg->setGroup(SequencerOptionsConfigGroup);

    //  -------------- Synchronisation tab -----------------
    //
    frame = new QFrame(m_tabWidget);
    layout = new QGridLayout(frame, 7, 2, 10, 5);

    row = 0;

    layout->setRowSpacing(row, 15);
    ++row;

    // MIDI Clock and System Realtime Messages
    //
    label = new QLabel(i18n("MIDI Clock and System messages"), frame);
    layout->addWidget(label, row, 0);
    m_midiSync = new KComboBox(frame);
    layout->addWidget(m_midiSync, row, 1);

    m_midiSync->insertItem(i18n("Off"));
    m_midiSync->insertItem(i18n("Send MIDI Clock, Start and Stop"));
    m_midiSync->insertItem(i18n("Accept Start, Stop and Continue"));

    int midiClock = m_cfg->readNumEntry("midiclock", 0);
    if (midiClock < 0 || midiClock > 2)
        midiClock = 0;
    m_midiSync->setCurrentItem(midiClock);

    ++row;

    // MMC Transport
    //
    label = new QLabel(i18n("MIDI Machine Control mode"), frame);
    layout->addWidget(label, row, 0);

    m_mmcTransport = new KComboBox(frame);
    layout->addWidget(m_mmcTransport, row, 1); //, Qt::AlignHCenter);

    m_mmcTransport->insertItem(i18n("Off"));
    m_mmcTransport->insertItem(i18n("MMC Master"));
    m_mmcTransport->insertItem(i18n("MMC Slave"));

    int mmcMode = m_cfg->readNumEntry("mmcmode", 0);
    if (mmcMode < 0 || mmcMode > 2)
        mmcMode = 0;
    m_mmcTransport->setCurrentItem(mmcMode);
    
    ++row;

    // MTC transport
    //
    label = new QLabel(i18n("MIDI Time Code mode"), frame);
    layout->addWidget(label, row, 0);

    m_mtcTransport = new KComboBox(frame);
    layout->addWidget(m_mtcTransport, row, 1);

    m_mtcTransport->insertItem(i18n("Off"));
    m_mtcTransport->insertItem(i18n("MTC Master"));
    m_mtcTransport->insertItem(i18n("MTC Slave"));

    int mtcMode = m_cfg->readNumEntry("mtcmode", 0);
    if (mtcMode < 0 || mtcMode > 2)
        mtcMode = 0;
    m_mtcTransport->setCurrentItem(mtcMode);

    ++row;

    QHBox *hbox = new QHBox(frame);
    hbox->setSpacing(5);
    layout->addMultiCellWidget(hbox, row, row, 0, 1);

    label = new QLabel(i18n("Automatically connect sync output to all devices in use"), hbox);
//    layout->addWidget(label, row, 0);
    m_midiSyncAuto = new QCheckBox(hbox);
//    layout->addWidget(m_midiSyncAuto, row, 1);

    m_midiSyncAuto->setChecked(m_cfg->readBoolEntry("midisyncautoconnect", false));

    ++row;

    layout->setRowStretch(row, 10);

    addTab(frame, i18n("MIDI Sync"));
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
    QString path = KFileDialog::getOpenFileName(":SFXLOAD", QString::null, this, i18n("sfxload path"));
    m_sfxLoadPath->setText(path);
}

void
MIDIConfigurationPage::slotSoundFontChoose()
{
    QString path = KFileDialog::getOpenFileName(":SOUNDFONTS", "*.sb *.sf2 *.SF2 *.SB", this, i18n("Soundfont path"));
    m_soundFontPath->setText(path);
}

void
MIDIConfigurationPage::apply()
{
    m_cfg->setGroup(SequencerOptionsConfigGroup);

    m_cfg->writeEntry("alwayssendcontrollers",
                      m_sendControllersAtPlay->isChecked());

    m_cfg->writeEntry("sfxloadenabled", m_sfxLoadEnabled->isChecked());
    m_cfg->writeEntry("sfxloadpath", m_sfxLoadPath->text());
    m_cfg->writeEntry("soundfontpath", m_soundFontPath->text());

    m_cfg->writeEntry("timer", m_timer->currentText());
    if (m_timer->currentText() != m_origTimer) {
        m_doc->setCurrentTimer(m_timer->currentText());
    }

    // Write the entries
    //
    m_cfg->writeEntry("mmcmode", m_mmcTransport->currentItem());
    m_cfg->writeEntry("mtcmode", m_mtcTransport->currentItem());
    m_cfg->writeEntry("midisyncautoconnect", m_midiSyncAuto->isChecked());

    // Now send
    //
    MappedEvent mEmccValue(MidiInstrumentBase,  // InstrumentId
                           MappedEvent::SystemMMCTransport,
                           MidiByte(m_mmcTransport->currentItem()));

    StudioControl::sendMappedEvent(mEmccValue);

    MappedEvent mEmtcValue(MidiInstrumentBase,  // InstrumentId
                           MappedEvent::SystemMTCTransport,
                           MidiByte(m_mtcTransport->currentItem()));

    StudioControl::sendMappedEvent(mEmtcValue);

    MappedEvent mEmsaValue(MidiInstrumentBase,  // InstrumentId
                           MappedEvent::SystemMIDISyncAuto,
                           MidiByte(m_midiSyncAuto->isChecked() ? 1 : 0));

    StudioControl::sendMappedEvent(mEmsaValue);


    // ------------- MIDI Clock and System messages ------------
    //
    int midiClock = m_midiSync->currentItem();
    m_cfg->writeEntry("midiclock", midiClock);

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

    m_cfg->setGroup(GeneralOptionsConfigGroup);

    bool deftstudio = getUseDefaultStudio();
    m_cfg->writeEntry("alwaysusedefaultstudio", deftstudio);

    int octave = m_midiPitchOctave->value();
    m_cfg->writeEntry("midipitchoctave", octave);
}

}
#include "MIDIConfigurationPage.moc"
