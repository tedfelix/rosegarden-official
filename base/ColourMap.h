// -*- c-basic-offset: 4 -*-


/*
    Rosegarden-4
    A sequencer and musical notation editor.

    This program is Copyright 2000-2003
        Guillaume Laurent   <glaurent@telegraph-road.org>,
        Chris Cannam        <cannam@all-day-breakfast.com>,
        Richard Bown        <bownie@bownie.com>

    This file is Copyright 2003
        Mark Hymers         <markh@linuxfromscratch.org>

    The moral right of the authors to claim authorship of this work
    has been asserted.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#include <utility>
#include <map>
#include <string>
#include "Colour.h"

#ifndef _BASE_COLOURMAP_H_
#define _BASE_COLOURMAP_H_

// These is the default default colour
#define COLOUR_DEF_R 230
#define COLOUR_DEF_B 230
#define COLOUR_DEF_G 230

namespace Rosegarden 
{
	using std::map;
	using std::pair;
	using std::string;
	using std::less;
	using std::make_pair;
	typedef map<unsigned int, pair<RColour, string>, less<unsigned int> > RCMap;

/**
 * RColourMap is our table which maps the unsigned integer keys stored in
 *  segments to both a RColour and a String containing the 'name'
 */

class RColourMap
{
public:
	// Functions:

	/**
	 * Initialises an RColourMap with a default element set to
	 * whatever COLOUR_DEF_X defines the colour to be (see the source file)
	 * Also sets up m_map_iterator to point to m_map.begin()
	 */
	RColourMap();
	/**
	 * Initialises an RColourMap with a default element set to
	 * the value of the RColour passed in.  Also sets up m_map_iterator to
	 * point to m_map.begin()
	 */
	RColourMap(RColour& input);
	~RColourMap();

	/**
	 * Returns the RColour associated with the item_num passed in.  Note that
	 * if the item_num doesn't represent a valid item, the routine returns
	 * the value of the Default colour.  This means that if somehow some of
	 * the Segments get out of sync with the ColourMap and have invalid
	 * colour values, they'll be set to the Composition default colour.
	 */
	RColour getColourByIndex(unsigned int item_num);

	/**
	 * Returns the string associated with the item_num passed in.  If the
	 * item_num doesn't exist, it'll return "" (the same name as the default
	 * colour has - for internationalization reasons).
	 */
	string getNameByIndex(unsigned int item_num);

	/**
	 * If item_num exists, this routine deletes it from the map.
	 * WARNING: If true is returned it implies that m_map_iterator has
	 * been reset to m_map.begin() as otherwise it may no longer be valid
	 */
	bool deleteItemByIndex(unsigned int item_num);

	/**
	 * This routine adds a Colour using the lowest possible index.
	 * WARNING: This routine will reset m_map_iterator to m_map.begin()
	 * as otherwise it may no longer be valid
	 */
	bool addItem(RColour colour, string name);

	/**
	 * If the item with item_num exists and isn't the default, this
	 * routine modifies the string associated with it
	 */
	bool modifyNameByIndex(unsigned int item_num, string name);

	/**
	 * If the item with item_num exists, this routine modifies the 
	 * RColour associated with it
	 */
	bool modifyColourByIndex(unsigned int item_num, RColour colour);

	/**
	 * If both items exist, swap them.  WARNING: If true is returned,
	 * the m_map_iterator has been reset to m_map.begin() as otherwise
	 * it may no longer be valid
	 */
	bool swapItems(unsigned int item_1, unsigned int item_2);

	/**
	 * This moves m_map_iterator to m_map.begin()
	 */
	void iteratorStart();

	/**
	 * Is the iterator == m_map.end() ?
	 */
	bool iteratorAtEnd();

	// Members 

	/**
	 * This is a method to allow iteration over the m_map in read only mode
	 */
	RCMap::const_iterator m_map_iterator;

private:
	RCMap m_map;
};

}

#endif
