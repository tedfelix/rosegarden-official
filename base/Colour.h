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

#ifndef _BASE_COLOUR_H_
#define _BASE_COLOUR_H_

namespace Rosegarden 
{

/**
 * RColour is our internal colour storage mechanism; it's extremely basic
 * but does what it needs to
 */

class RColour
{
public:
	/**
	 * Create a RColour with values initialised to r=0, g=0, b=0
	 * i.e. Black. 
	 */
	RColour();

	/**
	 * Create a RColour with the specified red, green, blue values.
	 * If out of specification (i.e. < 0 || > 255 the value will be set to 0.
	 */
	RColour(const int red, const int green, const int blue);
	RColour(const RColour& input);

	~RColour();
	RColour& operator= (const RColour& input);

	/**
	 * Set the colour as appropriate; as with the constructor invalid values
	 * will be set to 0.
	 */
	void setColour(const int red, const int green, const int blue);

	/**
	 * Sets the three pointers to the values stored in the colour.
	 */ 
	void getColour(int *red, int *green, int *blue) const;

	/**
	 * Returns the current Red value of the colour as an integer.
	 */
	inline int getRed() const;

	/**
	 * Returns the current Blue value of the colour as an integer.
	 */
	inline int getBlue() const;

	/**
	 * Returns the current Green value of the colour as an integer.
	 */
	inline int getGreen() const;

	/**
	 * Sets the Red value of the current colour.  If the value isn't
	 * between 0 and 255 inclusive, it will set to 0
	 */
	void setRed(const int input);

	/**
	 * Sets the Blue value of the current colour.  If the value isn't
	 * between 0 and 255 inclusive, it will set to 0
	 */
	void setBlue(const int input);

	/**
	 * Sets the Green value of the current colour.  If the value isn't
	 * between 0 and 255 inclusive, it will set to 0
	 */
	void setGreen(const int input);

private:
	int m_r, m_g, m_b;
};

	/**
	 * This uses a simple calculation to give us a contrasting colour to
	 * the one supplied.  Useful for working out a visible text colour given
	 * any background colour
	 */
	RColour getContrastColour(const RColour &input);

	/**
	 * This works out a colour which is directly in between the two inputs.
	 * Useful for working out what overlapping Segments should be coloured as
	 */
	RColour getCombColour(const RColour &input1, const RColour &input2);

}

#endif
