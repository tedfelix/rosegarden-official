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

#define RG_MODULE_STRING "[Rotary]"
#define RG_NO_DEBUG_PRINT

#include "Rotary.h"

#include "misc/Debug.h"
#include "gui/dialogs/FloatEdit.h"
#include "gui/general/GUIPalette.h"
#include "gui/general/ThornStyle.h"
#include "TextFloat.h"

#include <QDialog>
#include <QImage>
#include <QPainter>
#include <QPen>
#include <QPixmap>
#include <QPoint>
#include <QMouseEvent>
#include <QColormap>

#include <math.h>
#include <map>

// You can turn caching back on by setting this to 1.
// ??? Need to do some benchmarking of caching vs. no caching.
#define CACHING 0


namespace Rosegarden
{


namespace
{

    void a_drawTick(QPainter &paint, double angle, int size, bool internal)
    {
        const double halfSize = double(size) / 2.0;
        // 0 degrees is at 6 o'clock and goes CW.
        const int x0 = lround(halfSize - (halfSize - 1) * sin(angle));
        const int y0 = lround(halfSize + (halfSize - 1) * cos(angle));

        const double tickLen = halfSize / 4;

        // If not the first or last ticks, draw them extending inward.  "- tickLen"
        if (internal) {
            const int x1 = lround(halfSize - (halfSize - tickLen) * sin(angle));
            const int y1 = lround(halfSize + (halfSize - tickLen) * cos(angle));
            paint.drawLine(x0, y0, x1, y1);
        } else {  // First and last ticks extend outward.  "+ tickLen"
            const int x1 = lround(halfSize - (halfSize + tickLen) * sin(angle));
            const int y1 = lround(halfSize + (halfSize + tickLen) * cos(angle));
            paint.drawLine(x0, y0, x1, y1);
        }
    }

#if CACHING
    struct CacheIndex {

        CacheIndex(int s, int c, int a, int n, int ct) :
            size(s), colour(c), angle(a), numTicks(n), centred(ct) { }

        bool operator<(const CacheIndex &i) const {
            // ??? This can be simplified:
            //     if (size != i.size)
            //        return size < i.size;
            //     else if (colour != i.colour)
            //        return colour < i.colour;
            //     else if (angle != i.angle)
            //        return angle < i.angle;
            //     ... you get the idea.
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
    // Cache holds pixmaps of every position the knob has visited, for every
    // color, size, and mode the knob has been in.
    // ??? This appears to be here because the knob image is scaled up and
    //     back down for anti-aliasing.  Caching the resulting images should
    //     improve performance a little.  We need to MEASURE.  Do we still need
    //     this with modern hardware?
    //
    //     QPainter offers anti-aliasing with floats.  Might not work
    //     with all engines.  Need to test.  I did.  It looks bad.
    //
    //     Bear in mind that only one of these will
    //     ever be moving (unless we implement automation one day) and only
    //     in response to user input (unless automation).
    //
    //     Key concern is that when we allow the user to resize, this cache
    //     will grow to be enormous.  Right now it's pretty small since the
    //     Rotary objects can never be resized.  We could purge the cache on
    //     resize.  That should help.
    Q_GLOBAL_STATIC(PixmapCache, rotaryPixmapCache)
#endif

}


Rotary::Rotary(QWidget *parent,
               float minimum,
               float maximum,
               float step,
               float pageStep,
               float initialPosition,
               int size,
               TickMode ticks,
               bool centred,
               bool logarithmic) :
    QWidget(parent),
    m_minimum(minimum),
    m_maximum(maximum),
    m_step(step),
    m_pageStep(pageStep),
    m_size(size),
    m_tickMode(ticks),
    m_centred(centred),
    m_logarithmic(logarithmic),
    m_initialPosition(initialPosition),
    m_position(initialPosition),
    m_snapPosition(initialPosition)
{
    setObjectName("RotaryWidget");

    setAttribute(Qt::WA_NoSystemBackground);

    updateToolTip();

    setFixedSize(size, size);
}

void
Rotary::setLabel(const QString &label)
{
    m_label = label;
    updateToolTip();
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
    //Profiler profiler("Rotary::paintEvent");

    constexpr double rotaryMin{0.25 * M_PI};
    constexpr double rotaryMax{1.75 * M_PI};
    constexpr double rotaryRange{rotaryMax - rotaryMin};

    // Convert m_snapPosition to angle in radians.
    const double angle = rotaryMin // offset
                         + (rotaryRange *
                            (double(m_snapPosition - m_minimum) /
                             (double(m_maximum) - double(m_minimum))));
    constexpr double radiansToDegrees = 180 / M_PI;
    const int degrees = int(angle * radiansToDegrees);

    //    RG_DEBUG << "degrees: " << degrees << ", size " << m_size << ", pixel " << m_knobColour.pixel();

    int numTicks = 0;
    switch (m_tickMode) {
//    case LimitTicks:
//        numTicks = 2;
//        break;
//    case IntervalTicks:
//        numTicks = 5;
//        break;
    case TicksNoSnap:
        numTicks = 11;
        break;
//    case PageStepTicks:
//        numTicks = 1 + (m_maximum + 0.0001 - m_minimum) / m_pageStep;
//        break;
    case StepTicks:
        numTicks = 1 + (m_maximum + 0.0001 - m_minimum) / m_step;
        break;
    case NoTicks:
    default:
        break;
    }

    // Check the cache.

#if CACHING
    const QColormap colorMap = QColormap::instance();
    const uint pixel(colorMap.pixel(m_knobColour));

    CacheIndex index(m_size, pixel, degrees, numTicks, m_centred);

    PixmapCache *pixmapCache = rotaryPixmapCache();

    // If it's in the cache, use it.
    if (pixmapCache->find(index) != pixmapCache->end()) {
        QPainter paint;
        paint.begin(this);
        paint.drawPixmap(0, 0, (*pixmapCache)[index]);
        paint.end();
        return;
    }
#endif

    // Cache miss.  Have to draw from scratch.

    // ??? Caching was in here for speed.  I suspect we can get a similar boost
    //     by keeping a "background" pixmap as a member which would have the
    //     knob circle, ticks, and trough.  Then the position range and pointer
    //     can be drawn over top of that background.  The background would only
    //     be redrawn on size change.

    // Draw at four times the required size for anti-aliasing.
    constexpr int scale = 4;
    const int width = m_size * scale;

    // Temporary pixmap to draw on.
    QPixmap map(width, width);

    const QColor bg = ThornStyle::isEnabled() ?
            QColor::fromRgb(0x40, 0x40, 0x40) :
            palette().window().color();
    map.fill(bg);

    QPainter paint;
    paint.begin(&map);

    // Knob Circle

    // If the client set a color, use it.
    if (m_knobColour != QColor(Qt::black)) {
        paint.setBrush(m_knobColour);
    } else {
        // Go with white to get our attention that the color was not set.
        // ??? Should probably have a bool indicating the color was not set.
        paint.setBrush(Qt::white);
    }

    int indent = width * 0.15 + 1;

    QPen pen;
    pen.setWidth(scale);
    // Black outline around knob.
    pen.setColor(QColor(0x10, 0x10, 0x10));
    paint.setPen(pen);

    // draw a base knob color circle
    paint.drawEllipse(indent, indent, width - 2*indent, width - 2*indent);

#if 0
    // Highlight

    // Draw a highlight to make the knob look slightly raised and round
    // on top.  Easiest to see with larger knobs.
    // This is really subtle.  Removing this to simulate flat matte knob tops
    // which is more of a modern look for 2025.
    QColor color = m_knobColour;
    pen.setWidth(2 * scale);
    int pos = indent + (width - 2 * indent) / 8;
    int darkWidth = (width - 2 * indent) * 2 / 3;
    while (darkWidth) {
        color = color.lighter(101);
        pen.setColor(color);
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
#endif

    // Ticks

    paint.setBrush(Qt::NoBrush);

    pen.setColor(QColor(0xAA, 0xAA, 0xAA));
    pen.setWidth(scale);
    paint.setPen(pen);

    // For each tick...
    for (int tick = 0; tick < numTicks; ++tick) {
        int div = numTicks;
        if (div > 1)
            --div;
        a_drawTick(paint,
                   rotaryMin + (rotaryMax - rotaryMin) * tick / div,  // angle
                   width,  // size
                   tick != 0 && tick != numTicks - 1);  // internal
    }

    // Position Range (bright orange)

    pen.setColor(GUIPalette::getColour(GUIPalette::RotaryMeter));
    pen.setWidth(indent - scale);
    paint.setPen(pen);

    if (m_centred) {
        paint.drawArc(indent / 2 + 1, indent / 2, width - indent, width - indent,
                      90 * 16, -(degrees - 180) * 16);
    } else {
        paint.drawArc(indent / 2 + 1, indent / 2, width - indent, width - indent,
                      (180 + 45) * 16, -(degrees - 45) * 16);
    }

    // Trough

    pen.setWidth(scale);
    // same color as slider grooves and VU meter backgrounds
    pen.setColor(QColor(0x10, 0x10, 0x10));
    paint.setPen(pen);
    paint.drawArc(scale / 2,
                  scale / 2,
                  width - scale,
                  width - scale,
                  lround(-rotaryMin * radiansToDegrees - 90) * 16,
                  lround(-(rotaryMax - rotaryMin) * radiansToDegrees) * 16);

    // Pointer

    // calculate and draw the pointer
    const double halfWidth = double(width) / 2.0;
    double len = halfWidth - indent;
    --len;

    // Start in the middle.
    const double x0 = halfWidth;
    const double y0 = halfWidth;

    const double x = halfWidth - len * sin(angle);
    const double y = halfWidth + len * cos(angle);

    pen.setWidth(scale * 2);
    pen.setColor(Qt::black);
    paint.setPen(pen);

    paint.drawLine(int(x0), int(y0), int(x), int(y));

    paint.end();

    // Scale it down to target size with smoothing.

    QImage image = map.toImage().scaled(
            m_size,
            m_size,
            Qt::IgnoreAspectRatio,
            Qt::SmoothTransformation);
#if CACHING
    // Add to pixmap cache.
    (*pixmapCache)[index] = QPixmap::fromImage(image);
#endif

    // Draw on the screen.

    paint.begin(this);
#if CACHING
    paint.drawPixmap(0, 0, (*pixmapCache)[index]);
#else
    paint.drawImage(0, 0, image);
#endif
    paint.end();
}

void
Rotary::snapPosition()
{
    m_snapPosition = m_position;

    //if (m_snapToTicks) {

        switch (m_tickMode) {

        case NoTicks:
        case TicksNoSnap:
            // No snapping in these cases.
            break;

//        case LimitTicks:
//            if (m_position < (m_minimum + m_maximum) / 2.0) {
//                m_snapPosition = m_minimum;
//            } else {
//                m_snapPosition = m_maximum;
//            }
//            break;

//        case IntervalTicks:
//            m_snapPosition = m_minimum +
//                             (m_maximum - m_minimum) / 4.0 *
//                             int((m_snapPosition - m_minimum) /
//                                 ((m_maximum - m_minimum) / 4.0));
//            break;

//        case PageStepTicks:
//            m_snapPosition = m_minimum +
//                             m_pageStep *
//                             int((m_snapPosition - m_minimum) / m_pageStep);
//            break;

        case StepTicks:
            m_snapPosition = m_minimum +
                             m_step *
                             int((m_snapPosition - m_minimum) / m_step);
            break;
        }
    //}

    updateToolTip();

}

void
Rotary::mousePressEvent(QMouseEvent *e)
{
    if (e->button() == Qt::LeftButton) {
        m_buttonPressed = true;
        m_lastY = e->pos().y();
        m_lastX = e->pos().x();
    } else if (e->button() == Qt::MiddleButton) {  // reset to centre position
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

    if (e->button() == Qt::RightButton || e->button() == Qt::MiddleButton) {
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
            (m_lastY - float(e->pos().y()) + float(e->pos().x()) - m_lastX)
            * m_step;

        if (newValue > m_maximum)
            m_position = m_maximum;
        else
            if (newValue < m_minimum)
                m_position = m_minimum;
            else
                m_position = newValue;

        m_lastY = e->pos().y();
        m_lastX = e->pos().x();

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
    // ??? Wheel direction is reversed.  Up should increase.  Down should
    //     reduce.  Wonder if anyone will notice a change?

    // We'll handle this.  Don't pass to parent.
    e->accept();

    if (e->angleDelta().y() > 0)
        m_position -= m_pageStep;
    else if (e->angleDelta().y() < 0)
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
#if (QT_VERSION >= QT_VERSION_CHECK(6, 0, 0))
Rotary::enterEvent(QEnterEvent *)
#else
Rotary::enterEvent(QEvent *)
#endif
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

void
Rotary::updateToolTip()
{
    QString toolTip = tr("<qt><p>%1: %2</p><p>Click and drag up and down or left and right to modify.</p><p>Double click to edit value directly.</p></qt>").
            arg(m_label).
            arg(m_snapPosition);
    setToolTip(toolTip);
}


}
