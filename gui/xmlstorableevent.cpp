// -*- c-basic-offset: 4 -*-

/*
    Rosegarden-4
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

#include "xmlstorableevent.h"
#include "NotationTypes.h"
#include "SegmentNotationHelper.h"
#include "BaseProperties.h"
#include "Quantizer.h"
#include "XmlExportable.h"

#include "rosestrings.h"
#include "notationstrings.h"
#include "rosedebug.h"

using Rosegarden::Event;
using Rosegarden::Note;
using Rosegarden::Bool;
using Rosegarden::Int;
using Rosegarden::String;
using Rosegarden::timeT;


XmlStorableEvent::XmlStorableEvent(const QXmlAttributes &attributes,
				   timeT &absoluteTime)
{
    setDuration(0);

    for (int i = 0; i < attributes.length(); ++i) {

	QString attrName(attributes.qName(i)),
            attrVal(attributes.value(i));

	if (attrName == "package") {

            RG_DEBUG << "XmlStorableEvent::XmlStorableEvent: Warning: XML still uses deprecated \"package\" attribute" << endl;

        } else if (attrName == "type") {

            setType(qstrtostr(attrVal));

        } else if (attrName == "subordering") {

            bool isNumeric = true;
            int o = attrVal.toInt(&isNumeric);

            if (!isNumeric) {
                RG_DEBUG << "XmlStorableEvent::XmlStorableEvent: Bad subordering: " << attrVal << endl;
            } else {
                if (o != 0) setSubOrdering(o);
            }

        } else if (attrName == "duration") {

            bool isNumeric = true;
            timeT d = attrVal.toInt(&isNumeric);

            if (!isNumeric) {
		try {
		    Note n(NotationStrings::getNoteForName(attrVal));
		    setDuration(n.getDuration());
		} catch (NotationStrings::MalformedNoteName m) {
                    RG_DEBUG << "XmlStorableEvent::XmlStorableEvent: Bad duration: " << attrVal << " (" << m.getMessage() << ")" << endl;
		}
            } else {
		setDuration(d);
	    }

	} else if (attrName == "absoluteTime") {

	    bool isNumeric = true;
	    timeT t = attrVal.toInt(&isNumeric);

	    if (!isNumeric) {
		RG_DEBUG << "XmlStorableEvent::XmlStorableEvent: Bad absolute time: " << attrVal << endl;
	    } else {
		absoluteTime = t;
	    }

	} else if (attrName == "timeOffset") {

	    bool isNumeric = true;
	    timeT t = attrVal.toInt(&isNumeric);

	    if (!isNumeric) {
		RG_DEBUG << "XmlStorableEvent::XmlStorableEvent: Bad time offset: " << attrVal << endl;
	    } else {
		absoluteTime += t;
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

                set<Bool>(qstrtostr(attrName), valLowerCase == "true");

            } else {

                // Not a bool, check if integer val
                numVal = val.toInt(&isNumeric);
                if (isNumeric) {
                    set<Int>(qstrtostr(attrName), numVal);
                } else {
                    // not an int either, default to string
                    set<String>(qstrtostr(attrName), qstrtostr(attrVal));
                }
            }
        }
    }

    setAbsoluteTime(absoluteTime);
}

XmlStorableEvent::XmlStorableEvent(Event &e) :
    Event(e)
{
}


void
XmlStorableEvent::setPropertyFromAttributes(const QXmlAttributes &attributes,
					    bool persistent)
{
    bool have = false;
    QString name = attributes.value("name");
    if (name == "") {
        RG_DEBUG << "XmlStorableEvent::setProperty: no property name found, ignoring" << endl;
        return;
    }

    for (int i = 0; i < attributes.length(); ++i) {
	QString attrName(attributes.qName(i)),
            attrVal(attributes.value(i));

	if (attrName == "name") {
            continue;
        } else if (have) {
            RG_DEBUG << "XmlStorableEvent::setProperty: multiple values found, ignoring all but the first" << endl;
            continue;
        } else if (attrName == "bool") {
            set<Bool>(qstrtostr(name), attrVal.lower() == "true",
		      persistent);
            have = true;
        } else if (attrName == "int") {
            set<Int>(qstrtostr(name), attrVal.toInt(), persistent);
            have = true;
        } else if (attrName == "string") {
            set<String>(qstrtostr(name), qstrtostr(attrVal), persistent);
            have = true;
        } else {
            RG_DEBUG << "XmlStorableEvent::setProperty: unknown attribute name \"" << name << "\", ignoring" << endl;
        }
    }

    if (!have) RG_DEBUG << "XmlStorableEvent::setProperty: Warning: no property value found for property " << name << endl;
}


