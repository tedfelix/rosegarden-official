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

#include "Colour.h"

namespace Rosegarden 
{

// The Colour Class

Colour::Colour()
{
    m_r = 0;
    m_g = 0;
    m_b = 0;
}

Colour::Colour(const unsigned int red, const unsigned int green, const unsigned int blue)
{
    this->setColour(red, green, blue);
}

Colour::Colour(const Colour& input)
{
    this->setColour(input.getRed(), input.getGreen(), input.getBlue());
}

Colour::~Colour()
{

}

Colour&
Colour::operator= (const Colour& input)
{
    // We don't have to check for assignment to self because it'll have
    //  no nasty effects (in fact, it'll do what it should - nothing)
    this->setColour(input.getRed(), input.getGreen(), input.getBlue());
    return *this;
}

void
Colour::setColour(const unsigned int red, const unsigned int green, const unsigned int blue)
{
    ((red>=0)&&(red<=255)) ? m_r=red : m_r=0;
    ((green>=0)&&(green<=255)) ? m_g=green : m_g=0;
    ((blue>=0)&&(blue<=255)) ? m_b=blue : m_b=0;
}

void
Colour::getColour(unsigned int &red, unsigned int &green, unsigned int &blue) const
{
    red = m_r;
    green = m_g;
    blue = m_b;
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

void
Colour::setRed(const unsigned int red)
{
    ((red>=0)&&(red<=255)) ? m_r=red : m_r=0;
}

void
Colour::setBlue(const unsigned int blue)
{
    ((blue>=0)&&(blue<=255)) ? m_b=blue: m_b=0;
}

void
Colour::setGreen(const unsigned int green)
{
    ((green>=0)&&(green<=255)) ? m_g=green : m_g=0;
}

Colour
Colour::getContrastingColour() const
{
    Colour ret(255-m_r, 255-m_g, 255-m_b);
    return ret;
}

// Generic Colour routines:

Colour
Rosegarden::getCombinationColour(const Colour &input1, const Colour &input2)
{
    Colour ret((input1.getRed()+input2.getRed())/2,
                (input1.getGreen()+input2.getGreen())/2,
                (input1.getBlue()+input2.getBlue())/2);
    return ret;

}

}
