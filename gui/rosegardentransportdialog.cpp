// -*- c-basic-offset: 4 -*-

/*
    Rosegarden-4
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

#include "rosestrings.h"
#include "rosegardentransportdialog.h"
#include "MidiPitchLabel.h"
#include <kglobal.h>
#include <kstddirs.h>
#include <klocale.h>

#include <iostream>

#include <qlabel.h>
#include <qpushbutton.h>
#include <qspinbox.h>
#include <qlineedit.h>
#include <qtimer.h>

#include "widgets.h"

namespace Rosegarden
{

RosegardenTransportDialog::RosegardenTransportDialog(QWidget *parent,
                                                     const char *name,
                                                     WFlags flags):
    RosegardenTransport(parent, name, flags),
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
    m_lastNegative(false),
    m_lastMode(RealMode),
    m_currentMode(RealMode),
    m_tempo(0),
    m_framesPerSecond(24),
    m_bitsPerFrame(80)
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
            SLOT(slotClearMidiInLabel()));

    connect(m_midiOutTimer, SIGNAL(timeout()),
            SLOT(slotClearMidiOutLabel()));

    TimeDisplayLabel->hide();
    ToEndLabel->hide();

    connect(TimeDisplayButton, SIGNAL(released()),
	    SLOT(slotChangeTimeDisplay()));

    connect(ToEndButton, SIGNAL(released()),
	    SLOT(slotChangeToEnd()));

    connect(LoopButton, SIGNAL(released()),
            SLOT(slotLoopButtonReleased()));

    connect(PanelOpenButton, SIGNAL(released()),
	    SLOT(slotPanelOpenButtonReleased()));

    connect(PanelCloseButton, SIGNAL(released()),
	    SLOT(slotPanelCloseButtonReleased()));

    connect(PanicButton, SIGNAL(released()), SIGNAL(panic()));

    m_panelOpen = *PanelOpenButton->pixmap();
    m_panelClosed = *PanelCloseButton->pixmap();

    // clear labels
    //
    slotClearMidiInLabel();
    slotClearMidiOutLabel();

    // and by default we close the lower panel
    //
    int rfh = RecordingFrame->height();
    RecordingFrame->hide();
    setFixedSize(width(), height() - rfh);
    PanelOpenButton->setOn(false);
    PanelOpenButton->setPixmap(m_panelClosed);

    // and since by default we show real time (not SMPTE), by default
    // we hide the small colon pixmaps
    //
    SecondColonPixmap->hide();
    HundredthColonPixmap->hide();

    // We have to specify these settings in this class (copied
    // from rosegardentransport.cpp) as we're using a specialised
    // widgets for TempoDisplay.  Ugly but works - does mean that
    // if the rest of the Transport ever changes then this code
    // will have to as well.
    //
    //
    QPalette pal;
    pal.setColor(QColorGroup::Foreground, QColor(192, 216, 255));

    TempoDisplay->setPalette(pal);
    TempoDisplay->setAlignment(int(QLabel::AlignVCenter | QLabel::AlignRight));

    TimeSigDisplay->setPalette(pal);
    TimeSigDisplay->setAlignment(int(QLabel::AlignVCenter | QLabel::AlignRight));

    QFont localFont(  OutDisplay->font() );
    localFont.setFamily( "lucida" );
    localFont.setBold( TRUE );

    TempoDisplay->setFont( localFont );
    TimeSigDisplay->setFont( localFont );

    // Now the reason why we have to do the above fiddling
    connect(TempoDisplay, SIGNAL(doubleClicked()),
            this, SLOT(slotEditTempo()));

    // Now the reason why we have to do the above fiddling
    connect(TimeSigDisplay, SIGNAL(doubleClicked()),
            this, SLOT(slotEditTimeSignature()));

    // accelerator object
    //
    m_accelerators = new QAccel(this);
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
    resetFont(ToEndLabel);
    resetFont(TimeDisplayLabel);
}

void
RosegardenTransportDialog::resetFont(QWidget *w)
{
    QFont font = w->font();
    font.setPixelSize(10);
    w->setFont(font);
}

void
RosegardenTransportDialog::setSMPTEResolution(int framesPerSecond,
					      int bitsPerFrame)
{
    m_framesPerSecond = framesPerSecond;
    m_bitsPerFrame = bitsPerFrame;
}

void
RosegardenTransportDialog::getSMPTEResolution(int &framesPerSecond,
					      int &bitsPerFrame)
{
    framesPerSecond = m_framesPerSecond;
    bitsPerFrame = m_bitsPerFrame;
}

void
RosegardenTransportDialog::slotChangeTimeDisplay()
{
    switch (m_currentMode) {
    case RealMode:
	TimeDisplayLabel->setText("SMPTE");
	TimeDisplayLabel->show();
	m_currentMode = SMPTEMode;
	break;

    case SMPTEMode:
	TimeDisplayLabel->setText("BAR");
	TimeDisplayLabel->show();
	m_currentMode = BarMode;
	break;

    case BarMode:
	TimeDisplayLabel->hide();
	m_currentMode = RealMode;
	break;
    }
}

void
RosegardenTransportDialog::slotChangeToEnd()
{
    if (ToEndButton->isOn()) {
	ToEndLabel->show();
    } else {
	ToEndLabel->hide();
    }
}

bool
RosegardenTransportDialog::isShowingTimeToEnd()
{
    return ToEndButton->isOn();
}

void
RosegardenTransportDialog::displayRealTime(const Rosegarden::RealTime &rt)
{
    Rosegarden::RealTime st = rt;

    if (m_lastMode != RealMode) {
	HourColonPixmap->show();
	SecondColonPixmap->hide();
	HundredthColonPixmap->hide();
	m_lastMode = RealMode;
    }

    // If time is negative then reverse the time and set the minus flag
    //
    if (st < Rosegarden::RealTime(0,0))
    {
        st = Rosegarden::RealTime(0,0) - st;
        if (!m_lastNegative) {
	    NegativePixmap->setPixmap(m_lcdNegative);
	    m_lastNegative = true;
	}
    }
    else // don't show the flag
    {
	if (m_lastNegative) {
	    NegativePixmap->clear();
	    m_lastNegative = false;
	}
    }
         
    m_tenThousandths = ( st.usec / 100 ) % 10;
    m_thousandths = ( st.usec / 1000 ) % 10;
    m_hundreths = ( st.usec / 10000 ) % 10;
    m_tenths = ( st.usec / 100000 ) % 10;

    m_unitSeconds = ( st.sec ) % 10;
    m_tenSeconds = ( st.sec / 10 ) % 6;

    m_unitMinutes = ( st.sec / 60 ) % 10;
    m_tenMinutes = ( st.sec / 600 ) % 6;
    
    m_unitHours = ( st.sec / 3600 ) % 10;
    m_tenHours = (st.sec / 36000 ) % 10;
    
    updateTimeDisplay();
}

void
RosegardenTransportDialog::displaySMPTETime(const Rosegarden::RealTime &rt)
{
    Rosegarden::RealTime st = rt;

    if (m_lastMode != SMPTEMode) {
	HourColonPixmap->show();
	SecondColonPixmap->show();
	HundredthColonPixmap->show();
	m_lastMode = SMPTEMode;
    }

    // If time is negative then reverse the time and set the minus flag
    //
    if (st < Rosegarden::RealTime(0,0))
    {
        st = Rosegarden::RealTime(0,0) - st;
        if (!m_lastNegative) {
	    NegativePixmap->setPixmap(m_lcdNegative);
	    m_lastNegative = true;
	}
    }
    else // don't show the flag
    {
	if (m_lastNegative) {
	    NegativePixmap->clear();
	    m_lastNegative = false;
	}
    }

    m_tenThousandths =
	(( st.usec * m_framesPerSecond * m_bitsPerFrame) / 1000000 ) % 10;
    m_thousandths =
	(( st.usec * m_framesPerSecond * m_bitsPerFrame) / 10000000 ) % 
	(m_bitsPerFrame / 10);
    m_hundreths =
	(( st.usec * m_framesPerSecond) / 1000000 ) % 10;
    m_tenths = 
	(( st.usec * m_framesPerSecond) / 10000000 ) % 10;

    m_unitSeconds = ( st.sec ) % 10;
    m_tenSeconds = ( st.sec / 10 ) % 6;

    m_unitMinutes = ( st.sec / 60 ) % 10;
    m_tenMinutes = ( st.sec / 600 ) % 6;
    
    m_unitHours = ( st.sec / 3600 ) % 10;
    m_tenHours = ( st.sec / 36000 ) % 10;
    
    updateTimeDisplay();
}

void
RosegardenTransportDialog::displayBarTime(int bar, int beat, int unit)
{
    if (m_lastMode != BarMode) {
	HourColonPixmap->hide();
	SecondColonPixmap->hide();
	HundredthColonPixmap->hide();
	m_lastMode = BarMode;
    }

    // If time is negative then reverse the time and set the minus flag
    //
    if (bar < 0)
    {
        bar = -bar;
        if (!m_lastNegative) {
	    NegativePixmap->setPixmap(m_lcdNegative);
	    m_lastNegative = true;
	}
    }
    else // don't show the flag
    {
	if (m_lastNegative) {
	    NegativePixmap->clear();
	    m_lastNegative = false;
	}
    }

    m_tenThousandths = ( unit ) % 10;
    m_thousandths = ( unit / 10 ) % 10;
    m_hundreths = ( unit / 100 ) % 10;
    m_tenths = ( unit / 1000 ) % 10;

    if (m_tenths == 0) {
	m_tenths = -1;
	if (m_hundreths == 0) {
	    m_hundreths = -1;
	    if (m_thousandths == 0) {
		m_thousandths = -1;
	    }
	}
    }

    m_unitSeconds = ( beat ) % 10;
    m_tenSeconds = ( beat / 10 ) % 6;

    if (m_tenSeconds == 0) {
	m_tenSeconds = -1;
    }

    m_unitMinutes = ( bar ) % 10;
    m_tenMinutes = ( bar / 10 ) % 10;
    
    m_unitHours = ( bar / 100 ) % 10;
    m_tenHours = ( bar / 1000 ) % 10;
    
    if (m_tenHours == 0) {
	m_tenHours = -1;
	if (m_unitHours == 0) {
	    m_unitHours = -1;
	    if (m_tenMinutes == 0) {
		m_tenMinutes = -1;
	    }
	}
    }

    updateTimeDisplay();
}

void
RosegardenTransportDialog::updateTimeDisplay()
{
    if (m_tenThousandths != m_lastTenThousandths)
    {
	if (m_tenThousandths < 0) TenThousandthsPixmap->clear();
        else TenThousandthsPixmap->setPixmap(m_lcdList[m_tenThousandths]);
        m_lastTenThousandths = m_tenThousandths;
    }

    if (m_thousandths != m_lastThousandths)
    {
	if (m_thousandths < 0) ThousandthsPixmap->clear();
	else ThousandthsPixmap->setPixmap(m_lcdList[m_thousandths]);
        m_lastThousandths = m_thousandths;
    }

    if (m_hundreths != m_lastHundreths)
    {
	if (m_hundreths < 0) HundredthsPixmap->clear();
        else HundredthsPixmap->setPixmap(m_lcdList[m_hundreths]);
        m_lastHundreths = m_hundreths;
    }

    if (m_tenths != m_lastTenths)
    {
	if (m_tenths < 0) TenthsPixmap->clear();
        else TenthsPixmap->setPixmap(m_lcdList[m_tenths]);
        m_lastTenths = m_tenths;
    }

    if (m_unitSeconds != m_lastUnitSeconds)
    {
	if (m_unitSeconds < 0) UnitSecondsPixmap->clear();
        else UnitSecondsPixmap->setPixmap(m_lcdList[m_unitSeconds]);
        m_lastUnitSeconds = m_unitSeconds;
    }
 
    if (m_tenSeconds != m_lastTenSeconds)
    {
	if (m_tenSeconds < 0) TenSecondsPixmap->clear();
        else TenSecondsPixmap->setPixmap(m_lcdList[m_tenSeconds]);
        m_lastTenSeconds = m_tenSeconds;
    }

    if (m_unitMinutes != m_lastUnitMinutes)
    {
        if (m_unitMinutes < 0) UnitMinutesPixmap->clear();
        else UnitMinutesPixmap->setPixmap(m_lcdList[m_unitMinutes]);
        m_lastUnitMinutes = m_unitMinutes;
    }

    if (m_tenMinutes != m_lastTenMinutes)
    {
        if (m_tenMinutes < 0) TenMinutesPixmap->clear();
        else TenMinutesPixmap->setPixmap(m_lcdList[m_tenMinutes]);
        m_lastTenMinutes = m_tenMinutes;
    }

    if (m_unitHours != m_lastUnitHours)
    {
        if (m_unitHours < 0) UnitHoursPixmap->clear();
        else UnitHoursPixmap->setPixmap(m_lcdList[m_unitHours]);
        m_lastUnitHours = m_unitHours;
    }

    if (m_tenHours != m_lastTenHours)
    {
        if (m_tenHours < 0) TenHoursPixmap->clear();
        else TenHoursPixmap->setPixmap(m_lcdList[m_tenHours]);
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

void
RosegardenTransportDialog::setTimeSignature(const Rosegarden::TimeSignature &timeSig)
{
    int numerator = timeSig.getNumerator();
    int denominator = timeSig.getDenominator();
    if (m_numerator == numerator && m_denominator == denominator) return;
    m_numerator = numerator;
    m_denominator = denominator;

    QString timeSigString;
    timeSigString.sprintf("%d/%d", numerator, denominator);
    TimeSigDisplay->setText(timeSigString);
}


// Set the midi label to this MappedEvent
//
void
RosegardenTransportDialog::setMidiInLabel(const Rosegarden::MappedEvent *mE)
{
    assert(mE > 0);

    switch (mE->getType())
    {
        case MappedEvent::MidiNote:
        case MappedEvent::MidiNoteOneShot:
            {
                // don't do anything if we've got an effective NOTE OFF
                //
                if (mE->getVelocity() == 0) return;

                MidiPitchLabel *mPL = new MidiPitchLabel(mE->getPitch());
                InDisplay->setText(mPL->getQString() +
		       QString("  %1").arg(mE->getVelocity()));
            }
            break;

        case MappedEvent::MidiPitchBend:
            {
                InDisplay->setText(i18n("PITCH WHEEL"));
            }
            break;

        case MappedEvent::MidiController:
            {
                InDisplay->setText(i18n("CONTROLLER"));
            }
            break;
        case MappedEvent::MidiProgramChange:
            {
                InDisplay->setText(i18n("PROG CHNGE"));
            }
            break;

        case MappedEvent::MidiKeyPressure:
        case MappedEvent::MidiChannelPressure:
            {
                InDisplay->setText(i18n("PRESSURE"));
            }
            break;

        case MappedEvent::MidiSystemExclusive:
            {
                InDisplay->setText(i18n("SYS EXCLSVE"));
            }
            break;


        default:  // do nothing
            return;
    }

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
RosegardenTransportDialog::slotClearMidiInLabel()
{
    InDisplay->setText(i18n(QString("NO EVENTS")));
}


// Set the outgoing MIDI label
//
void
RosegardenTransportDialog::setMidiOutLabel(const Rosegarden::MappedEvent *mE)
{
    assert(mE > 0);

    switch (mE->getType())
    {
        case MappedEvent::MidiNote:
        case MappedEvent::MidiNoteOneShot:
            {
                MidiPitchLabel *mPL = new MidiPitchLabel(mE->getPitch());
                OutDisplay->setText(mPL->getQString() +
		       QString("  %1").arg(mE->getVelocity()));
            }
            break;

        case MappedEvent::MidiPitchBend:
            {
                OutDisplay->setText(i18n("PITCH WHEEL"));
            }
            break;

        case MappedEvent::MidiController:
            {
                OutDisplay->setText(i18n("CONTROLLER"));
            }
            break;
        case MappedEvent::MidiProgramChange:
            {
                OutDisplay->setText(i18n("PROG CHNGE"));
            }
            break;

        case MappedEvent::MidiKeyPressure:
        case MappedEvent::MidiChannelPressure:
            {
                OutDisplay->setText(i18n("PRESSURE"));
            }
            break;

        case MappedEvent::MidiSystemExclusive:
            {
                OutDisplay->setText(i18n("SYS EXCLSVE"));
            }
            break;

        default:  // do nothing
            return;
    }

    // Reset the timer if it's already running
    //
    if (m_midiOutTimer->isActive())
        m_midiOutTimer->stop();

    // 200 millisecond timeout
    //
    m_midiOutTimer->start(200, true);
}


// Clear the outgoing MIDI label
//
void
RosegardenTransportDialog::slotClearMidiOutLabel()
{
    OutDisplay->setText(i18n(QString("NO EVENTS")));
}

void
RosegardenTransportDialog::closeEvent (QCloseEvent * e)
{
    e->accept();  // accept the close event here
    emit closed();
}

void
RosegardenTransportDialog::slotLoopButtonReleased()
{
    if (LoopButton->isOn())
    {
        emit setLoop();
    }
    else
    {
        emit unsetLoop();
    }
}

void
RosegardenTransportDialog::slotPanelOpenButtonReleased()
{
    int rfh = RecordingFrame->height();

    if (RecordingFrame->isVisible()) {
	RecordingFrame->hide();
	setFixedSize(width(), height() - rfh);
	PanelOpenButton->setPixmap(m_panelClosed);
    } else {
	setFixedSize(width(), height() + rfh);
	RecordingFrame->show();
	PanelOpenButton->setPixmap(m_panelOpen);
    }
}

void
RosegardenTransportDialog::slotPanelCloseButtonReleased()
{
    int rfh = RecordingFrame->height();

    if (RecordingFrame->isVisible()) {
	RecordingFrame->hide();
	setFixedSize(width(), height() - rfh);
	PanelOpenButton->setOn(false);
	PanelOpenButton->setPixmap(m_panelClosed);
    }
}


bool 
RosegardenTransportDialog::isExpanded()
{
    return (RecordingFrame->isVisible());
}

void
RosegardenTransportDialog::slotEditTempo()
{
    emit editTempo(this);
}

void
RosegardenTransportDialog::slotEditTimeSignature()
{
    emit editTimeSignature(this);
}

}


