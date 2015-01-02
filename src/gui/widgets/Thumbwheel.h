/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*-  vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2015 the Rosegarden development team.

    This file originally from Sonic Visualiser, copyright 2007 Queen
    Mary, University of London.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#ifndef RG_THUMBWHEEL_H
#define RG_THUMBWHEEL_H

#include <QWidget>
#include <QImage>

#include <map>

namespace Rosegarden
{

class Thumbwheel : public QWidget
{
    Q_OBJECT

public:
    Thumbwheel(Qt::Orientation orientation, bool useRed = false, QWidget *parent = 0);
    virtual ~Thumbwheel();

    int getMinimumValue() const;
    int getMaximumValue() const;
    int getDefaultValue() const;
    float getSpeed() const;
    bool getTracking() const;
    bool getShowScale() const;
    int getValue() const;
    void setBright(const bool v);

    void setShowToolTip(bool show);

    QSize sizeHint() const;

signals:
    void valueChanged(int);

    void mouseEntered();
    void mouseLeft();

public slots:
    void setMinimumValue(int min);
    void setMaximumValue(int max);
    void setDefaultValue(int deft);
    void setSpeed(float speed);
    void setTracking(bool tracking);
    void setShowScale(bool show);
    void setValue(int value);
    void scroll(bool up);
    void resetToDefault();

protected:
    virtual void mousePressEvent(QMouseEvent *e);
    virtual void mouseDoubleClickEvent(QMouseEvent *e);
    virtual void mouseMoveEvent(QMouseEvent *e);
    virtual void mouseReleaseEvent(QMouseEvent *e);
    virtual void wheelEvent(QWheelEvent *e);
    virtual void paintEvent(QPaintEvent *e);
    virtual void enterEvent(QEvent *);
    virtual void leaveEvent(QEvent *);

    int m_min;
    int m_max;
    int m_default;
    int m_value;
    float m_rotation;
    Qt::Orientation m_orientation;
    float m_speed;
    bool m_tracking;
    bool m_showScale;
    bool m_clicked;
    bool m_atDefault;
    QPoint m_clickPos;
    float m_clickRotation;
    bool m_showTooltip;
    QImage m_cache;
    bool m_Thorn;
    bool m_bright;

    // I wanted a red wheel for the segment changer.  It would be much nicer to
    // step back and build some quality mechanism for setting the color of the
    // wheel from the outside, but it's more of a refactoring project than I
    // wanted to do today just to get a red wheel, so I used a cheap hack this
    // time.  If we ever want to have a green wheel and so on, or for making
    // this wheel generally more useful to other Qt projects, it would be nice
    // to do something more involved with this at a later time, and replace this
    // cheap hack with that much nicer mechanism.
    bool m_useRed;
};

}

#endif
