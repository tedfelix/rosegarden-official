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

#include "TextFloat.h"

#include "misc/Debug.h"
#include "gui/dialogs/FloatEdit.h"
#include "gui/general/GUIPalette.h"
#include "gui/general/ThornStyle.h"

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

// Turn caching back on by setting this to 1.
// Caching takes us from 14358.9FPS to 156250FPS.  14358.9FPS seems ok.
// I think we should remove this caching.  While it does result in a 10x
// performance improvement, it is complex, and given that we want to be
// able to dynamically size Rotary in the future, this cache is going to get
// really big.
#define CACHING 0

// See profile().
#define PROFILER 0


namespace Rosegarden
{


namespace
{

    // Draw at four times the required size for anti-aliasing.
    constexpr int a_scale = 4;

    constexpr double a_radiansToDegrees = 180 / M_PI;
    constexpr double a_rotaryMin{0.25 * M_PI};
    constexpr double a_rotaryMax{1.75 * M_PI};
    constexpr double a_rotaryRange{a_rotaryMax - a_rotaryMin};

    void a_drawTick(QPainter &painter, double angle, int size, bool internal)
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
            painter.drawLine(x0, y0, x1, y1);
        } else {  // First and last ticks extend outward.  "+ tickLen"
            const int x1 = lround(halfSize - (halfSize + tickLen) * sin(angle));
            const int y1 = lround(halfSize + (halfSize + tickLen) * cos(angle));
            painter.drawLine(x0, y0, x1, y1);
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
    m_logarithmic(logarithmic)
{
    setObjectName("RotaryWidget");
    setAttribute(Qt::WA_NoSystemBackground);

    m_scaledWidth = m_size * a_scale;
    m_indent = m_scaledWidth * 0.15 + 1;

    if (m_logarithmic) {
        // The only user of log mode is PluginControl.

        // Log can never reach 0, so we need special handling if someone
        // asks for numbers close to 0 or below.
        // This handling might be specific to plugins.  If changes are needed
        // to this, be sure to regression test PluginControl to make sure those
        // changes don't affect it.  Might be a good idea to add a plugin mode
        // to this class if multiple behaviors are needed.
        constexpr float logMinimum = -10;
        // .0000000001, 1e-10
        const float linearMinimum = powf(10, logMinimum);
        if (m_minimum > linearMinimum) {
            m_minimum = log10f(m_minimum);
        } else {
            if (m_maximum > 1)
                m_minimum = 0;  // ??? Why not logMinimum in both cases?
            else
                m_minimum = logMinimum;
        }
        if (m_maximum > linearMinimum)
            m_maximum = log10f(m_maximum);
        else
            m_maximum = logMinimum;

        // Override step and pageStep since they make little sense in
        // the log domain.
        // 100 steps
        m_step = (m_maximum - m_minimum) / 100;
        // 10 pages
        m_pageStep = m_step * 10;

        initialPosition = log10f(initialPosition);
    }
    m_initialPosition = initialPosition;
    m_position = initialPosition;
    m_snapPosition = initialPosition;

    m_numTicks = 0;
    switch (m_tickMode) {
    case TicksNoSnap:
        m_numTicks = 11;
        break;
    case StepTicks:
        m_numTicks = 1 + (m_maximum + 0.0001 - m_minimum) / m_step;
        break;
    case NoTicks:
    default:
        break;
    }

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
    m_colorSet = true;

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

void Rotary::updateBackground()
{
    m_backgroundPixmap = QPixmap(m_scaledWidth, m_scaledWidth);

    const QColor backgroundColor = ThornStyle::isEnabled() ?
            QColor::fromRgb(0x40, 0x40, 0x40) :
            palette().window().color();
    m_backgroundPixmap.fill(backgroundColor);

    QPainter painter(&m_backgroundPixmap);

    // Knob Circle

    // If the client set a color, use it.
    if (m_colorSet) {
        painter.setBrush(m_knobColour);
    } else {
        // Go with white to get our attention that the color was not set.
        painter.setBrush(Qt::white);
    }

    QPen pen;

    pen.setWidth(a_scale);
    // Black outline around knob.
    pen.setColor(QColor(0x10, 0x10, 0x10));
    painter.setPen(pen);

    // draw a base knob color circle
    painter.drawEllipse(m_indent, m_indent,
                       m_scaledWidth - 2*m_indent, m_scaledWidth - 2*m_indent);

#if 0
    // Highlight

    // Draw a highlight to make the knob look slightly raised and round
    // on top.  Easiest to see with larger knobs.
    // This is really subtle.  Removing this to simulate flat matte knob tops
    // which is more of a modern look for 2025.
    QColor color = m_knobColour;
    pen.setWidth(2 * a_scale);
    int pos = indent + (m_scaledWidth - 2 * indent) / 8;
    int darkWidth = (m_scaledWidth - 2 * indent) * 2 / 3;
    while (darkWidth) {
        color = color.lighter(101);
        pen.setColor(color);
        painter.setPen(pen);
        painter.drawEllipse(pos, pos, darkWidth, darkWidth);
        if (!--darkWidth)
            break;
        painter.drawEllipse(pos, pos, darkWidth, darkWidth);
        if (!--darkWidth)
            break;
        painter.drawEllipse(pos, pos, darkWidth, darkWidth);
        ++pos;
        --darkWidth;
    }
#endif

    // Ticks

    painter.setBrush(Qt::NoBrush);

    pen.setColor(QColor(0xAA, 0xAA, 0xAA));
    pen.setWidth(a_scale);
    painter.setPen(pen);

    // For each tick...
    for (int tick = 0; tick < m_numTicks; ++tick) {
        int div = m_numTicks;
        if (div > 1)
            --div;
        a_drawTick(painter,
                   a_rotaryMin + (a_rotaryMax - a_rotaryMin) * tick / div,  // angle
                   m_scaledWidth,  // size
                   tick != 0 && tick != m_numTicks - 1);  // internal
    }

    // Trough

    pen.setWidth(a_scale);
    // same color as slider grooves and VU meter backgrounds
    pen.setColor(QColor(0x10, 0x10, 0x10));
    painter.setPen(pen);
    painter.drawArc(a_scale / 2,
                  a_scale / 2,
                  m_scaledWidth - a_scale,
                  m_scaledWidth - a_scale,
                  lround(-a_rotaryMin * a_radiansToDegrees - 90) * 16,
                  lround(-(a_rotaryMax - a_rotaryMin) * a_radiansToDegrees) * 16);

    m_backgroundPixmapValid = true;
}

void
Rotary::paintEvent(QPaintEvent *)
{
#if PROFILER
    // Prevent infinite recursion.
    static bool inProfiler{false};
    if (!inProfiler) {
        inProfiler = true;
        profile();
        inProfiler = false;
    }
#endif

    // Convert m_snapPosition to angle in radians.
    const double angle = a_rotaryMin // offset
                         + (a_rotaryRange *
                            (double(m_snapPosition - m_minimum) /
                             (double(m_maximum) - double(m_minimum))));
    const int degrees = int(angle * a_radiansToDegrees);

    // Check the cache.

#if CACHING
    const QColormap colorMap = QColormap::instance();
    const uint pixel(colorMap.pixel(m_knobColour));

    //RG_DEBUG << "degrees: " << degrees << ", size " << m_size << ", pixel " << m_knobColour.pixel();

    CacheIndex index(m_size, pixel, degrees, m_numTicks, m_centred);

    PixmapCache *pixmapCache = rotaryPixmapCache();

    // If it's in the cache, use it.
    if (pixmapCache->find(index) != pixmapCache->end()) {
        QPainter painter;
        painter.begin(this);
        painter.drawPixmap(0, 0, (*pixmapCache)[index]);
        painter.end();
        return;
    }
#endif

    // Cache miss.  Have to draw from scratch.

    // Temporary pixmap to draw on.
    QPixmap pixmap(m_scaledWidth, m_scaledWidth);

    QPen pen;

    QPainter painter;
    painter.begin(&pixmap);

    // If the cached background isn't valid, fill it in.
    if (!m_backgroundPixmapValid)
        updateBackground();

    // Copy background to pixmap.
    painter.drawPixmap(0, 0, m_backgroundPixmap);

    // Position Range (bright orange)

    pen.setColor(GUIPalette::getColour(GUIPalette::RotaryMeter));
    pen.setWidth(m_indent - a_scale);
    painter.setPen(pen);

    if (m_centred) {
        painter.drawArc(m_indent / 2 + 1, m_indent / 2, m_scaledWidth - m_indent, m_scaledWidth - m_indent,
                      90 * 16, -(degrees - 180) * 16);
    } else {
        painter.drawArc(m_indent / 2 + 1, m_indent / 2, m_scaledWidth - m_indent, m_scaledWidth - m_indent,
                      (180 + 45) * 16, -(degrees - 45) * 16);
    }

    // Pointer

    const double halfWidth = double(m_scaledWidth) / 2.0;
    double len = halfWidth - m_indent;
    --len;

    // Start in the middle.
    const double x0 = halfWidth;
    const double y0 = halfWidth;

    const double x = halfWidth - len * sin(angle);
    const double y = halfWidth + len * cos(angle);

    pen.setWidth(a_scale * 2);
    pen.setColor(Qt::black);
    painter.setPen(pen);

    painter.drawLine(int(x0), int(y0), int(x), int(y));

    painter.end();

    // Scale the QPixmap down to target size with smoothing.

#if CACHING
    QImage image = pixmap.toImage().scaled(
            m_size,
            m_size,
            Qt::IgnoreAspectRatio,
            Qt::SmoothTransformation);
    // Add to pixmap cache.
    (*pixmapCache)[index] = QPixmap::fromImage(image);
#endif

    // Draw on the screen.

    painter.begin(this);
#if CACHING
    painter.drawPixmap(0, 0, (*pixmapCache)[index]);
#else
    pixmap = pixmap.scaled(
            m_size,
            m_size,
            Qt::IgnoreAspectRatio,
            Qt::SmoothTransformation);
    painter.drawPixmap(0, 0, pixmap);
#endif
    painter.end();
}

void
Rotary::snapPosition()
{
    m_snapPosition = m_position;

    switch (m_tickMode) {

    case NoTicks:
    case TicksNoSnap:
        // No snapping in these cases.
        break;

    case StepTicks:
        // Snap to m_step.
        m_snapPosition = m_minimum +
                         m_step *
                         int((m_snapPosition - m_minimum) / m_step);
        break;
    }

    updateToolTip();
}

void
Rotary::valueChanged2()
{
    float value = m_snapPosition;
    if (m_logarithmic)
        value = powf(10, value);

    emit valueChanged(value);
}

void
Rotary::updateTextFloat()
{
    TextFloat *textFloat = TextFloat::getInstance();
    if (m_logarithmic)
        textFloat->setText(QString("%1").arg(powf(10, m_snapPosition)));
    else
        textFloat->setText(QString("%1").arg(m_snapPosition));
}

void
Rotary::positionTextFloat()
{
#if 1
    // Original positioning approach.  Putting this back in as there
    // appears to be a problem related to double-clicks if this isn't done.
    // The TextFloat is getting the second click and the double-click never
    // happens.
    const QPoint offset = QPoint(width() + width() / 5, height() / 5);
    TextFloat::getInstance()->display(offset);
#else
    // Nudge it just a little closer.
    // This looks better, but might be interfering with double-clicks.
    TextFloat::getInstance()->display(QPoint(0, 5));
#endif
}

void
Rotary::mousePressEvent(QMouseEvent *e)
{
    if (e->button() == Qt::LeftButton) {
        m_buttonPressed = true;
        m_lastY = e->pos().y();
        m_lastX = e->pos().x();
    } else if (e->button() == Qt::MiddleButton) {
        // reset to centre position
        m_position = (m_maximum + m_minimum) / 2.0;
        snapPosition();
        update();
        valueChanged2();
    } else if (e->button() == Qt::RightButton) {
        // reset to default
        m_position = m_initialPosition;
        snapPosition();
        update();
        valueChanged2();
    }

    // Set up the tooltip (TextFloat) while moving.

    updateTextFloat();
    positionTextFloat();

    TextFloat *textFloat = TextFloat::getInstance();

    //RG_DEBUG << "mousePressEvent: logarithmic = " << m_logarithmic << ", position = " << m_position;

    if (e->button() == Qt::RightButton  ||  e->button() == Qt::MiddleButton) {
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
        step = (maxv - minv) / 100.0;
        if (step > 1.0) {
            step = .1;
        } else if (step > .1) {
            step = .01;
        } else {
            step = .001;
        }
    }

    FloatEdit dialog(this,  // parent
                     tr("Rosegarden"),  // title
                     m_label,  // text
                     minv,  // min
                     maxv,  // max
                     val,  // value
                     step);  // step

#if 0
    // Reposition the dialog.

    // ??? This always ends up in the middle of the parent window for me.

    // we need to sum the relative positions up to the
    // topLevel or dialog to please move(). Move just top/right of the rotary
    //
    // (Copied from the text float moving code Yves fixed up.)
    QWidget *parent2 = parentWidget();
    QPoint totalPos = pos();
    while (parent2->parentWidget()  &&  !parent2->isWindow()) {
        parent2 = parent2->parentWidget();
        totalPos += parent2->pos();
    }

    // ??? Can you even move a dialog before calling exec()?
    dialog.move(totalPos + QPoint(width() + 2, -height() / 2));
#endif

    if (dialog.exec() != QDialog::Accepted)
        return;

    const float newval = dialog.getValue();
    if (m_logarithmic) {
        m_position = log10f(newval);
        if (m_position < -10)
            m_position = -10;
    } else {
        m_position = newval;
    }
    snapPosition();
    update();
    valueChanged2();
}

void
Rotary::mouseReleaseEvent(QMouseEvent *e)
{
    if (e->button() == Qt::LeftButton) {
        m_buttonPressed = false;
        m_lastX = 0;
        m_lastY = 0;

        // Hide the float text
        TextFloat::getInstance()->hideAfterDelay(500);
    }
}

void
Rotary::mouseMoveEvent(QMouseEvent *e)
{
    // Not dragging?  Bail.
    if (!m_buttonPressed)
        return;

    const int x = e->pos().x();
    const int y = e->pos().y();

    // Dragging by x or y axis modifies value.
    // Dragging diagonally moves either really fast (Manhattan Distance) or
    // not at all.
    float newValue = m_position + (m_lastY - y + x - m_lastX) * m_step;
    if (newValue < m_minimum)
        newValue = m_minimum;
    else if (newValue > m_maximum)
        newValue = m_maximum;

    m_position = newValue;

    snapPosition();

    m_lastX = x;
    m_lastY = y;

    update();
    valueChanged2();
    updateTextFloat();
}

void
Rotary::wheelEvent(QWheelEvent *e)
{
    // We'll handle this.  Don't pass to parent.
    e->accept();

    // Wheel going down?  Decrease the value.  This is consistent with
    // QSpinBox.
    if (e->angleDelta().y() < 0)
        m_position -= m_pageStep;
    else if (e->angleDelta().y() > 0)  // Wheel going up?
        m_position += m_pageStep;

    if (m_position > m_maximum)
        m_position = m_maximum;
    if (m_position < m_minimum)
        m_position = m_minimum;

    snapPosition();
    update();
    updateTextFloat();

    TextFloat *textFloat = TextFloat::getInstance();

    positionTextFloat();

    // Keep text float visible for 500ms
    textFloat->hideAfterDelay(500);

    valueChanged2();
}

void
#if (QT_VERSION >= QT_VERSION_CHECK(6, 0, 0))
Rotary::enterEvent(QEnterEvent *)
#else
Rotary::enterEvent(QEvent *)
#endif
{
    // Take over the TextFloat.
    TextFloat::getInstance()->attach(this);
}

float
Rotary::getPosition() const
{
    // Test Case: Use the "Copy" button on the AudioPluginDialog.

    // ??? Why doesn't this deal in m_snapPosition?  PluginControl is the only
    //     one that might notice.  Need to see if we can come up with a
    //     test case that makes a mess of this.  Or perhaps we will
    //     find out that snap is useless and can be removed.

    if (m_logarithmic)
        return powf(10, m_position);

    return m_position;
}

void
Rotary::setPosition(float position)
{
    // Test Case: Use the "Paste" button on the AudioPluginDialog.

    if (m_logarithmic)
        position = log10f(position);

    // No change?  Bail.
    if (m_position == position)
        return;

    m_position = position;
    snapPosition();
    update();
}

void
Rotary::updateToolTip()
{
    float value;
    if (m_logarithmic)
        value = powf(10, m_snapPosition);
    else
        value = m_snapPosition;

    QString toolTip = tr("<qt><p>%1: %2</p><p>Click and drag up and down or left and right to modify.</p><p>Double click to edit value directly.</p></qt>").
            arg(m_label).
            arg(value);
    setToolTip(toolTip);
}

void
Rotary::profile()
{
    RG_WARNING << "profile()";

    constexpr int iterations = 1000;
    // usec clock.  See CLOCKS_PER_SEC.
    const clock_t start = clock();

    for (int i = 0; i < iterations; ++i)
        paintEvent(nullptr);
    const clock_t end = clock();

    const clock_t elapsed = end - start;
    const double oneIteration = double(elapsed) / double(iterations);
    const double fps = double(iterations) / (double(elapsed) / CLOCKS_PER_SEC);

    RG_WARNING << "  elapsed:" << elapsed << "usec CPU";
    RG_WARNING << "  one iteration:" << oneIteration << "usec CPU";
    RG_WARNING << "  FPS:" << fps;

    // Small knob (MIPP) results:
    //   136029 (136.029usec, 7351.37 FPS) No cache of any kind.
    //    69643 (69.643usec, 14358.9 FPS) Background cache only.
    //    66426 (66.426usec, 15054.3 FPS) No background, no cache.
    //     6400 (6.4usec, 156250 FPS) with cache.
}


}
