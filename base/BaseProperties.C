/*
    Rosegarden-4 v0.1
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

#include "BaseProperties.h"
#include <vector>

#if (__GNUC__ < 3)
#include <strstream>
#define stringstream strstream
#else
#include <sstream>
#endif

namespace Rosegarden
{

namespace BaseProperties
{

// Some of the most basic property names are defined in individual
// classes in NotationTypes.h -- those are the ones that are used to
// store the value of a clef/key/timesig event, whereas things like
// notes have their values calculated from the duration property.

// We mostly define persistent properties with lower-case names and
// non-persistent ones with mixed-case.  That's just because lower-
// case looks nicer in XML, whereas mixed-case is friendlier for the
// sorts of long names sometimes found in cached calculations.

const PropertyName PITCH		= "pitch";
const PropertyName VELOCITY		= "velocity";
const PropertyName ACCIDENTAL		= "accidental";

const PropertyName NOTE_TYPE            = "NoteType";
const PropertyName NOTE_DOTS            = "NoteDots";

const PropertyName SELECTED		= "selected";

const PropertyName MARK_COUNT		= "marks";

PropertyName getMarkPropertyName(int markNo)
{
    static std::vector<PropertyName> firstFive;

    if (firstFive.size() == 0) {
	firstFive.push_back(PropertyName("mark1"));
	firstFive.push_back(PropertyName("mark2"));
	firstFive.push_back(PropertyName("mark3"));
	firstFive.push_back(PropertyName("mark4"));
	firstFive.push_back(PropertyName("mark5"));
    }

    if (markNo < 5) return firstFive[markNo];

    // This is slower than it looks, because it means we need to do
    // the PropertyName interning process for each string -- hence the
    // firstFive cache

    std::stringstream markPropertyName;
    markPropertyName << "mark" << (markNo + 1) << std::ends;
    return markPropertyName.str();
}

const PropertyName TIED_BACKWARD	= "tiedback";
const PropertyName TIED_FORWARD		= "tiedforward";

const PropertyName BEAMED_GROUP_ID               = "groupid";
const PropertyName BEAMED_GROUP_TYPE		 = "grouptype";

const PropertyName BEAMED_GROUP_TUPLET_BASE	 = "tupletbase";
const PropertyName BEAMED_GROUP_TUPLED_COUNT	 = "tupledcount";
const PropertyName BEAMED_GROUP_UNTUPLED_COUNT	 = "untupledcount";

// non-persistent, calculated from the performance data + counts, with some
// rounding -- unless otherwise specified
const PropertyName TUPLET_NOMINAL_DURATION	 = "TupletNominalDuration";


/*!!!
const PropertyName BEAMED_GROUP_TUPLED_LENGTH	 = "BGroupTupledLength";
const PropertyName BEAMED_GROUP_TUPLED_COUNT	 = "BGroupTupledCount";
const PropertyName BEAMED_GROUP_UNTUPLED_LENGTH	 = "BGroupUntupledLength";

// This one is persistent, though, because it obviously isn't the same
// for each event in a group:

const PropertyName TUPLET_NOMINAL_DURATION	 = "TupletNominalDuration";
*/

const std::string GROUP_TYPE_BEAMED		 = "beamed";
const std::string GROUP_TYPE_TUPLED		 = "tupled";
const std::string GROUP_TYPE_GRACE		 = "grace";

}

}

