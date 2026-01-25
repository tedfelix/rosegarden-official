/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*-  vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2025 the Rosegarden development team.

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

    // ??? Break useRed out into member setter.  Majority of callers do
    //     not use it.
    Thumbwheel(Qt::Orientation orientation,
               bool useRed = false,
               QWidget *parent = nullptr);

    void setMinimumValue(int min);
    int getMinimumValue() const  { return m_min; }

    void setMaximumValue(int max);
    int getMaximumValue() const  { return m_max; }

    void setDefaultValue(int deft);
    int getDefaultValue() const;

    //void setTracking(bool tracking)  { m_tracking = tracking; }
    //bool getTracking() const  { return m_tracking; }

    void setShowScale(bool showScale);
    //bool getShowScale() const  { return m_showScale; }

    void setValue(int value);
    int getValue() const;
    //void scroll(bool up);

    void setSpeed(float speed);
    //float getSpeed() const  { return m_speed; }

    void setBright(const bool v);

    QSize sizeHint() const override;

signals:

    void valueChanged(int value);

protected:

    // QWidget overrides.
    void mousePressEvent(QMouseEvent *e) override;
    void mouseDoubleClickEvent(QMouseEvent *mouseEvent) override;
    void mouseMoveEvent(QMouseEvent *e) override;
    void mouseReleaseEvent(QMouseEvent *e) override;
    void wheelEvent(QWheelEvent *e) override;
    void paintEvent(QPaintEvent *e) override;

private:

    Qt::Orientation m_orientation;
    /// Red for the Segment changer.
    bool m_useRed;

    int m_min{0};
    int m_max{100};
    int m_default{50};
    void resetToDefault();

    // Current value.
    int m_value{50};

    /// Normalized [0,1] current position of the wheel.
    float m_rotation{.5};

    /// Position of the mouse when the wheel was clicked.
    QPoint m_clickPos;
    /// Position of the wheel when it was clicked.  See m_rotation.
    float m_clickRotation{0};

    float m_speed{1};
    /// Send value changed while the thumb wheel is moving.
    /**
     * Otherwise only send on mouse release.
     *
     * ??? This is never set to anything other than true.  Get rid of
     *     this and just do tracking always.
     */
    bool m_tracking{true};
    bool m_showScale{true};
    bool m_clicked{false};
    bool m_atDefault{true};
    QImage m_cache;
    bool m_bright{true};

};

}

#endif
