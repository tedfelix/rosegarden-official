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

#ifndef RG_MATRIXSELECTOR_H
#define RG_MATRIXSELECTOR_H

#include <QGraphicsRectItem>
#include "MatrixTool.h"
#include <QString>
#include <QList>
#include "base/Event.h"


namespace Rosegarden
{

class ViewElement;
class MatrixViewSegment;
class MatrixElement;
class EventSelection;
class Event;


class MatrixSelector : public MatrixTool
{
    Q_OBJECT

    friend class MatrixToolBox;

public:
    void handleLeftButtonPress(const MatrixMouseEvent *) override;
    void handleMidButtonPress(const MatrixMouseEvent *) override;
    FollowMode handleMouseMove(const MatrixMouseEvent *) override;
    void handleMouseRelease(const MatrixMouseEvent *) override;
    void handleMouseDoubleClick(const MatrixMouseEvent *) override;
    virtual void handleMouseTripleClick(const MatrixMouseEvent *);

    /**
     * Create the selection rect
     *
     * We need this because MatrixScene deletes all scene items along
     * with it. This happens before the MatrixSelector is deleted, so
     * we can't delete the selection rect in ~MatrixSelector because
     * that leads to double deletion.
     */
    void ready() override;

    /**
     * Delete the selection rect.
     */
    void stow() override;

    static QString ToolName();

public slots:
    /**
     * Hide the selection rectangle
     *
     * Should be called after a cut or a copy has been
     * performed
     */
    void slotHideSelection();

    void slotClickTimeout();

    /**
     * Respond to an event being deleted -- it may be the one the tool
     * is remembering as the current event.
     */
    void handleEventRemoved(Event *event) override;

protected slots:
    void slotMatrixScrolled(int x, int y); //!!! do we need this? probably not

signals:
    void gotSelection(); // inform that we've got a new selection
    void editTriggerSegment(int);
    
protected:
    MatrixSelector(MatrixWidget *);

    void setContextHelpFor(const MatrixMouseEvent *, bool ctrlPressed = false);

    void setViewCurrentSelection(bool always);

    /**
     * Returns the currently selected events in selection.
     * Return false if unchanged from last time.
     * The returned result is owned by the caller.
     */
    bool getSelection(EventSelection *&selection);
    
    //--------------- Data members ---------------------------------

    QGraphicsRectItem *m_selectionRect;
    QPointF m_selectionOrigin;
    bool m_updateRect;

    MatrixViewSegment *m_currentViewSegment;
    MatrixElement *m_clickedElement;
    /// Event associated with m_clickedElement.
    Event *m_event;

    // tool to delegate to
    MatrixTool *m_dispatchTool;

    bool m_justSelectedBar;

    EventSelection *m_selectionToMerge;

    QList<QGraphicsItem *> m_previousCollisions;
};



}

#endif

