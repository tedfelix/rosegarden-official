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

const QColor ColourMap::defaultSegmentColor(255, 234, 182);

ColourMap::ColourMap()
{
    // Set up entry 0 as the default colour.
    // ??? Does this really matter?  I suspect this is clobbered by
    //     RoseXmlHandler, so it's not needed.  Might be worth doing
    //     some comprehensive testing to see if removing this would
    //     make any difference.
    colours[0] = Entry();
}
#if 0
ColourMap::ColourMap(const QColor &color)
{
    // Set up the default colour based on the input
    colours[0] = Entry(color, "");
}
#endif
bool
ColourMap::deleteItemByIndex(unsigned int item_num)
{
    // We explicitly refuse to delete the default colour
    if (item_num == 0) 
        return false;

    unsigned int n_e = colours.erase(item_num);
    if (n_e != 0)
    {
        return true;
    }

    // Otherwise we didn't find the right item
    return false;
}

QColor
ColourMap::getColourByIndex(unsigned int item_num) const
{
    // Iterate over the m_map and if we find a match, return the
    // QColor.  If we don't match, return the default colour.  m_map
    // was initialised with at least one item in the ctor, so this is
    // safe.
    QColor ret = (*colours.begin()).second.color;

    for (MapType::const_iterator position = colours.begin();
	 position != colours.end(); ++position)
        if (position->first == item_num)
            ret = position->second.color;

    return ret;
}

std::string
ColourMap::getNameByIndex(unsigned int item_num) const
{
    // Iterate over the m_map and if we find a match, return the name.
    // If we don't match, return the default colour's name.  m_map was
    // initialised with at least one item in the ctor, so this is
    // safe.
    std::string ret = (*colours.begin()).second.name;

    for (MapType::const_iterator position = colours.begin();
	 position != colours.end(); ++position)
        if (position->first == item_num)
            ret = position->second.name;

    return ret;
}

bool
ColourMap::addItem(QColor colour, std::string name)
{
    // If we want to limit the number of colours, here's the place to do it
    unsigned int highest=0;

    for (MapType::iterator position = colours.begin(); position != colours.end(); ++position)
    {
        if (position->first != highest)
            break;

        ++highest;
    }

    colours[highest] = Entry(colour, name);

    return true;
}

void
ColourMap::addItem(unsigned colorIndex, QColor colour, std::string name)
{
    colours[colorIndex] = Entry(colour, name);
}

bool
ColourMap::modifyNameByIndex(unsigned int item_num, std::string name)
{
    // We don't allow a name to be given to the default colour
    if (item_num == 0)
        return false;

    for (MapType::iterator position = colours.begin(); position != colours.end(); ++position)
        if (position->first == item_num)
        {
            position->second.name = name;
            return true;
        }

    // We didn't find the element
    return false;
}

bool
ColourMap::modifyColourByIndex(unsigned int item_num, QColor colour)
{
    for (MapType::iterator position = colours.begin(); position != colours.end(); ++position)
        if (position->first == item_num)
        {
            position->second.color = colour;
            return true;
        }

    // We didn't find the element
    return false;
}

#if 0
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
    for (MapType::iterator position = colours.begin(); position != colours.end(); ++position)
    {
        if (position->first == item_1) one = position->first;
        if (position->first == item_2) two = position->first;
    }

    // If they both exist, do it
    // There's probably a nicer way to do this
    if ((one != 0) && (two != 0))
    {
        QColor tempC = colours[one].color;
        std::string tempS = colours[one].name;
        colours[one].color = colours[two].color;
        colours[one].name = colours[two].name;
        colours[two].color = tempC;
        colours[two].name = tempS;

        return true;
    }

    // Else they didn't
    return false;
}
#endif
#if 0
ColourMap::MapType::const_iterator
ColourMap::begin()
{
    MapType::const_iterator ret = colours.begin();
    return ret;
}

ColourMap::MapType::const_iterator
ColourMap::end()
{
    MapType::const_iterator ret = colours.end();
    return ret;
}

unsigned int
ColourMap::size() const
{
    return (unsigned int)colours.size();
}
#endif

std::string
ColourMap::toXmlString(std::string name) const
{
    std::stringstream output;

    output << "        <colourmap name=\"" << XmlExportable::encode(name)
           << "\">" << std::endl;

    for (MapType::const_iterator pos = colours.begin(); pos != colours.end(); ++pos)
    {
        const QColor &color = pos->second.color;

        output << "  " << "            <colourpair id=\"" << pos->first
               << "\" name=\"" << XmlExportable::encode(pos->second.name)
               << "\" "
               << "red=\"" << color.red()
               << "\" green=\"" << color.green()
               << "\" blue=\"" << color.blue()
               << "\""
               << "/>" << std::endl;
    }

    output << "        </colourmap>" << std::endl;

    return output.str();

}


}
