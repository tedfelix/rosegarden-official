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

#include "AudioInstrumentParameterPanel.h"
#include "misc/Debug.h"
#include "MIDIInstrumentParameterPanel.h"

#include <QFrame>
#include <QStackedWidget>
#include <QVBoxLayout>


namespace Rosegarden
{


InstrumentParameterBox::InstrumentParameterBox(QWidget *parent) :
    RosegardenParameterBox(tr("Instrument Parameters"), parent),
    m_stackedWidget(new QStackedWidget(this)),
    m_emptyFrame(new QFrame),
    m_mipp(new MIDIInstrumentParameterPanel(0)),
    m_aipp(new AudioInstrumentParameterPanel(0))
{
    setObjectName("Instrument Parameter Box");

    m_stackedWidget->setFont(m_font);
    m_emptyFrame->setFont(m_font);
    m_mipp->setFont(m_font);
    m_aipp->setFont(m_font);

    // ??? Use QStackedLayout instead of QStackedWidget.

    m_stackedWidget->addWidget(m_mipp);
    m_stackedWidget->addWidget(m_aipp);
    m_stackedWidget->addWidget(m_emptyFrame);

    // Set up a layout manager to make sure the QFrame
    // (RosegardenParameterBox) is always the right size for its contents.
    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->addWidget(m_stackedWidget);
    // Prevent this layout from introducing even more margin space.
    layout->setMargin(0);
    setLayout(layout);
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
    // If the Track has no Instrument, go with a blank frame.
    if (!instrument) {
        m_stackedWidget->setCurrentWidget(m_emptyFrame);
        return;
    }

    // MIPP for MIDI
    if (instrument->getType() == Instrument::Midi) {
        m_mipp->displayInstrument(instrument);
        m_stackedWidget->setCurrentWidget(m_mipp);
        return;
    }

    // AIPP for Audio or SoftSynth
    if (instrument->getType() == Instrument::Audio ||
        instrument->getType() == Instrument::SoftSynth) {
        // Update the audio panel and bring it to the top.
        m_aipp->setupForInstrument(instrument);
        m_stackedWidget->setCurrentWidget(m_aipp);
        return;
    }

}


}

