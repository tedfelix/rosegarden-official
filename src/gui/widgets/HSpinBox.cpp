/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2008 the Rosegarden development team.
 
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

#include <iostream>

namespace Rosegarden
{

QString HSpinBox::mapValueToText(int j)
{
    QString str = QString("%1").arg(double(j) / m_scaleFactor,
                                    m_digits + 3, 'f', m_digits);
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
    : QSpinBox(minV,maxV,step,parent),
      m_scaleFactor(1),
      m_digits(0)
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
                 (int)(top*pow(10.0, decimals)), step, parent),
      m_scaleFactor(1),
      m_digits(0)
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
    m_digits = digits;
    m_scaleFactor = pow(10.0, digits);
}
        
        
}
