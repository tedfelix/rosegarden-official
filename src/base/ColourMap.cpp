/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */


/*
    Rosegarden
    A sequencer and musical notation editor.
    Copyright 2000-2018 the Rosegarden development team.
    See the AUTHORS file for more details.

    This file is Copyright 2003
        Mark Hymers         <markh@linuxfromscratch.org>

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#include <string>

#include <sstream>

#include "ColourMap.h"
#include "XmlExportable.h"

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
ColourMap::deleteItemByIndex(unsigned int item_num)
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
ColourMap::getColourByIndex(unsigned int item_num) const
{
    // Iterate over the m_map and if we find a match, return the
    // Colour.  If we don't match, return the default colour.  m_map
    // was initialised with at least one item in the ctor, so this is
    // safe.
    Colour ret = (*m_map.begin()).second.first;

    for (RCMap::const_iterator position = m_map.begin();
	 position != m_map.end(); ++position)
        if (position->first == item_num)
            ret = position->second.first;

    return ret;
}

std::string
ColourMap::getNameByIndex(unsigned int item_num) const
{
    // Iterate over the m_map and if we find a match, return the name.
    // If we don't match, return the default colour's name.  m_map was
    // initialised with at least one item in the ctor, so this is
    // safe.
    std::string ret = (*m_map.begin()).second.second;

    for (RCMap::const_iterator position = m_map.begin();
	 position != m_map.end(); ++position)
        if (position->first == item_num)
            ret = position->second.second;

    return ret;
}

bool
ColourMap::addItem(Colour colour, std::string name)
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

// WARNING: This version of addItem is only for use by rosexmlhandler.cpp
bool
ColourMap::addItem(Colour colour, std::string name, unsigned int id)
{
    m_map[id] = make_pair(colour, name);

    return true;
}

bool
ColourMap::modifyNameByIndex(unsigned int item_num, std::string name)
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
ColourMap::modifyColourByIndex(unsigned int item_num, Colour colour)
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
ColourMap::swapItems(unsigned int item_1, unsigned int item_2)
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

unsigned int
ColourMap::size() const
{
    return (unsigned int)m_map.size();
}

std::string
ColourMap::toXmlString(std::string name) const
{
    std::stringstream output;

    output << "        <colourmap name=\"" << XmlExportable::encode(name)
           << "\">" << std::endl;

    for (RCMap::const_iterator pos = m_map.begin(); pos != m_map.end(); ++pos)
    {
        output << "  " << "            <colourpair id=\"" << pos->first
               << "\" name=\"" << XmlExportable::encode(pos->second.second)
               << "\" " << pos->second.first.dataToXmlString() << "/>" << std::endl;
    }

    output << "        </colourmap>" << std::endl;


    return output.str();

}

}
