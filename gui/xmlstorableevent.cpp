/***************************************************************************
                          xmlstorableelement.cpp  -  description
                             -------------------
    begin                : Thu Aug 3 2000
    copyright            : (C) 2000 by Guillaume Laurent, Chris Cannam, Rich Bown
    email                : glaurent@telegraph-road.org, cannam@all-day-breakfast.com, bownie@bownie.com
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "xmlstorableevent.h"

#include "rosedebug.h"

Event::duration
XMLStorableEvent::noteName2Duration(const QString &nn)
{
    if (m_noteName2DurationMap.empty())
        initMap();

    string noteName(nn.latin1());

    namedurationmap::iterator it(m_noteName2DurationMap.find(noteName));

    if (it == m_noteName2DurationMap.end()) {
        // note name doesn't exist
        kdDebug(KDEBUG_AREA) << "Bad note name : " << nn << endl;
        return 0;
    }


    return it->second;
}

void
XMLStorableEvent::initMap()
{
    if (! m_noteName2DurationMap.empty())
        return;

    m_noteName2DurationMap["64th"]               = 6;
    m_noteName2DurationMap["hemidemisemiquaver"] = 6;

    m_noteName2DurationMap["32nd"]           = 12;
    m_noteName2DurationMap["demisemiquaver"] = 12;

    m_noteName2DurationMap["16th"]       = 24;
    m_noteName2DurationMap["semiquaver"] = 24;

    m_noteName2DurationMap["8th"]    = 48;
    m_noteName2DurationMap["quaver"] = 48;

    m_noteName2DurationMap["quarter"]  = 96;
    m_noteName2DurationMap["crotchet"] = 96;

    m_noteName2DurationMap["half"]  = 192;
    m_noteName2DurationMap["minim"] = 192;

    m_noteName2DurationMap["whole"]     = 384;
    m_noteName2DurationMap["semibreve"] = 384;

    // what is the american name ??
    m_noteName2DurationMap["breve"] = 768;

}


XMLStorableEvent::XMLStorableEvent(const QXmlAttributes &attributes)
{
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
            Event::duration d = attrVal.toUInt(&isNumeric);
            if (!isNumeric) {
                // It may be one of the accepted strings : breve, semibreve...
                // whole, half-note, ...
                d = noteName2Duration(attrVal);
                if (!d)
                    kdDebug(KDEBUG_AREA) << "Bad duration : " << attrVal << endl;
            }

            kdDebug(KDEBUG_AREA) << "Setting duration to : " << d << endl;
            setDuration(d);

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

XMLStorableEvent::namedurationmap
XMLStorableEvent::m_noteName2DurationMap;


