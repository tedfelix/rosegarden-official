// -*- c-basic-offset: 4 -*-
/*
    Rosegarden-4
    A sequencer and musical notation editor.

    This program is Copyright 2000-2003
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
#include <iostream>

namespace Rosegarden
{

std::string XmlExportable::encode(const std::string &s0)
{
    std::string s1;
    std::string multibyte;

    // Escape any xml special characters, and also make sure we have
    // valid utf8 -- otherwise we won't be able to re-read the xml.
    // Amazing how complicated this gets.

    bool warned = false; // no point in warning forever for long bogus strings

    for (unsigned int i = 0; i < s0.length(); ++i) {

	unsigned char c = s0[i];

	if (((c & 0xc0) == 0xc0) || !(c & 0x80)) {

	    // 11xxxxxx or 0xxxxxxx: first byte of a character sequence

	    if (multibyte != "") {

		// does multibyte contain a valid sequence?
		unsigned int length = 
		    (!(multibyte[0] & 0x20)) ? 2 :
		    (!(multibyte[0] & 0x10)) ? 3 :
		    (!(multibyte[0] & 0x08)) ? 4 :
		    (!(multibyte[0] & 0x04)) ? 5 : 0;

		if (length == 0 || multibyte.length() == length) {
		    s1 += multibyte;
		} else {
		    if (!warned) {
			std::cerr
			    << "WARNING: Invalid utf8 char width in string \""
			    << s0 << "\" at index " << i << " ("
			    << multibyte.length() << " octet"
			    << (multibyte.length() != 1 ? "s" : "")
			    << ", expected " << length << ")" << std::endl;
			warned = true;
		    }
		    // and drop the character
		}
	    }

	    multibyte = "";

	    if (!(c & 0x80)) { // ascii
		
		switch (c) {
		case '&' : s1 += "&amp;";  break;
		case '<' : s1 += "&lt;";   break;
		case '>' : s1 += "&gt;";   break;
		case '"' : s1 += "&quot;"; break;
		case '\'': s1 += "&apos;"; break;
		case 0x9:
		case 0xa:
		case 0xd:
		    // convert these special cases to plain whitespace:
		    s1 += ' ';
		    break;
		default:
		    if (c >= 32) s1 += c;
		    else {
			if (!warned) {
			    std::cerr
				<< "WARNING: Invalid utf8 octet in string \""
				<< s0 << "\" at index " << i << " ("
				<< (int)c << " < 32)" << std::endl;
			}
			warned = true;
		    }
		}

	    } else {

		// store in multibyte rather than straight to s1, so that
		// we know we're in the middle of something (below) 
		multibyte += c;
	    }			

	} else {

	    // second or subsequent byte

	    if (multibyte == "") { // ... without a first byte!
		if (!warned) {
		    std::cerr
			<< "WARNING: Invalid utf8 octet sequence in string \""
			<< s0 << "\" at index " << i << std::endl;
		    warned = true;
		}
	    } else {
		multibyte += c;
	    }
	}
    }

    if (multibyte != "") s1 += multibyte;

    return s1;
}

}

