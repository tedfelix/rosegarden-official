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
#ifndef GUITAR_NOTE_H_
#define GUITAR_NOTE_H_

#include <qdom.h>
#include <qstring.h>
#include <qpainter.h>

namespace Rosegarden
{
class PropertyName;
}

namespace Guitar
{
class GuitarNeck;

class Note
{
public:

    static const Rosegarden::PropertyName NOTE_FRET;
    static const Rosegarden::PropertyName NOTE_STRING;

    //! Constructor
    Note ();

    Note ( unsigned int str_val, unsigned int fret_val );

    Note ( Note const& rhs );

    //! Create Note object from XML data
    void load ( QDomNode const& obj, GuitarNeck* arrangement );

    //! Save Note object as XML data
    void save ( QDomNode& obj, GuitarNeck* arrangement );

    //! Set the base fret for Note
    //! fret_change: the difference between the existing first fret and the new location
    void setFirstFret ( int const& fret_change );

    //! Return the present fret number
    unsigned int getFret ( void ) const;

    //! Return the present string number
    unsigned int getStringNumber ( void ) const;

    //! Set data for Note object
    void setNote ( unsigned int str_val, unsigned int fret_val );

    //! Display Note object using QPainter object
    void drawContents ( QPainter* p,
                        unsigned int start_fret,
                        unsigned int string_count,
                        unsigned int frets_displayed );

    //! Display Note object data as a text string
    QString toString ( void ) const;

    //! Get underlying Event
    //Rosegarden::Event* getData ( void ) const;

    bool operator== ( Note const& rhs ) const;

private:

    //! Data
    unsigned int m_string;
    unsigned int m_fret;

    //! Return string representing note name.
    QString noteName ( void );

};

}

#endif /* GUITAR_NOTE_H_ */
