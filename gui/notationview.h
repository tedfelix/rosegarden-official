// -*- c-basic-offset: 4 -*-

/*
    Rosegarden-4 v0.1
    A sequencer and musical notation editor.

    This program is Copyright 2000-2001
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

#ifndef NOTATIONVIEW_H
#define NOTATIONVIEW_H

#include <kmainwindow.h>

#include "notationelement.h"
#include "viewelementsmanager.h"
#include "notationhlayout.h"
#include "notationvlayout.h"
#include "notationcanvasview.h"
#include "staff.h"
#include "NotationTypes.h"

class QCanvasItem;
class QCanvasSimpleSprite;
namespace Rosegarden { class Track; }
class RosegardenGUIDoc;
class NotationTool;

/**
 * NotationView is a view for one or more Staff objects, each of
 * which contains the notation data associated with a Track.
 * NotationView owns the Staff objects it displays.
 * 
 * This class manages the relationship between NotationHLayout/
 * NotationVLayout and Staff data, as well as using rendering the
 * actual notes (using NotePixmapFactory to generate the pixmaps).
 */
class NotationView : public KMainWindow
{
    friend class NoteInserter;
    friend class ClefInserter;
    friend class NotationEraser;

    Q_OBJECT

public:
    NotationView(RosegardenGUIDoc *doc,
		 std::vector<Rosegarden::Track *> tracks,
		 QWidget *parent,
                 int resolution);

    ~NotationView();

    const RosegardenGUIDoc *getDocument() const { return m_document; }
    RosegardenGUIDoc *getDocument() { return m_document; }

    /// draw all elements
    virtual bool showElements(int staffNo);

    /// draw all elements in range at coordinates relative to staff
    virtual bool showElements(Staff *staff,
                              NotationElementList::iterator from,
                              NotationElementList::iterator to,
                              bool positionOnly = false);

    /// Calls all the relevant preparse and layout methods
    virtual bool applyLayout(int staffNo = -1);

    void setHorizontalLayoutEngine(NotationHLayout* e) { m_hlayout = e; }
    void setVerticalLayoutEngine  (NotationVLayout* e) { m_vlayout = e; }

    LayoutEngine* getHorizontalLayoutEngine() { return m_hlayout; }
    LayoutEngine* getVerticalLayoutEngine()   { return m_vlayout; }

    /// Return the number of staffs
    int getStaffCount() { return m_staffs.size(); }

    /// Return a pointer to the staff at the specified index
    Staff* getStaff(int i) { return m_staffs[i]; }

    QCanvas* canvas() { return m_canvasView->canvas(); }

    /**
     * Set the note or rest selected by the user from the toolbars
     */
    void setCurrentSelectedNote(bool isRest, Rosegarden::Note::Type,
				int dots = 0);

public slots:

    /**
     * undo
     */
    void slotEditUndo();

    /**
     * redo
     */
    void slotEditRedo();
    
    /**
     * put the marked text/object into the clipboard and remove * it
     * from the document
     */
    void slotEditCut();

    /**
     * put the marked text/object into the clipboard
     */
    void slotEditCopy();

    /**
     * paste the clipboard into the document
     */
    void slotEditPaste();

    /**
     * toggles the main toolbar
     */
    void slotToggleToolBar();

    /**
     * toggles the notes toolbar
     */
    void slotToggleNotesToolBar();

    /**
     * toggles the rests toolbar
     */
    void slotToggleRestsToolBar();

    /**
     * toggles the accidentals toolbar
     */
    void slotToggleAccidentalsToolBar();

    /**
     * toggles the clefs toolbar
     */
    void slotToggleClefsToolBar();

    /**
     * toggles the statusbar
     */
    void slotToggleStatusBar();

    /** 
     * Changes the statusbar contents for the standard label permanently,
     * used to indicate current actions.
     *
     * @param text the text that is displayed in the statusbar
     */
    void slotStatusMsg(const QString &text);

    /**
     * Changes the status message of the whole statusbar for two
     * seconds, then restores the last status. This is used to display
     * statusbar messages that give information about actions for
     * toolbar icons and menuentries.
     *
     * @param text the text that is displayed in the statusbar
     */
    void slotStatusHelpMsg(const QString &text);


    /// note switch slots
    void slotBreve();
    void slotWhole();
    void slotHalf();
    void slotQuarter();
    void slot8th();
    void slot16th();
    void slot32nd();
    void slot64th();

    /// dotted note switch slots
    void slotDottedBreve();
    void slotDottedWhole();
    void slotDottedHalf();
    void slotDottedQuarter();
    void slotDotted8th();
    void slotDotted16th();
    void slotDotted32nd();
    void slotDotted64th();

    /// rest switch slots
    void slotRBreve();
    void slotRWhole();
    void slotRHalf();
    void slotRQuarter();
    void slotR8th();
    void slotR16th();
    void slotR32nd();
    void slotR64th();

    /// dotted rest switch slots
    void slotDottedRBreve();
    void slotDottedRWhole();
    void slotDottedRHalf();
    void slotDottedRQuarter();
    void slotDottedR8th();
    void slotDottedR16th();
    void slotDottedR32nd();
    void slotDottedR64th();

    /// accidental switch slots
    void slotNoAccidental();
    void slotSharp();
    void slotFlat();
    void slotNatural();
    void slotDoubleSharp();
    void slotDoubleFlat();

    /// clef switch slots
    void slotTrebleClef();
    void slotTenorClef();
    void slotAltoClef();
    void slotBassClef();

    /// edition tools
    void slotEraseSelected();

    /// Canvas actions slots
    void itemClicked(int height, const QPoint&, NotationElement*);

    /**
     * Called when the mouse cursor moves over a different height on
     * the staff
     *
     * @see NotationCanvasView#hoveredOverNoteChange()
     */
    void hoveredOverNoteChanged(const QString&);

    /**
     * Called when the mouse cursor moves over a note which is at a
     * different time on the staff
     *
     * @see NotationCanvasView#hoveredOverNoteChange()
     */
    void hoveredOverAbsoluteTimeChange(unsigned int);

    /**
     * Set the time pointer position during playback
     */
    void setPositionPointer(const int &position);

signals:
    void changeCurrentNote(bool isRest, Rosegarden::Note::Type);

protected:

    /**
     * save general Options like all bar positions and status as well
     * as the geometry and the recent file list to the configuration
     * file
     */
    void saveOptions();

    /**
     * read general Options again and initialize all variables like the recent file list
     */
    void readOptions();

    /**
     * create menus and toolbars
     */
    void setupActions();

    /**
     * setup status bar
     */
    void initStatusBar();

    /**
     * Helper function to toggle a toolbar given its name
     */
    void toggleNamedToolBar(const QString& toolBarName);
    
    /**
     * redo the layout after insertion.  default is all staffs
     */
    void redoLayout(int staffNo = -1, Rosegarden::timeT startTime = 0,
                    Rosegarden::timeT endTime = -1); // -1 => end of staff

    /**
     * readjust the width of the canvas after a layout
     *
     * Checks the total width computed by the horizontal layout
     *
     * @see NotationHLayout#getTotalWidth()
     */
    void readjustCanvasWidth();
    
    /**
     * show bar lines
     */
    bool showBars(int staffNo);
    
    /**
     * find the Staff whose Y coord range includes y, and return the
     * index of that Staff in m_staffs.  If no Staff is suitable,
     * return -1.
     */
    int findClosestStaff(double y);

    /**
     * find the NotationElement which X coord is closest to x
     *
     * If the closest event is further than \a proximityThreshold,
     * (in pixels), end() is returned;
     */
    NotationElementList::iterator findClosestNote(double x,
                                                  Rosegarden::Event *&timeSignature,
                                                  Rosegarden::Event *&clef,
                                                  Rosegarden::Event *&key,
                                                  int staffNo,
                                                  unsigned int proximityThreshold = 10);

    /**
     * Return a QCanvasSimpleSprite representing the NotationElement
     * pointed to by the given iterator
     */
    QCanvasSimpleSprite* makeNoteSprite(NotationElementList::iterator);

    /**
     * Set the current Notation tool (note inserter, rest inserter, eraser...)
     *
     * Called when the user selects a new item on one of the notation toolbars
     * (notes toolbars, rests toolbars...)
     */
    void setTool(NotationTool*);



    KConfig* m_config;

    RosegardenGUIDoc* m_document;

    /// Displayed in the status bar, holds the pixmap of the current note
    QLabel* m_currentNotePixmap;

    /// Displayed in the status bar, shows the pitch the cursor is at
    QLabel* m_hoveredOverNoteName;

    /// Displayed in the status bar, shows the absolute time the cursor is at
    QLabel* m_hoveredOverAbsoluteTime;

    NotationCanvasView* m_canvasView;

    int m_currentStaff;

    std::vector<Staff *> m_staffs;

    NotePixmapFactory m_notePixmapFactory;
    NotePixmapFactory m_toolbarNotePixmapFactory;
    
    NotationHLayout* m_hlayout;
    NotationVLayout* m_vlayout;

    NotationTool* m_tool;

    KAction* m_selectDefaultNote;

    QCanvasLine *m_pointer;
};

//////////////////////////////////////////////////////////////////////
//               Notation Tools
//////////////////////////////////////////////////////////////////////

/**
 * Notation tool base class.
 *
 * A NotationTool represents one of the items on the notation toolbars
 * (notes, rests, clefs, eraser, etc...). It handle mouse click events
 * for the NotationView (classic 'State' design pattern)
 *
 * @see NotationView#setTool()
 */
class NotationTool
{
public:
    NotationTool(NotationView&);
    virtual ~NotationTool();

    virtual void handleClick(int height, const QPoint &eventPos,
                             NotationElement* el) = 0;
protected:
    NotationView& m_parentView;
};

namespace Rosegarden { class TrackNotationHelper; }

/**
 * This tool will insert notes on mouse click events
 */
class NoteInserter : public NotationTool
{
public:
    NoteInserter(Rosegarden::Note::Type, unsigned int dots, NotationView&);
    
    virtual void handleClick(int height, const QPoint &eventPos,
                             NotationElement* el);

    /// Set the accidental for the notes which will be inserted
    static void setAccidental(Rosegarden::Accidental);

protected:
    virtual void doInsert(Rosegarden::TrackNotationHelper&,
                          Rosegarden::timeT absTime,
                          const Rosegarden::Note&, int pitch,
                          Rosegarden::Accidental);

    Rosegarden::Note::Type m_noteType;
    unsigned int m_noteDots;

    static Rosegarden::Accidental m_accidental;
};

/**
 * This tool will insert rests on mouse click events
 */
class RestInserter : public NoteInserter
{
public:
    RestInserter(Rosegarden::Note::Type, unsigned int dots, NotationView&);
    
protected:
    virtual void doInsert(Rosegarden::TrackNotationHelper&,
                          Rosegarden::timeT absTime,
                          const Rosegarden::Note&, int pitch,
                          Rosegarden::Accidental);
};

/**
 * This tool will insert clefs on mouse click events
 */
class ClefInserter : public NotationTool
{
public:
    ClefInserter(std::string clefType, NotationView&);
    
    virtual void handleClick(int height, const QPoint &eventPos,
                             NotationElement* el);
protected:
    Rosegarden::Clef m_clef;
};


/**
 * This tool will erase a note on mouse click events
 */
class NotationEraser : public NotationTool
{
public:
    NotationEraser(NotationView&);

    virtual void handleClick(int height, const QPoint &eventPos,
                             NotationElement* el);
};

#endif
