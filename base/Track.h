// -*- c-basic-offset: 4 -*-

/*
    Rosegarden-4 v0.1
    A sequencer and musical notation editor.

    This program is Copyright 2000-2001
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

// Representation of a Track
//
//


#ifndef _TRACK_H_
#define _TRACK_H_

#include <string>

// A Track contains information pertaining to a graphical
// track on the sequencer.  This class is basically an
// abstract concept (not an abstract class) which has no
// dependency upon a specific system or sound hardware.
//
//

namespace Rosegarden
{

class Track
{

public:
    enum TrackType { midi, audio };

    Track();
    ~Track();

    bool isMuted() { return m_muted; }
    bool getNumber() { return m_number; }
    

private:

    int  m_number;
    bool m_muted;
    TrackType m_type;
    string m_name;

};

}

#endif // _TRACK_H_
 
