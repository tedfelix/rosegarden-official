
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

#ifndef _RG_VUMETER_H_
#define _RG_VUMETER_H_

#include <qcolor.h>
#include <qlabel.h>


class QWidget;
class QTimer;
class QPaintEvent;
class QPainter;


namespace Rosegarden
{

class VelocityColour;


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


}

#endif
