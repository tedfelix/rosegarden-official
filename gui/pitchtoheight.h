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

Pitch : 0  - C
Pitch : 1  - C#
Pitch : 2  - D
Pitch : 3  - D#
Pitch : 4  - E
Pitch : 5  - F
Pitch : 6  - F#
Pitch : 7  - G
Pitch : 8  - G#
Pitch : 9  - A
Pitch : 10 - A#
Pitch : 11 - B

  *@author Guillaume Laurent
  */

class PitchToHeight : public vector<int> {
public: 
	PitchToHeight(unsigned short notePixmapHeight);
};

#endif
