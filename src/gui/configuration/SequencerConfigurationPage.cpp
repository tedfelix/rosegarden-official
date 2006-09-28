/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
 
    This program is Copyright 2000-2006
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


#include "SequencerConfigurationPage.h"

#include "sound/Midi.h"
#include "document/ConfigGroups.h"
#include "base/MidiProgram.h"
#include "base/Studio.h"
#include "ConfigurationPage.h"
#include "document/RosegardenGUIDoc.h"
#include "gui/dialogs/ShowSequencerStatusDialog.h"
#include "gui/seqmanager/SequenceManager.h"
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
#include <qslider.h>
#include <qspinbox.h>
#include <qstring.h>
#include <qstringlist.h>
#include <qtabwidget.h>
#include <qtooltip.h>
#include <qwidget.h>


namespace Rosegarden
{

SequencerConfigurationPage::SequencerConfigurationPage(
    RosegardenGUIDoc *doc,
    KConfig *cfg,
    QWidget *parent,
    const char *name):
        TabbedConfigurationPage(cfg, parent, name)
{
    // set the document in the super class
    m_doc = doc;

    m_cfg->setGroup(SequencerOptionsConfigGroup);

    // ---------------- General tab ------------------
    //
    QFrame *frame = new QFrame(m_tabWidget);
    QGridLayout *layout = new QGridLayout(frame, 6, 4, 10, 5);

    layout->addWidget(new QLabel(i18n("Sequencer status"), frame), 0, 0);

    QString status(i18n("Unknown"));
    SequenceManager *mgr = doc->getSequenceManager();
    if (mgr) {
        int driverStatus = mgr->getSoundDriverStatus() & (AUDIO_OK | MIDI_OK);
        switch (driverStatus) {
        case AUDIO_OK:
            status = i18n("No MIDI, audio OK");
            break;
        case MIDI_OK:
            status = i18n("MIDI OK, no audio");
            break;
        case AUDIO_OK | MIDI_OK:
            status = i18n("MIDI OK, audio OK");
            break;
        default:
            status = i18n("No driver");
            break;
        }
    }

    layout->addWidget(new QLabel(status, frame), 0, 1);

    QPushButton *showStatusButton = new QPushButton(i18n("Show detailed status"),
                                    frame);
    QObject::connect(showStatusButton, SIGNAL(clicked()),
                     this, SLOT(slotShowStatus()));
    layout->addWidget(showStatusButton, 0, 2);

    // Send Controllers
    //
    QLabel *label = new QLabel(i18n("Send all MIDI Controllers at start of playback\n     (will incur noticeable delay)"), frame);

    QString controllerTip = i18n("Rosegarden can send all MIDI Controllers (Pan, Reverb etc) to all MIDI devices every\ntime you hit play if you so wish.  Please note that this option will usually incur a\ndelay at the start of playback due to the amount of data being transmitted.");
    QToolTip::add
        (label, controllerTip);
    layout->addWidget(label, 1, 0);

    m_sendControllersAtPlay = new QCheckBox(frame);
    bool sendControllers = m_cfg->readBoolEntry("alwayssendcontrollers", false);
    m_sendControllersAtPlay->setChecked(sendControllers);
    QToolTip::add
        (m_sendControllersAtPlay, controllerTip);
    layout->addMultiCellWidget(m_sendControllersAtPlay, 1, 1, 1, 2);

    // Command-line
    //
    /*
        layout->addWidget(new QLabel(i18n("Sequencer command line options\n     (takes effect only from next restart)"), frame), 2, 0);
     
        m_sequencerArguments = new QLineEdit("", frame);
        layout->addMultiCellWidget(m_sequencerArguments, 2, 2, 1, 2);
     
        // Get the options
        //
        QString commandLine = m_cfg->readEntry("commandlineoptions", "");
        m_sequencerArguments->setText(commandLine);
    */ 
    // SoundFont loading
    //
    QLabel* lbl = new QLabel(i18n("Load SoundFont to SoundBlaster card at startup"), frame);
    QString tooltip = i18n("Check this box to enable soundfont loading on EMU10K-based cards when Rosegarden is launched");
    QToolTip::add
        (lbl, tooltip);
    layout->addWidget(lbl, 2, 0);

    m_sfxLoadEnabled = new QCheckBox(frame);
    layout->addMultiCellWidget(m_sfxLoadEnabled, 2, 2, 1, 2);
    QToolTip::add
        (m_sfxLoadEnabled, tooltip);

    layout->addWidget(new QLabel(i18n("Path to 'asfxload' or 'sfxload' command"), frame), 3, 0);
    m_sfxLoadPath = new QLineEdit(m_cfg->readEntry("sfxloadpath", "/bin/sfxload"), frame);
    layout->addMultiCellWidget(m_sfxLoadPath, 3, 3, 1, 2);
    m_sfxLoadChoose = new QPushButton("...", frame);
    layout->addWidget(m_sfxLoadChoose, 3, 3);

    layout->addWidget(new QLabel(i18n("SoundFont"), frame), 4, 0);
    m_soundFontPath = new QLineEdit(m_cfg->readEntry("soundfontpath", ""), frame);
    layout->addMultiCellWidget(m_soundFontPath, 4, 4, 1, 2);
    m_soundFontChoose = new QPushButton("...", frame);
    layout->addWidget(m_soundFontChoose, 4, 3);

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


    addTab(frame, i18n("General"));


    // --------------------- Startup control ----------------------
    //
#ifdef HAVE_LIBJACK
#define OFFER_JACK_START_OPTION 1
#ifdef OFFER_JACK_START_OPTION

    frame = new QFrame(m_tabWidget);
    layout = new QGridLayout(frame, 8, 4, 10, 5);

    label = new QLabel(i18n("Rosegarden can start the JACK audio daemon (jackd) for you\nautomatically if it isn't already running when Rosegarden starts.\n\nThis is recommended for beginners and those who use Rosegarden as their main\naudio application, but it might not be to the liking of advanced users.\n\nIf you want to start JACK automatically, make sure the command includes a full\npath where necessary as well as any command-line arguments you want to use.\n\nFor example: /usr/local/bin/jackd -d alsa -d hw -r44100 -p 2048 -n 2\n"), frame);

    layout->addMultiCellWidget(label, 1, 1, 0, 3);

    // JACK control things
    //
    bool startJack = m_cfg->readBoolEntry("jackstart", false);
    m_startJack = new QCheckBox(frame);
    m_startJack->setChecked(startJack);

    connect(m_startJack, SIGNAL(released()),
            this, SLOT(slotJackToggled()));

    layout->addWidget(new QLabel(i18n("Start JACK when Rosegarden starts"), frame), 2, 0);

    layout->addWidget(m_startJack, 2, 1);

    layout->addWidget(new QLabel(i18n("JACK command (including path as necessary)"), frame),
                      3, 0);

    QString jackPath = m_cfg->readEntry("jackcommand",
                                        //                                        "/usr/local/bin/jackd -d alsa -d hw -r 44100 -p 2048 -n 2");
                                        "/usr/bin/qjackctl -s");
    m_jackPath = new QLineEdit(jackPath, frame);

    layout->addMultiCellWidget(m_jackPath, 3, 3, 1, 3);

    // set the initial state
    slotJackToggled();

    addTab(frame, i18n("Startup"));

#endif // OFFER_JACK_START_OPTION
#endif // HAVE_LIBJACK

    frame = new QFrame(m_tabWidget);
    layout = new QGridLayout(frame, 10, 3,
                             10, 5);

    // Fetch the sample rate for showing conversions between time and
    // memory usage

    QCString replyType;
    QByteArray replyData;
    m_sampleRate = 0;

    if (rgapp->sequencerCall("getSampleRate()", replyType, replyData)) {
        QDataStream streamIn(replyData, IO_ReadOnly);
        unsigned int result;
        streamIn >> result;
        m_sampleRate = result;
    }

    layout->addMultiCellWidget(new QLabel(i18n("Longer buffers usually improve playback quality, but use more memory and slow response."), frame),
                               0, 0,
                               0, 3);

    layout->addWidget(new QLabel(i18n("Event read-ahead"), frame), 1, 0);
    layout->addWidget(new QLabel(i18n("Audio mix buffer"), frame), 3, 0);
    layout->addWidget(new QLabel(i18n("Audio file read buffer"), frame), 5, 0);
    layout->addWidget(new QLabel(i18n("Audio file write buffer"), frame), 7, 0);
    layout->addWidget(new QLabel(i18n("Per-file limit for cacheable audio files"), frame), 9, 0);

    // Each tick doubles the value, though let's round down to 2sf.
    // Appropriate ranges are:
    //
    // Event read-ahead: 20ms -> 5sec (9 ticks)
    // Audio mix: 10ms -> 2.5sec, always shorter than event read-ahead (9 ticks)
    // Audio file read: 10ms -> 2.5sec (9 ticks)
    // Audio file write: 100ms -> 50sec (10 ticks)

    m_readAhead = new QSlider(Horizontal, frame);

    m_readAhead->setMinValue(1);
    m_readAhead->setMaxValue(9);
    m_readAhead->setLineStep(1);
    m_readAhead->setPageStep(1);
    //    m_readAhead->setTickmarks(QSlider::Below);

    m_readAheadLabel = new QLabel(frame);

    layout->addWidget(new QLabel(i18n("20 msec"), frame), 1, 1);
    layout->addWidget(new QLabel(i18n("5 sec"), frame), 1, 3);
    layout->addWidget(m_readAhead, 1, 2);
    layout->addWidget(m_readAheadLabel, 2, 2, Qt::AlignHCenter);

    int readAheadValue =
        m_cfg->readLongNumEntry("readaheadsec", 0) * 1000 +
        m_cfg->readLongNumEntry("readaheadusec", 80000) / 1000;
    updateTimeSlider(readAheadValue, 1, 9, 10, m_readAhead, m_readAheadLabel,
                     0);

    connect(m_readAhead,
            SIGNAL(valueChanged(int)),
            SLOT(slotReadAheadChanged(int)));

    m_audioMix = new QSlider(Horizontal, frame);

    m_audioMix->setMinValue(0);
    m_audioMix->setMaxValue(8);
    m_audioMix->setLineStep(1);
    m_audioMix->setPageStep(1);
    //    m_audioMix->setTickmarks(QSlider::Below);

    m_audioMixLabel = new QLabel(frame);

    layout->addWidget(new QLabel("10 msec", frame), 3, 1);
    layout->addWidget(new QLabel("2.5 sec", frame), 3, 3);
    layout->addWidget(m_audioMix, 3, 2);
    layout->addWidget(m_audioMixLabel, 4, 2, Qt::AlignHCenter);

    int audioMixValue =
        m_cfg->readLongNumEntry("audiomixsec", 0) * 1000 +
        m_cfg->readLongNumEntry("audiomixusec", 60000) / 1000;
    updateTimeSlider(audioMixValue, 0, 8, 10, m_audioMix, m_audioMixLabel,
                     i18n("per audio instrument"));

    connect(m_audioMix,
            SIGNAL(valueChanged(int)),
            SLOT(slotAudioMixChanged(int)));

    m_audioRead = new QSlider(Horizontal, frame);

    m_audioRead->setMinValue(1);
    m_audioRead->setMaxValue(9);
    m_audioRead->setLineStep(1);
    m_audioRead->setPageStep(1);
    //    m_audioRead->setTickmarks(QSlider::Below);

    m_audioReadLabel = new QLabel(frame);

    layout->addWidget(new QLabel("20 msec", frame), 5, 1);
    layout->addWidget(new QLabel("5 sec", frame), 5, 3);
    layout->addWidget(m_audioRead, 5, 2);
    layout->addWidget(m_audioReadLabel, 6, 2, Qt::AlignHCenter);

    int audioReadValue =
        m_cfg->readLongNumEntry("audioreadsec", 0) * 1000 +
        m_cfg->readLongNumEntry("audioreadusec", 80000) / 1000;
    updateTimeSlider(audioReadValue, 1, 9, 10, m_audioRead, m_audioReadLabel,
                     i18n("per file"));

    connect(m_audioRead,
            SIGNAL(valueChanged(int)),
            SLOT(slotAudioReadChanged(int)));


    m_audioWrite = new QSlider(Horizontal, frame);

    m_audioWrite->setMinValue(0);
    m_audioWrite->setMaxValue(9);
    m_audioWrite->setLineStep(1);
    m_audioWrite->setPageStep(1);
    //    m_audioWrite->setTickmarks(QSlider::Below);

    m_audioWriteLabel = new QLabel(frame);

    layout->addWidget(new QLabel("100 msec", frame), 7, 1);
    layout->addWidget(new QLabel("50 sec", frame), 7, 3);
    layout->addWidget(m_audioWrite, 7, 2);
    layout->addWidget(m_audioWriteLabel, 8, 2, Qt::AlignHCenter);

    int audioWriteValue =
        m_cfg->readLongNumEntry("audiowritesec", 0) * 1000 +
        m_cfg->readLongNumEntry("audiowriteusec", 200000) / 1000;
    updateTimeSlider(audioWriteValue, 0, 9, 100, m_audioWrite, m_audioWriteLabel, "");

    connect(m_audioWrite,
            SIGNAL(valueChanged(int)),
            SLOT(slotAudioWriteChanged(int)));


    m_smallFile = new QSlider(Horizontal, frame);

    m_smallFile->setMinValue(5);
    m_smallFile->setMaxValue(15);
    m_smallFile->setLineStep(1);
    m_smallFile->setPageStep(1);

    int smallFileValue = m_cfg->readLongNumEntry("smallaudiofilekbytes", 128);
    int powerOfTwo = 1;
    while (1 << powerOfTwo < smallFileValue)
        ++powerOfTwo;
    m_smallFile->setValue(powerOfTwo);
    //    m_smallFile->setTickmarks(QSlider::Below);

    if (smallFileValue < 1024) {
        m_smallFileLabel = new QLabel(QString("%1KB").arg(smallFileValue),
                                      frame);
    } else {
        m_smallFileLabel = new QLabel(QString("%1MB").arg(smallFileValue / 1024),
                                      frame);
    }

    layout->addWidget(new QLabel(i18n("32KB"), frame), 9, 1);
    layout->addWidget(new QLabel(i18n("32MB"), frame), 9, 3);
    layout->addWidget(m_smallFile, 9, 2);
    layout->addWidget(m_smallFileLabel, 10, 2, Qt::AlignHCenter);

    connect(m_smallFile,
            SIGNAL(valueChanged(int)),
            SLOT(slotSmallFileChanged(int)));

    frame->hide(); //!!!
    /*
        addTab(frame, i18n("Buffers"));
    */

    // ------------------ Record tab ---------------------
    //
    frame = new QFrame(m_tabWidget);
    layout = new QGridLayout(frame, 6, 2, 10, 5);

    int increment = 0;

#ifdef HAVE_LIBJACK

    label = new QLabel(i18n("Audio mix and monitor mode:"), frame);
    m_lowLatencyMode = new KComboBox(frame);
    m_lowLatencyMode->insertItem(i18n("Low latency"));
    m_lowLatencyMode->insertItem(i18n("Buffered"));
    m_lowLatencyMode->setCurrentItem(m_cfg->readBoolEntry("audiolowlatencymonitoring", true) ? 0 : 1);
    layout->addWidget(label, 0, 0);
    layout->addWidget(m_lowLatencyMode, 0, 1);
    ++increment;

    label = new QLabel(i18n("Create post-fader outputs for audio instruments"), frame);
    m_createFaderOuts = new QCheckBox(frame);
    m_createFaderOuts->setChecked(m_cfg->readBoolEntry("audiofaderouts", false));

    layout->addWidget(label, 1, 0);
    layout->addWidget(m_createFaderOuts, 1, 1);
    ++increment;

    label = new QLabel(i18n("Create post-fader outputs for submasters"), frame);
    m_createSubmasterOuts = new QCheckBox(frame);
    m_createSubmasterOuts->setChecked(m_cfg->readBoolEntry("audiosubmasterouts",
                                      false));

    layout->addWidget(label, 2, 0);
    layout->addWidget(m_createSubmasterOuts, 2, 1);
    ++increment;

    label = new QLabel(i18n("Record audio files as:"), frame);
    m_audioRecFormat = new KComboBox(frame);
    m_audioRecFormat->insertItem(i18n("16-bit PCM WAV format (smaller files)"));
    m_audioRecFormat->insertItem(i18n("32-bit float WAV format (higher quality)"));
    m_audioRecFormat->setCurrentItem(m_cfg->readUnsignedNumEntry("audiorecordfileformat", 1));
    layout->addWidget(label, 3, 0);
    layout->addWidget(m_audioRecFormat, 3, 1);
    ++increment;

#endif // HAVE_LIBJACK

    /*  #1045380 ("minutes of audio recording" just insanely confusing)
        Remove this option.
     
        label = new QLabel(i18n("Minutes of audio recording:"), frame);
        m_audioRecordMinutes = new QSpinBox(frame);
     
        layout->addWidget(label,                0 + increment, 0);
        layout->addWidget(m_audioRecordMinutes, 0 + increment, 1);
     
        int audioRecordMinutes = m_cfg->readNumEntry("audiorecordminutes", 5);
     
        m_audioRecordMinutes->setValue(audioRecordMinutes);
        m_audioRecordMinutes->setMinValue(1);
        m_audioRecordMinutes->setMaxValue(60);
    */
    addTab(frame, i18n("Record and Mix"));

    //  -------------- Synchronisation tab -----------------
    //
    frame = new QFrame(m_tabWidget);
    layout = new QGridLayout(frame, 7, 2, 10, 5);

    // Timer selection
    //
    label = new QLabel(i18n("Sequencer timer"), frame);
    layout->addWidget(label, 0, 0);

    m_timer = new KComboBox(frame);
    layout->addWidget(m_timer, 0, 1); //, Qt::AlignHCenter);

    QStringList timers = m_doc->getTimers();
    m_origTimer = m_doc->getCurrentTimer();
    QString currentTimer = m_cfg->readEntry("timer", m_origTimer);

    for (unsigned int i = 0; i < timers.size(); ++i) {
        m_timer->insertItem(timers[i]);
        if (timers[i] == currentTimer)
            m_timer->setCurrentItem(i);
    }

    // MIDI Clock and System Realtime Messages
    //
    label = new QLabel(i18n("MIDI Clock and System messages"), frame);
    layout->addWidget(label, 2, 0);
    m_midiSync = new KComboBox(frame);
    layout->addWidget(m_midiSync, 2, 1);

    m_midiSync->insertItem(i18n("Off"));
    m_midiSync->insertItem(i18n("Send MIDI Clock, Start and Stop"));
    m_midiSync->insertItem(i18n("Accept Start, Stop and Continue"));

    int midiClock = m_cfg->readNumEntry("midiclock", 0);
    if (midiClock < 0 || midiClock > 2)
        midiClock = 0;
    m_midiSync->setCurrentItem(midiClock);

    // JACK Transport
    //
    label = new QLabel(i18n("JACK transport mode"), frame);
    layout->addWidget(label, 1, 0);

    m_jackTransport = new KComboBox(frame);
    layout->addWidget(m_jackTransport, 1, 1);

    m_jackTransport->insertItem(i18n("Ignore JACK transport"));
    m_jackTransport->insertItem(i18n("Sync"));

    /*!!! Removed as not yet implemented
        m_jackTransport->insertItem(i18n("Sync, and offer timebase master"));
    */

    bool jackMaster = m_cfg->readBoolEntry("jackmaster", false);
    bool jackTransport = m_cfg->readBoolEntry("jacktransport", false);

    if (jackTransport)
        m_jackTransport->setCurrentItem(1);
    else
        m_jackTransport->setCurrentItem(0);

    // MMC Transport
    //
    label = new QLabel(i18n("MIDI Machine Control mode"), frame);
    layout->addWidget(label, 3, 0);

    m_mmcTransport = new KComboBox(frame);
    layout->addWidget(m_mmcTransport, 3, 1); //, Qt::AlignHCenter);

    m_mmcTransport->insertItem(i18n("Off"));
    m_mmcTransport->insertItem(i18n("MMC Master"));
    m_mmcTransport->insertItem(i18n("MMC Slave"));

    int mmcMode = m_cfg->readNumEntry("mmcmode", 0);
    if (mmcMode < 0 || mmcMode > 2)
        mmcMode = 0;
    m_mmcTransport->setCurrentItem(mmcMode);

    // MTC transport
    //
    label = new QLabel(i18n("MIDI Time Code mode"), frame);
    layout->addWidget(label, 4, 0);

    m_mtcTransport = new KComboBox(frame);
    layout->addWidget(m_mtcTransport, 4, 1);

    m_mtcTransport->insertItem(i18n("Off"));
    m_mtcTransport->insertItem(i18n("MTC Master"));
    m_mtcTransport->insertItem(i18n("MTC Slave"));

    int mtcMode = m_cfg->readNumEntry("mtcmode", 0);
    if (mtcMode < 0 || mtcMode > 2)
        mtcMode = 0;
    m_mtcTransport->setCurrentItem(mtcMode);

    label = new QLabel(i18n("Automatically connect sync output to all devices in use"), frame);
    layout->addWidget(label, 5, 0);
    m_midiSyncAuto = new QCheckBox(frame);
    layout->addWidget(m_midiSyncAuto, 5, 1);

    m_midiSyncAuto->setChecked(m_cfg->readBoolEntry("midisyncautoconnect", true));

    addTab(frame, i18n("Synchronisation"));
}

int
SequencerConfigurationPage::updateTimeSlider(int msec,
        int minPower, int maxPower,
        int multiplier,
        QSlider *slider,
        QLabel *label,
        QString klabel)
{
    int tick;
    int actual = 0;

    for (tick = minPower;
            tick <= maxPower &&
            msec > (actual = ((1 << tick) * multiplier));
            ++tick)
        ;

    slider->setValue(tick);

    if (klabel && m_sampleRate) {

        size_t bytes = int(float(m_sampleRate) * actual / 1000)
                       * 2 * sizeof(float);
        int kb = bytes / 1024;

        if (actual < 1000) {
            if (kb < 1024) {
                label->setText(i18n("%1 msec / %2 KB %3")
                               .arg(actual).arg(kb).arg(klabel));
            } else {
                label->setText(i18n("%1 msec / %2 MB %3")
                               .arg(actual).arg(kb / 1024).arg(klabel));
            }
        } else {
            if (kb < 1024) {
                label->setText(i18n("%1 sec / %2 KB %3")
                               .arg(float(actual / 100) / 10).arg(kb).arg(klabel));
            } else {
                label->setText(i18n("%1 sec / %2 MB %3")
                               .arg(float(actual / 100) / 10).arg(kb / 1024).arg(klabel));
            }
        }
    } else {
        if (actual < 1000) {
            label->setText(i18n("%1 msec").arg(actual));
        } else {
            label->setText(i18n("%1 sec").arg(float(actual / 100) / 10));
        }
    }

    return actual;
}

void
SequencerConfigurationPage::slotReadAheadChanged(int v)
{
    // Event read-ahead must always be more than the audio mix buffer.
    // The mix slider is marked up with lower values already, so we
    // only need to ensure it's showing no greater tick.
    if (m_audioMix->value() >= v)
        m_audioMix->setValue(v - 1);

    m_readAhead->blockSignals(true);
    updateTimeSlider((1 << v) * 10, 1, 9, 10, m_readAhead, m_readAheadLabel, 0);
    m_readAhead->blockSignals(false);
}

void
SequencerConfigurationPage::slotAudioMixChanged(int v)
{
    if (m_readAhead->value() <= v)
        m_readAhead->setValue(v + 1);

    m_audioMix->blockSignals(true);
    updateTimeSlider((1 << v) * 10, 0, 8, 10, m_audioMix, m_audioMixLabel,
                     i18n("per audio instrument"));
    m_audioMix->blockSignals(false);
}

void
SequencerConfigurationPage::slotAudioReadChanged(int v)
{
    m_audioRead->blockSignals(true);
    updateTimeSlider((1 << v) * 10, 1, 9, 10, m_audioRead, m_audioReadLabel,
                     i18n("per file"));
    m_audioRead->blockSignals(false);
}

void
SequencerConfigurationPage::slotAudioWriteChanged(int v)
{
    m_audioWrite->blockSignals(true);
    updateTimeSlider((1 << v) * 100, 0, 9, 100, m_audioWrite, m_audioWriteLabel,
                     "");
    m_audioWrite->blockSignals(false);
}

void
SequencerConfigurationPage::slotSmallFileChanged(int v)
{
    QString text;
    v = 1 << v;
    if (v < 1024)
        text = i18n("%1 KB").arg(v);
    else
        text = i18n("%1 MB").arg(v / 1024);
    m_smallFileLabel->setText(text);
}

void
SequencerConfigurationPage::slotShowStatus()
{
    ShowSequencerStatusDialog dialog(this);
    dialog.exec();
}

void
SequencerConfigurationPage::slotJackToggled()
{
#ifdef HAVE_LIBJACK
    /*
    if (m_startJack->isChecked())
        m_jackPath->setDisabled(false);
    else
        m_jackPath->setDisabled(true);

        */
#endif // HAVE_LIBJACK
}

void
SequencerConfigurationPage::slotSoundFontToggled(bool isChecked)
{
    m_sfxLoadPath->setEnabled(isChecked);
    m_sfxLoadChoose->setEnabled(isChecked);
    m_soundFontPath->setEnabled(isChecked);
    m_soundFontChoose->setEnabled(isChecked);
}

void
SequencerConfigurationPage::slotSfxLoadPathChoose()
{
    QString path = KFileDialog::getOpenFileName(":SFXLOAD", QString::null, this, i18n("sfxload path"));
    m_sfxLoadPath->setText(path);
}

void
SequencerConfigurationPage::slotSoundFontChoose()
{
    QString path = KFileDialog::getOpenFileName(":SOUNDFONTS", "*.sb *.sf2 *.SF2 *.SB", this, i18n("Soundfont path"));
    m_soundFontPath->setText(path);
}

void
SequencerConfigurationPage::apply()
{
    m_cfg->setGroup(SequencerOptionsConfigGroup);

    // ---------- General -----------
    //
    //    m_cfg->writeEntry("commandlineoptions", m_sequencerArguments->text());
    m_cfg->writeEntry("alwayssendcontrollers",
                      m_sendControllersAtPlay->isChecked());

    m_cfg->writeEntry("sfxloadenabled", m_sfxLoadEnabled->isChecked());
    m_cfg->writeEntry("sfxloadpath", m_sfxLoadPath->text());
    m_cfg->writeEntry("soundfontpath", m_soundFontPath->text());

    long usec = (10 * (1 << m_readAhead->value())) * 1000;
    m_cfg->writeEntry("readaheadusec", usec % 1000000L);
    m_cfg->writeEntry("readaheadsec", usec / 1000000L);

    usec = (10 * (1 << m_audioMix->value())) * 1000;
    m_cfg->writeEntry("audiomixusec", usec % 1000000L);
    m_cfg->writeEntry("audiomixsec", usec / 1000000L);

    usec = (10 * (1 << m_audioRead->value())) * 1000;
    m_cfg->writeEntry("audioreadusec", usec % 1000000L);
    m_cfg->writeEntry("audioreadsec", usec / 1000000L);

    usec = (100 * (1 << m_audioWrite->value())) * 1000;
    m_cfg->writeEntry("audiowriteusec", usec % 1000000L);
    m_cfg->writeEntry("audiowritesec", usec / 1000000L);

    m_cfg->writeEntry("smallaudiofilekbytes", 1 << m_smallFile->value());

#ifdef HAVE_LIBJACK

#ifdef OFFER_JACK_START_OPTION
    // Jack control
    //
    m_cfg->writeEntry("jackstart", m_startJack->isChecked());
    m_cfg->writeEntry("jackcommand", m_jackPath->text());
#endif // OFFER_JACK_START_OPTION

    // Jack audio inputs
    //
    m_cfg->writeEntry("audiolowlatencymonitoring", m_lowLatencyMode->currentItem() == 0);
    m_cfg->writeEntry("audiofaderouts", m_createFaderOuts->isChecked());
    m_cfg->writeEntry("audiosubmasterouts", m_createSubmasterOuts->isChecked());
    m_cfg->writeEntry("audiorecordfileformat", m_audioRecFormat->currentItem());

    // Audio record minutes
    //
    /*  #1045380 ("minutes of audio recording" just insanely confusing) -- remove
        m_cfg->writeEntry("audiorecordminutes", m_audioRecordMinutes->value());
    */

    MidiByte ports = 0;
    if (m_createFaderOuts->isChecked()) {
        ports |= MappedEvent::FaderOuts;
    }
    if (m_createSubmasterOuts->isChecked()) {
        ports |= MappedEvent::SubmasterOuts;
    }
    MappedEvent mEports
    (MidiInstrumentBase,
     MappedEvent::SystemAudioPorts,
     ports);

    StudioControl::sendMappedEvent(mEports);

    MappedEvent mEff
    (MidiInstrumentBase,
     MappedEvent::SystemAudioFileFormat,
     m_audioRecFormat->currentItem());
    StudioControl::sendMappedEvent(mEff);

    m_cfg->writeEntry("timer", m_timer->currentText());
    if (m_timer->currentText() != m_origTimer) {
        m_doc->setCurrentTimer(m_timer->currentText());
    }

    // Write the JACK entry
    //
    int jackValue = m_jackTransport->currentItem();
    bool jackTransport, jackMaster;

    switch (jackValue) {
    case 2:
        jackTransport = true;
        jackMaster = true;
        break;

    case 1:
        jackTransport = true;
        jackMaster = false;
        break;

    default:
        jackValue = 0;

    case 0:
        jackTransport = false;
        jackMaster = false;
        break;
    }

    // Write the items
    //
    m_cfg->writeEntry("jacktransport", jackTransport);
    m_cfg->writeEntry("jackmaster", jackMaster);

    // Now send it
    //
    MappedEvent mEjackValue(MidiInstrumentBase,  // InstrumentId
                            MappedEvent::SystemJackTransport,
                            MidiByte(jackValue));

    StudioControl::sendMappedEvent(mEjackValue);
#endif // HAVE_LIBJACK

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
    //bool midiClock = m_midiClockEnabled->isChecked();
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
}

}
