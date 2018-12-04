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


#include "Rotary.h"

#include "misc/Debug.h"
#include "misc/ConfigGroups.h"
#include "base/Profiler.h"
#include "gui/dialogs/FloatEdit.h"
#include "gui/general/GUIPalette.h"
#include "gui/general/ThornStyle.h"
#include "TextFloat.h"

#include <QApplication>
#include <QBrush>
#include <QColor>
#include <QDialog>
#include <QImage>
#include <QPainter>
#include <QPalette>
#include <QPen>
#include <QPixmap>
#include <QPoint>
#include <QString>
#include <QTimer>
#include <QToolTip>
#include <QWidget>
#include <QMouseEvent>
#include <QColormap>

#include <cmath>
#include <map>

namespace Rosegarden
{

#define ROTARY_MIN (0.25 * M_PI)
#define ROTARY_MAX (1.75 * M_PI)
#define ROTARY_RANGE (ROTARY_MAX - ROTARY_MIN)

struct CacheIndex {

    CacheIndex(int s, int c, int a, int n, int ct) :
        size(s), colour(c), angle(a), numTicks(n), centred(ct) { }

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
Q_GLOBAL_STATIC(PixmapCache, rotaryPixmapCache)


Rotary::Rotary(QWidget *parent,
               float minimum,
               float maximum,
               float step,
               float pageStep,
               float initialPosition,
               int size,
               TickMode ticks,
               bool snapToTicks,
               bool centred,
               bool logarithmic) :
        QWidget(parent),
        m_minimum(minimum),
        m_maximum(maximum),
        m_step(step),
        m_pageStep(pageStep),
        m_size(size),
        m_tickMode(ticks),
        m_snapToTicks(snapToTicks),
        m_centred(centred),
        m_logarithmic(logarithmic),
        m_position(initialPosition),
        m_snapPosition(m_position),
        m_initialPosition(initialPosition),
        m_buttonPressed(false),
        m_lastY(0),
        m_lastX(0),
        m_knobColour(0, 0, 0)
{
    setObjectName("RotaryWidget");

    setAttribute(Qt::WA_NoSystemBackground);

    this->setToolTip(tr("<qt><p>Click and drag up and down or left and right to modify.</p><p>Double click to edit value directly.</p></qt>"));
    setFixedSize(size, size);
}

Rotary::~Rotary()
{
}

void
Rotary::setMinimum(float min)
{
    if (m_minimum == min)
        return;

    m_minimum = min;
    update();
}

void
Rotary::setMaximum(float max)
{
    if (m_maximum == max)
        return;

    m_maximum = max;
    update();
}

void
Rotary::setStep(float step)
{
    if (m_step == step)
        return;

    m_step = step;
    update();
}

void
Rotary::setPageStep(float step)
{
    if (m_pageStep == step)
        return;

    m_pageStep = step;
    update();
}

void
Rotary::setKnobColour(const QColor &colour)
{
    if (m_knobColour == colour)
        return;

    m_knobColour = colour;
    update();
}

void
Rotary::setCentered(bool centred)
{
    if (m_centred == centred)
        return;

    m_centred = centred;
    update();
}

void
Rotary::paintEvent(QPaintEvent *)
{
    Profiler profiler("Rotary::paintEvent");

    //!!! This should be pulled from GUIPalette eventually.  We're no longer
    // relying on Qt to come up with dark and mid and highlight and whatnot
    // colors, because they're getting inverted to a far lighter color than is
    // appropriate, even against my stock lightish slate blue default KDE
    // background, let alone the new darker scheme we're imposing through the
    // stylesheet.

    // same color as slider grooves and VU meter backgrounds
//    const QColor Dark = QColor(0x20, 0x20, 0x20);
    const QColor Dark = QColor(0x10, 0x10, 0x10);

    // this is the undefined color state for a knob, which probably indicates
    // some issue with our internal color hanlding.  It looks like this was a
    // hack to try to get around black knobs related to a bug setting up the
    // color index when making new knobs in the studio controller editor.  We'll
    // just make these a really obvious ah hah color then:
    const QColor Base = Qt::white;

    // the knob pointer should be a sharp, high contrast color
    const QColor Pointer = Qt::black;

    // tick marks should contrast against Dark and Base
    const QColor Ticks = QColor(0xAA, 0xAA, 0xAA);


    QPainter paint;

    double angle = ROTARY_MIN // offset
                   + (ROTARY_RANGE *
                      (double(m_snapPosition - m_minimum) /
                       (double(m_maximum) - double(m_minimum))));
    int degrees = int(angle * 180.0 / M_PI);

    //    RG_DEBUG << "degrees: " << degrees << ", size " << m_size << ", pixel " << m_knobColour.pixel();

    int numTicks = 0;
    switch (m_tickMode) {
    case LimitTicks:
        numTicks = 2;
        break;
    case IntervalTicks:
        numTicks = 5;
        break;
    case PageStepTicks:
        numTicks = 1 + (m_maximum + 0.0001 - m_minimum) / m_pageStep;
        break;
    case StepTicks:
        numTicks = 1 + (m_maximum + 0.0001 - m_minimum) / m_step;
        break;
    case NoTicks:
    default:
        break;
    }

    //From Qt doc

    QColormap colorMap = QColormap::instance();
    uint pixel(colorMap.pixel(m_knobColour));

    CacheIndex index(m_size, pixel, degrees, numTicks, m_centred);

    PixmapCache *pixmapCache = rotaryPixmapCache();

    if (pixmapCache->find(index) != pixmapCache->end()) {
        paint.begin(this);
        paint.drawPixmap(0, 0, (*pixmapCache)[index]);
        paint.end();
        return ;
    }

    int scale = 4;
    int width = m_size * scale;
    QPixmap map(width, width);
    QColor bg = ThornStyle::isEnabled() ? QColor::fromRgb(0x40, 0x40, 0x40) : palette().window().color();
    map.fill(bg);
    paint.begin(&map);

    QPen pen;
    pen.setColor(Dark);
    pen.setWidth(scale);
    paint.setPen(pen);

    if (m_knobColour != QColor(Qt::black)) {
        paint.setBrush(m_knobColour);
    } else {
        paint.setBrush(Base);
    }

    QColor c(m_knobColour);
    pen.setColor(c);
    paint.setPen(pen);

    int indent = width * 0.15 + 1;

    // draw a base knob color circle
    paint.drawEllipse(indent, indent, width - 2*indent, width - 2*indent);

    // draw a highlight computed from the knob color
    pen.setWidth(2 * scale);
    int pos = indent + (width - 2 * indent) / 8;
    int darkWidth = (width - 2 * indent) * 2 / 3;
    while (darkWidth) {
        c = c.light(101);
        pen.setColor(c);
        paint.setPen(pen);
        paint.drawEllipse(pos, pos, darkWidth, darkWidth);
        if (!--darkWidth)
            break;
        paint.drawEllipse(pos, pos, darkWidth, darkWidth);
        if (!--darkWidth)
            break;
        paint.drawEllipse(pos, pos, darkWidth, darkWidth);
        ++pos;
        --darkWidth;
    }

    paint.setBrush(Qt::NoBrush);

    // draw the tick marks on larger sized knobs
    pen.setColor(Ticks);
    pen.setWidth(scale);
    paint.setPen(pen);

    for (int i = 0; i < numTicks; ++i) {
        int div = numTicks;
        if (div > 1)
            --div;
        drawTick(paint, ROTARY_MIN + (ROTARY_MAX - ROTARY_MIN) * i / div,
                 width, i != 0 && i != numTicks - 1);
    }


    // draw the bright metering bit
    pen.setColor(GUIPalette::getColour(GUIPalette::RotaryMeter));
    pen.setWidth(indent);
    paint.setPen(pen);

    if (m_centred) {
        paint.drawArc(indent / 2, indent / 2, width - indent, width - indent,
                      90 * 16, -(degrees - 180) * 16);
    } else {
        paint.drawArc(indent / 2, indent / 2, width - indent, width - indent,
                      (180 + 45) * 16, -(degrees - 45) * 16);
    }

    pen.setWidth(scale);
    paint.setPen(pen);

    // draw a dark circle to outline the knob
    int shadowAngle = -720;
    c = Dark;
    for (int arc = 120; arc < 2880; arc += 240) {
        pen.setColor(c);
        paint.setPen(pen);
        paint.drawArc(indent, indent, width - 2*indent, width - 2*indent, shadowAngle + arc, 240);
        paint.drawArc(indent, indent, width - 2*indent, width - 2*indent, shadowAngle - arc, 240);
        c = c.lighter(110);
    }

    // draw a computed trough thingie all the way around the knob
    shadowAngle = 2160;
    c = Dark;
    for (int arc = 120; arc < 2880; arc += 240) {
        pen.setColor(c);
        paint.setPen(pen);
        paint.drawArc(scale / 2, scale / 2, width - scale, width - scale, shadowAngle + arc, 240);
        paint.drawArc(scale / 2, scale / 2, width - scale, width - scale, shadowAngle - arc, 240);
        c = c.lighter(109);
    }

    // and un-draw the bottom part of the arc
    pen.setColor(bg);
    paint.setPen(pen);
    paint.drawArc(scale / 2, scale / 2, width - scale, width - scale,
                  -45 * 16, -90 * 16);

    // calculate and draw the pointer
    double hyp = double(width) / 2.0;
    double len = hyp - indent;
    --len;

    double x0 = hyp;
    double y0 = hyp;

    double x = hyp - len * sin(angle);
    double y = hyp + len * cos(angle);

    pen.setWidth(scale * 2);
    pen.setColor(Pointer);
    paint.setPen(pen);

    paint.drawLine(int(x0), int(y0), int(x), int(y));

    paint.end();

    QImage i = map.toImage().scaled(m_size, m_size, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
    (*pixmapCache)[index] = QPixmap::fromImage(i);
    paint.begin(this);
    paint.drawPixmap(0, 0, (*pixmapCache)[index]);
    paint.end();
}

void
Rotary::drawTick(QPainter &paint, double angle, int size, bool internal)
{
    double hyp = double(size) / 2.0;
    double x0 = hyp - (hyp - 1) * sin(angle);
    double y0 = hyp + (hyp - 1) * cos(angle);

    if (internal) {

        double len = hyp / 4;
        double x1 = hyp - (hyp - len) * sin(angle);
        double y1 = hyp + (hyp - len) * cos(angle);

        paint.drawLine(int(x0), int(y0), int(x1), int(y1));

    } else {

        double len = hyp / 4;
        double x1 = hyp - (hyp + len) * sin(angle);
        double y1 = hyp + (hyp + len) * cos(angle);

        paint.drawLine(int(x0), int(y0), int(x1), int(y1));
    }
}

void
Rotary::snapPosition()
{
    m_snapPosition = m_position;

    if (m_snapToTicks) {

        switch (m_tickMode) {

        case NoTicks:
            break; // meaningless

        case LimitTicks:
            if (m_position < (m_minimum + m_maximum) / 2.0) {
                m_snapPosition = m_minimum;
            } else {
                m_snapPosition = m_maximum;
            }
            break;

        case IntervalTicks:
            m_snapPosition = m_minimum +
                             (m_maximum - m_minimum) / 4.0 *
                             int((m_snapPosition - m_minimum) /
                                 ((m_maximum - m_minimum) / 4.0));
            break;

        case PageStepTicks:
            m_snapPosition = m_minimum +
                             m_pageStep *
                             int((m_snapPosition - m_minimum) / m_pageStep);
            break;

        case StepTicks:
            m_snapPosition = m_minimum +
                             m_step *
                             int((m_snapPosition - m_minimum) / m_step);
            break;
        }
    }
}

void
Rotary::mousePressEvent(QMouseEvent *e)
{
    if (e->button() == Qt::LeftButton) {
        m_buttonPressed = true;
        m_lastY = e->y();
        m_lastX = e->x();
    } else if (e->button() == Qt::MidButton) {  // reset to centre position
        m_position = (m_maximum + m_minimum) / 2.0;
        snapPosition();
        update();
        emit valueChanged(m_snapPosition);
    } else if (e->button() == Qt::RightButton) {  // reset to default
        m_position = m_initialPosition;
        snapPosition();
        update();
        emit valueChanged(m_snapPosition);
    }

    TextFloat *textFloat = TextFloat::getTextFloat();


    if (m_logarithmic) {
        textFloat->setText(QString("%1").arg(powf(10, m_position)));
    } else {
        textFloat->setText(QString("%1").arg(m_position));
    }

    QPoint offset = QPoint(width() + width() / 5, height() / 5);
    textFloat->display(offset);

//    std::cerr << "Rotary::mousePressEvent: logarithmic = " << m_logarithmic
//              << ", position = " << m_position << std::endl;

    if (e->button() == Qt::RightButton || e->button() == Qt::MidButton) {
        // wait 500ms then hide text float
        textFloat->hideAfterDelay(500);
    }
}

void
Rotary::mouseDoubleClickEvent(QMouseEvent * /*e*/)
{
    float minv = m_minimum;
    float maxv = m_maximum;
    float val = m_position;
    float step = m_step;

    if (m_logarithmic) {
        minv = powf(10, minv);
        maxv = powf(10, maxv);
        val = powf(10, val);
//      step = powf(10, step);
//      if (step > 0.001) step = 0.001;
        step = (maxv - minv) / 100.0;
        if (step > 1.0) {
            step = .1;
        } else if (step > .1) {
            step = .01;
        } else {
            step = .001;
        }
    }

    FloatEdit dialog(this,
                     tr("Select a new value"),
                     tr("Enter a new value"),
                     minv,
                     maxv,
                     val,
                     step);

    // Reposition - we need to sum the relative positions up to the
    // topLevel or dialog to please move(). Move just top/right of the rotary
    //
    // (Copied from the text float moving code Yves fixed up.)
    //
    dialog.reparent(this);

    QWidget *par = parentWidget();
    QPoint totalPos = this->pos();
    while (par->parentWidget() && !par->isWindow()) {
        par = par->parentWidget();
        totalPos += par->pos();
    }

    dialog.move(totalPos + QPoint(width() + 2, -height() / 2));
    dialog.show();

    if (dialog.exec() == QDialog::Accepted) {
        float newval = dialog.getValue();
        if (m_logarithmic) {
//          if (m_position < powf(10, -10)) m_position = -10;
            if (m_position < -10) m_position = -10;
            else m_position = log10f(newval);
        } else {
            m_position = newval;
        }
        snapPosition();
        update();

        emit valueChanged(m_snapPosition);
    }
}

void
Rotary::mouseReleaseEvent(QMouseEvent *e)
{
    if (e->button() == Qt::LeftButton) {
        m_buttonPressed = false;
        m_lastY = 0;
        m_lastX = 0;

        // Hide the float text
        //
        TextFloat::getTextFloat()->hideAfterDelay(500);
    }
}

void
Rotary::mouseMoveEvent(QMouseEvent *e)
{
    if (m_buttonPressed) {
        // Dragging by x or y axis when clicked modifies value
        //
        float newValue = m_position +
                         (m_lastY - float(e->y()) + float(e->x()) - m_lastX) * m_step;

        if (newValue > m_maximum)
            m_position = m_maximum;
        else
            if (newValue < m_minimum)
                m_position = m_minimum;
            else
                m_position = newValue;

        m_lastY = e->y();
        m_lastX = e->x();

        snapPosition();

        // don't update if there's nothing to update
        //        if (m_lastPosition == m_snapPosition) return;

        update();

        emit valueChanged(m_snapPosition);

        // draw on the float text
        TextFloat *textFloat = TextFloat::getTextFloat();
        if (m_logarithmic) {
            textFloat->setText(QString("%1").arg(powf(10, m_snapPosition)));
        } else {
            textFloat->setText(QString("%1").arg(m_snapPosition));
        }
    }
}

void
Rotary::wheelEvent(QWheelEvent *e)
{
    if (e->delta() > 0)
        m_position -= m_pageStep;
    else
        m_position += m_pageStep;

    if (m_position > m_maximum)
        m_position = m_maximum;

    if (m_position < m_minimum)
        m_position = m_minimum;

    snapPosition();
    update();

    TextFloat *textFloat = TextFloat::getTextFloat();

    // draw on the float text
    if (m_logarithmic) {
        textFloat->setText(QString("%1").arg(powf(10, m_snapPosition)));
    } else {
        textFloat->setText(QString("%1").arg(m_snapPosition));
    }

    // Move just top/right of the rotary
    QPoint offset = QPoint(width() + width() / 5, height() / 5);
    textFloat->display(offset);

    // Keep text float visible for 500ms
    textFloat->hideAfterDelay(500);

    emit valueChanged(m_snapPosition);
}

void
Rotary::enterEvent(QEvent *)
{
    TextFloat::getTextFloat()->attach(this);
}


void
Rotary::setPosition(float position)
{
    if (m_position == position)
        return;

    m_position = position;
    snapPosition();
    update();
}

}
