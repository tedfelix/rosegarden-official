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


#include <qlabel.h>
#include <qpainter.h>
#include <qtimer.h>

#ifndef _VUMETER_H_
#define _VUMETER_H_

class VUMeter : public QLabel
{
Q_OBJECT

public:
    typedef enum
    {
        Plain,
        PeakHold
    } VUMeterType;

    VUMeter(QWidget *parent = 0,
            const VUMeterType &type = Plain,
            const int &width = 0,
            const int &height = 0,
            const char *name = 0);
    ~VUMeter();

    void setLevel(const double &level);

    virtual void paintEvent(QPaintEvent*);

protected:
    virtual void meterStart() = 0;
    virtual void meterStop() = 0;

private slots:
    void reduceLevel();
    void stopShowingPeak();

protected:
    int         m_originalHeight;

private:

    void drawMeterLevel(QPainter* paint);

    VUMeterType m_type;
    int         m_level;          // percentage
    int         m_peakLevel;      // percentage
    int         m_levelStep;
    QTimer      m_fallTimer;
    QTimer      m_peakTimer;
    bool        m_showPeakLevel;

    // At what percentage we cut from low to high band colour
    //
    int         m_colourKnee;

    // Convenient storage of mix colours and steps
    //
    double      m_loStartRed;
    double      m_loStartGreen;
    double      m_loStartBlue;
    double      m_loStepRed;
    double      m_loStepGreen;
    double      m_loStepBlue;

    double      m_hiStartRed;
    double      m_hiStartGreen;
    double      m_hiStartBlue;
    double      m_hiStepRed;
    double      m_hiStepGreen;
    double      m_hiStepBlue;


};

#endif // _VUMETER_H_
