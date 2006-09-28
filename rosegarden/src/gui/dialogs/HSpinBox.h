
/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.

    This program is Copyright 2000-2006
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

#ifndef _RG_HSPINBOX_H_
#define _RG_HSPINBOX_H_

#include <qobject.h>
#include <qspinbox.h>
#include <qstring.h>


class QWidget;
class NULL;


namespace Rosegarden
{



class HSpinBox : public QSpinBox
{
    QString mapValueToText(int j)
    {
        QString str;
        str.sprintf(m_format, float(j) / m_scaleFactor);
        return str;
    }

    int mapTextToValue( bool* ok )
    {
        *ok = true;
        float f = atof(text());
        return int(f * m_scaleFactor);
    }
  
public:
    HSpinBox( int minV, int maxV, int step, QWidget* parent,
              double bottom, double top, int decimals, float initialValue)
      : QSpinBox(minV,maxV,step,parent)
    {
        setValidator(new QDoubleValidator(bottom,top,decimals,this));
        initialize(decimals);
        setValuef(initialValue);
    }

  //constructor with default settings
    HSpinBox( QWidget* parent,  float initialValue = 0.2, int step=1, 
              double bottom=-25.0, double top=25.0, int decimals=3,
              const QObject* recv=NULL, const char* mem=NULL)
      : QSpinBox((int)(bottom*pow(10.0, decimals)), 
                 (int)(top*pow(10.0, decimals)), step, parent)
    {
        setValidator(new QDoubleValidator(bottom,top,decimals,this));
        initialize(decimals);
        setValuef(initialValue);
        if (recv != NULL && mem != NULL)
          QObject::connect(this, SIGNAL(valueChanged(int)), recv, mem);
    }
  
    float valuef() { return float(value()) / m_scaleFactor; }
    void setValuef(float v) { setValue(static_cast<int>(v * m_scaleFactor)); }

private:
    void initialize(int digits) {
        m_scaleFactor = pow(10.0, digits);
        sprintf(m_format, "%c%1i.%1if", '%', digits+3, digits);
    }

private: 

    float   m_scaleFactor; //scale of the value
    char    m_format[3];   //text format
  
};


}

#endif
