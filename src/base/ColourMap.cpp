/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2020 the Rosegarden development team.

    This file is Copyright 2003
        Mark Hymers         <markh@linuxfromscratch.org>

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#include "ColourMap.h"

#include "XmlExportable.h"

#include <sstream>

namespace Rosegarden 
{


const QColor ColourMap::defaultSegmentColour(255, 234, 182);

ColourMap::ColourMap()
{
    // Set up entry 0 as the default colour.
    // ??? Does this really matter?  I suspect this is clobbered by
    //     RoseXmlHandler, so it's not needed.  Might be worth doing
    //     some comprehensive testing to see if removing this would
    //     make any difference.
    colours[0] = Entry();
}

bool
ColourMap::deleteEntry(unsigned int item_num)
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
ColourMap::getColour(unsigned int item_num) const
{
    // Iterate over the m_map and if we find a match, return the
    // QColor.  If we don't match, return the default colour.  m_map
    // was initialised with at least one item in the ctor, so this is
    // safe.
    QColor ret = (*colours.begin()).second.colour;

    for (MapType::const_iterator position = colours.begin();
	 position != colours.end(); ++position)
        if (position->first == item_num)
            ret = position->second.colour;

    return ret;
}

std::string
ColourMap::getName(unsigned int item_num) const
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
ColourMap::addEntry(QColor colour, std::string name)
{
    // If we want to limit the number of colours, here's the place to do it
    unsigned int highest=0;

    for (MapType::iterator position = colours.begin();
         position != colours.end();
         ++position)
    {
        if (position->first != highest)
            break;

        ++highest;
    }

    colours[highest] = Entry(colour, name);

    return true;
}

void
ColourMap::addEntry(unsigned colorIndex, QColor colour, std::string name)
{
    colours[colorIndex] = Entry(colour, name);
}

bool
ColourMap::modifyName(unsigned int item_num, std::string name)
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
ColourMap::modifyColour(unsigned int item_num, QColor colour)
{
    for (MapType::iterator position = colours.begin();
         position != colours.end();
         ++position)
        if (position->first == item_num)
        {
            position->second.colour = colour;
            return true;
        }

    // We didn't find the element
    return false;
}

std::string
ColourMap::toXmlString(std::string name) const
{
    std::stringstream output;

    output << "        <colourmap name=\"" << XmlExportable::encode(name)
           << "\">" << std::endl;

    for (MapType::const_iterator pos = colours.begin(); pos != colours.end(); ++pos)
    {
        const QColor &color = pos->second.colour;

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
