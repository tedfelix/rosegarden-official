// -*- c-basic-offset: 4 -*-

/*
    Rosegarden-4
    A sequencer and musical notation editor.

    This program is Copyright 2000-2004
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

#ifndef _COMMONDIALOGS_H_
#define _COMMONDIALOGS_H_

#include <kdialogbase.h>
#include <qstring.h>
#include <qcanvas.h>
#include <qspinbox.h>
#include <qvalidator.h>

#include <cmath>
#include <string>

#include "NotationTypes.h"
#include "editcommands.h"
#include "notepixmapfactory.h"

// Definitions of various simple dialogs that may be used in multiple
// different editing views.
//
//


// HSpinBox courtesy of Kevin Liang (xkliang@rhpcs.mcmaster.ca)
//
// http://hal.rhpcs.mcmaster.ca/~xkliang/epidemics/html/classHSpinBox.html
//
//

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

class RosegardenFloatEdit : public KDialogBase
{
    Q_OBJECT

public:
    RosegardenFloatEdit(QWidget *parent,
                        const QString &title,
                        const QString &text,
                        float min,
                        float max,
                        float value,
                        float step);

    float getValue() const;

protected:

    QLabel            *m_text;
    HSpinBox          *m_spin;
};


#endif   // _COMMONDIALOGS_H_
