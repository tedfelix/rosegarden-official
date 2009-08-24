/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2009 the Rosegarden development team.

    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#ifndef _RG_NOTEINSERTER_H_
#define _RG_NOTEINSERTER_H_

#include "base/NotationTypes.h"
#include "NotationTool.h"
#include "NotationElement.h"
#include "NoteStyle.h"
#include <QString>
#include "base/Event.h"


namespace Rosegarden
{

class ViewElement;
class Segment;
class NotationWidget;
class NotationStaff;
class Event;


/**
 * This tool will insert notes on mouse click events
 */
class NoteInserter : public NotationTool
{
    Q_OBJECT

    friend class NotationToolBox;

public:
    ~NoteInserter();

    virtual void handleLeftButtonPress(const NotationMouseEvent *);

    virtual FollowMode handleMouseMove(const NotationMouseEvent *);

    virtual void handleMouseRelease(const NotationMouseEvent *);

    virtual void ready();

    Note getCurrentNote() {
        return Note(m_noteType, m_noteDots);
    }

    /// Insert a note as if the user has clicked at the given time & pitch
    void insertNote(Segment &segment,
                    timeT insertionTime,
                    int pitch,
                    Accidental accidental,
                    bool suppressPreview = false);

    static const QString ToolName;

    /** Hacky, but at least this way the calling code doesn't need to know so
     * many internal details.  In practice, the alternate ctor is only called by
     * RestInserter with a "menu name" of "RestInserter."  This function returns
     * which of our ctors was called, which effectively tells the calling code
     * whether this is a NoteInserter for inserting notes, or a NoteInserter for
     * inserting rests.  It's all really a bit of a fuster cluck, but I didn't
     * build the fuster cluck, I just made it even worse.  Yay me!
     */
    bool isaRestInserter() { return m_isaRestInserter; };

public slots:
    /// Set the type of note (quaver, breve...) which will be inserted
    void slotSetNote(Note::Type);

    /// Set the number of dots the inserted note will have
    void slotSetDots(unsigned int dots);
 
    /// Set the accidental for the notes which will be inserted
    void slotSetAccidental(Accidental, bool follow);

protected:
    NoteInserter(NotationWidget *);

    /// this ctor is used by RestInserter
    NoteInserter(QString rcFileName, QString menuName, NotationWidget *);

    timeT getOffsetWithinRest(NotationStaff *,
                              const NotationElementList::iterator&,
                              double &canvasX);

    int getOttavaShift(Segment &segment, timeT time);

    virtual Event *doAddCommand(Segment &,
                                timeT time,
                                timeT endTime,
                                const Note &,
                                int pitch, Accidental);

    virtual bool computeLocationAndPreview(const NotationMouseEvent *e);
    virtual void showPreview();
    virtual void clearPreview();

protected slots:
    // RMB menu slots
    void slotNoAccidental();
    void slotFollowAccidental();
    void slotSharp();
    void slotFlat();
    void slotNatural();
    void slotDoubleSharp();
    void slotDoubleFlat();
    void slotToggleDot();
    void slotToggleAutoBeam();

    void slotEraseSelected();
    void slotSelectSelected();
    void slotRestsSelected();

protected:
    //--------------- Data members ---------------------------------

    Note::Type m_noteType;
    unsigned int m_noteDots;
    bool m_autoBeam;
    bool m_matrixInsertType;
    NoteStyleName m_defaultStyle;

    bool m_clickHappened;
    timeT m_clickTime;
    int m_clickSubordering;
    int m_clickPitch;
    int m_clickHeight;
    NotationStaff *m_clickStaff;
    double m_clickInsertX;
    float m_targetSubordering;

    Accidental m_accidental;
    Accidental m_lastAccidental;
    bool m_followAccidental;
    bool m_isaRestInserter;

    static const char* m_actionsAccidental[][4];
};


}

#endif
