
/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.

    This program is Copyright 2000-2006
        Guillaume Laurent   <glaurent@telegraph-road.org>,
        Chris Cannam        <cannam@all-day-breakfast.com>,
        Richard Bown        <richard.bown@ferventsoftware.com>

    The moral rights of Guillaume Laurent, Chris Cannam, and Richard
    Bown to claim authorship of this work have been asserted.

    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#ifndef _RG_MATRIXSELECTOR_H_
#define _RG_MATRIXSELECTOR_H_

#include "MatrixTool.h"
#include <qstring.h>
#include "base/Event.h"


class QMouseEvent;
class QCanvasRectangle;


namespace Rosegarden
{

class ViewElement;
class MatrixView;
class MatrixStaff;
class MatrixElement;
class EventSelection;
class Event;
class EditTool;


class MatrixSelector : public MatrixTool
{
    Q_OBJECT

    friend class MatrixToolBox;

public:

    virtual void handleLeftButtonPress(timeT time,
                                       int height,
                                       int staffNo,
                                       QMouseEvent *event,
                                       ViewElement *element);

    virtual void handleMidButtonPress(timeT time,
                                      int height,
                                      int staffNo,
                                      QMouseEvent *event,
                                      ViewElement *element);

    virtual int handleMouseMove(timeT time,
                                int height,
                                QMouseEvent *event);

    virtual void handleMouseRelease(timeT,
                                    int height,
                                    QMouseEvent *event);

    /**
     * Double-click: edit an event or make a whole-bar selection
     */
    virtual void handleMouseDoubleClick(timeT time,
                                        int height,
                                        int staffNo,
                                        QMouseEvent* event,
                                        ViewElement *element);

    /**
     * Triple-click: maybe make a whole-staff selection
     */
    virtual void handleMouseTripleClick(timeT time,
                                        int height,
                                        int staffNo,
                                        QMouseEvent* event,
                                        ViewElement *element);


    /**
     * Create the selection rect
     *
     * We need this because MatrixView deletes all QCanvasItems
     * along with it. This happens before the MatrixSelector is
     * deleted, so we can't delete the selection rect in
     * ~MatrixSelector because that leads to double deletion.
     */
    virtual void ready();

    /**
     * Delete the selection rect.
     */
    virtual void stow();

    /**
     * Returns the currently selected events
     *
     * The returned result is owned by the caller
     */
    EventSelection* getSelection();

    /**
     * Respond to an event being deleted -- it may be the one the tool
     * is remembering as the current event.
     */
    virtual void handleEventRemoved(Event *event);

    static const QString ToolName;

public slots:
    /**
     * Hide the selection rectangle
     *
     * Should be called after a cut or a copy has been
     * performed
     */
    void slotHideSelection();

    void slotClickTimeout();

protected slots:

    void slotMatrixScrolled(int x, int y);

signals:
    void gotSelection(); // inform that we've got a new selection
    void editTriggerSegment(int);
    
protected:
    MatrixSelector(MatrixView*);

    void setContextHelpFor(QPoint p, bool ctrlPressed = false);

    void setViewCurrentSelection();
    
    //--------------- Data members ---------------------------------

    QCanvasRectangle* m_selectionRect;
    bool m_updateRect;

    int m_clickedStaff;
    MatrixStaff* m_currentStaff;

    MatrixElement* m_clickedElement;

    // tool to delegate to
    EditTool*    m_dispatchTool;

    bool m_justSelectedBar;

    MatrixView * m_matrixView;

    EventSelection *m_selectionToMerge;
};



}

#endif
