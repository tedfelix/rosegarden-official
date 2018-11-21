/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2018 the Rosegarden development team.

    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#ifndef RG_FADER_H
#define RG_FADER_H

#include "base/AudioLevel.h"
#include <QColor>
#include <QWidget>
#include <utility>


class QWheelEvent;
class QTimer;
class QPixmap;
class QPaintEvent;
class QMouseEvent;


namespace Rosegarden
{


class Fader : public QWidget
{
    Q_OBJECT

public:
    /**
     * Construct a dB fader.  The fader calculates its orientation
     * based on the given dimensions.
     */
    Fader(AudioLevel::FaderType,
                    int width, int height, QWidget *parent);

    /**
     * Construct a fader on an integral scale.  The fader calculates
     * its orientation based on the given dimensions.
     */
    Fader(int min, int max, int deflt,
                    int width, int height, QWidget *parent);

    /**
     * Construct a fader on an integral scale, with a 1:1 ratio of
     * pixel positions and values.
     */
    Fader(int min, int max, int deflt,
                    bool vertical, QWidget *parent);

    ~Fader() override;

    void setOutlineColour(QColor);

    float getFaderLevel() const;

public slots:
    void setFader(float value);

signals:
    void faderChanged(float);

protected:
    void paintEvent(QPaintEvent *) override;
    void mousePressEvent(QMouseEvent *) override;
    void mouseReleaseEvent(QMouseEvent *) override;
    void mouseMoveEvent(QMouseEvent *) override;
    void wheelEvent(QWheelEvent *) override;
    void enterEvent(QEvent *e) override;

    float position_to_value(int);
    int value_to_position(float);

    void calculateGroovePixmap();
    void calculateButtonPixmap();
    void showFloatText();

    bool m_integral;
    bool m_vertical;

    int m_sliderMin;
    int m_sliderMax;
    float m_value;

    int m_min;
    int m_max;
    int m_default;
    AudioLevel::FaderType m_type;

    int m_clickMousePos;
    int m_clickButtonPos;

    QPixmap *groovePixmap();
    QPixmap *buttonPixmap();

    QColor m_outlineColour;
};


// AudioVUMeter - a vertical audio meter.  Default is stereo.
//

}

#endif
