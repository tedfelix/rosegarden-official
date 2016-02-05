/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2016 the Rosegarden development team.

    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#ifndef RG_NOTERESTINSERTER_H
#define RG_NOTERESTINSERTER_H

#include "base/Event.h"
#include "base/NotationTypes.h"
#include "NotationTool.h"
#include "NotationElement.h"
#include "NoteStyle.h"
#include "NotationMouseEvent.h"
#include <QString>

namespace Rosegarden
{

class ViewElement;
class Segment;
class NotationWidget;
class NotationStaff;
class Event;


/**
 * This tool will insert notes or rests on mouse click events
 */
class NoteRestInserter : public NotationTool
{
    Q_OBJECT

    friend class NotationToolBox;

public:
    ~NoteRestInserter();

    virtual void handleLeftButtonPress(const NotationMouseEvent *);
    
    virtual void handleMidButtonPress(const NotationMouseEvent *);

    virtual FollowMode handleMouseMove(const NotationMouseEvent *);

    virtual void handleMouseRelease(const NotationMouseEvent *);
    
    virtual void handleWheelTurned(int delta);

    virtual bool needsWheelEvents() { return m_quickEdit; }

    virtual void ready();

    virtual void stow();

    Note getCurrentNote() {
        return Note(m_noteType, m_noteDots);
    }

    /// Insert a note as if the user has clicked at the given time & pitch
    void insertNote(Segment &segment,
                    timeT insertionTime,
                    int pitch,
                    Accidental accidental,
                    int velocity,
                    bool suppressPreview = false);
    
    /**
     * Useful to get the tool name from a NotationTool object
     */ 
    virtual const QString getToolName() { return ToolName; }

    static const QString ToolName;

    /**
     * Returns the state of the tool.  true, if it in rest insertert mode.
     */
    bool isaRestInserter() { return m_isaRestInserter; };

    /**
     * Sets the state of the tool. true for rest inserting, false for
     * note inserting.
     */
    void setToRestInserter(bool rest) {m_isaRestInserter = rest;};

    /**
     * Show the menu if there is one.
     * This is an over ride of BaseTool::showMenu().
     */
    virtual void showMenu();

public slots:
    /// Set the type of note (quaver, breve...) which will be inserted
    void slotSetNote(Note::Type nt);

    /// Set the number of dots the inserted note will have
    void slotSetDots(unsigned int dots);
 
    /// Set the accidental for the notes which will be inserted
    void slotSetAccidental(Accidental, bool follow);

protected:
    NoteRestInserter(NotationWidget *);

    NoteRestInserter(QString rcFileName, QString menuName, NotationWidget *);

    timeT getOffsetWithinRest(NotationStaff *,
                              const NotationElementList::iterator&,
                              double &canvasX);

    int getOttavaShift(Segment &segment, timeT time);

    virtual Event *doAddCommand(Segment &,
                                timeT time,
                                timeT endTime,
                                const Note &,
                                int pitch, Accidental,
                                int velocity = 0);

    virtual bool computeLocationAndPreview(const NotationMouseEvent *e,
                                           bool play);
    virtual void showPreview(bool play);
    virtual void clearPreview();
    
    void setCursorShape();
    Accidental getAccidentalFromDeadKeys(const NotationMouseEvent *e);
    
    void synchronizeWheel();

protected slots:
    // RMB menu slots
    void slotToggleDot();
    void slotToggleAutoBeam();

    void slotEraseSelected();
    void slotSelectSelected();
    void slotRestsSelected();
    void slotNotesSelected();

protected:
    //--------------- Data members ---------------------------------

    Note::Type m_noteType;
    unsigned int m_noteDots;
    bool m_autoBeam;
    bool m_autoTieBarlines;
    bool m_matrixInsertType;
    NoteStyleName m_defaultStyle;
    bool m_alwaysPreview;
    bool m_quickEdit;         // Select durations with mouse wheel

    bool m_leftButtonDown;
    bool m_clickHappened;
    timeT m_clickTime;
    int m_clickSubordering;
    int m_clickPitch;
    int m_clickHeight;
    NotationStaff *m_clickStaff;
    Accidental m_clickQuickAccidental;
    double m_clickInsertX;
    float m_targetSubordering;

    Accidental m_accidental;
    Accidental m_lastAccidental;
    bool m_followAccidental;
    bool m_isaRestInserter;
    
    int m_wheelIndex;              // Index of current duration
    bool m_processingWheelTurned;  // Use by synchronizeWheel()
    NotationMouseEvent m_lastMouseEvent;

// Obsolete ?
//    static const char* m_actionsAccidental[][4];
};


}

#endif
