
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
    for (int i = 0; i < attributes.length(); ++i) {
	QString attrName(attributes.qName(i)),
            attrVal(attributes.value(i));

	if (attrName == "package") {

            kdDebug(KDEBUG_AREA) << "XmlStorableEvent::XmlStorableEvent: Warning: XML still uses deprecated \"package\" attribute" << endl;

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

    if (getType().length())
        res += QString(" type=\"%1\"").arg(getType().c_str());

    res += QString(" duration=\"%1\"").arg(getDuration());

    PropertyNames propertyNames(getPersistentPropertyNames());
    for (PropertyNames::const_iterator i = propertyNames.begin();
         i != propertyNames.end(); ++i) {

        res += QString(" %1=\"%2\"")
            .arg((*i).c_str())
            .arg(getAsString(*i).c_str());
    }
    
    res += "/>";
    return res;
}

QString
XmlStorableEvent::toXmlString(const Event &e)
{
    QString res = "<event";

    if (e.getType().length())
        res += QString(" type=\"%1\"").arg(e.getType().c_str());

    res += QString(" duration=\"%1\"").arg(e.getDuration());

    PropertyNames propertyNames(e.getPersistentPropertyNames());
    for (PropertyNames::const_iterator i = propertyNames.begin();
         i != propertyNames.end(); ++i) {

        res += QString(" %1=\"%2\"")
            .arg((*i).c_str())
            .arg(e.getAsString(*i).c_str());
    }
    
    res += "/>";
    return res;
}

