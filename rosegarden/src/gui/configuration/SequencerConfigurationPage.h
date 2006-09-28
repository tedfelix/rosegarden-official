
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

#ifndef _RG_SEQUENCERCONFIGURATIONPAGE_H_
#define _RG_SEQUENCERCONFIGURATIONPAGE_H_

#include "TabbedConfigurationPage.h"
#include <qstring.h>
#include <klocale.h>


class QWidget;
class QSpinBox;
class QSlider;
class QPushButton;
class QLineEdit;
class QLabel;
class QComboBox;
class QCheckBox;
class KConfig;
class KComboBox;


namespace Rosegarden
{

class RosegardenGUIDoc;


class SequencerConfigurationPage : public TabbedConfigurationPage
{
    Q_OBJECT
public:
    SequencerConfigurationPage(RosegardenGUIDoc *doc,
                               KConfig *cfg,
                               QWidget *parent=0,
                               const char *name=0);

    virtual void apply();

    static QString iconLabel() { return i18n("Sequencer"); }
    static QString title()     { return i18n("Sequencer Settings"); }
    static QString iconName()  { return "player_play"; }

#ifdef HAVE_LIBJACK
    QString getJackPath() { return m_jackPath->text(); }
#endif // HAVE_LIBJACK

protected slots:

    void slotReadAheadChanged(int);
    void slotAudioMixChanged(int);
    void slotAudioReadChanged(int);
    void slotAudioWriteChanged(int);
    void slotSmallFileChanged(int);

    void slotShowStatus();
    void slotJackToggled();

    void slotSoundFontToggled(bool);
    void slotSfxLoadPathChoose();
    void slotSoundFontChoose();

protected:

    int updateTimeSlider(int msec, int minPower, int maxPower, int multiplier,
			 QSlider *slider, QLabel *label, QString klabel);

    //--------------- Data members ---------------------------------

    // General
    QLineEdit *m_sequencerArguments;
    QCheckBox *m_sendControllersAtPlay;

    QCheckBox   *m_sfxLoadEnabled;
    QLineEdit   *m_sfxLoadPath;
    QPushButton *m_sfxLoadChoose;
    QLineEdit   *m_soundFontPath;
    QPushButton *m_soundFontChoose;


#ifdef HAVE_LIBJACK
    QCheckBox *m_startJack;
    QLineEdit *m_jackPath;
#endif // HAVE_LIBJACK

    // Sync and timing
    //
    //QCheckBox *m_midiClockEnabled;
    QComboBox *m_midiSync;
    QString    m_origTimer;
    QComboBox *m_timer;
    QComboBox *m_jackTransport;
    QComboBox *m_mmcTransport;
    QComboBox *m_mtcTransport;
    QCheckBox *m_midiSyncAuto;

    int      m_sampleRate;
    QSlider* m_readAhead;
    QSlider* m_audioMix;
    QSlider* m_audioRead;
    QSlider* m_audioWrite;
    QSlider* m_smallFile;
    QLabel*  m_readAheadLabel;
    QLabel*  m_audioMixLabel;
    QLabel*  m_audioReadLabel;
    QLabel*  m_audioWriteLabel;
    QLabel*  m_smallFileLabel;

#ifdef HAVE_LIBJACK
    // Number of JACK input ports our RG client creates - 
    // this decides how many audio input destinations
    // we have.
    //
    QCheckBox    *m_createFaderOuts;
    QCheckBox    *m_createSubmasterOuts;

    QComboBox    *m_audioRecFormat;

    KComboBox    *m_lowLatencyMode;

#endif // HAVE_LIBJACK

    // How many minutes of audio recording should we allow?
    //
/*  #1045380 ("minutes of audio recording" just insanely confusing) -- remove
    QSpinBox     *m_audioRecordMinutes;
*/

};
 


}

#endif
