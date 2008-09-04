/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2008 the Rosegarden development team.

    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#ifndef _RG_MIDICONFIGURATIONPAGE_H_
#define _RG_MIDICONFIGURATIONPAGE_H_

#include "TabbedConfigurationPage.h"
#include <QString>
#include <klocale.h>
#include <QLineEdit>
#include <QCheckBox>


class QWidget;
class QSpinBox;
class QSlider;
class QPushButton;
class QLabel;
class QComboBox;
class KConfig;
class QComboBox;


namespace Rosegarden
{

class RosegardenGUIDoc;


class MIDIConfigurationPage : public TabbedConfigurationPage
{
    Q_OBJECT
public:
    MIDIConfigurationPage(RosegardenGUIDoc *doc,
                               QSettings *cfg,
                               QWidget *parent=0,
                               const char *name=0);

    virtual void apply();

    static QString iconLabel() { return i18n("MIDI"); }
    static QString title()     { return i18n("MIDI Settings"); }
    static QString iconName()  { return "configure-midi"; }

protected slots:
    void slotSoundFontToggled(bool);
    void slotSfxLoadPathChoose();
    void slotSoundFontChoose();

protected:
    bool getUseDefaultStudio()      { return m_studio->isChecked(); }

    //--------------- Data members ---------------------------------

    // General
    QCheckBox *m_sendControllersAtPlay;

    QCheckBox   *m_sfxLoadEnabled;
    QLineEdit   *m_sfxLoadPath;
    QPushButton *m_sfxLoadChoose;
    QLineEdit   *m_soundFontPath;
    QPushButton *m_soundFontChoose;

    // Sync and timing
    //
    //QCheckBox *m_midiClockEnabled;
    QComboBox *m_midiSync;
    QString    m_origTimer;
    QComboBox *m_timer;
    QComboBox *m_mmcTransport;
    QComboBox *m_mtcTransport;
    QCheckBox *m_midiSyncAuto;

    QCheckBox* m_studio;
    QSpinBox*  m_midiPitchOctave;

};
 


}

#endif
