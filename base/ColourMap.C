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

ColourMap::ColourMap()
{
    // Set up the default colour.  The #defines can be found in ColourMap.h
    Colour tempcolour(COLOUR_DEF_R, COLOUR_DEF_G, COLOUR_DEF_B);
    m_map[0] = make_pair(tempcolour, std::string(""));
}

ColourMap::ColourMap(const Colour& input)
{
    // Set up the default colour based on the input
    m_map[0] = make_pair(input, std::string(""));
}

ColourMap::~ColourMap()
{
    // Everything should destroy itself automatically
}

bool
ColourMap::deleteItemByIndex(const unsigned int item_num)
{
    // We explicitly refuse to delete the default colour
    if (item_num == 0) 
        return false;

    unsigned int n_e = m_map.erase(item_num);
    if (n_e != 0)
    {
        return true;
    }

    // Otherwise we didn't find the right item
    return false;
}

Colour
ColourMap::getColourByIndex(const unsigned int item_num)
{
    // Iterate over the m_map and if we find a match, return the Colour
    // If we don't match, return the default colour
    Colour ret = m_map[0].first;

    for (RCMap::iterator position = m_map.begin(); position != m_map.end(); ++position)
        if (position->first == item_num)
            ret = position->second.first;

    return ret;
}

std::string
ColourMap::getNameByIndex(const unsigned int item_num)
{
    // Iterate over the m_map and if we find a match, return the name
    std::string ret = m_map[0].second;

    for (RCMap::iterator position = m_map.begin(); position != m_map.end(); ++position)
        if (position->first == item_num)
            ret = position->second.second;

    return ret;
}

bool
ColourMap::addItem(const Colour colour, const std::string name)
{
    // If we want to limit the number of colours, here's the place to do it
    unsigned int highest=0;

    for (RCMap::iterator position = m_map.begin(); position != m_map.end(); ++position)
    {
        if (position->first != highest)
            break;

        ++highest;
    }

    m_map[highest] = make_pair(colour, name);

    return true;
}

bool
ColourMap::modifyNameByIndex(const unsigned int item_num, const std::string name)
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
ColourMap::modifyColourByIndex(const unsigned int item_num, const Colour colour)
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
ColourMap::swapItems(const unsigned int item_1, const unsigned int item_2)
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
        Colour tempC = m_map[one].first;
        std::string tempS = m_map[one].second;
        m_map[one].first = m_map[two].first;
        m_map[one].second = m_map[two].second;
        m_map[two].first = tempC;
        m_map[two].second = tempS;

        return true;
    }

    // Else they didn't
    return false;
}

RCMap::const_iterator
ColourMap::begin()
{
    RCMap::const_iterator ret = m_map.begin();
    return ret;
}

RCMap::const_iterator
ColourMap::end()
{
    RCMap::const_iterator ret = m_map.end();
    return ret;
}

}
