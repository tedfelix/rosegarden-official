/*
    Rosegarden-4
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

#ifndef _BASE_PROPERTIES_H_
#define _BASE_PROPERTIES_H_

#include "PropertyName.h"

namespace Rosegarden
{

namespace BaseProperties
{

extern const PropertyName PITCH;
extern const PropertyName VELOCITY;
extern const PropertyName ACCIDENTAL;

extern const PropertyName MARK_COUNT;
extern PropertyName getMarkPropertyName(int markNo);

extern const PropertyName TIED_BACKWARD;
extern const PropertyName TIED_FORWARD;

// Next one used to be in NotationProperties, but it's useful for doing
// things like distinguishing chords for performance & analysis as well.
// I suppose NotationProperties is happiest only holding things that are
// local to a single specific notation view.
extern const PropertyName STEM_UP;

extern const PropertyName BEAMED_GROUP_ID;
extern const PropertyName BEAMED_GROUP_TYPE;

extern const PropertyName BEAMED_GROUP_TUPLET_BASE;
extern const PropertyName BEAMED_GROUP_TUPLED_COUNT;
extern const PropertyName BEAMED_GROUP_UNTUPLED_COUNT;

extern const PropertyName TUPLET_NOMINAL_DURATION;

extern const PropertyName IS_GRACE_NOTE;
extern const PropertyName HAS_GRACE_NOTES;

extern const std::string GROUP_TYPE_BEAMED;
extern const std::string GROUP_TYPE_TUPLED;
extern const std::string GROUP_TYPE_GRACE;

}

}

#endif

