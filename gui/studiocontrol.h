/*
    Rosegarden-4 v0.2
    A sequencer and musical notation editor.

    This program is Copyright 2000-2002
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

#include "MappedCommon.h"
#include "MappedStudio.h"

#ifndef _STUDIOCONTROL_H_
#define _STUDIOCONTROL_H_

// Access and control the sequencer studio from the gui
//

namespace Rosegarden
{

class StudioControl
{
public:

    // Object management
    //
    static MappedObjectId
        createStudioObject(MappedObject::MappedObjectType type);
    static MappedObjectId
        getStudioObjectByType(MappedObject::MappedObjectType type);
    static bool destroyStudioObject(MappedObjectId id);

    // Properties
    //
    static MappedObjectPropertyList
                  getStudioObjectProperty(MappedObjectId id,
                                          const MappedObjectProperty &property);

    static bool setStudioObjectProperty(MappedObjectId id,
                                        const MappedObjectProperty &property,
                                        MappedObjectValue value);

    // cheat so we can avoid making calls() during playback
    //
    static void setStudioPluginPort(MappedObjectId pluginId,
                                    unsigned long portId,
                                    MappedObjectValue value);
};

}

#endif // _STUDIOCONTROL_H_

