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

#include "XmlExportable.h"

namespace Rosegarden
{

std::string XmlExportable::encode(const std::string &s0)
{
    std::string s1;
    
    for (unsigned int i = 0; i < s0.length(); ++i) {

	char c = s0[i];

	switch (c) {
	case '&': s1 += "&amp;"; break;
	case '<': s1 += "&lt;";  break;
	case '>': s1 += "&gt;";  break;
	default : s1 += c;       break;
	}
    }

    return s1;
}

}

