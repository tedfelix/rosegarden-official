// -*- c-basic-offset: 4 -*-

/*
    Rosegarden-4
    A sequencer and musical notation editor.

    This program is Copyright 2000-2004
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

#ifndef _PLUGIN_IDENTIFIER_H_
#define _PLUGIN_IDENTIFIER_H_

#include <qstring.h>


// A plugin identifier is simply a string; this class provides methods
// to parse it into its constituent bits (plugin type, DLL path and label).

namespace Rosegarden {

class PluginIdentifier {

public:
 
    static QString createIdentifier(QString type, QString soName, QString label);

    static void parseIdentifier(QString identifier,
				QString &type, QString &soName, QString &label);

    static bool areIdentifiersSimilar(QString id1, QString id2);

    // Not strictly related to identifiers
    static QString RESERVED_PROJECT_DIRECTORY_KEY;
};

}

#endif
