// -*- c-basic-offset: 4 -*-

/*
    Rosegarden-4
    A sequencer and musical notation editor.

    This program is Copyright 2000-2003
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

#include "velocitycolour.h"

#ifndef _VUMETER_H_
#define _VUMETER_H_

class VUMeter : public QLabel
{
Q_OBJECT

public:
    typedef enum
    {
        Plain,
        PeakHold,
        AudioPeakHold
    } VUMeterType;

    typedef enum
    {
        Horizontal,
        Vertical
    } VUAlignment;

    // Mono and stereo level setting
    //
    void setLevel(double level);
    void setLevel(double leftLevel, double rightLevel);

    virtual void paintEvent(QPaintEvent*);

protected:
    // Constructor is protected - we can only create an object
    // from a sub-class of this type from a sub-class.
    //
    VUMeter(QWidget *parent = 0,
            VUMeterType type = Plain,
            bool stereo = false,
            int width = 0,
            int height = 0,
            VUAlignment alignment = Horizontal,
            const char *name = 0);
    ~VUMeter();

    virtual void meterStart() = 0;
    virtual void meterStop() = 0;

    int         m_originalHeight;

private slots:
    void slotReduceLevelLeft();
    void slotStopShowingPeakLeft();

    void slotReduceLevelRight();
    void slotStopShowingPeakRight();

private:

    void drawMeterLevel(QPainter* paint);

    VUMeterType m_type;
    VUAlignment m_alignment;

    int         m_levelLeft;          // percentage
    int         m_peakLevelLeft;      // percentage
    int         m_levelStepLeft;
    QTimer     *m_fallTimerLeft;
    QTimer     *m_peakTimerLeft;

    int         m_levelRight;          // percentage
    int         m_peakLevelRight;      // percentage
    int         m_levelStepRight;
    QTimer     *m_fallTimerRight;
    QTimer     *m_peakTimerRight;

    bool        m_showPeakLevel;
    int         m_baseLevelStep;

    bool        m_stereo;

    // We use this to work out our colours
    //
    VelocityColour *m_velocityColour;


};

#endif // _VUMETER_H_
