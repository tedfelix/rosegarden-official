/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */


/*
    Rosegarden
    A sequencer and musical notation editor.
    Copyright 2000-2017 the Rosegarden development team.
    See the AUTHORS file for more details.

    This file is Copyright 2003
        Mark Hymers         <markh@linuxfromscratch.org>

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#ifndef RG_COLOUR_H
#define RG_COLOUR_H

#include <QColor>
#include <string>

namespace Rosegarden 
{

/**
 * ??? This class should be removed.  QColor should be used instead everywhere
 *     this is used.  The one feature that this class offers
 *     (dataToXmlString()) should be inlined into its caller.
 */
class Colour
{
public:
    /**
     * Create a Colour with values initialised to r=0, g=0, b=0
     * i.e. Black. 
     */
    Colour();

    /**
     * Create a Colour with the specified red, green, blue values.
     * If out of specification (i.e. < 0 || > 255 the value will be set to 0.
     */
    Colour(unsigned int red, unsigned int green, unsigned int blue);
    ~Colour();

    /**
     * Returns the current Red value of the colour as an integer.
     */
    unsigned int getRed() const;

    /**
     * Returns the current Blue value of the colour as an integer.
     */
    unsigned int getBlue() const;

    /**
     * Returns the current Green value of the colour as an integer.
     */
    unsigned int getGreen() const;

    /// Called by ColourMap::toXmlString().  INLINE and REMOVE.
    std::string dataToXmlString() const;

    QColor toQColor() const { return QColor(m_r, m_g, m_b); }

private:
    unsigned int m_r, m_g, m_b;
};

    /**
     * This works out a colour which is directly in between the two inputs.
     * Useful for working out what overlapping Segments should be coloured as
     */
//    Colour getCombinationColour(const Colour &input1, const Colour &input2);

}

#endif
