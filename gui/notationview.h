
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
#include "NotationTypes.h"

class QCanvasItem;
class QCanvasSimpleSprite;
namespace Rosegarden { class Track; }
class RosegardenGUIDoc;
class NotationTool;

/**
  *@author Guillaume Laurent, Chris Cannam, Richard Bown
  */

class NotationView : public KMainWindow
{
    friend class NoteInserter;
    friend class ClefInserter;
    friend class NotationEraser;

    Q_OBJECT
public:

    NotationView(RosegardenGUIDoc* doc, Rosegarden::Track*, QWidget *parent,
                 int resolution);
    ~NotationView();

    const RosegardenGUIDoc *getDocument() const { return m_document; }
    RosegardenGUIDoc *getDocument() { return m_document; }

    /// draw all elements
    virtual bool showElements(NotationElementList::iterator from,
                              NotationElementList::iterator to);

    /// same, with dx,dy offset
    virtual bool showElements(NotationElementList::iterator from,
                              NotationElementList::iterator to,
                              double dxoffset, double dyoffset);

    /// same, relative to the specified item
    virtual bool showElements(NotationElementList::iterator from,
                              NotationElementList::iterator to,
                              QCanvasItem*);

    /// Normally calls applyHorizontalLayout() then applyVerticalLayout()
    virtual bool applyLayout();

    /// Calculate cached values for use in layout
    virtual bool applyHorizontalPreparse();

    /// Set the 'x'-coord on all doc elements - should be called after applyHorizontalPreparse()
    virtual bool applyHorizontalLayout();

    /// Set the 'y'-coord on all doc elements - should be called after applyHorizontalLayout()
    virtual bool applyVerticalLayout();
    
    void setHorizontalLayoutEngine(NotationHLayout* e) { m_hlayout = e; }
    void setVerticalLayoutEngine(NotationVLayout* e)   { m_vlayout = e; }

    LayoutEngine* getHorizontalLayoutEngine() { return m_hlayout; }
    LayoutEngine* getVerticalLayoutEngine()   { return m_vlayout; }

    QCanvas* canvas() { return m_canvasView->canvas(); }

    Rosegarden::Track& getTrack() { return m_viewElementsManager->getTrack(); }

    void setCurrentSelectedNote(bool isRest, Rosegarden::Note::Type,
                                int dots = 0);

    const NotationElementList* getNotationElements() const { return m_notationElements; }
    NotationElementList* getNotationElements()  { return m_notationElements; }

public slots:

    /** undo
     */
    void slotEditUndo();
    /** redo
     */
    void slotEditRedo();
    
    /** put the marked text/object into the clipboard and remove
     *	it from the document
     */
    void slotEditCut();

    /** put the marked text/object into the clipboard
     */
    void slotEditCopy();

    /** paste the clipboard into the document
     */
    void slotEditPaste();

    /** toggles the toolbar
     */
    void slotToggleToolBar();

    void slotToggleNotesToolBar();
    void slotToggleRestsToolBar();
    void slotToggleAccidentalsToolBar();
    void slotToggleClefsToolBar();

    /** toggles the statusbar
     */
    void slotToggleStatusBar();

    /** changes the statusbar contents for the standard label permanently, used to indicate current actions.
     * @param text the text that is displayed in the statusbar
     */
    void slotStatusMsg(const QString &text);

    /** changes the status message of the whole statusbar for two seconds, then restores the last status. This is used to display
     * statusbar messages that give information about actions for toolbar icons and menuentries.
     * @param text the text that is displayed in the statusbar
     */
    void slotStatusHelpMsg(const QString &text);


    // note switch slots
    void slotBreve();
    void slotWhole();
    void slotHalf();
    void slotQuarter();
    void slot8th();
    void slot16th();
    void slot32nd();
    void slot64th();

    // dotted note switch slots
    void slotDottedBreve();
    void slotDottedWhole();
    void slotDottedHalf();
    void slotDottedQuarter();
    void slotDotted8th();
    void slotDotted16th();
    void slotDotted32nd();
    void slotDotted64th();

    // rest switch slots
    void slotRBreve();
    void slotRWhole();
    void slotRHalf();
    void slotRQuarter();
    void slotR8th();
    void slotR16th();
    void slotR32nd();
    void slotR64th();

    // dotted rest switch slots
    void slotDottedRBreve();
    void slotDottedRWhole();
    void slotDottedRHalf();
    void slotDottedRQuarter();
    void slotDottedR8th();
    void slotDottedR16th();
    void slotDottedR32nd();
    void slotDottedR64th();

    // accidental switch slots
    void slotNoAccidental();
    void slotSharp();
    void slotFlat();
    void slotNatural();
    void slotDoubleSharp();
    void slotDoubleFlat();

    // clef switch slots
    void slotTrebleClef();
    void slotTenorClef();
    void slotAltoClef();
    void slotBassClef();

    // edition tools
    void slotEraseSelected();

    // Canvas actions slots
    void itemClicked(int height, const QPoint&, NotationElement*);
//     void insertNote(NotationElementList::iterator closestNote,
// 		    Rosegarden::Event *timesig, int pitch);
//     void deleteNote(NotationElement* note);

    void hoveredOverNoteChanged(const QString&);
    void hoveredOverAbsoluteTimeChange(unsigned int);

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

    void toggleNamedToolBar(const QString& toolBarName);
    
    /**
     * redo the layout after insertion
     */
    void redoLayout(NotationElementList::iterator from);

    /**
     * readjust the width of the canvas after a layout
     */
    void readjustCanvasWidth();
    
    /**
     * show bar lines
     */
    bool showBars(NotationElementList::iterator from,
                  NotationElementList::iterator to);

    /**
     * find the NotationElement which X coord is closest to x
     *
     * If the closest event is further than \a proximityThreshold,
     * (in pixels), end() is returned;
     */
    NotationElementList::iterator findClosestNote
      (double x,
       Rosegarden::Event *&timeSignature,
       Rosegarden::Event *&clef,
       Rosegarden::Event *&key,
       unsigned int proximityThreshold = 10);

    QCanvasSimpleSprite *makeNoteSprite(NotationElementList::iterator);

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

    Staff* m_mainStaff;
    Staff* m_currentStaff;
    NotePixmapFactory &m_notePixmapFactory;
    NotePixmapFactory m_toolbarNotePixmapFactory;

    ViewElementsManager* m_viewElementsManager;
    NotationElementList* m_notationElements;
    
    NotationHLayout* m_hlayout;
    NotationVLayout* m_vlayout;

    NotationTool* m_tool;

    KAction* m_selectDefaultNote;

    QCanvasLine *m_pointer;
};

//////////////////////////////////////////////////////////////////////
//               Notation Tools
//////////////////////////////////////////////////////////////////////

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

class NoteInserter : public NotationTool
{
public:
    NoteInserter(Rosegarden::Note::Type, unsigned int dots, NotationView&);
    
    virtual void handleClick(int height, const QPoint &eventPos,
                             NotationElement* el);

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

class ClefInserter : public NotationTool
{
public:
    ClefInserter(std::string clefType, NotationView&);
    
    virtual void handleClick(int height, const QPoint &eventPos,
                             NotationElement* el);
protected:
    Rosegarden::Clef m_clef;
};


class NotationEraser : public NotationTool
{
public:
    NotationEraser(NotationView&);

    virtual void handleClick(int height, const QPoint &eventPos,
                             NotationElement* el);
};

#endif
