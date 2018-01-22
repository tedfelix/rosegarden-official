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

#include "Colour.h"

#include <sstream>

namespace Rosegarden 
{

// The Colour Class

Colour::Colour()
{
    m_r = 0;
    m_g = 0;
    m_b = 0;
}

Colour::Colour(unsigned int red, unsigned int green, unsigned int blue)
{
    m_r = red <= 255 ? red : 0;
    m_g = green <= 255 ? green : 0;
    m_b = blue <= 255 ? blue : 0;
}

Colour::~Colour()
{

}

unsigned int
Colour::getRed() const
{
    return m_r;
}

unsigned int
Colour::getBlue() const
{
    return m_b;
}

unsigned int
Colour::getGreen() const
{
    return m_g;
}

std::string
Colour::dataToXmlString() const
{
    std::stringstream output;
    output << "red=\"" << m_r
           << "\" green=\"" << m_g
           << "\" blue=\"" << m_b
           << "\"";

    return output.str();
}

#if 0
void
Colour::getColour(unsigned int &red, unsigned int &green, unsigned int &blue) const
{
    red = m_r;
    green = m_g;
    blue = m_b;
}
#endif
#if 0
void
Colour::setRed(unsigned int red)
{
    (red<=255) ? m_r=red : m_r=0;
}

void
Colour::setBlue(unsigned int blue)
{
    (blue<=255) ? m_b=blue: m_b=0;
}

void
Colour::setGreen(unsigned int green)
{
    (green<=255) ? m_g=green : m_g=0;
}
#endif
#if 0
Colour
Colour::getContrastingColour() const
{
    Colour ret(255-m_r, 255-m_g, 255-m_b);
    return ret;
}
#endif
#if 0
std::string
Colour::toXmlString() const
{
    std::stringstream output;

    output << "<colour red=\"" << m_r
           << "\" green=\"" << m_g
           << "\" blue=\"" << m_b
           << "\"/>" << std::endl;

    return output.str();
}
#endif
// Generic Colour routines:
#if 0
Colour
getCombinationColour(const Colour &input1, const Colour &input2)
{
    Colour ret((input1.getRed()+input2.getRed())/2,
                (input1.getGreen()+input2.getGreen())/2,
                (input1.getBlue()+input2.getBlue())/2);
    return ret;

}
#endif

}
