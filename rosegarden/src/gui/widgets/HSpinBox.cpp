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


#include "HSpinBox.h"

#include <qstring.h>
#include <cmath>
#include <algorithm>

namespace Rosegarden
{

QString HSpinBox::mapValueToText(int j)
{
    QString str;
    str.sprintf(m_format, float(j) / m_scaleFactor);
    return str;
}

int HSpinBox::mapTextToValue( bool* ok )
{
    *ok = true;
    float f = atof(text());
    return int(f * m_scaleFactor);
}
  
HSpinBox::HSpinBox( int minV, int maxV, int step, QWidget* parent,
              double bottom, double top, int decimals, float initialValue)
      : QSpinBox(minV,maxV,step,parent)
{
    setValidator(new QDoubleValidator(bottom,top,decimals,this));
    initialize(decimals);
    setValuef(initialValue);
}

  //constructor with default settings
HSpinBox::HSpinBox( QWidget* parent,  float initialValue, int step, 
              double bottom, double top, int decimals,
              const QObject* recv, const char* mem)
      : QSpinBox((int)(bottom*pow(10.0, decimals)), 
                 (int)(top*pow(10.0, decimals)), step, parent)
{
    setValidator(new QDoubleValidator(bottom,top,decimals,this));
    initialize(decimals);
    setValuef(initialValue);
    if (recv != NULL && mem != NULL)
      QObject::connect(this, SIGNAL(valueChanged(int)), recv, mem);
}
  
float HSpinBox::valuef() { return float(value()) / m_scaleFactor; }
void HSpinBox::setValuef(float v) { setValue(static_cast<int>(v * m_scaleFactor)); }

void HSpinBox::initialize(int digits) {
    m_scaleFactor = pow(10.0, digits);
    sprintf(m_format, "%c%1i.%1if", '%', digits+3, digits);
}
        
        
}
