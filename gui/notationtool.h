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

#ifndef NOTATIONTOOL_H
#define NOTATIONTOOL_H

#include <qevent.h>
#include <qobject.h>

#include <kxmlguiclient.h>

#include "NotationTypes.h"
#include "Segment.h"

class QCanvasRectangle;

class NotationView;
class NotationElement;
class EventSelection;
class QPopupMenu;

//////////////////////////////////////////////////////////////////////
//               Notation Tools
//////////////////////////////////////////////////////////////////////

/**
 * Notation tool base class.
 *
 * A NotationTool represents one of the items on the notation toolbars
 * (notes, rests, clefs, eraser, etc...). It handle mouse click events
 * for the NotationView ('State' design pattern).
 *
 * This class is a singleton
 *
 * @see NotationView#setTool()
 */
class NotationTool : public QObject, public KXMLGUIClient
{
public:
    virtual ~NotationTool();

    /**
     * Is called by NotationView after creation
     * Add any signal/slot connection here
     */
    virtual void finalize();

    /**
     * Dispatch the event to Left/Middle/Right MousePress
     */
    virtual void handleMousePress(int height, int staffNo,
                                  QMouseEvent *event,
                                  NotationElement*);

    /**
     * Main operation of the tool
     */
    virtual void handleLeftButtonPress(int height, int staffNo,
                                       QMouseEvent *event,
                                       NotationElement*) = 0;

    /**
     * Do nothing
     */
    virtual void handleMidButtonPress(int height, int staffNo,
                                      QMouseEvent*,
                                      NotationElement*);

    /**
     * Show option menu
     */
    virtual void handleRightButtonPress(int height, int staffNo,
                                        QMouseEvent*,
                                        NotationElement*);

    /**
     * Do nothing
     */
    virtual void handleMouseDblClick(int height, int staffNo,
                                     QMouseEvent*,
                                     NotationElement*);

    /**
     * Do nothing
     */
    virtual void handleMouseMove(QMouseEvent*);

    /**
     * Do nothing
     */
    virtual void handleMouseRelease(QMouseEvent*);

    /**
     * Show the menu if there is one
     */
    virtual void showMenu();

    void setParentView(NotationView*);

protected:
    /**
     * Create a new NotationTool
     *
     * \a menuName : the name of the menu defined in the XML rc file
     */
    NotationTool(const QString& menuName, NotationView*);

    void createMenu(const QString& rcFileName);

    const QString m_menuName;

    NotationView* m_parentView;

    QPopupMenu* m_menu;

    static NotationTool* m_instance;
};

namespace Rosegarden { class SegmentNotationHelper; }

/**
 * This tool will insert notes on mouse click events
 */
class NoteInserter : public NotationTool
{
    Q_OBJECT

public:
    static NotationTool* getInstance(NotationView*);

    ~NoteInserter();

    virtual void handleLeftButtonPress(int height, int staffNo,
                                       QMouseEvent*,
                                       NotationElement* el);

    virtual void finalize();

public slots:
    /// Set the type of note (quaver, breve...) which will be inserted
    void setNote(Rosegarden::Note::Type);

    /// Set the nb of dots the inserted note will have
    void setDots(unsigned int dots);
 
    /// Set the accidental for the notes which will be inserted
    void setAccidental(Rosegarden::Accidental);

    /**
     * Set the accidental for the notes which will be inserted
     * and put the parent view toolbar in sync
     */
    void setAccidentalSync(Rosegarden::Accidental);

protected:
    NoteInserter(NotationView*);

    /// this ctor is used by RestInserter
    NoteInserter(const QString& menuName, NotationView*);

    virtual Rosegarden::Event *doAddCommand(Rosegarden::Segment &,
					    Rosegarden::timeT time,
					    Rosegarden::timeT endTime,
					    const Rosegarden::Note &,
					    int pitch, Rosegarden::Accidental);

protected slots:
    // RMB menu slots
    void slotNoAccidental();
    void slotSharp();
    void slotFlat();
    void slotNatural();
    void slotDoubleSharp();
    void slotDoubleFlat();
    void slotToggleDot();

    void slotEraseSelected();
    void slotSelectSelected();

protected:
    Rosegarden::Note::Type m_noteType;
    unsigned int m_noteDots;

    Rosegarden::Accidental m_accidental;

    static const char* m_actionsAccidental[][4];
    static NotationTool* m_instance;
};

/**
 * This tool will insert rests on mouse click events
 */
class RestInserter : public NoteInserter
{
public:
    static NotationTool* getInstance(NotationView*);

protected:
    RestInserter(NotationView*);

    virtual Rosegarden::Event *doAddCommand(Rosegarden::Segment &,
					    Rosegarden::timeT time,
					    Rosegarden::timeT endTime,
					    const Rosegarden::Note &,
					    int pitch, Rosegarden::Accidental);

    static NotationTool* m_instance;
};

/**
 * This tool will insert clefs on mouse click events
 */
class ClefInserter : public NotationTool
{
public:
    static NotationTool* getInstance(NotationView*);

    void setClef(std::string clefType);

    virtual void finalize();

    virtual void handleLeftButtonPress(int height, int staffNo,
                                       QMouseEvent*,
                                       NotationElement* el);
protected:
    ClefInserter(NotationView*);
    
    Rosegarden::Clef m_clef;

    static NotationTool* m_instance;
};


/**
 * This tool will erase a note on mouse click events
 */
class NotationEraser : public NotationTool
{
    Q_OBJECT
public:
    static NotationTool* getInstance(NotationView*);

    virtual void finalize();

    virtual void handleLeftButtonPress(int height, int staffNo,
                                       QMouseEvent*,
                                       NotationElement* el);

public slots:
    void toggleRestCollapse();
    
protected:
    NotationEraser(NotationView*);

    bool m_collapseRest;

    static NotationTool* m_instance;
};

/**
 * Rectangular note selection
 */
class NotationSelector : public NotationTool
{
    Q_OBJECT

public:
    static NotationTool* getInstance(NotationView*);

    ~NotationSelector();
    
    virtual void handleLeftButtonPress(int height, int staffNo,
                                       QMouseEvent*,
                                       NotationElement* el);

    virtual void handleMouseMove(QMouseEvent*);
    virtual void handleMouseRelease(QMouseEvent*);

    virtual void handleMouseDblClick(int height, int staffNo,
                                     QMouseEvent*,
                                     NotationElement*);

    /**
     * Returns the currently selected events
     *
     * The returned result is owned by the caller
     */
    EventSelection* getSelection();

public slots:
    /**
     * Hide the selection rectangle
     *
     * Should be called after a cut or a copy has been
     * performed
     */
    void hideSelection();
    
protected:
    NotationSelector(NotationView*);

    /**
     * Set the current selection on the parent NotationView
     */
    void setViewCurrentSelection();

    QCanvasRectangle* m_selectionRect;
    bool m_updateRect;

    int m_clickedStaff;
    NotationElement *m_clickedElement;

    static NotationTool* m_instance;
};


/**
 * Selection pasting
 */
class NotationSelectionPaster : public NotationTool
{
public:
    static NotationTool* getInstance(NotationView*);

    ~NotationSelectionPaster();
    
    virtual void handleLeftButtonPress(int height, int staffNo,
                                       QMouseEvent*,
                                       NotationElement* el);

protected:
    NotationSelectionPaster(EventSelection&,
                            NotationView*);

    EventSelection& m_selection;

    static NotationTool* m_instance;
};

#endif
