/***************************************************************************
                          scale.h  -  description
                             -------------------
    begin                : Mon May 7 2001
    copyright            : (C) 2001 by Guillaume Laurent, Chris Cannam, Rich Bown
    email                : glaurent@telegraph-road.org, cannam@all-day-breakfast.com, bownie@bownie.com
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef SCALE_H
#define SCALE_H

#include <vector>

class NotationElement;

/**This class represents a music scale in a given key.
It is used by horizontal layout to know whether a given pitch
belongs to a scale or not.
  *@author Guillaume Laurent, Chris Cannam, Rich Bown
  */

class Scale {
public: 

    enum KeySignature {
        C,      // no sharps
        G,      // 1 sharp
        D,      // 2 sharps
        A,      // 3 sharps
        E,      // 4 sharps
        B,      // 5 sharps
        Fsharp, // 6 sharps
        Csharp, // 7 sharps
        F,      // 1 flat
        Bflat,  // 2 flats
        Eflat,  // 3 flats
        Aflat,  // 4 flats
        Dflat,  // 5 flats
        Gflat,  // 6 flats
        Cflat   // 7 flats
    };

    Scale(KeySignature keysig);
    KeySignature key() const { return m_keySignature; }
    bool useSharps() const { return m_useSharps; }
    bool pitchIsInScale(unsigned int pitch) const;
    bool pitchIsDecorated(unsigned int pitch) const { return !pitchIsInScale(pitch); }
    bool noteIsDecorated(const NotationElement &el) const;

protected:
    KeySignature m_keySignature;
    bool m_useSharps;
    vector<bool> m_notes;
};

#endif
