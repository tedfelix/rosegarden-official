/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2011 the Rosegarden development team.

    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#ifndef _RG_ROSEGARDENROTARY_H_
#define _RG_ROSEGARDENROTARY_H_

#include <map>
#include <QColor>
#include <QWidget>


class QWheelEvent;
class QPaintEvent;
class QPainter;
class QMouseEvent;


namespace Rosegarden
{


class Rotary : public QWidget
{
    Q_OBJECT
public:

    enum TickMode {
        NoTicks,        // plain circle with no marks for end points etc
        LimitTicks,     // marks at end points but not any intermediate points
        IntervalTicks,  // end points plus quarter, half, three-quarters
        PageStepTicks,  // end points plus every page-step interval
        StepTicks       // end points plus every step interval
    };

    Rotary(QWidget *parent,
           float minimum = 0.0,
           float maximum = 100.0,
           float step = 1.0,
           float pageStep = 10.0,
           float initialPosition = 50.0,
           int size = 20,
           TickMode ticks = NoTicks,
           bool snapToTicks = false,
           bool centred = false,
           bool logarithmic = false); // extents are logs, exp for display
    ~Rotary();

    void setMinimum(float min) { m_minimum = min; }
    float getMinValue() const { return m_minimum; }

    void setMaximum(float max) { m_maximum = max; }
    float getMaxValue() const { return m_maximum; }

    void setStep(float step) { m_step = step; }
    float getStep() const { return m_step; }

    void setPageStep(float step) { m_pageStep = step; }
    float getPageStep() const { return m_pageStep; }

    int getSize() const { return m_size; }

    // Position
    //
    float getPosition() const { return m_position; }
    void setPosition(float position);

    // Set the colour of the knob
    //
    void setKnobColour(const QColor &colour);
    QColor getKnobColour() const { return m_knobColour; }
    
    // Centered status
    //
    bool getCentered() const { return m_centred; }
    void setCentered(bool centred) { m_centred = centred; }

signals:
    void valueChanged(float);

protected:
    virtual void paintEvent(QPaintEvent *e);
    virtual void mousePressEvent(QMouseEvent *e);
    virtual void mouseReleaseEvent(QMouseEvent *e);
    virtual void mouseMoveEvent(QMouseEvent *e);
    virtual void mouseDoubleClickEvent(QMouseEvent *e);
    virtual void wheelEvent(QWheelEvent *e);
    virtual void enterEvent(QEvent *e);

    void snapPosition();
    void drawPosition();
    void drawTick(QPainter &paint, double angle, int size, bool internal);

    float                m_minimum;
    float                m_maximum;
    float                m_step;
    float                m_pageStep;
    int                  m_size;
    TickMode             m_tickMode;
    bool                 m_snapToTicks;
    bool                 m_centred;
    bool                 m_logarithmic;

    float                m_position;
    float                m_snapPosition;
    float                m_initialPosition;
    bool                 m_buttonPressed;
    int                  m_lastY;
    int                  m_lastX;

    QColor               m_knobColour;

    struct CacheIndex {

        CacheIndex(int _s, int _c, int _a, int _n, int _ct) :
            size(_s), colour(_c), angle(_a), numTicks(_n), centred(_ct) { }

        bool operator<(const CacheIndex &i) const {
            // woo!
            if (size < i.size) return true;
            else if (size > i.size) return false;
            else if (colour < i.colour) return true;
            else if (colour > i.colour) return false;
            else if (angle < i.angle) return true;
            else if (angle > i.angle) return false;
            else if (numTicks < i.numTicks) return true;
            else if (numTicks > i.numTicks) return false;
            else if (centred == i.centred) return false;
            else if (!centred) return true;
            return false;
        }

        int          size;
        unsigned int colour;
        int          angle;
        int          numTicks;
        bool         centred;
    };

    typedef std::map<CacheIndex, QPixmap> PixmapCache;
    static PixmapCache m_pixmaps;
    bool m_Thorn;
};


}

#endif
