/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2018 the Rosegarden development team.

    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#ifndef RG_MATRIXVIEWSEGMENT_H
#define RG_MATRIXVIEWSEGMENT_H

#include "base/ViewSegment.h"

namespace Rosegarden
{

class MatrixScene;
class Segment;
class MatrixElement;
class MidiKeyMapping;

class MatrixViewSegment : public ViewSegment
{
public:
    MatrixViewSegment(MatrixScene *,
                      Segment *,
                      bool drumMode);
    virtual ~MatrixViewSegment();

    void endMarkerTimeChanged(const Segment *segment, bool shorten);

    SegmentRefreshStatus &getRefreshStatus() const;
    void resetRefreshStatus();

    void updateElements(timeT from, timeT to);

protected:
//!!!    const MidiKeyMapping *getKeyMapping() const;

    /**
     * Override from ViewSegment
     * Wrap only notes 
     */
    virtual bool wrapEvent(Event*);

    /**
     * Override from ViewSegment
     */
    virtual void eventAdded(const Segment *, Event *);

    /**
     * Override from ViewSegment
     * Let tools know if their current element has gone
     */
    virtual void eventRemoved(const Segment *, Event *);

    virtual ViewElement* makeViewElement(Event *);

    MatrixScene *m_scene;
    bool m_drum;
    unsigned int m_refreshStatusId;
};

}

#endif
