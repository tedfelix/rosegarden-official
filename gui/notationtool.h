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

#include "NotationTypes.h"

class QCanvasRectangle;

class NotationView;
class NotationElement;
class EventSelection;

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

    virtual void handleMousePress(int height, int staffNo,
                                  const QPoint &eventPos,
                                  NotationElement*) = 0;

    /// defaults to doing nothing
    virtual void handleMouseDblClick(int height, int staffNo,
                                     const QPoint &eventPos,
                                     NotationElement*);

    /// does nothing by default
    virtual void handleMouseMove(QMouseEvent*);

    /// does nothing by default
    virtual void handleMouseRelease(QMouseEvent*);

protected:
    NotationView& m_parentView;
};

namespace Rosegarden { class SegmentNotationHelper; }

/**
 * This tool will insert notes on mouse click events
 */
class NoteInserter : public NotationTool
{
public:
    NoteInserter(Rosegarden::Note::Type, unsigned int dots, NotationView&);
    ~NoteInserter();
    
    virtual void handleMousePress(int height, int staffNo,
                                  const QPoint &eventPos,
                                  NotationElement* el);

    /// Set the accidental for the notes which will be inserted
    static void setAccidental(Rosegarden::Accidental);

protected:
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
    RestInserter(Rosegarden::Note::Type, unsigned int dots, NotationView&);
    
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
    ClefInserter(std::string clefType, NotationView&);
    
    virtual void handleMousePress(int height, int staffNo,
                                  const QPoint &eventPos,
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

    virtual void handleMousePress(int height, int staffNo,
                                  const QPoint &eventPos,
                                  NotationElement* el);
protected:

    bool m_collapseRest;
};

/**
 * Rectangular note selection
 */
class NotationSelector : public QObject, public NotationTool
{
    Q_OBJECT

public:
    NotationSelector(NotationView&);
    ~NotationSelector();
    
    virtual void handleMousePress(int height, int staffNo,
                                  const QPoint &eventPos,
                                  NotationElement* el);

    virtual void handleMouseMove(QMouseEvent*);
    virtual void handleMouseRelease(QMouseEvent*);

    virtual void handleMouseDblClick(int height, int staffNo,
                                     const QPoint &eventPos,
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
    NotationSelectionPaster(NotationView&, EventSelection&);
    ~NotationSelectionPaster();
    
    virtual void handleMousePress(int height, int staffNo,
                                  const QPoint &eventPos,
                                  NotationElement* el);

protected:
    EventSelection& m_selection;
};

#endif
