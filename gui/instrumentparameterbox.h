// -*- c-basic-offset: 4 -*-

/*
    Rosegarden-4 v0.2
    A sequencer and musical notation editor.

    This program is Copyright 2000-2002
        Guillaume Laurent   <glaurent@telegraph-road.org>,
        Chris Cannam        <cannam@all-day-breakfast.com>,
        Richard Bown        <bownie@bownie.com>

    The moral right of the authors to claim authorship of this work
    has been asserted.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#include <qgroupbox.h>
#include <qlabel.h>

#include "widgets.h"
#include "Instrument.h"
#include "MappedEvent.h"
#include "MappedInstrument.h"
#include "MappedCommon.h"

#ifndef _INSTRUMENTPARAMETERBOX_H_
#define _INSTRUMENTPARAMETERBOX_H_

// Display and allow modification of Instrument parameters
//

class RosegardenComboBox;
class QCheckBox;
class QSlider;
class QPushButton;

namespace Rosegarden { class AudioPluginManager; }

class InstrumentParameterBox : public RosegardenParameterBox
{
Q_OBJECT

public:
    InstrumentParameterBox(Rosegarden::AudioPluginManager *pluginManager,
                           QWidget *parent = 0);
    ~InstrumentParameterBox();

    void useInstrument(Rosegarden::Instrument *instrument);

    // To update all InstrumentParamterBoxen for an Instrument.
    //
    void updateAllBoxes();

    Rosegarden::Instrument* getSelectedInstrument()
        { return m_selectedInstrument; }

public slots:
    void slotSelectProgram(int index);
    void slotSelectPan(int index);
    void slotSelectVelocity(int index);
    void slotSelectBank(int index);
    void slotSelectChannel(int index);

    void slotActivateProgramChange(bool value);
    void slotActivateVelocity(bool value);
    void slotActivatePan(bool value);
    void slotActivateBank(bool value);

    // select a plugin to go at an index number
    //
    void slotSelectPlugin(int index);

    // and for the signals that come back
    //
    void slotPluginSelected(int index, int plugin);
    void slotPluginPortChanged(int pluginIndex, int portIndex, float value);

signals:

    // Emit a MIDI controller for immediate processing.
    // This is necessary for controlling MIDI devices in
    // real time during playback.
    //
    void sendMappedEvent(Rosegarden::MappedEvent *mE);
    void changeInstrumentLabel(Rosegarden::InstrumentId id, QString label);

    void sendMappedInstrument(const Rosegarden::MappedInstrument &mI);

protected:

    void populateProgramList();
    void initBox();

    //--------------- Data members ---------------------------------

    QLabel             *m_instrumentLabel;
    QLabel             *m_channelLabel;
    QLabel             *m_panLabel;
    QLabel             *m_velocityLabel;
    QLabel             *m_programLabel;
    QLabel             *m_bankLabel;

    RosegardenComboBox *m_bankValue;
    RosegardenComboBox *m_channelValue;
    RosegardenComboBox *m_programValue;
    RosegardenComboBox *m_panValue;
    RosegardenComboBox *m_velocityValue;

    QCheckBox          *m_bankCheckBox;
    QCheckBox          *m_programCheckBox;
    QCheckBox          *m_panCheckBox;
    QCheckBox          *m_velocityCheckBox;
    RosegardenFader    *m_volumeFader;
    QLabel             *m_volumeValue;
    QLabel             *m_volumeLabel;
    QLabel             *m_pluginLabel;

    RosegardenRotary   *m_chorusRotary;
    RosegardenRotary   *m_reverbRotary;
    RosegardenRotary   *m_highPassRotary;
    RosegardenRotary   *m_resonanceRotary;
    QLabel             *m_chorusLabel;
    QLabel             *m_reverbLabel;
    QLabel             *m_highPassLabel;
    QLabel             *m_resonanceLabel;

    std::vector<PluginButton*>       m_pluginButtons;

    Rosegarden::Instrument          *m_selectedInstrument;

    Rosegarden::AudioPluginManager  *m_pluginManager;

};

// Global references
//
static std::vector<InstrumentParameterBox*> instrumentParamBoxes;


#endif // _INSTRUMENTPARAMETERBOX_H_
