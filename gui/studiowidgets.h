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

#ifndef _STUDIOWIDGETS_H_
#define _STUDIOWIDGETS_H_

#include <qwidget.h>
#include <qpushbutton.h>

#include "widgets.h"
#include "trackvumeter.h"

/*
class AudioVUMeter : public VUMeter 
{
public:
    AudioVUMeter(QWidget *parent);

    virtual void meterStart();
    virtual void meterStop();

protected:

};
*/


class AudioFaderWidget : public QWidget
{
    Q_OBJECT

public:
    AudioFaderWidget(QWidget *parent, const char *name=0);

protected:
    virtual void paintEvent(QPaintEvent *e);
    virtual void mousePressEvent(QMouseEvent *e);
    virtual void mouseReleaseEvent(QMouseEvent *e);
    virtual void mouseMoveEvent(QMouseEvent *e);
    virtual void wheelEvent(QWheelEvent *e);

    std::vector<QPushButton*>  m_plugins;
    AudioVUMeter              *m_vuMeter;
    QSlider                   *m_fader;
    QPushButton               *m_muteButton;
    QPushButton               *m_soloButton;
    QPushButton               *m_stereoButton;
    RosegardenRotary          *m_pan;

};


#endif // _STUDIOWIDGETS_H_
