
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

#ifndef _RG_MATRIXMOVER_H_
#define _RG_MATRIXMOVER_H_

#include "MatrixTool.h"
#include <qstring.h>
#include "base/Event.h"


class QMouseEvent;


namespace Rosegarden
{

class ViewElement;
class MatrixView;
class MatrixStaff;
class MatrixElement;
class Event;


class MatrixMover : public MatrixTool
{
    Q_OBJECT

    friend class MatrixToolBox;

public:
    virtual void handleLeftButtonPress(timeT,
                                       int height,
                                       int staffNo,
                                       QMouseEvent *event,
                                       ViewElement*);

    /**
     * Set the duration of the element
     */
    virtual int handleMouseMove(timeT,
                                int height,
                                QMouseEvent*);

    /**
     * Actually insert the new element
     */
    virtual void handleMouseRelease(timeT,
                                    int height,
                                    QMouseEvent*);

    static const QString ToolName;

    /**
     * Respond to an event being deleted -- it may be the one the tool
     * is remembering as the current event.
     */
    virtual void handleEventRemoved(Event *event);

    virtual void ready();
    virtual void stow();

signals:
    void hoveredOverNoteChanged(int evPitch, bool haveEvent, timeT evTime);

protected slots:
    void slotMatrixScrolled(int x, int y);

protected:
    MatrixMover(MatrixView*);

    void setBasicContextHelp(bool ctrlPressed = false);

    timeT getDragTime(QMouseEvent *e, timeT candidate);

    MatrixElement* m_currentElement;
    MatrixStaff* m_currentStaff;

    std::vector<MatrixElement *> m_duplicateElements;
    bool m_quickCopy;

    int m_lastPlayedPitch;
    int m_clickX;
};



}

#endif
