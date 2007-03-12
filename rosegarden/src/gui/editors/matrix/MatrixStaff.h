
/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.

    This program is Copyright 2000-2007
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

#ifndef _RG_MATRIXSTAFF_H_
#define _RG_MATRIXSTAFF_H_

#include "base/Staff.h"
#include "gui/general/LinedStaff.h"
#include "base/Event.h"


class QCanvas;


namespace Rosegarden
{

class ViewElement;
class SnapGrid;
class Segment;
class MidiKeyMapping;
class MatrixView;
class MatrixElement;
class Event;


class MatrixStaff : public LinedStaff
{
public:
    MatrixStaff(QCanvas *canvas,
                Segment *segment, 
                SnapGrid *snapGrid,
                int id, 
                int vResolution,
                MatrixView *view);
    virtual ~MatrixStaff();

protected:
    virtual int getLineCount() const;
    virtual int getLegerLineCount() const;
    virtual int getBottomLineHeight() const;
    virtual int getHeightPerLine() const;
    virtual bool elementsInSpaces() const;
    virtual bool showBeatLines() const;

    const MidiKeyMapping *getKeyMapping() const;

    /**
     * Override from Staff<T>
     * Wrap only notes 
     */
    virtual bool wrapEvent(Event*);

    /**
     * Override from Staff<T>
     * Let tools know if their current element has gone
     */
    virtual void eventRemoved(const Segment *, Event *);

    virtual ViewElement* makeViewElement(Event*);

public:
    LinedStaff::setResolution;

//     double getTimeScaleFactor() const { return m_scaleFactor * 2; } // TODO: GROSS HACK to enhance matrix resolution (see also in matrixview.cpp) - BREAKS MATRIX VIEW, see bug 1000595
    double getTimeScaleFactor() const { return m_scaleFactor; }
    void setTimeScaleFactor(double f) { m_scaleFactor = f; }

    int getElementHeight() { return m_resolution; }

    virtual void positionElements(timeT from,
                                  timeT to);

    virtual void positionElement(ViewElement*);

    // Get an element for an Event
    //
    MatrixElement* getElement(Event *event);

private:
    double m_scaleFactor;

    MatrixView     *m_view;
};


}

#endif
