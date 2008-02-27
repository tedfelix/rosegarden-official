/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
 
    This program is Copyright 2000-2008
        Guillaume Laurent   <glaurent@telegraph-road.org>,
        Chris Cannam        <cannam@all-day-breakfast.com>,
        Richard Bown        <richard.bown@ferventsoftware.com>
 
    The moral rights of Guillaume Laurent, Chris Cannam, and Richard
    Bown to claim authorship of this work have been asserted.
 
    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.
 
    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/


#include "InstrumentParameterBox.h"
#include <qlayout.h>

#include <klocale.h>
#include "misc/Debug.h"
#include "AudioInstrumentParameterPanel.h"
#include "base/Instrument.h"
#include "base/MidiProgram.h"
#include "document/RosegardenGUIDoc.h"
#include "MIDIInstrumentParameterPanel.h"
#include "RosegardenParameterArea.h"
#include "RosegardenParameterBox.h"
#include <ktabwidget.h>
#include <qfont.h>
#include <qframe.h>
#include <qscrollview.h>
#include <qstring.h>
#include <qvbox.h>
#include <qwidget.h>
#include <qwidgetstack.h>


namespace Rosegarden
{

InstrumentParameterBox::InstrumentParameterBox(RosegardenGUIDoc *doc,
                                               QWidget *parent)
    : RosegardenParameterBox(i18n("Instrument"),
                             i18n("Instrument Parameters"),
                             parent),
      m_widgetStack(new QWidgetStack(this)),
      m_noInstrumentParameters(new QVBox(this)),
      m_midiInstrumentParameters(new MIDIInstrumentParameterPanel(doc, this)),
      m_audioInstrumentParameters(new AudioInstrumentParameterPanel(doc, this)),
      m_selectedInstrument(-1),
      m_doc(doc),
      m_lastShowAdditionalControlsArg(false)
{
    m_widgetStack->setFont(m_font);
    m_noInstrumentParameters->setFont(m_font);
    m_midiInstrumentParameters->setFont(m_font);
    m_audioInstrumentParameters->setFont(m_font);

    bool contains = false;

    std::vector<InstrumentParameterBox*>::iterator it =
        instrumentParamBoxes.begin();

    for (; it != instrumentParamBoxes.end(); it++)
        if ((*it) == this)
            contains = true;

    if (!contains)
        instrumentParamBoxes.push_back(this);

    m_widgetStack->addWidget(m_midiInstrumentParameters);
    m_widgetStack->addWidget(m_audioInstrumentParameters);
    m_widgetStack->addWidget(m_noInstrumentParameters);

    m_midiInstrumentParameters->adjustSize();
    m_audioInstrumentParameters->adjustSize();
    m_noInstrumentParameters->adjustSize();

    connect(m_audioInstrumentParameters, SIGNAL(updateAllBoxes()),
            this, SLOT(slotUpdateAllBoxes()));

    connect(m_audioInstrumentParameters,
            SIGNAL(instrumentParametersChanged(InstrumentId)),
            this,
            SIGNAL(instrumentParametersChanged(InstrumentId)));

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
            SIGNAL(instrumentParametersChanged(InstrumentId)),
            this,
            SIGNAL(instrumentParametersChanged(InstrumentId)));

    // Layout the groups left to right.

    QBoxLayout* layout = new QVBoxLayout(this);
    layout->addWidget(m_widgetStack);

}

InstrumentParameterBox::~InstrumentParameterBox()
{
    // deregister this parameter box
    std::vector<InstrumentParameterBox*>::iterator it =
        instrumentParamBoxes.begin();

    for (; it != instrumentParamBoxes.end(); it++) {
        if ((*it) == this) {
            instrumentParamBoxes.erase(it);
            break;
        }
    }
}

Instrument *
InstrumentParameterBox::getSelectedInstrument()
{
    if (m_selectedInstrument < 0) return 0;
    if (!m_doc) return 0;
    return m_doc->getStudio().getInstrumentById(m_selectedInstrument);
}

QString
InstrumentParameterBox::getPreviousBox(RosegardenParameterArea::Arrangement arrangement) const
{
    return i18n("Track");
}

void
InstrumentParameterBox::setAudioMeter(float ch1, float ch2, float ch1r, float ch2r)
{
    m_audioInstrumentParameters->setAudioMeter(ch1, ch2, ch1r, ch2r);
}

void
InstrumentParameterBox::setDocument(RosegardenGUIDoc* doc)
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
    RG_DEBUG << "useInstrument() - populate Instrument\n";

    if (instrument == 0) {
        m_widgetStack->raiseWidget(m_noInstrumentParameters);
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
        m_widgetStack->raiseWidget(m_audioInstrumentParameters);

    } else { // Midi

        m_midiInstrumentParameters->setupForInstrument(getSelectedInstrument());
        m_midiInstrumentParameters->showAdditionalControls(m_lastShowAdditionalControlsArg);
        m_widgetStack->raiseWidget(m_midiInstrumentParameters);
        emit instrumentPercussionSetChanged(instrument);

    }

}

void
InstrumentParameterBox::slotUpdateAllBoxes()
{
    emit instrumentPercussionSetChanged(getSelectedInstrument());

    std::vector<InstrumentParameterBox*>::iterator it =
        instrumentParamBoxes.begin();

    // To update all open IPBs
    //
    for (; it != instrumentParamBoxes.end(); it++) {
        if ((*it) != this && getSelectedInstrument() &&
            (*it)->getSelectedInstrument() == getSelectedInstrument())
            (*it)->useInstrument(getSelectedInstrument());
    }
}

void
InstrumentParameterBox::slotInstrumentParametersChanged(InstrumentId id)
{
    std::vector<InstrumentParameterBox*>::iterator it =
        instrumentParamBoxes.begin();

    blockSignals(true);

    for (; it != instrumentParamBoxes.end(); it++) {
        if ((*it)->getSelectedInstrument()) {
            if ((*it)->getSelectedInstrument()->getId() == id) {
                (*it)->useInstrument((*it)->getSelectedInstrument()); // refresh
            }
        }
    }

    blockSignals(false);
}

void
InstrumentParameterBox::showAdditionalControls(bool showThem)
{
    m_midiInstrumentParameters->showAdditionalControls(showThem);
    m_lastShowAdditionalControlsArg = showThem;
}

}
#include "InstrumentParameterBox.moc"
