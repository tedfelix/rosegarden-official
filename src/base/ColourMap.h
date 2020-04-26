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

#ifndef RG_COLOURMAP_H
#define RG_COLOURMAP_H

#include <QColor>

#include <map>
#include <string>

namespace Rosegarden 
{


/// Maps a color ID to a QColor and a name.
class ColourMap
{
public:
    /// Create a ColourMap with only the default segment color.
    ColourMap();

    /**
     * Initialises an ColourMap with a default element set to
     * the value of the QColor passed in.
     */
    //ColourMap(const QColor& input);

    static const QColor defaultSegmentColor;

    // ??? colorIndex should probably be colorID.  "id" is used in the
    //     xml to refer to this.  And it's a more accurate description
    //     than index.

    /**
     * If the colorIndex isn't in the map, the routine returns the value of
     * the default colour (at index 0 in the table).  This means that if
     * somehow some of the Segments get out of sync with the ColourMap and
     * have invalid colour values, they'll be set to the Composition default
     * colour.
     */
    QColor getColourByIndex(unsigned int colorIndex) const;

    /**
     * If the colorIndex isn't in the map, the name of the entry at index 0 is
     * returned.  Usually this is "", for internationalization reasons.
     */
    std::string getNameByIndex(unsigned int colorIndex) const;

    /// Delete colorIndex from the map.
    bool deleteItemByIndex(unsigned int colorIndex);

    /// Add a colour entry using the lowest available index.
    bool addItem(QColor colour, std::string name);

    /// Add a colour map entry given index, colour, and name.
    /**
     * !!! ONLY FOR USE IN rosexmlhandler.cpp !!!
     *
     * ??? Inline this into the only caller.
     */
    void addItem(unsigned colorIndex, QColor colour, std::string name);

    /// Returns false if colorIndex not found.
    bool modifyNameByIndex(unsigned colorIndex, std::string name);

    /// Returns false if colorIndex not found.
    bool modifyColourByIndex(unsigned colorIndex, QColor colour);

    std::string toXmlString(std::string name) const;


    struct Entry
    {
        Entry() :
            color(defaultSegmentColor),
            name()
        {
        }

        Entry(const QColor &i_color, const std::string &i_name) :
            color(i_color),
            name(i_name)
        {
        }

        QColor color;
        std::string name;
    };

    typedef std::map<unsigned /* colorIndex */, Entry> MapType;
    MapType colours;

private:
    // unused
    /**
     * If both items exist, swap them.
     */
    //bool swapItems(unsigned int item_1, unsigned int item_2);
    //void replace(ColourMap &input);
    //MapType::const_iterator begin();
    //MapType::const_iterator end();
    //unsigned int size() const;

};

}

#endif
