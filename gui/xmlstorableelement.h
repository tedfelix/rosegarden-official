/***************************************************************************
                          xmlstorableelement.h  -  description
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

#ifndef XMLSTORABLEELEMENT_H
#define XMLSTORABLEELEMENT_H

#include <qdom.h>

#include "Element2.h"

/**
  *@author Guillaume Laurent, Chris Cannam, Rich Bown
  */

class XMLStorableElement : public Element2  {
public:
    XMLStorableElement(const QDomNamedNodeMap &attributes,
                       const QDomNodeList &children);
protected:
    duration noteName2Duration(const QString &noteName);
    void initMap();

    typedef hash_map<string, duration, hashstring, eqstring> namedurationmap;

    static namedurationmap m_noteName2DurationMap;
};

#endif
