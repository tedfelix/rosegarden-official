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

#ifndef _ROSEGARDENTRANSPORTDIALOG_H_
#define _ROSEGARDENTRANSPORTDIALOG_H_

#include "rosegardentransport.h"
#include <map>
#include <qpixmap.h>

namespace Rosegarden
{

class RosegardenTransportDialog : public RosegardenTransport
{
public:
  RosegardenTransportDialog(QWidget *parent=0,
                            const char *name=0,
                            const double &ppq = 384.0);
  ~RosegardenTransportDialog();

  void displayTime(const int &position);

  void setTempo(const double &tempo);

private:
  void loadPixmaps();

  map<int, QPixmap> m_lcdList;

  int m_lastTenHours;
  int m_lastUnitHours;
  int m_lastTenMinutes;
  int m_lastUnitMinutes;
  int m_lastTenSeconds;
  int m_lastUnitSeconds;
  int m_lastTenths;
  int m_lastHundreths;
  int m_lastThousandths;
  int m_lastTenThousandths;

  int m_tenHours;
  int m_unitHours;
  int m_tenMinutes;
  int m_unitMinutes;
  int m_tenSeconds;
  int m_unitSeconds;
  int m_tenths;
  int m_hundreths;
  int m_thousandths;
  int m_tenThousandths;


  double m_tempo;
  double m_ppq;

  int m_microSeconds;

};

}
 


#endif // _ROSEGARDENTRANSPORTDIALOG_H_
