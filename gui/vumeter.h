// -*- c-basic-offset: 4 -*-

/*
    Rosegarden-4
    A sequencer and musical notation editor.

    This program is Copyright 2000-2006
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
	AudioPeakHoldShort, 
        AudioPeakHoldLong,
	AudioPeakHoldIEC,
	AudioPeakHoldIECLong,
        FixedHeightVisiblePeakHold
    } VUMeterType;

    typedef enum
    {
        Horizontal,
        Vertical
    } VUAlignment;

    // Mono and stereo level setting.  The AudioPeakHold meter types
    // expect levels in dB; other types expect levels between 0 and 1.
    //
    void setLevel(double level);
    void setLevel(double leftLevel, double rightLevel);

    // Mono and stereo record level setting.  Same units.  Only
    // applicable if hasRecord true in constructor.
    //
    void setRecordLevel(double level);
    void setRecordLevel(double leftLevel, double rightLevel);

    virtual void paintEvent(QPaintEvent*);

protected:
    // Constructor is protected - we can only create an object
    // from a sub-class of this type from a sub-class.
    //
    VUMeter(QWidget *parent = 0,
            VUMeterType type = Plain,
            bool stereo = false,
	    bool hasRecord = false,
            int width = 0,
            int height = 0,
            VUAlignment alignment = Horizontal,
            const char *name = 0);
    ~VUMeter();

    virtual void meterStart() = 0;
    virtual void meterStop() = 0;

    int         m_originalHeight;
    bool        m_active;

    void setLevel(double leftLevel, double rightLevel, bool record);

private slots:
    void slotReduceLevelLeft();
    void slotStopShowingPeakLeft();

    void slotReduceLevelRight();
    void slotStopShowingPeakRight();

private:

    void drawMeterLevel(QPainter *paint);
    void drawColouredBar(QPainter *paint, int channel,
			 int x, int y, int w, int h);

    VUMeterType m_type;
    VUAlignment m_alignment;
    QColor      m_background;

    short       m_maxLevel;

    short       m_levelLeft;
    short       m_recordLevelLeft;
    short       m_peakLevelLeft;
    short       m_levelStepLeft;
    short       m_recordLevelStepLeft;
    QTimer     *m_fallTimerLeft;
    QTimer     *m_peakTimerLeft;

    short       m_levelRight;
    short       m_recordLevelRight;
    short       m_peakLevelRight;
    short       m_levelStepRight;
    short       m_recordLevelStepRight;
    QTimer     *m_fallTimerRight;
    QTimer     *m_peakTimerRight;

    bool        m_showPeakLevel;
    short       m_baseLevelStep;

    bool        m_stereo;
    bool        m_hasRecord;

    // We use this to work out our colours
    //
    VelocityColour *m_velocityColour;


};

#endif // _VUMETER_H_
