// -*- c-basic-offset: 4 -*-

/*
    Rosegarden-4 v0.1
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
     * Construct an XmlStorableEvent out of the XML attributes \a atts
     */
    XmlStorableEvent(const QXmlAttributes& atts);

    /**
     * Construct an XmlStorableEvent from the specified Event
     */
    XmlStorableEvent(const Rosegarden::Event&);

    /**
     * Set the Element properties from the XML attributes \a atts
     */
    void setPropertiesFromAttributes(const QXmlAttributes& atts);

    /**
     * Get the XML string representing the object
     */
    QString        toXmlString() const;

    /**
     * Get the XML string representing the specified Event
     */
    static QString toXmlString(const Event&);
};

#endif
