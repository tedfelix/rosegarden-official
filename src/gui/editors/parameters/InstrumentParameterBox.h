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

#ifndef RG_INSTRUMENTPARAMETERBOX_H
#define RG_INSTRUMENTPARAMETERBOX_H

#include "RosegardenParameterBox.h"

class QStackedWidget;
class QWidget;
class QFrame;


namespace Rosegarden
{

class AudioInstrumentParameterPanel;
class Instrument;
class MIDIInstrumentParameterPanel;
class RosegardenDocument;


/// Display and allow modification of Instrument parameters for a Track
/**
 * InstrumentParameterBox is the box in the lower left of the main window
 * with the title "Instrument Parameters".  Within this box is a stack
 * of three widgets.  One for MIDI parameters (MIDIInstrumentParameterPanel),
 * one for Audio parameters (AudioInstrumentParameterPanel), and an empty
 * QFrame for no parameters (a track without an instrument).
 *
 * Future Direction
 * Get rid if useInstrument().
 * The current design has each part of the UI connected to every other part
 * of the UI.  This results in a combinatorial explosion of connections.
 * A simpler design would use RosegardenDocument::documentModified().  This
 * way the various parts of the UI need only know about RosegardenDocument,
 * not each other.
 */
class InstrumentParameterBox : public RosegardenParameterBox
{

Q_OBJECT

public:
    InstrumentParameterBox(QWidget *parent);
    ~InstrumentParameterBox() override;

    /// Set the audio meter levels on the AIPP.
    /**
     * This is called by RMVW::updateMeters() and updateMonitorMeters()
     * which get the levels from SequencerDataBlock and display them
     * with this routine.
     *
     * Delegates to AIPP::setAudioMeter().
     *
     * ??? This should go away.  See comments on AIPP::setAudioMeter().
     */
    void setAudioMeter(float dBleft, float dBright,
                       float recDBleft, float recDBright);

private slots:

    /// Called when a new document is loaded.
    void slotDocumentLoaded(RosegardenDocument *);
    /// Called when the document is modified in some way.
    void slotDocumentModified(bool);

private:

    QStackedWidget *m_stackedWidget;

    QFrame *m_emptyFrame;
    MIDIInstrumentParameterPanel *m_mipp;
    AudioInstrumentParameterPanel *m_aipp;

};


}

#endif
