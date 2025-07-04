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

#ifndef RG_MATRIXMOVER_H
#define RG_MATRIXMOVER_H

#include "MatrixTool.h"
#include "base/TimeT.h"

#include <QString>

class QGraphicsRectItem;
class QKeyEvent;


namespace Rosegarden
{


class MatrixViewSegment;
class MatrixElement;
class Event;


class MatrixMover : public MatrixTool
{
    Q_OBJECT

public:

    MatrixMover(MatrixWidget *);

    void handleLeftButtonPress(const MatrixMouseEvent *) override;
    FollowMode handleMouseMove(const MatrixMouseEvent *) override;
    void handleMouseRelease(const MatrixMouseEvent *) override;

    void keyPressEvent(QKeyEvent *e) override;
    void keyReleaseEvent(QKeyEvent *e) override;

    /**
     * Respond to an event being deleted -- it may be the one the tool
     * is remembering as the current event.
     */
    void handleEventRemoved(Event *event) override;

    void ready() override;
    void stow() override;

    static QString ToolName();

signals:

    void hoveredOverNoteChanged(int evPitch, bool haveEvent, timeT evTime);

private:

    void setBasicContextHelp(bool ctrlPressed = false);

    MatrixElement *m_currentElement{nullptr};

    /// The Event associated with m_currentElement.
    Event *m_event{nullptr};
    MatrixViewSegment *m_currentViewSegment{nullptr};

    timeT m_clickSnappedLeftDeltaTime{0};

    std::vector<MatrixElement *> m_duplicateElements;
    bool m_quickCopy{false};

    int m_lastPlayedPitch{-1};

    void createDuplicates();
    void removeDuplicates();

    QPoint m_mousePressPos;
    bool m_dragConstrained{false};

    static constexpr int m_constraintSize{30};

    QGraphicsRectItem *m_constraintH{nullptr};
    QGraphicsRectItem *m_constraintV{nullptr};

};


}

#endif
