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

#include "xmlstorableelement.h"

#include <kdebug.h>

#define KDEBUG_AREA 1001 // ok, this is a duplicate, but right now
// I just don't care

Element2::duration
XMLStorableElement::noteName2Duration(const QString &nn)
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
XMLStorableElement::initMap()
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


XMLStorableElement::XMLStorableElement(const QDomNamedNodeMap &attributes,
                                       const QDomNodeList &children)
{
    for (unsigned int i = 0; i < attributes.length(); ++i) {
	QDomAttr n(attributes.item(i).toAttr());

        // special cases first : package, type, duration
	if (n.name() == "package") {

            setPackage(n.value().latin1());

        } else if (n.name() == "type") {

            setType(n.value().latin1());

        } else if (n.name() == "duration") {

            bool isNumeric = true;
            Element2::duration d = n.value().toUInt(&isNumeric);
            if (!isNumeric) {
                // It may be one of the accepted strings : breve, semibreve...
                // whole, half-note, ...
                d = noteName2Duration(n.value());
                if (!d)
                    kdDebug(KDEBUG_AREA) << "Bad duration : " << n.value() << endl;
            }

            kdDebug(KDEBUG_AREA) << "Setting duration to : " << d << endl;
            setDuration(d);

        } else {

            // set generic property
            //
            QString val(n.value());
            
            // Check if boolean val
            QString valLowerCase(val.lower());
            bool isNumeric;
            int numVal;

            if (valLowerCase == "true" || valLowerCase == "false") {

                set<Bool>(n.name().latin1(), valLowerCase == "true");

            } else {

                // Not a bool, check if integer val
                numVal = val.toInt(&isNumeric);
                if (isNumeric) {
                    set<Int>(n.name().latin1(), numVal);
                } else {
                    // not an int either, default to string
                    set<String>(n.name().latin1(), n.value().latin1());
                }
            }

        }

    }
}

XMLStorableElement::namedurationmap
XMLStorableElement::m_noteName2DurationMap;


