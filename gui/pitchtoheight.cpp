/***************************************************************************
                          pitchtoheight.cpp  -  description
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

#include "pitchtoheight.h"
#include "staff.h"

PitchToHeight*
PitchToHeight::m_instance(0);

PitchToHeight::PitchToHeight(unsigned short staffLineWidth)
    : m_pitchToHeight(new vector<int>(12))
{
    // commodity ref
    vector<int>& pitchToHeight(*m_pitchToHeight);

    int height =  4 * staffLineWidth - Staff::noteHeight / 2 + 2,
        offset = 0;

    for(unsigned int note = 0, pitch = 0;
        pitch < pitchToHeight.size();
        ++pitch)
        {
            pitchToHeight[pitch] = height;

            if(pitch != 4 && // E
               pitch != 11) // B
                {
                    pitchToHeight[++pitch] = height;
                }

            if(pitch == 1 ||
               pitch == 3 ||
               pitch == 4 ||
               pitch == 6 ||
               pitch == 8 ||
               pitch == 10) {
                ++note;
                if(pitch == 1 || pitch == 4 || pitch == 8) ++offset;
                else if(pitch == 3 || pitch == 6) {
                    --offset;
                    if (pitch == 6) ++offset;
                }

            }
	
            height -= staffLineWidth / 2 + offset;
        }

    for(unsigned int pitch = 0; pitch < pitchToHeight.size(); ++pitch)
        cout << "Pitch : " << pitch << " - " << pitchToHeight[pitch]
             << endl;
}


PitchToHeight&
PitchToHeight::instance()
{
    if (!m_instance) {
        m_instance = new PitchToHeight(Staff::lineWidth);
    }

    return *m_instance;
}

int
PitchToHeight::operator[](unsigned int pitch) const
{
    return (*m_pitchToHeight)[pitch % 12];
}
