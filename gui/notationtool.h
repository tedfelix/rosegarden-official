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
 * for the NotationView (classic 'State' design pattern)
 *
 * @see NotationView#setTool()
 */
class NotationTool : public QObject, public KXMLGUIClient
{
public:
    /**
     * Create a new NotationTool
     *
     * \a menuName : the name of the menu defined in the XML rc file
     */
    NotationTool(const QString& menuName, NotationView*);
    virtual ~NotationTool();

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

protected:
    void createMenu(const QString& rcFileName);

    const QString m_menuName;

    NotationView* m_parentView;

    QPopupMenu* m_menu;
};

namespace Rosegarden { class SegmentNotationHelper; }

/**
 * This tool will insert notes on mouse click events
 */
class NoteInserter : public NotationTool
{
public:
    NoteInserter(Rosegarden::Note::Type, unsigned int dots,
                 NotationView*);
    ~NoteInserter();
    
    virtual void handleLeftButtonPress(int height, int staffNo,
                                       QMouseEvent*,
                                       NotationElement* el);

    /// Set the accidental for the notes which will be inserted
    static void setAccidental(Rosegarden::Accidental);

protected:
    /// this ctor is used by RestInserter
    NoteInserter(Rosegarden::Note::Type, unsigned int dots,
                 const QString& menuName, NotationView*);

    virtual Rosegarden::Event *doInsert(Rosegarden::SegmentNotationHelper&,
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
    RestInserter(Rosegarden::Note::Type, unsigned int dots,
                 NotationView*);
    
protected:
    virtual Rosegarden::Event *doInsert(Rosegarden::SegmentNotationHelper&,
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
    ClefInserter(std::string clefType, NotationView*);
    
    virtual void handleLeftButtonPress(int height, int staffNo,
                                       QMouseEvent*,
                                       NotationElement* el);
protected:
    Rosegarden::Clef m_clef;
};


/**
 * This tool will erase a note on mouse click events
 */
class NotationEraser : public NotationTool
{
    Q_OBJECT
public:
    NotationEraser(NotationView*);

    virtual void handleLeftButtonPress(int height, int staffNo,
                                       QMouseEvent*,
                                       NotationElement* el);

public slots:
    void toggleRestCollapse();
    
protected:

    bool m_collapseRest;
};

/**
 * Rectangular note selection
 */
class NotationSelector : public NotationTool
{
    Q_OBJECT

public:
    NotationSelector(NotationView*);
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
    /**
     * Set the current selection on the parent NotationView
     */
    void setViewCurrentSelection();

    QCanvasRectangle* m_selectionRect;
    bool m_updateRect;

    int m_clickedStaff;
    NotationElement *m_clickedElement;
};


/**
 * Selection pasting
 */
class NotationSelectionPaster : public NotationTool
{
public:
    NotationSelectionPaster(EventSelection&,
                            NotationView*);
    ~NotationSelectionPaster();
    
    virtual void handleLeftButtonPress(int height, int staffNo,
                                       QMouseEvent*,
                                       NotationElement* el);

protected:
    EventSelection& m_selection;
};

#endif
