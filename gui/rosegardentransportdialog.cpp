// -*- c-basic-offset: 4 -*-

/*
    Rosegarden-4 v0.1
    A sequencer and musical notation editor.

    This program is Copyright 2000-2001
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
#include <kglobal.h>
#include <kstddirs.h>
#include <klocale.h>

#include <qlabel.h>
#include <qpushbutton.h>
#include <qspinbox.h>
#include <qlineedit.h>

#include <iostream>

namespace Rosegarden
{

RosegardenTransportDialog::RosegardenTransportDialog(QWidget *parent,
                                                     const char *name,
                                                     const double &ppq):
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
    m_ppq(ppq)
{
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

    // create the playbutton as toggleable
    //
    PlayButton->setToggleButton(true);

    // read only tempo
    //
    TempoLineEdit->setReadOnly(true);

    // fix and hold the size of the dialog
    //
    setMinimumSize(width(), height());
    setMaximumSize(width(), height());

    loadPixmaps();
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
        cerr << "RosegardenTransportDialog - failed to load pixmap \""
             << fileName << "\""<< endl;
      }
    }
}

// Display a time from a pointer position - calculates all
// the pixmap values and only updates if necessary
//
//
void
RosegardenTransportDialog::displayTime(const int &position)
{
    // work out the current time in seconds
    //
    m_microSeconds = (unsigned long long)(((double)60000000.0*(double)position)/
                                       ((double)m_ppq * (double)m_tempo));

    m_tenThousandths = ( m_microSeconds / 100 ) % 10;
    m_thousandths = ( m_microSeconds / 1000 ) % 10;
    m_hundreths = ( m_microSeconds / 10000 ) % 10;
    m_tenths = ( m_microSeconds / 100000 ) % 10;

    m_unitSeconds = ( m_microSeconds / 1000000 ) % 10;
    m_tenSeconds = ( m_microSeconds / 10000000 ) % 6;

    m_unitMinutes = (m_microSeconds / 60000000) % 10;
    m_tenMinutes = (m_microSeconds / 600000000) % 6;
    
    m_unitHours = ((unsigned int)m_microSeconds / 3600000000LL) % 10;
    m_tenHours = ((unsigned int)m_microSeconds / 36000000000LL) % 24;

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
  m_tempo = tempo;

  QString tempoString;
  tempoString.sprintf("%4.4f", tempo);
  TempoLineEdit->setText(tempoString);
}


}
