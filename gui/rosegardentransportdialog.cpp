// -*- c-basic-offset: 4 -*-

/*
    Rosegarden-4 v0.1
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

#include "rosegardentransportdialog.h"
#include "MidiPitchLabel.h"
#include <kglobal.h>
#include <kstddirs.h>
#include <klocale.h>

#include <qlabel.h>
#include <qpushbutton.h>
#include <qspinbox.h>
#include <qlineedit.h>
#include <qtimer.h>

#include <iostream>

namespace Rosegarden
{

RosegardenTransportDialog::RosegardenTransportDialog(QWidget *parent,
                                                     const char *name):
    RosegardenTransport(parent, name),
    m_lastTenHours(0),
    m_lastUnitHours(0),
    m_lastTenMinutes(0),
    m_lastUnitMinutes(0),
    m_lastTenSeconds(0),
    m_lastUnitSeconds(0),
    m_lastTenths(0),
    m_lastHundreths(0),
    m_lastThousandths(0),
    m_lastTenThousandths(0),
    m_tempo(0)
{
    resetFonts();

    // set the LCD frame background to black
    //
    LCDBoxFrame->setBackgroundColor(Qt::black);

    // set all the pixmap backgrounds to black to avoid
    // flickering when we update
    //
    TenThousandthsPixmap->setBackgroundColor(Qt::black);
    ThousandthsPixmap->setBackgroundColor(Qt::black);
    HundredthsPixmap->setBackgroundColor(Qt::black);
    TenthsPixmap->setBackgroundColor(Qt::black);
    UnitSecondsPixmap->setBackgroundColor(Qt::black);
    TenSecondsPixmap->setBackgroundColor(Qt::black);
    UnitMinutesPixmap->setBackgroundColor(Qt::black);
    TenMinutesPixmap->setBackgroundColor(Qt::black);
    UnitHoursPixmap->setBackgroundColor(Qt::black);
    TenHoursPixmap->setBackgroundColor(Qt::black);
    NegativePixmap->setBackgroundColor(Qt::black);
 
    // unset the negative sign to begin with
    NegativePixmap->clear();

    // Set our toggle buttons
    //
    PlayButton->setToggleButton(true);
    RecordButton->setToggleButton(true);

    // read only tempo
    //
//    TempoLineEdit->setReadOnly(true);

    // fix and hold the size of the dialog
    //
    setMinimumSize(width(), height());
    setMaximumSize(width(), height());

    loadPixmaps();

    // Create Midi label timers
    m_midiInTimer = new QTimer(this);
    m_midiOutTimer = new QTimer(this);

    connect(m_midiInTimer, SIGNAL(timeout()),
            SLOT(clearMidiInLabel()));

    connect(m_midiOutTimer, SIGNAL(timeout()),
            SLOT(clearMidiOutLabel()));

    // clear labels
    //
    clearMidiInLabel();
    clearMidiOutLabel();
}

RosegardenTransportDialog::~RosegardenTransportDialog()
{
}

// Load the LCD pixmaps
//
void
RosegardenTransportDialog::loadPixmaps()
{
    m_lcdList.clear();
    QString fileName;
    QString pixmapDir = KGlobal::dirs()->findResource("appdata", "pixmaps/");

    for (int i = 0; i < 10; i++)
    {
      fileName = QString("%1/transport/led-%2.xpm").arg(pixmapDir).arg(i);
      if (!m_lcdList[i].load(fileName))
      {
        std::cerr << "RosegardenTransportDialog - failed to load pixmap \""
                  << fileName << "\""<< std::endl;
      }
    }

    // Load the "negative" sign pixmap
    //
    fileName = QString("%1/transport/led--.xpm").arg(pixmapDir);
    m_lcdNegative.load(fileName);

}

void
RosegardenTransportDialog::resetFonts()
{
    resetFont(TimeSigLabel);
    resetFont(TimeSigDisplay);
    resetFont(TempoLabel);
    resetFont(TempoDisplay);
    resetFont(DivisionLabel);
    resetFont(DivisionDisplay);
    resetFont(InLabel);
    resetFont(InDisplay);
    resetFont(OutLabel);
    resetFont(OutDisplay);
}

void
RosegardenTransportDialog::resetFont(QWidget *w)
{
    QFont font = w->font();
    font.setPixelSize(10);
    w->setFont(font);
}

// Display a time from a pointer position - calculates all
// the pixmap values and only updates if necessary
//
//
void
RosegardenTransportDialog::displayTime(const Rosegarden::RealTime &rt)
{
    Rosegarden::RealTime st = rt;

    // If time is negative then reverse the time and set the minus flag
    //
    if (st < Rosegarden::RealTime(0,0))
    {
        st = Rosegarden::RealTime(0,0) - st;
        NegativePixmap->setPixmap(m_lcdNegative);
    }
    else // don't show the flag
    {
        NegativePixmap->clear();
    }
         
    m_tenThousandths = ( st.usec / 100 ) % 10;
    m_thousandths = ( st.usec / 1000 ) % 10;
    m_hundreths = ( st.usec / 10000 ) % 10;
    m_tenths = ( st.usec / 100000 ) % 10;

    m_unitSeconds = ( st.sec ) % 10;
    m_tenSeconds = ( st.sec / 10 ) % 6;

    m_unitMinutes = ( st.sec / 60) % 10;
    m_tenMinutes = ( st.sec / 600) % 6;
    
    m_unitHours = ( st.sec / 3600) % 10;
    m_tenHours = (st.sec / 36000) % 24;

    if (m_tenThousandths != m_lastTenThousandths)
    {
        TenThousandthsPixmap->setPixmap(m_lcdList[m_tenThousandths]);
        m_lastTenThousandths = m_tenThousandths;
    }

    if (m_thousandths != m_lastThousandths)
    {
        ThousandthsPixmap->setPixmap(m_lcdList[m_thousandths]);
        m_lastThousandths = m_thousandths;
    }

    if (m_hundreths != m_lastHundreths)
    {
        HundredthsPixmap->setPixmap(m_lcdList[m_hundreths]);
        m_lastHundreths = m_hundreths;
    }

    if (m_tenths != m_lastTenths)
    {
        TenthsPixmap->setPixmap(m_lcdList[m_tenths]);
        m_lastTenths = m_tenths;
    }

    if (m_unitSeconds != m_lastUnitSeconds)
    {
        UnitSecondsPixmap->setPixmap(m_lcdList[m_unitSeconds]);
        m_lastUnitSeconds = m_unitSeconds;
    }
 
    if (m_tenSeconds != m_lastTenSeconds)
    {
        TenSecondsPixmap->setPixmap(m_lcdList[m_tenSeconds]);
        m_lastTenSeconds = m_tenSeconds;
    }

    if (m_unitMinutes != m_lastUnitMinutes)
    {
        UnitMinutesPixmap->setPixmap(m_lcdList[m_unitMinutes]);
        m_lastUnitMinutes = m_unitMinutes;
    }

    if (m_tenMinutes != m_lastTenMinutes)
    {
        TenMinutesPixmap->setPixmap(m_lcdList[m_tenMinutes]);
        m_lastTenMinutes = m_tenMinutes;
    }

    if (m_unitHours != m_lastUnitHours)
    {
        UnitHoursPixmap->setPixmap(m_lcdList[m_unitHours]);
        m_lastUnitHours = m_unitHours;
    }

    if (m_tenHours != m_lastTenHours)
    {
        TenHoursPixmap->setPixmap(m_lcdList[m_tenHours]);
        m_lastTenHours = m_tenHours;
    }

}

void
RosegardenTransportDialog::setTempo(const double &tempo)
{
    if (m_tempo == tempo) return;
    m_tempo = tempo;

    QString tempoString;
    tempoString.sprintf("%4.3f", tempo);
    TempoDisplay->setText(tempoString);
}


// Set the midi label to this MappedEvent
//
void
RosegardenTransportDialog::setMidiInLabel(const Rosegarden::MappedEvent *mE)
{
    assert(mE > 0);

    MidiPitchLabel *midiPitchLabel = new MidiPitchLabel(mE->getPitch());
    InDisplay->setText(midiPitchLabel->getQString() +
		       QString("  %1").arg(mE->getVelocity()));

    // Reset the timer if it's already running
    //
    if (m_midiInTimer->isActive())
        m_midiInTimer->stop();

    // 1.5 second timeout for MIDI event
    //
    m_midiInTimer->start(1500, true);
}


// Clear the MIDI in label - used as timer callback
//
void
RosegardenTransportDialog::clearMidiInLabel()
{
    InDisplay->setText(i18n(QString("NO EVENTS")));
}


// Set the outgoing MIDI label
//
void
RosegardenTransportDialog::setMidiOutLabel(const Rosegarden::MappedEvent *mE)
{
    assert(mE > 0);

    MidiPitchLabel *midiPitchLabel = new MidiPitchLabel(mE->getPitch());
    OutDisplay->setText(midiPitchLabel->getQString() +
			QString("  %1").arg(mE->getVelocity()));

    // Reset the timer if it's already running
    //
    if (m_midiOutTimer->isActive())
        m_midiOutTimer->stop();

    // 1.5 second timeout
    //
    m_midiOutTimer->start(200, true);
}


// Clear the outgoing MIDI label
//
void
RosegardenTransportDialog::clearMidiOutLabel()
{
    OutDisplay->setText(i18n(QString("NO EVENTS")));
}

void
RosegardenTransportDialog::closeEvent (QCloseEvent * e)
{
    e->accept();  // accept the close event here
    emit closed();
}


}


