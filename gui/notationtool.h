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

#include "NotationTypes.h"
#include "Segment.h"

#include "edittool.h"

class QCanvasRectangle;

class NotationView;
class NotationElement;
class EventSelection;
class QPopupMenu;

//////////////////////////////////////////////////////////////////////
//               Notation Tools
//////////////////////////////////////////////////////////////////////

class NotationTool;

/**
 * NotationToolBox : maintains a single instance of each registered tool
 *
 * Tools are fetched from a name
 */
class NotationToolBox : public EditToolBox
{
public:
    NotationToolBox(NotationView* parent);

protected:
    virtual EditTool* createTool(const QString& toolName);

    //--------------- Data members ---------------------------------

    NotationView* m_nParentView;
};


/**
 * Notation tool base class.
 *
 * A NotationTool represents one of the items on the notation toolbars
 * (notes, rests, clefs, eraser, etc...). It handle mouse click events
 * for the NotationView ('State' design pattern).
 *
 * A NotationTool can have a menu, normally activated through a right
 * mouse button click. This menu is defined in an XML file, see
 * NoteInserter and noteinserter.rc for an example.
 *
 * This class is a "semi-singleton", that is, only one instance per
 * NotationView window is created. This is because menu creation is
 * slow, and the fact that a tool can trigger the setting of another
 * tool through a menu choice). This is maintained with the
 * NotationToolBox class This means we can't rely on the ctor/dtor to
 * perform setting up, like mouse cursor changes for instance. Use the
 * ready() and stow() method for this.
 *
 * @see NotationView#setTool()
 * @see NotationToolBox
 */
class NotationTool : public EditTool
{
    friend NotationToolBox;

public:
    virtual ~NotationTool();

    /**
     * Is called by NotationView when the tool is set as current
     * Add any setup here
     */
    virtual void ready();

protected:
    /**
     * Create a new NotationTool
     *
     * \a menuName : the name of the menu defined in the XML rc file
     */
    NotationTool(const QString& menuName, NotationView*);

    //--------------- Data members ---------------------------------

    NotationView* m_nParentView;
};

namespace Rosegarden { class SegmentNotationHelper; }

/**
 * This tool will insert notes on mouse click events
 */
class NoteInserter : public NotationTool
{
    Q_OBJECT

    friend NotationToolBox;

public:
    ~NoteInserter();

    virtual void handleLeftButtonPress(int height,
                                       Rosegarden::timeT,
                                       int staffNo,
                                       QMouseEvent*,
                                       Rosegarden::ViewElement* el);

    virtual void ready();

    static const QString ToolName;

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
    //--------------- Data members ---------------------------------

    Rosegarden::Note::Type m_noteType;
    unsigned int m_noteDots;

    Rosegarden::Accidental m_accidental;

    static const char* m_actionsAccidental[][5];
};

/**
 * This tool will insert rests on mouse click events
 */
class RestInserter : public NoteInserter
{
    friend NotationToolBox;

public:

    static const QString ToolName;

protected:
    RestInserter(NotationView*);

    virtual Rosegarden::Event *doAddCommand(Rosegarden::Segment &,
					    Rosegarden::timeT time,
					    Rosegarden::timeT endTime,
					    const Rosegarden::Note &,
					    int pitch, Rosegarden::Accidental);

};

/**
 * This tool will insert clefs on mouse click events
 */
class ClefInserter : public NotationTool
{
    friend NotationToolBox;

public:
    void setClef(std::string clefType);

    virtual void ready();

    virtual void handleLeftButtonPress(int height,
                                       Rosegarden::timeT,
                                       int staffNo,
                                       QMouseEvent*,
                                       Rosegarden::ViewElement* el);
    static const QString ToolName;

protected:
    ClefInserter(NotationView*);
    
    //--------------- Data members ---------------------------------

    Rosegarden::Clef m_clef;
};


/**
 * This tool will erase a note on mouse click events
 */
class NotationEraser : public NotationTool
{
    Q_OBJECT

    friend NotationToolBox;

public:

    virtual void ready();

    virtual void handleLeftButtonPress(int height,
                                       Rosegarden::timeT,
                                       int staffNo,
                                       QMouseEvent*,
                                       Rosegarden::ViewElement* el);
    static const QString ToolName;

public slots:
    void toggleRestCollapse();
    
protected:
    NotationEraser(NotationView*);

    //--------------- Data members ---------------------------------

    bool m_collapseRest;
};

/**
 * Rectangular note selection
 */
class NotationSelector : public NotationTool
{
    Q_OBJECT

    friend NotationToolBox;

public:

    virtual void handleLeftButtonPress(int height,
                                       Rosegarden::timeT,
                                       int staffNo,
                                       QMouseEvent*,
                                       Rosegarden::ViewElement* el);

    virtual void handleMouseMove(int height,
                                 Rosegarden::timeT,
                                 QMouseEvent*);

    virtual void handleMouseRelease(int height,
                                    Rosegarden::timeT time,
                                    QMouseEvent*);

    virtual void handleMouseDblClick(int height,
                                     Rosegarden::timeT,
                                     int staffNo,
                                     QMouseEvent*,
                                     Rosegarden::ViewElement*);

    /**
     * Create the selection rect
     *
     * We need this because so NotationView deletes all QCanvasItems
     * along with it. This happens before the NotationSelector is
     * deleted, so we can't delete the selection rect in
     * ~NotationSelector because that leads to double deletion.
     */
    virtual void ready();

    /**
     * Delete the selection rect.
     */
    virtual void stow();

    /**
     * Returns the currently selected events
     *
     * The returned result is owned by the caller
     */
    EventSelection* getSelection();

    static const QString ToolName;

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

    //--------------- Data members ---------------------------------

    QCanvasRectangle* m_selectionRect;
    bool m_updateRect;

    int m_clickedStaff;
    NotationElement *m_clickedElement;

};


/**
 * Selection pasting - unused at the moment
 */
class NotationSelectionPaster : public NotationTool
{
public:

    ~NotationSelectionPaster();
    
    virtual void handleLeftButtonPress(int height, int staffNo,
                                       Rosegarden::timeT,
                                       QMouseEvent*,
                                       Rosegarden::ViewElement* el);

protected:
    NotationSelectionPaster(EventSelection&,
                            NotationView*);

    //--------------- Data members ---------------------------------

    EventSelection& m_selection;

};

#endif
