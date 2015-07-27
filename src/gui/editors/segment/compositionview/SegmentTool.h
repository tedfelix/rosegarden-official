
/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2015 the Rosegarden development team.

    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#ifndef RG_SEGMENTTOOL_H
#define RG_SEGMENTTOOL_H

#include "gui/general/BaseTool.h"
#include "gui/general/ActionFileClient.h"
#include "CompositionItem.h"
#include "base/TimeT.h"

#include <QPoint>
#include <utility>
#include <vector>


class QMouseEvent;


namespace Rosegarden
{

class Command;
class SegmentToolBox;
class RosegardenDocument;
class CompositionView;


//////////////////////////////////////////////////////////////////////

class SegmentToolBox;
class SegmentSelector;

// Allow the tools to share the Selector tool's selection
// through these.
//
typedef std::pair<QPoint, CompositionItemPtr> SegmentItemPair;
typedef std::vector<SegmentItemPair> SegmentItemList;

class SegmentTool : public BaseTool, public ActionFileClient
{
    Q_OBJECT

public:
    friend class SegmentToolBox;

    virtual ~SegmentTool();

    /**
     * Is called by the parent View (EditView or SegmentCanvas) when
     * the tool is set as current.
     * Add any setup here
     */
    virtual void ready();

    /**
     * Derivers need to call this to make sure the right-click
     * context menu works.  See SegmentPencil::mousePressEvent().
     */
    virtual void mousePressEvent(QMouseEvent *);
    // ??? deprecated
    virtual void handleRightButtonPress(QMouseEvent*)  { }
    // ??? deprecated
    virtual void handleMouseButtonPress(QMouseEvent*)  { }
    virtual void handleMouseButtonRelease(QMouseEvent*)   = 0;
    virtual int  handleMouseMove(QMouseEvent*)            = 0;

    void addCommandToHistory(Command *command);

protected:
    SegmentTool(CompositionView*, RosegardenDocument *doc);

    virtual void createMenu();
    virtual bool hasMenu() { return true; }
    
    void setCurrentIndex(CompositionItemPtr item);

    SegmentToolBox* getToolBox();

    bool changeMade() { return m_changeMade; }
    void setChangeMade(bool c) { m_changeMade = c; }

    /// Sets the SnapGrid snap time based on the Shift key.
    void setSnapTime(QMouseEvent *e, timeT snapTime);

    //--------------- Data members ---------------------------------

    CompositionView*  m_canvas;
    CompositionItemPtr   m_currentIndex;
    RosegardenDocument* m_doc;
//    QPoint            m_origPos;
    bool              m_changeMade;

private slots:
    // This is just a mess of forwarding functions to RosegardenMainWindow.
    // Is there a better way to get the menu items to appear and to go to
    // RosegardenMainWindow?
    void slotEdit();
    void slotEditInMatrix();
    void slotEditInPercussionMatrix();
    void slotEditAsNotation();
    void slotEditInEventList();
    void slotEditInPitchTracker();
    void slotEditCut();
    void slotEditCopy();
    void slotEditPaste();
    void slotDeleteSelectedSegments();
    void slotJoinSegments();
    void slotQuantizeSelection();
    void slotRepeatQuantizeSelection();
    void slotRelabelSegments();
    void slotTransposeSegments();
    void slotPointerSelected();
    void slotMoveSelected();
    void slotDrawSelected();
    void slotEraseSelected();
    void slotResizeSelected();
    void slotSplitSelected();
};


}

#endif
