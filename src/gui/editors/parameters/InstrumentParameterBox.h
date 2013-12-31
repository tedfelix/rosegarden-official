
/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2014 the Rosegarden development team.

    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#ifndef RG_INSTRUMENTPARAMETERBOX_H
#define RG_INSTRUMENTPARAMETERBOX_H

#include "base/MidiProgram.h"
#include "RosegardenParameterArea.h"
#include "RosegardenParameterBox.h"
#include <QString>
#include <vector>

#include <QStackedWidget>


//class QWidgetStack;
class QWidget;
class QFrame;


namespace Rosegarden
{

class RosegardenDocument;
class MIDIInstrumentParameterPanel;
class Instrument;
class AudioInstrumentParameterPanel;


/**
 * Display and allow modification of Instrument parameters
 */
class InstrumentParameterBox : public RosegardenParameterBox
{
Q_OBJECT

public:
    InstrumentParameterBox(RosegardenDocument *doc,
                           QWidget *parent = 0);
    ~InstrumentParameterBox();

    void useInstrument(Instrument *instrument);

    Instrument* getSelectedInstrument();

    void setAudioMeter(float dBleft, float dBright,
                       float recDBleft, float recDBright);

    void setDocument(RosegardenDocument* doc);
    
    MIDIInstrumentParameterPanel * getMIDIInstrumentParameterPanel();
    
    virtual void showAdditionalControls(bool showThem);

    virtual QString getPreviousBox(RosegardenParameterArea::Arrangement) const;
    

public slots:

    // To update all InstrumentParameterBoxen for an Instrument.  Called
    // from one of the parameter panels when something changes.
    //
    void slotUpdateAllBoxes();

    // Update InstrumentParameterBoxes that are showing a given instrument.
    // Called from the Outside.
    //
    void slotInstrumentParametersChanged(InstrumentId id);

    // From Plugin dialog
    //
    void slotPluginSelected(InstrumentId id, int index, int plugin);
    void slotPluginBypassed(InstrumentId id, int pluginIndex, bool bp);

signals:

    void changeInstrumentLabel(InstrumentId id, QString label);

    void selectPlugin(QWidget*, InstrumentId id, int index);
    void showPluginGUI(InstrumentId id, int index);

    void instrumentParametersChanged(InstrumentId);
    void instrumentPercussionSetChanged(Instrument *);

protected:

    //--------------- Data members ---------------------------------
    QStackedWidget                  *m_widgetStack;
    QFrame                          *m_noInstrumentParameters;
    MIDIInstrumentParameterPanel    *m_midiInstrumentParameters;
    AudioInstrumentParameterPanel   *m_audioInstrumentParameters;

    // -1 if no instrument, InstrumentId otherwise
    int                              m_selectedInstrument;

    // So we can setModified()
    //
    RosegardenDocument                *m_doc;
    bool                            m_lastShowAdditionalControlsArg;
};

// Global references
//
static std::vector<InstrumentParameterBox*> instrumentParamBoxes;


}

#endif
