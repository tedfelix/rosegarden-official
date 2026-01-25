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

#include <QImage>
#include <QWidget>


namespace Rosegarden
{


class Thumbwheel : public QWidget
{
    Q_OBJECT

public:

    Thumbwheel(Qt::Orientation orientation,
               QWidget *parent = nullptr);

    // Appearance routines.
    void setBright(bool bright)  { m_bright = bright; }
    void setRed(bool red)  { m_red = red; }
    void setShowScale(bool showScale)  { m_showScale = showScale; }

    void setSpeed(float speed)  { m_speed = speed; }


    // Value

    void setMinimumValue(int min);
    int getMinimumValue() const  { return m_min; }

    void setMaximumValue(int max);
    int getMaximumValue() const  { return m_max; }

    void setDefaultValue(int defaultValue);
    int getDefaultValue() const  { return m_default; }

    void setValue(int value);
    int getValue() const  { return m_value; }


    /// Let layouts know our favorite size.
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

    // Configuration

    Qt::Orientation m_orientation;
    /// Brighten parts of the widget.
    bool m_bright{true};
    /// Red for the Segment changer.
    bool m_red{false};
    bool m_showScale{true};
    float m_speed{1};

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

    bool m_leftButtonPressed{false};

    /// Cache to speed up paintEvent().
    QImage m_cache;

};


}

#endif
