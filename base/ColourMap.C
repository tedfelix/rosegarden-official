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

#include <string>
#include "ColourMap.h"

namespace Rosegarden 
{

RColourMap::RColourMap()
{
	// Set up the default colour.  The #defines can be found in ColourMap.h
	RColour tempcolour(COLOUR_DEF_R, COLOUR_DEF_G, COLOUR_DEF_B);
	m_map[0] = make_pair(tempcolour, std::string(""));
	m_map_iterator = m_map.begin();
}

RColourMap::RColourMap(RColour& input)
{
	// Set up the default colour based on the input
	m_map[0] = make_pair(input, std::string(""));
	m_map_iterator = m_map.begin();
}

RColourMap::~RColourMap()
{
	// Everything should destroy itself automatically
}

bool
RColourMap::deleteItemByIndex(unsigned int item_num)
{
	// We explicitly refuse to delete the default colour
	if (item_num == 0) 
		return false;

	int n_e = m_map.erase(item_num);
	if (n_e != 0)
	{
		// We need to reset the iterator as it may no longer be valid
		m_map_iterator = m_map.begin();
		return true;
	}

	// Otherwise we didn't find the right item
	return false;
}

RColour
RColourMap::getColourByIndex(unsigned int item_num)
{
	// Iterate over the m_map and if we find a match, return the RColour
	// If we don't match, return the default colour
	RColour ret = m_map[0].first;

	for (RCMap::iterator position = m_map.begin(); position != m_map.end(); ++position)
		if (position->first == item_num)
			ret = position->second.first;

	return ret;
}

string
RColourMap::getNameByIndex(unsigned int item_num)
{
	// Iterate over the m_map and if we find a match, return the name
	string ret = m_map[0].second;

	for (RCMap::iterator position = m_map.begin(); position != m_map.end(); ++position)
		if (position->first == item_num)
			ret = position->second.second;

	return ret;
}

bool
RColourMap::addItem(RColour colour, string name)
{
	// If we want to limit the number of colours, here's the place to do it
	unsigned int highest=0;

	for (RCMap::iterator position = m_map.begin(); position != m_map.end(); ++position)
	{
		if (position->first != highest)
			goto gotnumber; // URGH - A goto; but it's is an easy way of getting out of the if and for loop 

		++highest;
	}

	gotnumber:
	m_map[highest] = make_pair(colour, name);
	// We should reset the iterator as it may no longer be valid
	m_map_iterator = m_map.begin();
	return true;
}

bool
RColourMap::modifyNameByIndex(unsigned int item_num, string name)
{
	// We don't allow a name to be given to the default colour
	if (item_num == 0)
		return false;

	for (RCMap::iterator position = m_map.begin(); position != m_map.end(); ++position)
		if (position->first == item_num)
		{
			position->second.second = name;
			return true;
		}

	// We didn't find the element
	return false;
}

bool
RColourMap::modifyColourByIndex(unsigned int item_num, RColour colour)
{
	for (RCMap::iterator position = m_map.begin(); position != m_map.end(); ++position)
		if (position->first == item_num)
		{
			position->second.first = colour;
			return true;
		}

	// We didn't find the element
	return false;
}

bool
RColourMap::swapItems(unsigned int item_1, unsigned int item_2)
{
	// It would make no difference but we return false because 
	//  we haven't altered the iterator (see docs in ColourMap.h)
	if (item_1 == item_2)
		return false; 

	// We refuse to swap the default colour for something else
	// Basically because what would we do with the strings?
	if ((item_1 == 0) || (item_2 == 0))
		return false; 

	unsigned int one = 0, two = 0;

	// Check that both elements exist
	// It's not worth bothering about optimising this
	for (RCMap::iterator position = m_map.begin(); position != m_map.end(); ++position)
	{
		if (position->first == item_1) one = position->first;
		if (position->first == item_2) two = position->first;
	}

	// If they both exist, do it
	// There's probably a nicer way to do this
	if ((one != 0) && (two != 0))
	{
		RColour tempC = m_map[one].first;
		string tempS = m_map[one].second;
		m_map[one].first = m_map[two].first;
		m_map[one].second = m_map[two].second;
		m_map[two].first = tempC;
		m_map[two].second = tempS;
		// We now have to reset the iterator as it may no longer be valid
		m_map_iterator = m_map.begin();
		return true;
	}

	// Else they didn't
	return false;
}

void
RColourMap::iteratorStart()
{
	m_map_iterator = m_map.begin();
}

bool
RColourMap::iteratorAtEnd()
{
	return m_map_iterator == m_map.end();
}

}
