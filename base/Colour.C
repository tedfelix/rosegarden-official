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

// The RColour Class

RColour::RColour()
{
	m_r = 0;
	m_g = 0;
	m_b = 0;
}

RColour::RColour(const int red, const int green, const int blue)
{
	this->setColour(red, green, blue);
}

RColour::RColour(const RColour& input)
{
	this->setColour(input.getRed(), input.getGreen(), input.getBlue());
}

RColour::~RColour()
{

}

RColour& 
RColour::operator= (const RColour& input)
{
	// We don't have to check for assignment to self because it'll have
	//  no nasty effects (in fact, it'll do what it should - nothing)
	this->setColour(input.getRed(), input.getGreen(), input.getBlue());
	return *this;
}

void
RColour::setColour(const int red, const int green, const int blue)
{
	((red>=0)&&(red<=255)) ? m_r=red : m_r=0;
	((green>=0)&&(green<=255)) ? m_g=green : m_g=0;
	((blue>=0)&&(blue<=255)) ? m_b=blue : m_b=0;
}

void
RColour::getColour(int *red, int *green, int *blue) const
{
	*red = m_r;
	*green = m_g;
	*blue = m_b;
}

inline int
RColour::getRed() const
{
	return m_r;
}

inline int
RColour::getBlue() const
{
	return m_b;
}

inline int
RColour::getGreen() const
{
	return m_g;
}

void
RColour::setRed(const int red)
{
	((red>=0)&&(red<=255)) ? m_r=red : m_r=0;
}

void
RColour::setBlue(const int blue)
{
	((blue>=0)&&(blue<=255)) ? m_b=blue: m_b=0;
}

void
RColour::setGreen(const int green)
{
	((green>=0)&&(green<=255)) ? m_g=green : m_g=0;
}

// Generic Colour routines:

RColour
Rosegarden::getContrastColour(const RColour &input)
{
	RColour ret(255-input.getRed(),255-input.getGreen(),255-input.getBlue());
	return ret;
}

RColour
Rosegarden::getCombColour(const RColour &input1, const RColour &input2)
{
	RColour ret((input1.getRed()+input2.getRed())/2,
                (input1.getGreen()+input2.getGreen())/2,
                (input1.getBlue()+input2.getBlue())/2);
	return ret;

}

}
