
/*
    Rosegarden-4 v0.1
    A sequencer and musical notation editor.

    This program is Copyright 2000-2001
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

#include "xmlstorableevent.h"
#include "NotationTypes.h"

#include "rosedebug.h"

XmlStorableEvent::XmlStorableEvent(const QXmlAttributes &attributes)
{
    setPackage("core"); //!!! sensible default for storable events?

    for (int i = 0; i < attributes.length(); ++i) {
	QString attrName(attributes.qName(i)),
            attrVal(attributes.value(i));

        // special cases first : package, type, duration
	if (attrName == "package") {

            setPackage(attrVal.latin1());

        } else if (attrName == "type") {

            setType(attrVal.latin1());

        } else if (attrName == "duration") {

            bool isNumeric = true;
            Event::timeT d = attrVal.toUInt(&isNumeric);

            if (!isNumeric) {
		try {
		    Note n(attrVal.latin1());
		    setDuration(n.getDuration());
		} catch (Note::BadType b) {
                    kdDebug(KDEBUG_AREA) << "XmlStorableEvent::XmlStorableEvent: Bad duration: " << attrVal << " (Note choked on \"" << b.type << "\")" << endl;
		}
            } else {
		setDuration(d);
	    }

        } else {

            // set generic property
            //
            QString val(attrVal);
            
            // Check if boolean val
            QString valLowerCase(val.lower());
            bool isNumeric;
            int numVal;

            if (valLowerCase == "true" || valLowerCase == "false") {

                set<Bool>(attrName.latin1(), valLowerCase == "true");

            } else {

                // Not a bool, check if integer val
                numVal = val.toInt(&isNumeric);
                if (isNumeric) {
                    set<Int>(attrName.latin1(), numVal);
                } else {
                    // not an int either, default to string
                    set<String>(attrName.latin1(), attrVal.latin1());
                }
            }

        }

    }
}

XmlStorableEvent::XmlStorableEvent(const Event &e)
    : Event(e)
{
}


QString
XmlStorableEvent::toXmlString() const
{
    QString res = "<event";

    if (package().length())
        res += QString(" package=\"%1\"").arg(package().c_str());

    if (type().length())
        res += QString(" type=\"%1\"").arg(type().c_str());

    res += QString(" duration=\"%1\"").arg(getDuration());


    for (PropertyMap::const_iterator i = properties().begin();
         i != properties().end(); ++i) {

        res += QString(" %1=\"%2\"")
            .arg((*i).first.c_str())
            .arg((*i).second->unparse().c_str());
    }
    

    res += "/>";
    return res;
}

QString
XmlStorableEvent::toXmlString(const Event &e)
{
    QString res = "<event";

    if (e.package().length())
        res += QString(" package=\"%1\"").arg(e.package().c_str());

    if (e.type().length())
        res += QString(" type=\"%1\"").arg(e.type().c_str());

    res += QString(" duration=\"%1\"").arg(e.getDuration());


    for (PropertyMap::const_iterator i = e.properties().begin();
         i != e.properties().end(); ++i) {

        res += QString(" %1=\"%2\"")
            .arg((*i).first.c_str())
            .arg((*i).second->unparse().c_str());
    }
    

    res += "/>";
    return res;
}

