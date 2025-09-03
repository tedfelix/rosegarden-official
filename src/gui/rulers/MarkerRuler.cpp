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

#define RG_MODULE_STRING "[MarkerRuler]"
#define RG_NO_DEBUG_PRINT

#include "MarkerRuler.h"

#include "misc/Debug.h"
#include "misc/Strings.h"
#include "base/Composition.h"
#include "base/RulerScale.h"
#include "base/SnapGrid.h"
#include "document/RosegardenDocument.h"
#include "gui/application/RosegardenMainWindow.h"
#include "gui/dialogs/MarkerModifyDialog.h"
#include "gui/general/GUIPalette.h"
#include "commands/edit/ModifyMarkerCommand.h"
#include "document/CommandHistory.h"

#include <QMouseEvent>
#include <QBrush>
#include <QCursor>
#include <QFont>
#include <QFontMetrics>
#include <QPainter>
#include <QPen>
#include <QPoint>
#include <QMenu>
#include <QRect>
#include <QSize>
#include <QString>
#include <QWidget>
#include <QAction>
#include <QToolTip>
#include <QRegion>


namespace Rosegarden
{


MarkerRuler::MarkerRuler(RosegardenDocument *doc,
                         RulerScale *rulerScale,
                         QWidget *parent,
                         const char *name) :
    QWidget(parent),
    m_doc(doc),
    m_rulerScale(rulerScale)
{
    setObjectName(name);

    QFont font;
    font.setPointSize((font.pointSize() * 9) / 10);
    setFont(font);

    createAction("insert_marker_here", &MarkerRuler::slotInsertMarkerHere);
    createAction("insert_marker_at_pointer",
                 &MarkerRuler::slotInsertMarkerAtPointer);
    createAction("delete_marker", &MarkerRuler::slotDeleteMarker);
    createAction("edit_marker", &MarkerRuler::slotEditMarker);
    createAction("manage_markers", &MarkerRuler::slotManageMarkers);

    setToolTip(tr("Click on a marker to move the playback pointer.\nShift-click to set a range between markers.\nDouble-click to open the marker editor."));
}

MarkerRuler::~MarkerRuler()
{
}

void
MarkerRuler::createMenu()
{
    createMenusAndToolbars("markerruler.rc");

    m_menu = findChild<QMenu *>("marker_ruler_menu");

//    if (!tmp) {
//        RG_DEBUG << "MarkerRuler::createMenu() menu not found\n"
//                 << domDocument().toString(4) << endl;
//    }

    if (!m_menu) {
        RG_DEBUG << "MarkerRuler::createMenu() failed\n";
    }
}


void
MarkerRuler::scrollHoriz(int x)
{
    m_currentXOffset = -x;
    update();
}

QSize
MarkerRuler::sizeHint() const
{
    int lastBar =
        m_rulerScale->getLastVisibleBar();
    double width =
        m_rulerScale->getBarPosition(lastBar) +
        m_rulerScale->getBarWidth(lastBar);

    return QSize(std::max(int(width), m_width), fontMetrics().height());
}

QSize
MarkerRuler::minimumSizeHint() const
{
    double firstBarWidth = m_rulerScale->getBarWidth(0);

    return QSize(static_cast<int>(firstBarWidth), fontMetrics().height());
}

void
MarkerRuler::slotInsertMarkerHere()
{
    SnapGrid snapGrid(m_rulerScale);
    snapGrid.setSnapTime(SnapGrid::SnapToBeat);
    const timeT clickTime = snapGrid.snapX(m_clickX - m_currentXOffset);
    emit addMarker(clickTime);
}

void
MarkerRuler::slotInsertMarkerAtPointer()
{
    emit addMarker(m_doc->getComposition().getPosition());
}

void
MarkerRuler::slotDeleteMarker()
{
    // ??? Move the code for this from RMW to here and get rid of the
    //     signal/slot stuff.

    RG_DEBUG << "MarkerRuler::slotDeleteMarker()\n";

    Rosegarden::Marker* marker = getMarkerAtClickPosition();

    if (marker)
        emit deleteMarker(marker->getID(),
                          marker->getTime(),
                          strtoqstr(marker->getName()),
                          strtoqstr(marker->getDescription()));
}

void
MarkerRuler::slotEditMarker()
{
    Rosegarden::Marker* marker = getMarkerAtClickPosition();

    if (!marker) return;

    // I think the ruler should be doing all this stuff itself, or
    // emitting signals connected to a dedicated marker model object,
    // not just relying on the app object.  Same goes for practically
    // everything else we do.  Hey ho.  Having this here is
    // inconsistent with the other methods, so if anyone wants to move
    // it, be my guest.
    // ??? The ruler should definitely be doing all this stuff itself.  E.g.
    //     RMW::slotDeleteMarker() belongs here.  See if we can move all the
    //     others to here.  A model object is overkill for so little
    //     functionality.

    MarkerModifyDialog dialog(this, marker);
    if (dialog.exec() == QDialog::Accepted) {
        ModifyMarkerCommand *command =
            new ModifyMarkerCommand(&m_doc->getComposition(),
                                    marker->getID(),
                                    dialog.getOriginalTime(),
                                    dialog.getTime(),
                                    qstrtostr(dialog.getText()),
                                    qstrtostr(dialog.getComment()));
        CommandHistory::getInstance()->addCommand(command);
    }
}

Rosegarden::Marker*
MarkerRuler::getMarkerAtClickPosition()
{
    // NO_QT3 NOTE:
    //
    // Let's try this.  We used to use QRect visibleRect() to get a rect for
    // further calculations.  Now the equivalent method returns a region instead
    // of a rect.  A region could be a complex shape, but our old code was
    // written with a rectangle in mind.  Let's try getting the boundingRect for
    // the entire region, and using that for our subsequent calculations,
    // instead of refactoring everything to take a region into account (which
    // requires deeper understanding of what the old code did than I have at a
    // glance).  This is a shot in the dark, and it's hard to predict how this
    // is going to behave until the code is running and testable.
    QRect clipRect = visibleRegion().boundingRect();

    int firstBar = m_rulerScale->getBarForX(clipRect.x() -
                                            m_currentXOffset);
    int lastBar = m_rulerScale->getLastVisibleBar();
    if (firstBar < m_rulerScale->getFirstVisibleBar()) {
        firstBar = m_rulerScale->getFirstVisibleBar();
    }

    Composition &comp = m_doc->getComposition();
    Composition::MarkerVector markers = comp.getMarkers();

    timeT start = comp.getBarStart(firstBar);
    timeT end = comp.getBarEnd(lastBar);

    // need these to calculate the visible extents of a marker tag
    QFontMetrics metrics = fontMetrics();

    for (Composition::MarkerVector::const_iterator i = markers.begin();
            i != markers.end(); ++i) {

        if ((*i)->getTime() >= start && (*i)->getTime() < end) {

            QString name(strtoqstr((*i)->getName()));

            int x = m_rulerScale->getXForTime((*i)->getTime())
                    + m_currentXOffset;

            int width = metrics.boundingRect(name).width() + 5;

            int nextX = -1;
            Composition::MarkerVector::const_iterator j = i;
            ++j;
            if (j != markers.end()) {
                nextX = m_rulerScale->getXForTime((*j)->getTime())
                        + m_currentXOffset;
            }

            if (m_clickX >= x && m_clickX <= x + width) {

                if (nextX < x || m_clickX <= nextX) {

                    return *i;
                }
            }
        }
    }

    return nullptr;
}

void
MarkerRuler::paintEvent(QPaintEvent*)
{
    QPainter painter(this);

    // See note elsewhere...
    QRect clipRect = visibleRegion().boundingRect();

    // In a stylesheet world, we have to paint our our own background to rescue
    // it from the muddle of QWidget background style hacks
    QBrush bg = QBrush(GUIPalette::getColour(GUIPalette::RulerBackground));
    painter.fillRect(clipRect, bg);

    // Now we set the pen dungle flungy to the newly defined foreground color in
    // GUIPalette to make the text all pretty like again.  (Whew.)
    painter.setPen(GUIPalette::getColour(GUIPalette::RulerForeground));

    int firstBar = m_rulerScale->getBarForX(clipRect.x() -
                                            m_currentXOffset);
    int lastBar = m_rulerScale->getLastVisibleBar();
    if (firstBar < m_rulerScale->getFirstVisibleBar()) {
        firstBar = m_rulerScale->getFirstVisibleBar();
    }

    painter.drawLine(m_currentXOffset, 0, clipRect.width(), 0);

    float minimumWidth = 25.0;
    float testSize = ((float)(m_rulerScale->getBarPosition(firstBar + 1) -
                              m_rulerScale->getBarPosition(firstBar)))
                     / minimumWidth;

    const int barHeight = height();

    int every = 0;
    int count = 0;

    if (testSize < 1.0) {
        every = (int(1.0 / testSize));

        if (every % 2 == 0)
            every++;
    }

    for (int i = firstBar; i <= lastBar; ++i) {

        double x = m_rulerScale->getBarPosition(i) + m_currentXOffset;

        // avoid writing bar numbers that will be overwritten
        if (i < lastBar) {
            double nextx = m_rulerScale->getBarPosition(i+1) + m_currentXOffset;
            if ((nextx - x) < 0.0001) continue;
        }

        if (x > clipRect.x() + clipRect.width())
            break;

        // always the first bar number
        if (every && i != firstBar) {
            if (count < every) {
                count++;
                continue;
            }

            // reset count if we passed
            count = 0;
        }

        // adjust count for first bar line
        if (every == firstBar)
            count++;

        if (i != lastBar) {
            painter.drawLine(static_cast<int>(x), 0, static_cast<int>(x), barHeight);

            if (i >= 0) {
                const int yText = painter.fontMetrics().ascent();
                const QPoint textDrawPoint(static_cast<int>(x + 4), yText);
                painter.drawText(textDrawPoint, QString("%1").arg(i + 1));
            }
        } else {
            const QPen normalPen = painter.pen();
            QPen endPen(Qt::black, 2);
            painter.setPen(endPen);
            painter.drawLine(static_cast<int>(x), 0, static_cast<int>(x), barHeight);
            painter.setPen(normalPen);
        }
    }

    if (m_doc) {
        Composition &comp = m_doc->getComposition();
        Composition::MarkerVector markers = comp.getMarkers();
        Composition::MarkerVector::const_iterator it;

        timeT start = comp.getBarStart(firstBar);
        timeT end = comp.getBarEnd(lastBar);

        QFontMetrics metrics = painter.fontMetrics();

        for (it = markers.begin(); it != markers.end(); ++it) {
            if ((*it)->getTime() >= start && (*it)->getTime() < end) {
                QString name(strtoqstr((*it)->getName()));

                double x = m_rulerScale->getXForTime((*it)->getTime())
                           + m_currentXOffset;

                painter.fillRect(static_cast<int>(x), 1,
                                 static_cast<int>(metrics.boundingRect(name).width() + 5),
                                 barHeight - 2,
                                 QBrush(GUIPalette::getColour(GUIPalette::MarkerBackground)));

                painter.drawLine(int(x), 1, int(x), barHeight - 2);
                painter.drawLine(int(x) + 1, 1, int(x) + 1, barHeight - 2);

                const QPoint textDrawPoint(static_cast<int>(x + 3), barHeight - 4);
                painter.drawText(textDrawPoint, name);
            }
        }
    }
}

void
MarkerRuler::mousePressEvent(QMouseEvent *e)
{
    if (!m_doc || !e)
        return;

    RG_DEBUG << "MarkerRuler::mousePressEvent: x = " << e->pos().x();

    m_clickX = e->pos().x();
    Rosegarden::Marker* clickedMarker = getMarkerAtClickPosition();

    // if right-click, show popup menu
    //
    if (e->button() == Qt::RightButton) {
        if (!m_menu)
            createMenu();
        if (m_menu) {
//             actionCollection()->action("delete_marker")->setEnabled(clickedMarker != 0);
//             actionCollection()->action("edit_marker")->setEnabled(clickedMarker != 0);
            findAction("delete_marker")->setEnabled(clickedMarker != nullptr);
            findAction("edit_marker")->setEnabled(clickedMarker != nullptr);

            m_menu->exec(QCursor::pos());
        }
        return;
    }

    bool shiftPressed = ((e->modifiers() & Qt::ShiftModifier) != 0);

    // Shift+Left-Click => set loop.
    if (shiftPressed) {

        Composition &comp = m_doc->getComposition();

        const Composition::MarkerVector &markers = comp.getMarkers();
        if (markers.empty())
            return;

        const timeT clickTime = m_rulerScale->getTimeForX
            (e->pos().x() - m_currentXOffset);

        timeT loopStart = 0;
        timeT loopEnd = 0;

        // For each marker, find the one that is after the clickTime.
        for (const Marker *marker : markers) {

            loopEnd = marker->getTime();

            // Found it.
            if (loopEnd >= clickTime)
                break;

            loopStart = loopEnd;
        }

        // Not found?  Select to the end.
        if (loopStart == loopEnd)
            loopEnd = comp.getEndMarker();

        comp.setLoopMode(Composition::LoopOn);
        comp.setLoopStart(loopStart);
        comp.setLoopEnd(loopEnd);
        emit m_doc->loopChanged();

        return;

    }

    // Left-click without modifiers, set pointer to clicked marker.

    if (clickedMarker)
        emit setPointerPosition(clickedMarker->getTime());
}

void
MarkerRuler::mouseDoubleClickEvent(QMouseEvent *)
{
    RosegardenMainWindow::self()->slotEditMarkers();
}

void
MarkerRuler::slotManageMarkers()
{
    RosegardenMainWindow::self()->slotEditMarkers();
}


}
