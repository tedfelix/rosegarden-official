// -*- c-basic-offset: 4 -*-

/*
    Rosegarden-4
    A sequencer and musical notation editor.

    This program is Copyright 2000-2003
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

#ifndef EDITVIEWBASE_H
#define EDITVIEWBASE_H

#include <vector>
#include <set>

#include <qaccel.h>
#include <kmainwindow.h>

#include "Event.h" // for timeT -- can't predeclare a typedef

namespace Rosegarden { class Segment; }

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


class EditViewBase : public KMainWindow
{
    static const unsigned int ID_STATUS_MSG;
    static const unsigned int NbLayoutRows;

    Q_OBJECT

public:

    /**
     * Create an EditViewBase for the segments \a segments from document \a doc.
     *
     * \arg cols : number of columns, main column is always rightmost
     *
     */
    EditViewBase(RosegardenGUIDoc *doc,
             std::vector<Rosegarden::Segment *> segments,
             unsigned int cols,
             QWidget *parent,
             const char *name = 0);

    virtual ~EditViewBase();

    const RosegardenGUIDoc *getDocument() const { return m_doc; }
    RosegardenGUIDoc *getDocument() { return m_doc; }

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

    /*
     * So that other people can create tools against our view
     *
     */
    EditToolBox* getToolBox() { return m_toolBox; }

    /*
     * Are these modifiers in use?
     */
    bool isShiftDown() { return m_shiftDown; }
    bool isControlDown() { return m_controlDown; }

    /**
     * Let tools know if their current element has gone
     */
    virtual void handleEventRemoved(Rosegarden::Event *event);

signals:
    /**
     * Tell the main view that the track being edited is the
     * current selected track
     * This is used by #slotToggleSolo
     */
    void selectTrack(int);

public slots:
    /**
     * close window
     */
    virtual void slotCloseWindow();

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
     * A command has happened; check the clipboard in case we
     * need to change state
     */
    virtual void slotTestClipboard();

    virtual void slotToggleSolo();

    void slotStateChanged(const QString&, bool noReverse);

protected:

    virtual void paintEvent(QPaintEvent* e);

    /**
     * @see #setInCtor
     */
    virtual void closeEvent(QCloseEvent* e);

    /**
     * ignore close events while we're in ctor
     */
    void setOutOfCtor() { m_inCtor = false; }

    /**
     * Check if we're still in ctor
     */
    bool isInCtor() { return m_inCtor; }
    
    /**
     * Set the current Notation tool (note inserter, rest inserter, eraser...)
     *
     * Called when the user selects a new item on one of the notation toolbars
     * (notes toolbars, rests toolbars...)
     */
    void setTool(EditTool*);

    /**
     * read general Options again and initialize all variables like the recent file list
     */
    virtual void readOptions();

    /**
     * create menus and toolbars
     */
    virtual void setupActions(QString rcFileName);

    /**
     * setup status bar
     */
    virtual void initStatusBar() = 0;
    
protected slots:
    /**
     * save general Options like all bar positions and status as well
     * as the geometry and the recent file list to the configuration
     * file
     */
    virtual void slotSaveOptions();
    virtual void slotConfigure();
    virtual void slotEditKeys();
    virtual void slotEditToolbars();
    virtual void slotUpdateToolbars();
    
protected:
    QFrame* getCentralFrame() { return m_centralFrame; }

    void initSegmentRefreshStatusIds();

    bool isCompositionModified();
    void setCompositionModified(bool);

    /**
     * Returns true if all of the segments contain
     * only rests
     */
    bool getSegmentsOnlyRests();

    /// Convenience function around actionCollection()->action()
    KToggleAction* getToggleAction(const QString& actionName);

    /**
     * Make a widget visible depending on the state of a
     * KToggleAction
     */
    virtual void toggleWidget(QWidget* widget, const QString& toggleActionName);

    void setRCFileName(QString s) { m_rcFileName = s; }
    QString getRCFileName()       { return m_rcFileName; }

    /**
     * Set the page index of the config dialog which corresponds to
     * this editview
     */
    void setConfigDialogPageIndex(int p) { m_configDialogPageIndex = p; }
    int getConfigDialogPageIndex()       { return m_configDialogPageIndex; }

    //--------------- Data members ---------------------------------
    QString m_rcFileName;

    static std::set<int> m_viewNumberPool;
    std::string makeViewLocalPropertyPrefix();
    int m_viewNumber;
    std::string m_viewLocalPropertyPrefix;

    KConfig* m_config;

    RosegardenGUIDoc* m_doc;
    std::vector<Rosegarden::Segment *> m_segments;
    std::vector<unsigned int> m_segmentsRefreshStatusIds;

    EditTool*    m_tool;
    EditToolBox* m_toolBox;

    QFrame      *m_centralFrame;
    QGridLayout *m_grid;

    unsigned int m_mainCol;
    unsigned int m_compositionRefreshStatusId;
    bool         m_needUpdate;

    QAccel      *m_accelerators;

    int          m_configDialogPageIndex;

    bool         m_shiftDown;
    bool         m_controlDown;

    bool         m_inCtor;
};

#endif
