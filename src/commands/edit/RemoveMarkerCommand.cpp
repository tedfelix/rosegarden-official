/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2025 the Rosegarden development team.

    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/


#include "RemoveMarkerCommand.h"

#include "base/Composition.h"
#include "base/Marker.h"


namespace Rosegarden
{


RemoveMarkerCommand::RemoveMarkerCommand(Composition *comp,
                                         int markerID) :
    NamedCommand(getGlobalName()),
    m_composition(comp),
    m_markerID(markerID)
{
}

RemoveMarkerCommand::~RemoveMarkerCommand()
{
    if (m_detached)
        delete m_marker;
}

void
RemoveMarkerCommand::execute()
{
    const Composition::MarkerVector &markers =
            m_composition->getMarkers();

    // For each marker...
    for (Marker *marker : markers) {
        // Found it?  Detach from Composition.
        if (marker->getID() == m_markerID) {
            m_marker = marker;
            m_composition->detachMarker(m_marker);
            m_detached = true;
            return;
        }
    }
}

void
RemoveMarkerCommand::unexecute()
{
    if (m_marker)
        m_composition->addMarker(m_marker);
    m_detached = false;
}


}
