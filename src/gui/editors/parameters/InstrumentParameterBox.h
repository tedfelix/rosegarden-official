/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2017 the Rosegarden development team.

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

#include "base/Instrument.h"
#include "RosegardenParameterBox.h"

class QStackedWidget;
class QWidget;
class QFrame;


namespace Rosegarden
{

class RosegardenDocument;
class MIDIInstrumentParameterPanel;
class AudioInstrumentParameterPanel;


/// Display and allow modification of Instrument parameters for a Track
/**
 * InstrumentParameterBox is the box in the lower left of the main window
 * with the title "Instrument Parameters".  Within this box is a stack
 * of three widgets.  One for MIDI parameters (MIDIInstrumentParameterPanel),
 * one for Audio parameters (AudioInstrumentParameterPanel), and an empty
 * QFrame for no parameters (a track without an instrument).
 *
 * Future Direction
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
    ~InstrumentParameterBox();

    // ??? This should not be necessary.  This class should instead handle
    //     the RosegardenDocument::documentModified() signal and select the
    //     appropriate panel to bring to the top.  It should not send a
    //     signal to the MatrixWidget as the MatrixWidget should take care
    //     of itself in response to documentModified().  It also should not
    //     send the Instrument to the MIPP and AIPP.  They should also
    //     handle documentModified() and figure this out on their own.
    //     IOW, all responsibilities beyond selecting the panel should be
    //     pushed down into other classes.
    void useInstrument(Instrument *instrument);

    void setAudioMeter(float dBleft, float dBright,
                       float recDBleft, float recDBright);

    MIDIInstrumentParameterPanel *getMIDIInstrumentParameterPanel();

    /// Update the MatrixWidget's PitchRuler.
    void emitInstrumentPercussionSetChanged();

public slots:

    // From Plugin dialog
    //
    void slotPluginSelected(InstrumentId id, int index, int plugin);
    void slotPluginBypassed(InstrumentId id, int pluginIndex, bool bp);

signals:

    void selectPlugin(QWidget *, InstrumentId id, int index);
    void showPluginGUI(InstrumentId id, int index);

    /// Update the MatrixWidget's PitchRuler.
    void instrumentPercussionSetChanged(Instrument *);

private:

    // ??? rename: m_stackedWidget
    QStackedWidget *m_widgetStack;

    // ??? rename: m_emptyFrame
    QFrame *m_noInstrumentParameters;
    MIDIInstrumentParameterPanel *m_midiInstrumentParameters;
    AudioInstrumentParameterPanel *m_audioInstrumentParameters;

    // -1 if no instrument, InstrumentId otherwise
    int m_selectedInstrument;
};


}

#endif
