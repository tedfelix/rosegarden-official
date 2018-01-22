/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2018 the Rosegarden development team.
 
    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.
 
    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/


#include "DefaultVelocityColour.h"

#include "gui/general/GUIPalette.h"
#include <QColor>
#include "VelocityColour.h"


namespace Rosegarden
{

DefaultVelocityColour::DefaultVelocityColour()
        : VelocityColour(GUIPalette::getColour(GUIPalette::LevelMeterRed),
                         GUIPalette::getColour(GUIPalette::LevelMeterOrange),
                         GUIPalette::getColour(GUIPalette::LevelMeterGreen),
                         127,  // max knee
                         115,  // red knee
                         75,   // orange knee
                         25)  // green knee
{}

DefaultVelocityColour* DefaultVelocityColour::getInstance()
{
    if (!m_instance) m_instance = new DefaultVelocityColour;
    
    return m_instance;
}

DefaultVelocityColour* DefaultVelocityColour::m_instance = 0;

}
