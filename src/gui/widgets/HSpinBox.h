
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

#ifndef _RG_HSPINBOX_H_
#define _RG_HSPINBOX_H_

#include <qobject.h>
#include <qspinbox.h>
#include <qstring.h>
#include <qvalidator.h>


namespace Rosegarden
{

class HSpinBox : public QSpinBox
{
    QString mapValueToText(int j);

    int mapTextToValue( bool* ok );
  
public:
    HSpinBox( int minV, int maxV, int step, QWidget* parent,
              double bottom, double top, int decimals, float initialValue);

  //constructor with default settings
    HSpinBox( QWidget* parent,  float initialValue = 0.2, int step=1, 
              double bottom=-25.0, double top=25.0, int decimals=3,
              const QObject* recv=NULL, const char* mem=NULL);
  
    float valuef();
    void setValuef(float v);
    void initialize(int digits);

private: 

    float   m_scaleFactor; //scale of the value
    char    m_format[3];   //text format
  
};


}

#endif
