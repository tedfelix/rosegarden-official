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

#include <string>
#include <kmainwindow.h>
#include <qslider.h>

#include "notationelement.h"
#include "notationhlayout.h"
#include "notationvlayout.h"
#include "notationcanvasview.h"
#include "notationstaff.h"
#include "NotationTypes.h"

class QCanvasItem;
namespace Rosegarden { class Segment; }
class RosegardenGUIDoc;
class NotationTool;
class StaffRuler;
class PositionCursor;
class ActiveItem;

/**
 * NotationView is a view for one or more Staff objects, each of
 * which contains the notation data associated with a Segment.
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
    friend class NotationSelectionPaster;

    Q_OBJECT

public:
    NotationView(RosegardenGUIDoc *doc,
                 std::vector<Rosegarden::Segment *> segments,
                 QWidget *parent);
    ~NotationView();

    const RosegardenGUIDoc *getDocument() const { return m_document; }
    RosegardenGUIDoc *getDocument() { return m_document; }

    /// Calls all the relevant preparse and layout methods
    virtual bool applyLayout(int staffNo = -1);

    /// Return the number of staffs
    int getStaffCount() { return m_staffs.size(); }

    /// Return a pointer to the staff at the specified index
    NotationStaff *getStaff(int i) { return m_staffs[i]; }

    /// Return a pointer to the staff corresponding to the given segment
    NotationStaff *getStaff(const Rosegarden::Segment &segment);

    QCanvas* canvas() { return m_canvasView->canvas(); }
    
    void setCanvasCursor(const QCursor &cursor) {
	m_canvasView->viewport()->setCursor(cursor);
    }

    /**
     * Set the note or rest selected by the user from the toolbars
     */
    void setCurrentSelectedNote(const char *pixmapName,
                                bool isRest, Rosegarden::Note::Type,
                                int dots = 0);

    /**
     * Set the current event selection
     * @see NotationSelector
     */
    void setCurrentSelection(EventSelection*);

    /**
     * Set the current event selection to a single event
     */
    void setSingleSelectedEvent(int staffNo,
				Rosegarden::Event *event);

    /**
     * Set the current event selection to a single event
     */
    void setSingleSelectedEvent(Rosegarden::Segment &segment,
				Rosegarden::Event *event);

    /// Changes the font of the staffs on the view
    void changeFont(std::string newFont);

    /// Changes the font and font size of the staffs on the view
    void changeFont(std::string newFont, int newSize);
    
    /**
     * find the Staff whose Y coord range includes y, and return the
     * index of that Staff in m_staffs.  If no Staff is suitable,
     * return -1.
     */
    int findClosestStaff(double y);


public slots:
    /**
     * close window
     */
    void closeWindow();

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
    void slotSelectSelected();

    /// group slots
    void slotGroupBeam();
    void slotGroupAutoBeam();
    void slotGroupBreak();

    // transforms slots
    void slotTransformsNormalizeRests();
    void slotTransformsMergeRests();

    /// Canvas actions slots

    /**
     * Called when a mouse press occurred on a notation element
     * or somewhere on a staff
     */
    void itemPressed(int height, int staffNo, QMouseEvent*, NotationElement*);

    /**
     * Called when a mouse press occurred on an active canvas item
     *
     * @see ActiveItem
     * @see QCanvasItem#setActive
     */
    void activeItemPressed(QMouseEvent*, QCanvasItem*);

    void mouseMove(QMouseEvent*);
    void mouseRelease(QMouseEvent*);

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

     /// Set the time pointer position during playback
    void setPositionPointer(const int &position);

    /// Changes the font of the staffs on the view
    void changeFont(const QString &newFont);

    /// Changes the font size of the staffs on the view
    void changeFontSize(int newSize);

    /// Changes the font size of the staffs on the view to the nth size in the available size list
    void changeFontSizeFromIndex(int n);

    /// Changes the hlayout stretch of the staffs on the view
    void changeStretch(int newStretch);

    /// Changes the display quantization of the staffs on the view
    void changeLegato(int newLegatoIndex);

    /// Sets the position of the cursor to the given pixel X coord
    void setCursorPosition(unsigned int);

    /// Gets the position of the cursor as a pixel X coord
    unsigned int getCursorPosition() const;

signals:
    /**
     * Emitted when the note selected in the palette changes
     */
    void changeCurrentNote(bool isRest, Rosegarden::Note::Type);

    /**
     * Emitted when the selection has been cut or copied
     *
     * @see NotationSelector#hideSelection
     */
    void usedSelection();

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
     * setup the "zoom" toolbar
     */
    void initFontToolbar(int legatoUnit);

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
     * Readjust the size of the canvas after a layout
     *
     * Checks the total width computed by the horizontal layout
     *
     * @see NotationHLayout#getTotalWidth()
     */
    void readjustCanvasSize();
    
    /**
     * show bar lines
     */
    void showBars(int staffNo);

    /**
     * update the top ruler according to bar lines
     */
    void updateRuler();

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
     * Set the current Notation tool (note inserter, rest inserter, eraser...)
     *
     * Called when the user selects a new item on one of the notation toolbars
     * (notes toolbars, rests toolbars...)
     */
    void setTool(NotationTool*);

    /**
     * Set the note pixmap factory
     *
     * The previous pixmap factory is deleted
     */
    void setNotePixmapFactory(NotePixmapFactory*);

    /**
     * Set the horizontal layout
     *
     * The previous layout is deleted
     */
    void setHLayout(NotationHLayout*);


    /**
     * Return the staff ruler
     */
    StaffRuler* getRuler() { return m_ruler; }

    /**
     * Return the cursor
     */
    PositionCursor* getCursor();

    /**
     * Set the active item
     */
    void setActiveItem(ActiveItem* i) { m_activeItem = i; }

    /**
     * Return the active item
     */
    ActiveItem* activeItem() { return m_activeItem; }


    //--------------- Data members ---------------------------------

    KConfig* m_config;

    RosegardenGUIDoc* m_document;

    /// The current selection of Events (for cut/copy/paste)
    EventSelection* m_currentEventSelection;

    /// Displayed in the status bar, holds the pixmap of the current note
    QLabel* m_currentNotePixmap;

    /// Displayed in the status bar, shows the pitch the cursor is at
    QLabel* m_hoveredOverNoteName;

    /// Displayed in the status bar, shows the absolute time the cursor is at
    QLabel* m_hoveredOverAbsoluteTime;

    NotationCanvasView* m_canvasView;

    int m_currentStaff;

    std::vector<NotationStaff*> m_staffs;

    StaffRuler* m_ruler;
    ActiveItem* m_activeItem;

    std::string m_fontName;
    int m_fontSize;

    NotePixmapFactory *m_notePixmapFactory;
    NotePixmapFactory m_toolbarNotePixmapFactory;
    
    NotationHLayout* m_hlayout;
    NotationVLayout* m_vlayout;

    NotationTool* m_tool;

    template <class T>
    class ZoomSlider : public QSlider
    {
    public:
        ZoomSlider(const std::vector<T> &sizes, T initialValue,
                   Orientation, QWidget * parent, const char * name=0);
        virtual ~ZoomSlider();
        
        void reinitialise(const std::vector<T> &sizes, T initialValue);
        
    protected:
        static int getIndex(const std::vector<T> &, T size);
        std::vector<T> m_sizes;
    };
    
    ZoomSlider<int> *m_fontSizeSlider;

    std::vector<int> m_legatoDurations;

    KAction* m_selectDefaultNote;

    QCanvasLine *m_pointer;

    typedef std::set<NotationView *> NotationViewSet;
    static NotationViewSet m_viewsExtant;

    void redoLayoutAdvised(Rosegarden::Segment *segment = 0,
			   Rosegarden::timeT startTime = 0,
			   Rosegarden::timeT endTime = -1); // -1 => end of staff
};

#endif
