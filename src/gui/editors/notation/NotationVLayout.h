
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

#ifndef RG_NOTATIONVLAYOUT_H
#define RG_NOTATIONVLAYOUT_H

#include "base/FastVector.h"
#include "base/LayoutEngine.h"
#include <map>
#include "base/Event.h"

#include "NotationElement.h"


class SlurList;
class QObject;


namespace Rosegarden
{

class ViewSegment;
class Quantizer;
class Composition;
class NotePixmapFactory;
class NotationStaff;
class NotationProperties;
class Composition;


/**
 * Vertical notation layout
 *
 * computes the Y coordinate of notation elements
 */

class NotationVLayout : public VerticalLayoutEngine,
                        public QObject  // For tr().  Can probably be cleaned up.
{
    //Q_OBJECT
public:
    NotationVLayout(Composition *c, NotePixmapFactory *npf,
                    const NotationProperties &properties,
                    QObject* parent);

    virtual ~NotationVLayout();

    void setNotePixmapFactory(NotePixmapFactory *npf) {
        m_npf = npf;
    }

    /**
     * Resets internal data stores for all staffs
     */
    void reset() override;

    /**
     * Lay out a single staff.
     */
    void scanViewSegment(ViewSegment &,
				 timeT startTime,
				 timeT endTime,
				 bool full) override;

    /**
     * Do any layout dependent on more than one staff.  As it
     * happens, we have none, but we do have some layout that
     * depends on the final results from the horizontal layout
     * (for slurs), so we should do that here
     */
    void finishLayout(timeT startTime,
                              timeT endTime,
			      bool full) override;

private:
    void positionSlur(NotationStaff &staff, NotationElementList::iterator i);

    typedef FastVector<NotationElementList::iterator> SlurList;
    typedef std::map<ViewSegment *, SlurList> SlurListMap;

    //--------------- Data members ---------------------------------

    SlurListMap m_slurs;
    SlurList &getSlurList(ViewSegment &);

    Composition *m_composition;
    NotePixmapFactory *m_npf;
    const Quantizer *m_notationQuantizer;
    const NotationProperties &m_properties;

    bool m_showRepeated;
    bool m_distributeVerses;
};


}

#endif
