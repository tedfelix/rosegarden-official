/***************************************************************************
                          pitchtoheight.h  -  description
                             -------------------
    begin                : Mon Jun 19 2000
    copyright            : (C) 2000 by Guillaume Laurent, Chris Cannam, Rich Bown
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

#ifndef PITCHTOHEIGHT_H
#define PITCHTOHEIGHT_H

#include <vector>

/**Creates a conversion table to compute a note pixmap's y coord (relative to a staff)
from a note's pitch :

Pitch : 0  - 40 - C
Pitch : 1  - 40 - C#
Pitch : 2  - 36 - D
Pitch : 3  - 36 - D#
Pitch : 4  - 32 - E
Pitch : 5  - 28 - F
Pitch : 6  - 28 - F#
Pitch : 7  - 24 - G
Pitch : 8  - 24 - G#
Pitch : 9  - 20 - A
Pitch : 10 - 20 - A#
Pitch : 11 - 16 - B

  *@author Guillaume Laurent
  */

class PitchToHeight : public vector<int> {
public: 
	PitchToHeight(unsigned short notePixmapHeight);
};

#endif
