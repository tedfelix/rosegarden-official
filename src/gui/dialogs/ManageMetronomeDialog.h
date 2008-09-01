
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

#ifndef _RG_MANAGEMETRONOMEDIALOG_H_
#define _RG_MANAGEMETRONOMEDIALOG_H_

#include "base/MidiMetronome.h"
#include <QDialog>
#include <QDialogButtonBox>


class QWidget;
class QSpinBox;
class QCheckBox;
class QComboBox;

namespace Rosegarden
{

class RosegardenGUIDoc;
class PitchChooser;
class InstrumentParameterBox;
class Device;


class ManageMetronomeDialog : public QDialog
{
    Q_OBJECT

public:
    ManageMetronomeDialog(QDialogButtonBox::QWidget *parent, RosegardenGUIDoc *doc);

    void setModified(bool value);

public slots:
    void slotOk();
    void slotApply();
    void slotSetModified();
    void slotResolutionChanged(int);
    void slotPreviewPitch(int);
    void slotInstrumentChanged(int);
    void slotPitchSelectorChanged(int);
    void slotPitchChanged(int);
    void populate(int dev);

protected:

    //--------------- Data members ---------------------------------

    RosegardenGUIDoc       *m_doc;

    QComboBox              *m_metronomeDevice;
    QComboBox              *m_metronomeInstrument;
    QComboBox              *m_metronomeResolution;
    QComboBox              *m_metronomePitchSelector;
    PitchChooser *m_metronomePitch;
    QSpinBox               *m_metronomeBarVely;
    QSpinBox               *m_metronomeBeatVely;
    QSpinBox               *m_metronomeSubBeatVely;
    InstrumentParameterBox *m_instrumentParameterBox;
    QCheckBox              *m_playEnabled;
    QCheckBox              *m_recordEnabled;
        
    bool                   m_modified;
    MidiByte   m_barPitch;
    MidiByte   m_beatPitch;
    MidiByte   m_subBeatPitch;

    bool isSuitable(Device *, bool *hasConnectionReturn = 0);
    void setMetronome(Device *, const MidiMetronome &);
    const MidiMetronome *getMetronome(Device *);
};


}

#endif
