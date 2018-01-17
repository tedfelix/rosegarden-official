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

#define RG_MODULE_STRING "[InstrumentParameterBox]"

#include "InstrumentParameterBox.h"

#include "misc/Debug.h"
#include "AudioInstrumentParameterPanel.h"
#include "base/Instrument.h"
#include "base/MidiProgram.h"
#include "document/RosegardenDocument.h"
#include "gui/application/RosegardenMainWindow.h"
#include "MIDIInstrumentParameterPanel.h"
#include "RosegardenParameterArea.h"
#include "RosegardenParameterBox.h"

#include <QFrame>
#include <QString>
#include <QStackedWidget>
#include <QLayout>


namespace Rosegarden
{


InstrumentParameterBox::InstrumentParameterBox(QWidget *parent)
    : RosegardenParameterBox(tr("Instrument Parameters"),
                             parent),
      m_widgetStack(new QStackedWidget(this)),
      m_noInstrumentParameters(new QFrame(this)),
      m_midiInstrumentParameters(new MIDIInstrumentParameterPanel(this)),
      m_audioInstrumentParameters(new AudioInstrumentParameterPanel(this)),
      m_selectedInstrument(-1)
{
    setObjectName("Instrument Parameter Box");

    m_widgetStack->setFont(m_font);
    m_noInstrumentParameters->setFont(m_font);
    m_midiInstrumentParameters->setFont(m_font);
    m_audioInstrumentParameters->setFont(m_font);

    m_widgetStack->addWidget(m_midiInstrumentParameters);
    m_widgetStack->addWidget(m_audioInstrumentParameters);
    m_widgetStack->addWidget(m_noInstrumentParameters);

    connect(m_audioInstrumentParameters,
            SIGNAL(showPluginGUI(InstrumentId, int)),
            this,
            SIGNAL(showPluginGUI(InstrumentId, int)));

    // Layout the groups left to right.

    QBoxLayout *layout = new QVBoxLayout(this);
    setLayout(layout);
    layout->setMargin(0);
    layout->addWidget(m_widgetStack);

}

InstrumentParameterBox::~InstrumentParameterBox()
{
}

void
InstrumentParameterBox::setAudioMeter(float ch1, float ch2, float ch1r, float ch2r)
{
    m_audioInstrumentParameters->setAudioMeter(ch1, ch2, ch1r, ch2r);
}

void
InstrumentParameterBox::useInstrument(Instrument *instrument)
{
    if (!instrument) {
        // Go with a blank frame.
        m_widgetStack->setCurrentWidget(m_noInstrumentParameters);

        m_selectedInstrument = -1;

        return;
    }

    m_selectedInstrument = instrument->getId();

    // Hide or Show according to Instrument type
    //
    if (instrument->getType() == Instrument::Audio ||
        instrument->getType() == Instrument::SoftSynth) {

        // Update the audio panel and bring it to the top.
        m_audioInstrumentParameters->setupForInstrument(instrument);
        m_widgetStack->setCurrentWidget(m_audioInstrumentParameters);

    } else { // Midi

        // Update the MIDI panel and bring it to the top.
        m_midiInstrumentParameters->displayInstrument(instrument);
        m_widgetStack->setCurrentWidget(m_midiInstrumentParameters);

    }

}


}

