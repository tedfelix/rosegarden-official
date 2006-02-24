// -*- c-basic-offset: 4 -*-

/*
    Rosegarden-4
    A sequencer and musical notation editor.

    This program is Copyright 2000-2005
        Guillaume Laurent   <glaurent@telegraph-road.org>,
        Chris Cannam        <cannam@all-day-breakfast.com>,
        Richard Bown        <bownie@bownie.com>

    The moral right of the authors to claim authorship of this work
    has been asserted.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/
#ifndef GUITAR_BARRE_H_
#define GUITAR_BARRE_H_

#include <qdom.h>
#include <qstring.h>
#include <qpainter.h>

namespace Rosegarden
{
	class PropertyName;
}

namespace Guitar
{
    class Barre
    {
    public:
 	static const Rosegarden::PropertyName BARRE_FRET;
 	static const Rosegarden::PropertyName BARRE_START;
	static const Rosegarden::PropertyName BARRE_END;

	//! Constructor
        Barre ();

	Barre (unsigned int fret, unsigned int start, unsigned int end);

	//! Create Barre object from XML data
        void load (QDomNode const& obj);

	//! Save Barre object as XML data
        void save (QDomNode& obj);

	//! Return present fret number where Barre is positioned
        unsigned int getFret (void) const;

	//! Return string number where Barre starts
        unsigned int getStart (void) const;

	//! Return string number where Barre ends
        unsigned int getEnd (void) const;

	//! Set Barre data
        void setBarre (unsigned int fret, unsigned int start, unsigned int end);

	//! Display Barre object using QPainter object
        void drawContents (QPainter* p,
			   unsigned int startFret,
			   unsigned int stringCount,
			   unsigned int fretDisplayed);

	//! Set the base fret for Barre object
        void setFirstFret (int const& fretChange);

	//! Display Barre object data as a text string
	QString toString (void) const;

    	bool operator== (Barre const& rhs) const;

    private:

	unsigned int m_fret;
	unsigned int m_start;
	unsigned int m_end;

    };

}

#endif /* GUITAR_BAR_H */
