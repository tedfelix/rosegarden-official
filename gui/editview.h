// -*- c-basic-offset: 4 -*-

/*
    Rosegarden-4 v0.1
    A sequencer and musical notation editor.

    This program is Copyright 2000-2002
        Guillaume Laurent   <glaurent@telegraph-road.org>,
        Chris Cannam        <cannam@all-day-breakfast.com>,
        Richard Bown        <bownie@bownie.com>

    The moral right of the authors to claim authorship of this work
    has been asserted.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#ifndef EDITIONVIEW_H
#define EDITIONVIEW_H

#include <vector>
#include <set>

#include <qaccel.h>
#include <kmainwindow.h>

#include "Event.h" // for timeT -- can't predeclare a typedef

namespace Rosegarden { class Segment; }

class QCanvasItem;
class QScrollView;
class QCanvasView;
class QVBox;
class QGridLayout;
class QVBoxLayout;
class QScrollBar;

class KCommand;
class KToggleAction;

class RosegardenGUIDoc;
class MultiViewCommandHistory;
class EditTool;
class EditToolBox;
class BasicCommand;
class ActiveItem;
class BarButtons;
class RosegardenCanvasView;

/**
 * An interface for canvas items which are capable of handling
 * mouse events
 */
class ActiveItem
{
public:
    virtual void handleMousePress(QMouseEvent*) = 0;
    virtual void handleMouseMove(QMouseEvent*) = 0;
    virtual void handleMouseRelease(QMouseEvent*) = 0;
};


class EditView : public KMainWindow
{
    static const unsigned int ID_STATUS_MSG;

    Q_OBJECT

public:

    /**
     * Create an EditView for the segments \a segments from document \a doc.
     *
     * \arg hasTwoCols : if true, the View's grid will have two
     * columns instead of just one
     */
    EditView(RosegardenGUIDoc *doc,
             std::vector<Rosegarden::Segment *> segments,
             bool hasTwoCols,
             QWidget *parent,
             const char *name = 0);

    virtual ~EditView();

    const RosegardenGUIDoc *getDocument() const { return m_document; }
    RosegardenGUIDoc *getDocument() { return m_document; }

    /**
     * Refresh part of a Segment following a modification made in this
     * or another view.  The startTime and endTime give the extents of
     * the modified region.  This method is called following a
     * modification to any Segment; no attempt has been made to check
     * that the given Segment is actually shown in this view, so take
     * care.
     *
     * If segment is null, refresh all segments.
     * If the startTime and endTime are equal, refresh the whole of
     * the relevant segments.
     */
    virtual void refreshSegment(Rosegarden::Segment *segment,
				Rosegarden::timeT startTime = 0,
				Rosegarden::timeT endTime = 0) = 0;

    /**
     * "Clever" readjustment of the view size
     * If the new size is larger, enlarge to that size plus a margin
     * If it is smaller, only shrink if the reduction is significant
     * (e.g. new size is less than 75% of the old one)
     *
     * @arg exact if true, then set to newSize exactly
     */
    virtual void readjustViewSize(QSize newSize, bool exact = false);

    /**
     * Get the document's global command history
     */
    virtual MultiViewCommandHistory *getCommandHistory();

    /**
     * Add a Command to the history
     */
    virtual void addCommandToHistory(KCommand *);

    /**
     * Update the view
     */
    virtual void updateView() = 0;

    /**
     * Return the active item
     */
    ActiveItem* activeItem() { return m_activeItem; }

    /**
     * Set the active item
     */
    void setActiveItem(ActiveItem* i) { m_activeItem = i; }

    /**
     * Return our local accelerator object
     */
    QAccel* getAccelerators() { return m_accelerators; }

    /**
     * Return a string unique to this view (amongst views currently
     * extant) that can be used (e.g. as a prefix) to distinguish
     * view-local properties.  It's up to the subclass or other user
     * of this string to manage the properties correctly, for example
     * by deleting them from the events when the view closes.
     */
    std::string getViewLocalPropertyPrefix() {
	return m_viewLocalPropertyPrefix;
    }

public slots:
    /**
     * close window
     */
    virtual void slotCloseWindow();

    virtual void slotScrollHoriz(int x);

    /**
     * put the indicationed text/object into the clipboard and remove * it
     * from the document
     */
    virtual void slotEditCut() = 0;

    /**
     * put the indicationed text/object into the clipboard
     */
    virtual void slotEditCopy() = 0;

    /**
     * paste the clipboard into the document
     */
    virtual void slotEditPaste() = 0;

    /**
     * toggles the main toolbar
     */
    virtual void slotToggleToolBar();

    /**
     * toggles the statusbar
     */
    virtual void slotToggleStatusBar();

    /** 
     * Changes the statusbar contents for the standard label permanently,
     * used to indicate current actions.
     *
     * @param text the text that is displayed in the statusbar
     */
    virtual void slotStatusMsg(const QString &text);

    /**
     * Changes the status message of the whole statusbar for two
     * seconds, then restores the last status. This is used to display
     * statusbar messages that give information about actions for
     * toolbar icons and menuentries.
     *
     * @param text the text that is displayed in the statusbar
     */
    virtual void slotStatusHelpMsg(const QString &text);

    /**
     * Called when a mouse press occurred on an active canvas item
     *
     * @see ActiveItem
     * @see QCanvasItem#setActive
     */
    virtual void slotActiveItemPressed(QMouseEvent*, QCanvasItem*);

protected:

    virtual void paintEvent(QPaintEvent* e);
    
    /**
     * Locate the given widget in the top bar-buttons position and
     * connect up its scrolling signals.
     */
    void setTopBarButtons(QWidget*);

    /**
     * Locate the given widget in the bottom bar-buttons position and
     * connect up its scrolling signals.
     */
    void setBottomBarButtons(QWidget*);

    /**
     * Locate the given widget right above the top bar-buttons and
     * connect up its scrolling signals.
     * The widget has to have a slotScrolllHoriz(int) slot
     */
    void addRuler(QWidget*);

    /**
     * Set the current Notation tool (note inserter, rest inserter, eraser...)
     *
     * Called when the user selects a new item on one of the notation toolbars
     * (notes toolbars, rests toolbars...)
     */
    void setTool(EditTool*);

    /**
     * save general Options like all bar positions and status as well
     * as the geometry and the recent file list to the configuration
     * file
     */
    virtual void saveOptions();

    /**
     * read general Options again and initialize all variables like the recent file list
     */
    virtual void readOptions();

    /**
     * create menus and toolbars
     */
    virtual void setupActions() = 0;

    /**
     * setup status bar
     */
    virtual void initStatusBar() = 0;

    /**
     * Abstract method to get the view size
     * Typically implemented as canvas()->size().
     */
    virtual QSize getViewSize() = 0;

    /**
     * Abstract method to set the view size
     * Typically implemented as canvas()->resize().
     */
    virtual void setViewSize(QSize) = 0;

    virtual RosegardenCanvasView* getCanvasView();
    virtual void setCanvasView(RosegardenCanvasView *cv);

    QFrame* getCentralFrame() { return m_centralFrame; }

    void initSegmentRefreshStatusIds();

    bool isCompositionModified();
    void setCompositionModified(bool);

    /// Convenience function around actionCollection()->action()
    KToggleAction* getToggleAction(const QString& actionName);

    //--------------- Data members ---------------------------------

    static std::set<int> m_viewNumberPool;
    std::string makeViewLocalPropertyPrefix();
    int m_viewNumber;
    std::string m_viewLocalPropertyPrefix;

    KConfig* m_config;

    RosegardenGUIDoc* m_document;
    std::vector<Rosegarden::Segment *> m_segments;
    std::vector<unsigned int> m_segmentsRefreshStatusIds;

    EditTool*    m_tool;
    EditToolBox* m_toolBox;

    ActiveItem* m_activeItem;

    RosegardenCanvasView *m_canvasView;

    QFrame      *m_centralFrame;
    QScrollBar  *m_horizontalScrollBar;
    QGridLayout *m_grid;
    QVBoxLayout *m_rulerBox;
    QWidget     *m_topBarButtons;
    QWidget     *m_bottomBarButtons;

    unsigned int m_mainCol;
    unsigned int m_compositionRefreshStatusId;

    QAccel      *m_accelerators;
};

#endif
