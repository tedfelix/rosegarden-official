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

#include "BaseProperties.h"

namespace Rosegarden
{

namespace BaseProperties
{

// Some of the most basic property names are defined in individual
// classes in NotationTypes.h -- those are the ones that are used to
// store the value of a clef/key/timesig event, whereas things like
// notes have their values calculated from the duration property

const PropertyName NOTE_TYPE            = "NoteType";
const PropertyName NOTE_DOTS            = "NoteDots";
const PropertyName PITCH		= "pitch";
const PropertyName VELOCITY		= "velocity";
const PropertyName ACCIDENTAL		= "accidental";

const PropertyName TIED_BACKWARD	= "TiedBackward";
const PropertyName TIED_FORWARD		= "TiedForward";

// Note that these should be non-persistent properties, because
// they're the same for each event in a group and so the code that
// writes out XML converts them to group properties instead of
// writing them as explicit properties of the events:

const PropertyName BEAMED_GROUP_ID               = "BGroupId";
const PropertyName BEAMED_GROUP_TYPE		 = "BGroupType";
const PropertyName BEAMED_GROUP_TUPLED_LENGTH	 = "BGroupTupledLength";
const PropertyName BEAMED_GROUP_TUPLED_COUNT	 = "BGroupTupledCount";
const PropertyName BEAMED_GROUP_UNTUPLED_LENGTH	 = "BGroupUntupledLength";

// This one is persistent, though, because it obviously isn't the same
// for each event in a group:

const PropertyName TUPLET_NOMINAL_DURATION	 = "TupletNominalDuration";

const std::string GROUP_TYPE_BEAMED		 = "beamed";
const std::string GROUP_TYPE_TUPLED		 = "tupled";
const std::string GROUP_TYPE_GRACE		 = "grace";

}

}

