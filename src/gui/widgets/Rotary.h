/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2025 the Rosegarden development team.

    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#ifndef RG_ROTARY_H
#define RG_ROTARY_H

#include <QColor>
#include <QPixmap>
#include <QString>
#include <QWidget>


class QWheelEvent;
class QPaintEvent;
class QMouseEvent;


namespace Rosegarden
{


/// Draw a rotary control with a knob and optional ticks.
/**
 * ??? The tool tip that this generates is pretty huge and might get in the
 *     way.  Can we offer a signal that provides the tool tip text which
 *     can be used by a parent window to update its status text on hover?
 *     The usual tool tip still needs to be available for windows that do
 *     not have status bars.  But windows with status bars can turn it off.
 *     We'll also still need the current value TextFloat regardless.
 */
class Rotary : public QWidget
{
    Q_OBJECT
public:

    enum TickMode {
        NoTicks,      // no ticks and no snap
        TicksNoSnap,  // 11 ticks and no snap
        StepTicks     // ticks at step interval, snap enabled
        //Log         // [proposed] log mode, 11 ticks, no snap
    };

    /**
     * ??? logarithmic is always TicksNoSnap.  Should we make logarithmic
     *     a "TickMode"?  Then we can get rid of the logarithmic param.
     *     We could also consider some log ticks to go with it.
     *
     * For log mode, step and pageStep are ignored.  step defaults to 100
     * divisions of the Rotary and pageStep to 10.
     *
     * centred: When set to true, draws a red arc from the top to the
     *          current position.
     *          When set to false, draws a red arc from minimum to the current
     *          position.
     */
    Rotary(QWidget *parent,
           float minimum,
           float maximum,
           float step,  // resolution
           float pageStep,  // mouse wheel step size
           float initialPosition,
           int size,
           TickMode ticks,
           bool centred,
           bool logarithmic);

    void setLabel(const QString &label);

    void setMinimum(float min);
    void setMaximum(float max);

    float getPosition() const;
    void setPosition(float position);

    // Set the colour of the knob
    void setKnobColour(const QColor &colour);

    /// Set "distance from center" mode.
    /**
     * In centered mode, the rotary shows the distance from the
     * center (12 o'clock) around the outside.  This is useful for
     * pan and eq controls where the 12 o'clock position is the default.
     *
     * When not in centered mode, the distance from minimum is shown
     * around the outside of the rotary.  This is appropriate for
     * volume.
     */
    void setCentered(bool centred);

signals:

    /// Emitted only when the user changes the Rotary.
    void valueChanged(float);

protected:

    // QWidget Overrides
    void paintEvent(QPaintEvent *e) override;
    void mousePressEvent(QMouseEvent *e) override;
    void mouseReleaseEvent(QMouseEvent *e) override;
    void mouseMoveEvent(QMouseEvent *e) override;
    void mouseDoubleClickEvent(QMouseEvent *e) override;
    void wheelEvent(QWheelEvent *e) override;
#if (QT_VERSION >= QT_VERSION_CHECK(6, 0, 0))
    void enterEvent(QEnterEvent *) override;
#else
    void enterEvent(QEvent *) override;
#endif

private:

    QString m_label{tr("Value")};
    QColor m_knobColour{Qt::black};
    /// true if the client set the color.
    bool m_colorSet{false};

    float m_minimum;
    float m_maximum;
    // Overall resolution of the rotary.
    float m_step;
    // Mouse wheel steps.
    float m_pageStep;
    int m_size;
    int m_scaledWidth;
    int m_indent;
    TickMode m_tickMode;
    int m_numTicks{0};
    bool m_centred;
    // In logarithmic mode, the log position is stored in the position
    // related member variables.  This includes m_minimum, m_maximum,
    // m_step and m_pageStep.
    bool m_logarithmic;

    float m_initialPosition;
    // The position while in motion.  No snap.
    float m_position;
    // The final reported position after snap.  Between m_minimum and m_maximum.
    float m_snapPosition;
    void snapPosition();
    void updateToolTip();
    void valueChanged2();

    void positionTextFloat();
    void updateTextFloat();

    // true while the left mouse button is pressed.
    bool m_buttonPressed{false};
    // Last position to detect how much the mouse has moved.
    int m_lastY{0};
    int m_lastX{0};

    QPixmap m_backgroundPixmap;
    bool m_backgroundPixmapValid{false};
    void updateBackground();

    void profile();

    QString m_toolTip;

};


}

#endif
