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
      m_stackedWidget(new QStackedWidget(this)),
      m_emptyFrame(new QFrame(this)),
      m_mipp(new MIDIInstrumentParameterPanel(this)),
      m_aipp(new AudioInstrumentParameterPanel(this))
{
    setObjectName("Instrument Parameter Box");

    m_stackedWidget->setFont(m_font);
    m_emptyFrame->setFont(m_font);
    m_mipp->setFont(m_font);
    m_aipp->setFont(m_font);

    m_stackedWidget->addWidget(m_mipp);
    m_stackedWidget->addWidget(m_aipp);
    m_stackedWidget->addWidget(m_emptyFrame);

    // Layout the groups left to right.

    QBoxLayout *layout = new QVBoxLayout(this);
    setLayout(layout);
    layout->setMargin(0);
    layout->addWidget(m_stackedWidget);

}

InstrumentParameterBox::~InstrumentParameterBox()
{
}

void
InstrumentParameterBox::setAudioMeter(float ch1, float ch2, float ch1r, float ch2r)
{
    m_aipp->setAudioMeter(ch1, ch2, ch1r, ch2r);
}

void
InstrumentParameterBox::useInstrument(Instrument *instrument)
{
    if (!instrument) {
        // Go with a blank frame.
        m_stackedWidget->setCurrentWidget(m_emptyFrame);

        return;
    }

    // Hide or Show according to Instrument type
    //
    if (instrument->getType() == Instrument::Audio ||
        instrument->getType() == Instrument::SoftSynth) {

        // Update the audio panel and bring it to the top.
        m_aipp->setupForInstrument(instrument);
        m_stackedWidget->setCurrentWidget(m_aipp);

    } else { // Midi

        // Update the MIDI panel and bring it to the top.
        m_mipp->displayInstrument(instrument);
        m_stackedWidget->setCurrentWidget(m_mipp);

    }

}


}

