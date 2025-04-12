/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2024 the Rosegarden development team.

    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#ifndef RG_MATRIXPAINTER_H
#define RG_MATRIXPAINTER_H

#include "MatrixTool.h"
#include <QString>
#include "base/Event.h"


namespace Rosegarden
{

class ViewElement;
class MatrixViewSegment;
class MatrixElement;
class Event;

// cppcheck-suppress noConstructor
 class MatrixPainter : public MatrixTool
{
    Q_OBJECT

    friend class MatrixToolBox;

public:
    void handleLeftButtonPress(const MatrixMouseEvent *) override;
    void handleMouseDoubleClick(const MatrixMouseEvent *) override;
    void handleMidButtonPress(const MatrixMouseEvent *) override;
    FollowMode handleMouseMove(const MatrixMouseEvent *) override;
    void handleMouseRelease(const MatrixMouseEvent *) override;

    void ready() override;
    void stow() override;

    static QString ToolName();

    void showPreview(const MatrixMouseEvent *e);
    void clearPreview();

public slots:
    /**
     * Respond to an event being deleted -- it may be the one the tool
     * is remembering as the current event.
     */
    void handleEventRemoved(Event *event) override;

protected slots:
    // unused void slotMatrixScrolled(int x, int y); //!!! do we need this? probably not

protected:
    explicit MatrixPainter(MatrixWidget *);
    ~MatrixPainter();

    void setBasicContextHelp();

    timeT m_clickTime;
    MatrixElement *m_currentElement;
    MatrixViewSegment *m_currentViewSegment;
private:
    Event* m_previewEvent;
    MatrixElement* m_previewElement;
};

}

#endif
