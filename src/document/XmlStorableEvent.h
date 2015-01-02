
/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2015 the Rosegarden development team.

    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#ifndef RG_XMLSTORABLEEVENT_H
#define RG_XMLSTORABLEEVENT_H

#include "base/Event.h"
#include <qxml.h>


class QXmlAttributes;


namespace Rosegarden
{



/**
 * An Event which can generate an XML representation of itself,
 * or which can be constructed from a set of XML attributes
 *
 * @see RoseXmlHandler
 */
class XmlStorableEvent : public Event
{
public:
    /**
     * Construct an XmlStorableEvent out of the XML attributes \a atts.
     * If the attributes do not include absoluteTime, use the given
     * value plus the value of any timeOffset attribute.  If the
     * attributes include absoluteTime or timeOffset, update the given
     * absoluteTime reference accordingly.
     */
    XmlStorableEvent(const QXmlAttributes& atts,
                     timeT &absoluteTime);

    /**
     * Construct an XmlStorableEvent from the specified Event.
     */
    XmlStorableEvent(Event&);

    /**
     * Set a property from the XML attributes \a atts
     */
    void setPropertyFromAttributes(const QXmlAttributes& atts,
                                   bool persistent);
};


}

#endif
