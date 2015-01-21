/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2015 the Rosegarden development team.
 
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
#include "MIDIInstrumentParameterPanel.h"
#include "RosegardenParameterArea.h"
#include "RosegardenParameterBox.h"

#include <QFrame>
#include <QString>
#include <QStackedWidget>
#include <QLayout>


namespace Rosegarden
{


InstrumentParameterBox::IPBVector InstrumentParameterBox::m_instrumentParamBoxes;

InstrumentParameterBox::InstrumentParameterBox(RosegardenDocument *doc,
                                               QWidget *parent)
    : RosegardenParameterBox(tr("Instrument"),
                             tr("Instrument Parameters"),
                             parent),
      m_widgetStack(new QStackedWidget(this)),
      m_noInstrumentParameters(new QFrame(this)),
      m_midiInstrumentParameters(new MIDIInstrumentParameterPanel(doc, this)),
      m_audioInstrumentParameters(new AudioInstrumentParameterPanel(doc, this)),
      m_selectedInstrument(-1),
      m_doc(doc)
{
    setObjectName("Instrument Parameter Box");

    m_widgetStack->setFont(m_font);
    m_noInstrumentParameters->setFont(m_font);
    m_midiInstrumentParameters->setFont(m_font);
    m_audioInstrumentParameters->setFont(m_font);

    bool contains = false;

    IPBVector::iterator it = m_instrumentParamBoxes.begin();

    for (; it != m_instrumentParamBoxes.end(); ++it)
        if ((*it) == this)
            contains = true;

    if (!contains)
        m_instrumentParamBoxes.push_back(this);

    m_widgetStack->addWidget(m_midiInstrumentParameters);
    m_widgetStack->addWidget(m_audioInstrumentParameters);
    m_widgetStack->addWidget(m_noInstrumentParameters);

    connect(m_audioInstrumentParameters, SIGNAL(updateAllBoxes()),
            this, SLOT(slotUpdateAllBoxes()));

    connect(m_audioInstrumentParameters,
            SIGNAL(instrumentParametersChanged(InstrumentId)),
            this,
            SIGNAL(instParamsChangedIPB(InstrumentId)));

    connect(m_audioInstrumentParameters,
            SIGNAL(selectPlugin(QWidget *, InstrumentId, int)),
            this,
            SIGNAL(selectPlugin(QWidget *, InstrumentId, int)));

    connect(m_audioInstrumentParameters,
            SIGNAL(showPluginGUI(InstrumentId, int)),
            this,
            SIGNAL(showPluginGUI(InstrumentId, int)));

    connect(m_midiInstrumentParameters, SIGNAL(updateAllBoxes()),
            this, SLOT(slotUpdateAllBoxes()));

    connect(m_midiInstrumentParameters,
            SIGNAL(changeInstrumentLabel(InstrumentId, QString)),
            this, SIGNAL(changeInstrumentLabel(InstrumentId, QString)));

    connect(m_audioInstrumentParameters,
            SIGNAL(changeInstrumentLabel(InstrumentId, QString)),
            this, SIGNAL(changeInstrumentLabel(InstrumentId, QString)));

    connect(m_midiInstrumentParameters,
            SIGNAL(instParamsChangedMIPP(InstrumentId)),
            this,
            SIGNAL(instParamsChangedIPB(InstrumentId)));

    // Layout the groups left to right.

    QBoxLayout *layout = new QVBoxLayout(this);
    setLayout(layout);
    layout->setMargin(0);
    layout->addWidget(m_widgetStack);

}

InstrumentParameterBox::~InstrumentParameterBox()
{
    // deregister this parameter box
    IPBVector::iterator it = m_instrumentParamBoxes.begin();

    for (; it != m_instrumentParamBoxes.end(); ++it) {
        if ((*it) == this) {
            m_instrumentParamBoxes.erase(it);
            break;
        }
    }
}


MIDIInstrumentParameterPanel * 
    InstrumentParameterBox::getMIDIInstrumentParameterPanel()
{
	if (!m_midiInstrumentParameters) return 0;
	return m_midiInstrumentParameters;
}


Instrument *
InstrumentParameterBox::getSelectedInstrument()
{
    if (m_selectedInstrument < 0) return 0;
    if (!m_doc) return 0;
    return m_doc->getStudio().getInstrumentById(m_selectedInstrument);
}

QString
InstrumentParameterBox::getPreviousBox(RosegardenParameterArea::Arrangement /* arrangement */) const
{
    return tr("Track");
}

void
InstrumentParameterBox::setAudioMeter(float ch1, float ch2, float ch1r, float ch2r)
{
    m_audioInstrumentParameters->setAudioMeter(ch1, ch2, ch1r, ch2r);
}

void
InstrumentParameterBox::setDocument(RosegardenDocument *doc)
{
    m_doc = doc;
    m_midiInstrumentParameters->setDocument(m_doc);
    m_audioInstrumentParameters->setDocument(m_doc);
}

void
InstrumentParameterBox::slotPluginSelected(InstrumentId id, int index, int plugin)
{
    m_audioInstrumentParameters->slotPluginSelected(id, index, plugin);
}

void
InstrumentParameterBox::slotPluginBypassed(InstrumentId id, int index, bool bypassState)
{
    m_audioInstrumentParameters->slotPluginBypassed(id, index, bypassState);
}

void
InstrumentParameterBox::useInstrument(Instrument *instrument)
{
    if (instrument == 0) {
        m_widgetStack->setCurrentWidget(m_noInstrumentParameters);	// was raiseWidget
        emit instrumentPercussionSetChanged(instrument);
        return ;
    }

    // ok
    if (instrument) {
        m_selectedInstrument = instrument->getId();
    } else {
        m_selectedInstrument = -1;
    }

    // Hide or Show according to Instrument type
    //
    if (instrument->getType() == Instrument::Audio ||
        instrument->getType() == Instrument::SoftSynth) {

        m_audioInstrumentParameters->setupForInstrument(getSelectedInstrument());
        m_widgetStack->setCurrentWidget(m_audioInstrumentParameters);
// 		m_widgetStack->raiseWidget(m_audioInstrumentParameters);

    } else { // Midi

        m_midiInstrumentParameters->setupForInstrument(getSelectedInstrument());
		m_widgetStack->setCurrentWidget(m_midiInstrumentParameters);
        emit instrumentPercussionSetChanged(instrument);

    }

}

void
InstrumentParameterBox::slotUpdateAllBoxes()
{
    // This causes the MatrixWidget to redraw the pitch ruler.
    // ??? Instead of monitoring the UI, the MatrixWidget should monitor
    //     changes to the Instrument and update itself based
    //     on that.  See if that is possible and give it a shot.
    emit instrumentPercussionSetChanged(getSelectedInstrument());

#if 0
// Now that there is only one of these, the rest of this can be
// removed.
    // For each IPB
    for (IPBVector::iterator it = m_instrumentParamBoxes.begin();
         it != m_instrumentParamBoxes.end();
         ++it) {
        if ((*it) != this && getSelectedInstrument() &&
            (*it)->getSelectedInstrument() == getSelectedInstrument()) {
            // Update this IPB to show the currently selected instrument.
            (*it)->useInstrument(getSelectedInstrument());
        }
    }
#endif
}

void
InstrumentParameterBox::slotInstrumentParametersChanged(InstrumentId id)
{
    // ??? Why?  There seems to be a lot of this around here.  Perhaps this
    //     is to break up endless update loops.  Maybe that's another side-
    //     effect of UI-to-UI notifications.
    blockSignals(true);

#if 0
    // For each IPB
    for (IPBVector::iterator it = m_instrumentParamBoxes.begin();
         it != m_instrumentParamBoxes.end();
         ++it) {
        if ((*it)->getSelectedInstrument()) {
            // If this IPB happens to be displaying the instrument whose
            // parameters are changing (id)
            if ((*it)->getSelectedInstrument()->getId() == id) {
                // Update this IPB to show the current parameters.
                (*it)->useInstrument((*it)->getSelectedInstrument());
            }
        }
    }
#else
// Since there is only one IPB, the above reduces to this.
    Instrument *instrument = getSelectedInstrument();
    if (instrument) {
        // If this IPB happens to be displaying the instrument whose
        // parameters are changing (id)
        if (instrument->getId() == id) {
            // Update this IPB to show the current parameters.
            useInstrument(instrument);
        }
    }
#endif

    blockSignals(false);
}


}

#include "InstrumentParameterBox.moc"
