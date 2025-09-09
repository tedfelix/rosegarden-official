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
#include <QMenu>
#include <QRect>
#include <QSize>
#include <QString>
#include <QAction>


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
    createAction("rename_marker", &MarkerRuler::slotRenameMarker);
    createAction("edit_marker", &MarkerRuler::slotEditMarker);
    createAction("manage_markers", &MarkerRuler::slotManageMarkers);

    setToolTip(tr("Click on a marker to move the playback pointer.\nShift-click to set a range between markers.\nDouble-click to open the marker editor."));
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

    return QSize(std::max(int(width), m_width), fontMetrics().height());
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
    const timeT pointerTime = m_doc->getComposition().getPosition();

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
    Rosegarden::Marker *marker = getMarkerAtClickPosition();
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
MarkerRuler::getMarkerAtClickPosition()
{
    // ??? This does not handle small Markers overlapping large ones.

    Composition &comp = m_doc->getComposition();

    int firstBar = m_rulerScale->getBarForX(-m_currentXOffset);
    if (firstBar < m_rulerScale->getFirstVisibleBar())
        firstBar = m_rulerScale->getFirstVisibleBar();
    const timeT rulerStart = comp.getBarStart(firstBar);

    const int lastBar = m_rulerScale->getLastVisibleBar();
    const timeT rulerEnd = comp.getBarEnd(lastBar);

    const Composition::MarkerVector &markers = comp.getMarkers();

    // need these to calculate the visible extents of a marker tag
    QFontMetrics metrics = fontMetrics();

    // For each Marker from left to right...
    for (Composition::MarkerVector::const_iterator markerIter = markers.begin();
         markerIter != markers.end();
         ++markerIter) {
        const Marker *marker = *markerIter;
        timeT markerTime = marker->getTime();

        // Not visible?  Try the next.
        if (markerTime < rulerStart)
            continue;
        if (markerTime >= rulerEnd)
            continue;

        // Compute the current Marker's x coord and width.
        const int x = m_rulerScale->getXForTime(markerTime) +
                m_currentXOffset;
        QString name(strtoqstr(marker->getName()));
        const int width = metrics.boundingRect(name).width() + 5;

        // Get the x coord for the next Marker.
        // Assume there is no next Marker.
        int nextX = -1;
        Composition::MarkerVector::const_iterator nextMarkerIter = markerIter + 1;
        if (nextMarkerIter != markers.end()) {
            nextX = m_rulerScale->getXForTime((*nextMarkerIter)->getTime()) +
                    m_currentXOffset;
        }

        // If the click x is within the marker...
        if (m_clickX >= x  &&  m_clickX <= x + width) {
            // If there is no next Marker or the click X is prior to the
            // next Marker, go with this Marker.
            if (nextX == -1  ||  m_clickX <= nextX)
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

    // Special handling when a bar is less than 25 pixels wide.
    const double minimumWidth = 25.0;
    double barWidth = m_rulerScale->getBarPosition(firstBar + 1) -
                      m_rulerScale->getBarPosition(firstBar);
    double testSize = barWidth / minimumWidth;

    // How often to display a bar number.
    //   every == 0: display every bar number.
    //   every == 1: display every other bar number.  1, 3, 5, ...
    //   every == 3: display every fourth bar number.  1, 5, 9, ...
    //   etc...
    int every = 0;
    if (testSize < 1.0) {
        every = (int(1.0 / testSize));

        // If every is even, go with the next.
        if (every % 2 == 0)
            ++every;
    }

    const int rulerHeight = height();
    // Counter to determine when to display a bar number.  See "every".
    int count = 0;

    // For each bar (0-based)...
    for (int bar = firstBar; bar <= lastBar; ++bar) {

        const double barX =
                m_rulerScale->getBarPosition(bar) + m_currentXOffset;

        // avoid writing bar numbers that will be overwritten
        if (bar < lastBar) {
            const double nextx = m_rulerScale->getBarPosition(bar+1) + m_currentXOffset;
            if ((nextx - barX) < 0.0001)
                continue;
        }

        // If x is beyond the right edge of the ruler, we're done.
        // ??? This is actually (x > clipRect.right()).
        if (barX > rulerRect.right())
            break;

        // If we are skipping bars and this is not the first bar...
        // (We always draw the first bar line.)
        if (every  &&  bar != firstBar) {
            // Still skipping?  Try the next.
            if (count < every) {
                ++count;
                continue;
            }

            // No longer skipping.  Reset the skip counter and draw the bar.
            count = 0;
        }

        // Bar line.
        painter.drawLine(barX, 0, barX, rulerHeight);

        // Only draw the bar number for bars 1 through the bar before
        // the last bar.
        // ??? But the bar number for the last bar will just be drawn off the
        //     edge.  There's no harm in that.  Reduce this to (bar >= 0)?
        if (bar != lastBar  &&  bar >= 0) {
            const int yText = painter.fontMetrics().ascent();
            const QPoint textDrawPoint(static_cast<int>(barX + 4), yText);
            painter.drawText(textDrawPoint, QString("%1").arg(bar + 1));
        }

    }

    // Draw the markers.

    if (m_doc) {
        Composition &comp = m_doc->getComposition();
        Composition::MarkerVector markers = comp.getMarkers();

        timeT start = comp.getBarStart(firstBar);
        timeT end = comp.getBarEnd(lastBar);

        QFontMetrics metrics = painter.fontMetrics();
        QBrush backgroundBrush{GUIPalette::getColour(GUIPalette::MarkerBackground)};
        const QPen widePen(Qt::black, 2);
        painter.setPen(widePen);

        // For each marker...
        for (const Marker *marker : markers) {
            const timeT markerTime = marker->getTime();

            // Out of range?  Try the next.
            if (markerTime < start)
                continue;
            if (markerTime >= end)
                continue;

            const QString name(strtoqstr(marker->getName()));

            const int x =
                    m_rulerScale->getXForTime(markerTime) + m_currentXOffset;

            // Background rect.
            painter.fillRect(x,  // x
                             1,  // y
                             metrics.boundingRect(name).width() + 5,  // w
                             rulerHeight - 2,  // h
                             backgroundBrush);

            // Thick black line to the left.
            painter.drawLine(x, 1, x, rulerHeight - 2);

            // Name
            const QPoint textDrawPoint(x + 3, rulerHeight - 4);
            painter.drawText(textDrawPoint, name);
        }
    }
}

void
MarkerRuler::mousePressEvent(QMouseEvent *e)
{
    if (!m_doc || !e)
        return;

    RG_DEBUG << "mousePressEvent(): x = " << e->pos().x();

    m_clickX = e->pos().x();
    Rosegarden::Marker* clickedMarker = getMarkerAtClickPosition();

    // if right-click, show popup menu
    if (e->button() == Qt::RightButton) {
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
        m_doc->slotSetPointerPosition(clickedMarker->getTime());
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
