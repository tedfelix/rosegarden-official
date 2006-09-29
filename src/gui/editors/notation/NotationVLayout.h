
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

#ifndef _RG_NOTATIONVLAYOUT_H_
#define _RG_NOTATIONVLAYOUT_H_

#include "base/FastVector.h"
#include "base/LayoutEngine.h"
#include "gui/general/ProgressReporter.h"
#include <map>
#include "base/Event.h"


class SlurList;
class QObject;


namespace Rosegarden
{

class Staff;
class Quantizer;
class NotePixmapFactory;
class NotationStaff;
class NotationProperties;
class Composition;


/**
 * Vertical notation layout
 *
 * computes the Y coordinate of notation elements
 */

class NotationVLayout : public ProgressReporter,
                        public VerticalLayoutEngine
{
public:
    NotationVLayout(Composition *c, NotePixmapFactory *npf,
                    const NotationProperties &properties,
                    QObject* parent, const char* name = 0);

    virtual ~NotationVLayout();

    void setNotePixmapFactory(NotePixmapFactory *npf) {
        m_npf = npf;
    }

    /**
     * Resets internal data stores for all staffs
     */
    virtual void reset();

    /**
     * Resets internal data stores for a specific staff
     */
    virtual void resetStaff(Staff &,
                            timeT = 0,
                            timeT = 0);

    /**
     * Lay out a single staff.
     */
    virtual void scanStaff(Staff &,
                           timeT = 0,
                           timeT = 0);

    /**
     * Do any layout dependent on more than one staff.  As it
     * happens, we have none, but we do have some layout that
     * depends on the final results from the horizontal layout
     * (for slurs), so we should do that here
     */
    virtual void finishLayout(timeT = 0,
                              timeT = 0);

private:

    void positionSlur(NotationStaff &staff, NotationElementList::iterator i);

    typedef FastVector<NotationElementList::iterator> SlurList;
    typedef std::map<Staff *, SlurList> SlurListMap;

    //--------------- Data members ---------------------------------

    SlurListMap m_slurs;
    SlurList &getSlurList(Staff &);

    Composition *m_composition;
    NotePixmapFactory *m_npf;
    const Quantizer *m_notationQuantizer;
    const NotationProperties &m_properties;
};


}

#endif
