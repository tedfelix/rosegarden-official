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
    enum TrackType { Midi, Audio };

    Track();
    Track(const int &id, const bool &muted,
          const TrackType &tt, const string &label,
          const int &position, const int &instrument);
    ~Track();

    int getID() const { return m_id; }
    bool isMuted() { return m_muted; }
    

private:

    int       m_id;
    bool      m_muted;
    TrackType m_type;
    string    m_label;
    int       m_position;
    int       m_instrument;

};

}

#endif // _TRACK_H_
 
