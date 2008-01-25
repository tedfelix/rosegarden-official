
/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.

    This program is Copyright 2000-2008
        Guillaume Laurent   <glaurent@telegraph-road.org>,
        Chris Cannam        <cannam@all-day-breakfast.com>,
        Richard Bown        <richard.bown@ferventsoftware.com>

    This file is Copyright 2006
	D. Michael McIntyre <dmmcintyr@users.sourceforge.net>

    The moral rights of Guillaume Laurent, Chris Cannam, and Richard
    Bown to claim authorship of this work have been asserted.

    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#ifndef _RG_PRESETELEMENT_H_
#define _RG_PRESETELEMENT_H_

#include <qstring.h>

#include <vector>



namespace Rosegarden
{

/*
 * A container class for storing a set of data describing a real world
 * instrument for which one is writing musical notation
 */
class PresetElement
{
public:

    PresetElement(QString name,
                  int clef,
                  int transpose,
                  int highAm,
                  int lowAm,
                  int highPro,
                  int lowPro);

    ~PresetElement();

    // accessors
    QString getName()    { return m_name;      }
    int getClef()        { return m_clef;      }
    int getTranspose()   { return m_transpose; }
    int getHighAm()      { return m_highAm;    }     
    int getLowAm()       { return m_lowAm;     }
    int getHighPro()     { return m_highPro;   }
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

typedef std::vector<PresetElement> ElementContainer;

}

#endif
