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
#include "gui/application/CompositionPosition.h"
#include "gui/application/RosegardenMainWindow.h"
#include "gui/dialogs/MarkerModifyDialog.h"
#include "gui/general/AutoScroller.h"
#include "gui/general/GUIPalette.h"
#include "gui/widgets/InputDialog.h"
#include "commands/edit/AddMarkerCommand.h"
#include "commands/edit/ModifyMarkerCommand.h"
#include "commands/edit/RemoveMarkerCommand.h"
#include "document/CommandHistory.h"

#include <QMouseEvent>
#include <QBrush>
#include <QCursor>
#include <QFont>
#include <QFontMetrics>
#include <QPainter>
#include <QPen>
#include <QPoint>
#include <QMainWindow>
#include <QMenu>
#include <QRect>
#include <QSize>
#include <QStatusBar>
#include <QString>
#include <QAction>


namespace Rosegarden
{


MarkerRuler::MarkerRuler(RosegardenDocument *doc,
                         RulerScale *rulerScale,
                         QWidget *parent) :
    QWidget(parent),
    m_doc(doc),
    m_rulerScale(rulerScale)
{
    QFont font;
    font.setPointSize((font.pointSize() * 9) / 10);
    setFont(font);

    createAction("insert_marker_here", &MarkerRuler::slotInsertMarkerHere);
    createAction("insert_marker_at_pointer",
                 &MarkerRuler::slotInsertMarkerAtPointer);
    createAction("delete_marker", &MarkerRuler::slotDeleteMarker);
    createAction("rename_marker", &MarkerRuler::slotRenameMarker);
    createAction("edit_marker", &MarkerRuler::slotEditMarker);
    createAction("manage_markers", &MarkerRuler::slotManageMarkers);

    setToolTip(tr("Click on a marker to move the playback pointer.\nClick and drag a marker to move it.\nShift-click to set a range between markers.\nDouble-click to rename a marker.\nRight-click for context menu."));

}

void
MarkerRuler::createMenu()
{
    createMenusAndToolbars("markerruler.rc");

    m_menu = findChild<QMenu *>("marker_ruler_menu");
    if (!m_menu)
        RG_WARNING << "createMenu() failed\n";
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
    const int lastBar = m_rulerScale->getLastVisibleBar();
    const double width =
        m_rulerScale->getBarPosition(lastBar) +
        m_rulerScale->getBarWidth(lastBar);

    return QSize(width, fontMetrics().height());
}

QSize
MarkerRuler::minimumSizeHint() const
{
    return QSize(m_rulerScale->getBarWidth(0), fontMetrics().height());
}

void
MarkerRuler::slotInsertMarkerHere()
{
    SnapGrid snapGrid(m_rulerScale);
    snapGrid.setSnapTime(SnapGrid::SnapToBeat);
    const timeT clickTime = snapGrid.snapX(m_clickX - m_currentXOffset);

    AddMarkerCommand *command = new AddMarkerCommand(
            &RosegardenDocument::currentDocument->getComposition(),
            clickTime,
            qStrToStrUtf8(tr("new marker")),
            qStrToStrUtf8(tr("no description")));

    CommandHistory::getInstance()->addCommand(command);
}

void
MarkerRuler::slotInsertMarkerAtPointer()
{
    const timeT pointerTime = CompositionPosition::getInstance()->get();

    AddMarkerCommand *command = new AddMarkerCommand(
            &RosegardenDocument::currentDocument->getComposition(),
            pointerTime,
            qStrToStrUtf8(tr("new marker")),
            qStrToStrUtf8(tr("no description")));

    CommandHistory::getInstance()->addCommand(command);
}

void
MarkerRuler::slotDeleteMarker()
{
    const Rosegarden::Marker *marker = getMarkerAtClickPosition();
    if (!marker)
        return;

    RemoveMarkerCommand *command = new RemoveMarkerCommand(
            &RosegardenDocument::currentDocument->getComposition(),
            marker->getID());

    CommandHistory::getInstance()->addCommand(command);
}

void
MarkerRuler::slotEditMarker()
{
    Rosegarden::Marker *marker = getMarkerAtClickPosition();
    if (!marker)
        return;

    MarkerModifyDialog dialog(this, marker);
    if (dialog.exec() == QDialog::Accepted) {
        ModifyMarkerCommand *command = new ModifyMarkerCommand(
                &m_doc->getComposition(),
                marker->getID(),
                dialog.getOriginalTime(),
                dialog.getTime(),
                qstrtostr(dialog.getText()),
                qstrtostr(dialog.getComment()));
        CommandHistory::getInstance()->addCommand(command);
    }
}

void
MarkerRuler::slotRenameMarker()
{
    const Rosegarden::Marker *marker = getMarkerAtClickPosition();
    if (!marker)
        return;

    // ??? Launch a rename dialog for now.  Really we should put a
    //     QLineEdit over top of the name on the ruler and give that
    //     focus.

    bool ok{false};

    QString newName = InputDialog::getText(
            this,  // parent
            tr("Rosegarden"),  // title
            tr("Enter new name:"),  // label
            LineEdit::Normal,  // mode
            strtoqstr(marker->getName()),  // text
            &ok);  // ok

    // Canceled?  Bail.
    if (!ok)
        return;

    // No change?  Bail.
    if (newName == strtoqstr(marker->getName()))
        return;

    ModifyMarkerCommand *command = new ModifyMarkerCommand(
            &m_doc->getComposition(),  // comp
            marker->getID(),  // id
            marker->getTime(),  // time
            marker->getTime(),  // newTime
            qstrtostr(newName),  // name
            marker->getDescription());  // des
    CommandHistory::getInstance()->addCommand(command);
}

Rosegarden::Marker *
MarkerRuler::getMarkerAtClickPosition(int *clickDelta)
{
    Composition &comp = m_doc->getComposition();
    QFontMetrics metrics = fontMetrics();

    const int lastBar = m_rulerScale->getLastVisibleBar();
    const timeT rulerEndTime = comp.getBarEnd(lastBar);

    const Composition::MarkerVector &markers = comp.getMarkers();

    // For each marker from right to left (backwards)...
    for (Composition::MarkerVector::const_reverse_iterator markerIter =
                 markers.rbegin();
         markerIter != markers.rend();
         ++markerIter) {
        const Marker *marker = *markerIter;
        const timeT markerTime = marker->getTime();

        // Skip any that are off to the right.
        if (markerTime >= rulerEndTime)
            continue;

        // Compute the current Marker's x coord and width.
        const int x = m_rulerScale->getXForTime(markerTime) +
                m_currentXOffset;
        QString name(strtoqstr(marker->getName()));
        const int width = metrics.boundingRect(name).width() + 5;

        // If the click x is within the marker, we've got it.
        if (m_clickX >= x  &&  m_clickX <= x + width) {
            if (clickDelta)
                *clickDelta = m_clickX - x;
            return *markerIter;
        }
    }

    return nullptr;
}

void
MarkerRuler::paintEvent(QPaintEvent *)
{
    QPainter painter(this);

    const QRect rulerRect = rect();

    // Background
    QBrush bg = QBrush(GUIPalette::getColour(GUIPalette::RulerBackground));
    painter.fillRect(rulerRect, bg);

    painter.setPen(GUIPalette::getColour(GUIPalette::RulerForeground));

    // Draw the top frame line.
    painter.drawLine(rulerRect.left(), 0, rulerRect.right(), 0);

    // Draw the bar lines and numbers.

    int firstBar = m_rulerScale->getBarForX(-m_currentXOffset);
    if (firstBar < m_rulerScale->getFirstVisibleBar())
        firstBar = m_rulerScale->getFirstVisibleBar();

    const int lastBar = m_rulerScale->getLastVisibleBar();

    // How many bar numbers to skip.
    //   skip == 0: display every bar number.  1, 2, 3, ...
    //   skip == 1: display every other bar number.  1, 3, 5, ...
    //   skip == 3: display every fourth bar number.  1, 5, 9, ...
    //   etc...
    int skip = 0;

    const double minimumWidth = 25;
    const double barWidth = m_rulerScale->getBarPosition(firstBar + 1) -
                            m_rulerScale->getBarPosition(firstBar);
    // If the bar is too small, compute an appropriate skip.
    if (barWidth < minimumWidth) {
        skip = minimumWidth / barWidth;

        // If skip is even, go with the next.
        if (skip % 2 == 0)
            ++skip;
    }

    const int rulerHeight = height();

    // For each bar (0-based)...
    for (int bar = firstBar; bar <= lastBar; ++bar) {

        const double barX =
                m_rulerScale->getBarPosition(bar) + m_currentXOffset;

        // Avoid writing bar numbers that will be overwritten.
        // This happens in the notation editor when it compresses away bars.
        // See Bug #1580 for an example .rg file to test this.
        if (bar < lastBar) {
            const double nextx =
                    m_rulerScale->getBarPosition(bar+1) + m_currentXOffset;
            // If the next bar would be drawn over top of this one, try the
            // next bar.
            if ((nextx - barX) < 0.0001)
                continue;
        }

        // If x is beyond the right edge of the ruler, we're done.
        if (barX > rulerRect.right())
            break;

        // Not a bar we draw?  Try the next.
        if (bar % (skip + 1) != 0)
            continue;

        // Bar line.
        painter.drawLine(barX, 0, barX, rulerHeight);

        // Bar number.
        // Only draw the bar number for bars 1 through the bar before
        // the last bar.
        if (bar != lastBar  &&  bar >= 0) {
            const int yText = painter.fontMetrics().ascent();
            const QPoint textDrawPoint(static_cast<int>(barX + 4), yText);
            painter.drawText(textDrawPoint, QString("%1").arg(bar + 1));
        }

    }

    // Draw the markers.

    if (!m_doc)
        return;

    const Composition &comp = m_doc->getComposition();
    const Composition::MarkerVector &markers = comp.getMarkers();

    const timeT end = comp.getBarEnd(lastBar);

    const QFontMetrics metrics = painter.fontMetrics();
    // ??? Add color as a Marker attribute so each marker can have a different
    //     color.
    const QBrush backgroundBrush(
            GUIPalette::getColour(GUIPalette::MarkerBackground));

    const QPen widePen(Qt::black, 2);
    painter.setPen(widePen);

    const Marker *draggedMarker{nullptr};

    // For each marker...
    for (const Marker *marker : markers) {

        // If we're dragging, don't draw the dragged marker until the end.
        if (m_dragging  &&  marker->getID() == m_dragMarkerID) {
            draggedMarker = marker;
            continue;
        }

        const timeT markerTime = marker->getTime();

        // Out of range?  Try the next.
        if (markerTime >= end)
            continue;

        // !!! This code is repeated below.  Make sure this is in sync so that
        //     static and dragged markers look the same.

        const QString name(strtoqstr(marker->getName()));

        const int x =
                m_rulerScale->getXForTime(markerTime) + m_currentXOffset;
        const QRect markerRect(
                x,  // left
                1,  // top
                metrics.boundingRect(name).width() + 5,  // width
                rulerHeight - 2);  // height

        // Right edge is not visible?  Try the next.
        if (markerRect.right() < 0)
            continue;

        // Background rect.
        painter.fillRect(markerRect, backgroundBrush);

        // Thick black line to the left.
        painter.drawLine(x, 1, x, rulerHeight - 2);

        // Name
        const QPoint textDrawPoint(x + 3, rulerHeight - 4);
        painter.drawText(textDrawPoint, name);

    }

    if (draggedMarker) {

        // Draw the dragged marker at its dragged position.

        const QString name(strtoqstr(draggedMarker->getName()));

        const int x =
                m_rulerScale->getXForTime(m_dragTime) + m_currentXOffset;
        const QRect markerRect(
                x,  // left
                1,  // top
                metrics.boundingRect(name).width() + 5,  // width
                rulerHeight - 2);  // height

        // We should add 26 to the MarkerBackground hue.  That gives a
        // different color.  187, 233, 255.
        // Manipulating a QColor that was RGB by HSV is a bit complicated.
        //const QBrush draggedBrush(
        //        GUIPalette::getColour(GUIPalette::MarkerBackground));
        const QBrush draggedBrush(QColor(187, 233, 255));

        // Background rect.
        painter.fillRect(markerRect, draggedBrush);

        // Thick black line to the left.
        painter.drawLine(x, 1, x, rulerHeight - 2);

        // Name
        const QPoint textDrawPoint(x + 3, rulerHeight - 4);
        painter.drawText(textDrawPoint, name);

    }
}

void
MarkerRuler::mousePressEvent(QMouseEvent *mouseEvent)
{
    if (!mouseEvent)
        return;
    if (!m_doc)
        return;

    RG_DEBUG << "mousePressEvent(): x = " << mouseEvent->pos().x();

    // No need to propagate.
    mouseEvent->accept();

    m_clickX = mouseEvent->pos().x();
    Rosegarden::Marker *clickedMarker =
            getMarkerAtClickPosition(&m_dragClickDelta);
    if (clickedMarker)
        m_dragMarkerID = clickedMarker->getID();

    // if right-click, show popup menu
    if (mouseEvent->button() == Qt::RightButton) {
        if (!m_menu)
            createMenu();
        if (m_menu) {
            // Enable items if a marker was actually clicked.
            findAction("delete_marker")->setEnabled(clickedMarker != nullptr);
            findAction("rename_marker")->setEnabled(clickedMarker != nullptr);
            findAction("edit_marker")->setEnabled(clickedMarker != nullptr);

            m_menu->exec(QCursor::pos());
        }
        return;
    }

    // Left-Click
    if (mouseEvent->button() == Qt::LeftButton) {

        m_leftPressed = true;

        // Shift+Left-Click => set loop.
        if (mouseEvent->modifiers() & Qt::ShiftModifier) {

            Composition &comp = m_doc->getComposition();

            const Composition::MarkerVector &markers = comp.getMarkers();
            if (markers.empty())
                return;

            const timeT clickTime = m_rulerScale->getTimeForX(
                    mouseEvent->pos().x() - m_currentXOffset);

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

            if (m_autoScroller) {
                m_autoScroller->setFollowMode(FOLLOW_HORIZONTAL);
                m_autoScroller->start();
            }

            return;

        }

        // Left-click without modifiers, set pointer to clicked marker.
        if (clickedMarker) {
            CompositionPosition::getInstance()->slotSet(
                    clickedMarker->getTime());
            if (m_autoScroller) {
                m_autoScroller->setFollowMode(FOLLOW_HORIZONTAL);
                m_autoScroller->start();
            }
            return;
        }

    }
}

void
MarkerRuler::mouseMoveEvent(QMouseEvent *mouseEvent)
{
    // No need to propagate.
    mouseEvent->accept();

    // Left-click drag?
    if (m_leftPressed) {

        // Not dragging yet and haven't overcome hysteresis?  Bail.
        if (!m_dragging  &&  abs(m_clickX - mouseEvent->pos().x()) < 5)
            return;

        // We are now dragging.

        m_dragging = true;

        if (m_mainWindow)
            m_mainWindow->statusBar()->showMessage(
                    tr("Hold Shift to avoid snapping to beat grid"), 10000);

        // Compute drag time.
        SnapGrid snapGrid(m_rulerScale);
        if (mouseEvent->modifiers() & Qt::ShiftModifier)
            snapGrid.setSnapTime(SnapGrid::NoSnap);
        else
            snapGrid.setSnapTime(SnapGrid::SnapToBeat);
        m_dragTime = snapGrid.snapX(
                mouseEvent->pos().x() - m_currentXOffset - m_dragClickDelta);

        update();

    }
}

void
MarkerRuler::mouseReleaseEvent(QMouseEvent *mouseEvent)
{
    // No need to propagate.
    mouseEvent->accept();

    if (mouseEvent->button() == Qt::LeftButton) {
        m_leftPressed = false;

        if (m_dragging) {
            m_dragging = false;

            if (m_mainWindow)
                m_mainWindow->statusBar()->clearMessage();

            Composition &comp = m_doc->getComposition();

            // Find the marker by ID.  We could have stored the Marker
            // pointer, but that's not safe if the Marker goes away.
            const Marker *marker = comp.findMarker(m_dragMarkerID);

            if (marker) {
                ModifyMarkerCommand *command = new ModifyMarkerCommand(
                        &comp,  // comp
                        marker->getID(),  // id
                        marker->getTime(),  // time
                        m_dragTime,  // newTime
                        marker->getName(),  // name
                        marker->getDescription());  // des
                CommandHistory::getInstance()->addCommand(command);
            }

            m_dragMarkerID = -1;
        }

        if (m_autoScroller)
            m_autoScroller->stop();
    }
}

void
MarkerRuler::mouseDoubleClickEvent(QMouseEvent *mouseEvent)
{
    m_clickX = mouseEvent->pos().x();
    slotRenameMarker();
}

void
MarkerRuler::slotManageMarkers()
{
    RosegardenMainWindow::self()->slotEditMarkers();
}


}
