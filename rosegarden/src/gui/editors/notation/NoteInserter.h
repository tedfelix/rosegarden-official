
/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.

    This program is Copyright 2000-2006
        Guillaume Laurent   <glaurent@telegraph-road.org>,
        Chris Cannam        <cannam@all-day-breakfast.com>,
        Richard Bown        <richard.bown@ferventsoftware.com>

    The moral rights of Guillaume Laurent, Chris Cannam, and Richard
    Bown to claim authorship of this work have been asserted.

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
#include <qstring.h>
#include "base/Event.h"


class QMouseEvent;


namespace Rosegarden
{

class ViewElement;
class Segment;
class NotationView;
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

    virtual void handleLeftButtonPress(timeT,
                                       int height,
                                       int staffNo,
                                       QMouseEvent*,
                                       ViewElement* el);

    virtual int handleMouseMove(timeT time,
                                int height,
                                QMouseEvent*);

    virtual void handleMouseRelease(timeT time,
                                    int height,
                                    QMouseEvent*);

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

public slots:
    /// Set the type of note (quaver, breve...) which will be inserted
    void slotSetNote(Note::Type);

    /// Set the nb of dots the inserted note will have
    void slotSetDots(unsigned int dots);
 
    /// Set the accidental for the notes which will be inserted
    void slotSetAccidental(Accidental, bool follow);

protected:
    NoteInserter(NotationView*);

    /// this ctor is used by RestInserter
    NoteInserter(const QString& menuName, NotationView*);

    timeT getOffsetWithinRest(int staffNo,
					  const NotationElementList::iterator&,
					  double &canvasX);

    int getOttavaShift(Segment &segment, timeT time);

    virtual Event *doAddCommand(Segment &,
					    timeT time,
					    timeT endTime,
					    const Note &,
					    int pitch, Accidental);

    virtual bool computeLocationAndPreview(QMouseEvent *e);
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
    int m_clickPitch;
    int m_clickHeight;
    int m_clickStaffNo;
    double m_clickInsertX;

    Accidental m_accidental;
    Accidental m_lastAccidental;
    bool m_followAccidental;

    static const char* m_actionsAccidental[][4];
};


}

#endif
