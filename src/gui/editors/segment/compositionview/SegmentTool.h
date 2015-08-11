
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

#include "gui/general/RosegardenScrollView.h"
#include "gui/general/BaseTool.h"
#include "gui/general/ActionFileClient.h"
#include "CompositionItem.h"
#include "CompositionModelImpl.h"
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
    virtual int mouseMoveEvent(QMouseEvent *)
            { return RosegardenScrollView::NoFollow; }
    virtual void mouseReleaseEvent(QMouseEvent *)  { }

    void addCommandToHistory(Command *command);

protected:
    SegmentTool(CompositionView*, RosegardenDocument *doc);

    virtual void createMenu();
    virtual bool hasMenu() { return true; }
    
    // ??? We also need a clearCurrentIndex() as there are many calls to
    //     this that pass CompositionItemPtr().
    // ??? And why is this called an "Index" instead of an Item?  It's
    //     not an Index (integer) in the usual sense, so Index is
    //     misleading.  Rename: setCurrentItem().
    // ??? We also need a getter and we need to make the member variable
    //     private.  This setter is *non-trivial* as it deletes the old.
    void setCurrentIndex(CompositionItemPtr item);

    SegmentToolBox* getToolBox();

    bool changeMade() { return m_changeMade; }
    void setChangeMade(bool c) { m_changeMade = c; }

    /// Sets the SnapGrid snap time based on the Shift key.
    void setSnapTime(QMouseEvent *e, timeT snapTime);

    /// Find the CompositionItem with the same Segment as referenceItem.
    static CompositionItemPtr findSiblingCompositionItem(
            const CompositionModelImpl::ChangingSegmentSet &items,
            CompositionItemPtr referenceItem);

    //--------------- Data members ---------------------------------

    CompositionView*  m_canvas;
    // ??? Make private.
    // ??? rename: m_currentItem
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
