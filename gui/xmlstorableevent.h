// -*- c-basic-offset: 4 -*-

/*
    Rosegarden-4 v0.2
    A sequencer and musical notation editor.

    This program is Copyright 2000-2002
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

#ifndef XMLSTORABLEELEMENT_H
#define XMLSTORABLEELEMENT_H

#include <qxml.h>

#include "Event.h"
namespace Rosegarden { class Quantizer; }

/**
 * An Event which can generate an XML representation of itself,
 * or which can be constructed from a set of XML attributes
 *
 * @see RoseXmlHandler
 */
class XmlStorableEvent : public Rosegarden::Event
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
		     Rosegarden::timeT &absoluteTime);

    /**
     * Construct an XmlStorableEvent from the specified Event.
     */
    XmlStorableEvent(Rosegarden::Event&);

    /**
     * Set the Element properties from the XML attributes \a atts
     */
    void setPropertiesFromAttributes(const QXmlAttributes& atts);

    /**
     * Get the XML string representing the object.  If the absolute
     * time of the event differs from the given absolute time, include
     * the difference between the two as a timeOffset attribute.
     * If expectedTime == 0, include an absoluteTime attribute instead.
     */
    QString toXmlString(Rosegarden::timeT expectedTime = 0) const;
};

#endif
