/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2015 the Rosegarden development team.

    This file is Copyright 2006-2009
	D. Michael McIntyre <dmmcintyr@users.sourceforge.net>

    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#ifndef RG_PRESETELEMENT_H
#define RG_PRESETELEMENT_H

#include <QString>

#include <vector>



namespace Rosegarden
{

/*
 * A container class for storing a concise description of the physical
 * capabilities of a real-world instrument.  This data serves as an aid to
 * composers, facilitating the writing of sensible parts for unfamiliar
 * instruments.
 *
 * \author D. Michael McIntyre
 */
class PresetElement
{
public:

    /** Create a PresetElement describing real-world instrument \c name,
     * containing its \c clef (index), \c transpose value (eg. -2 for Bb), and
     * highest and lowest playable notes for amateur and professional players.
     */
    PresetElement(QString name,
                  int clef,
                  int transpose,
                  int highAm,
                  int lowAm,
                  int highPro,
                  int lowPro);

    ~PresetElement();

    /** Return the real world instrument \c name */
    QString getName()    { return m_name;      }

    /** Return the preferred \c clef for this instrument */
    int getClef()        { return m_clef;      }

    /** Return the appropriate \c transpose value for this instrument */
    int getTranspose()   { return m_transpose; }

    /** Return the highest note an amateur player can be expected to reach on
     * this instrument, expressed as a MIDI pitch */ 
    int getHighAm()      { return m_highAm;    }    

    /** Return the lowest note an amateur player can be expected to reach on
     * this instrument, expressed as a MIDI pitch */
    int getLowAm()       { return m_lowAm;     }

    /** Return the highest note a professional player can be expected to reach on
     * this instrument, expressed as a MIDI pitch */ 
    int getHighPro()     { return m_highPro;   }

    /** Return the lowest note a professional player can be expected to reach on
     * this instrument, expressed as a MIDI pitch */
    int getLowPro()      { return m_lowPro;    }

private:
    QString m_name;
    int m_clef;
    int m_transpose;
    int m_highAm;
    int m_lowAm;
    int m_highPro;
    int m_lowPro;
}; // PresetElement

/** A container for storing a collection of PresetElement objects */
typedef std::vector<PresetElement> ElementContainer;

}

#endif
