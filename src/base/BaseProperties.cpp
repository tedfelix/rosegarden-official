/*
    Rosegarden
    A sequencer and musical notation editor.
    Copyright 2000-2015 the Rosegarden development team.
    See the AUTHORS file for more details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#include "base/BaseProperties.h"
#include <vector>

#include <sstream>

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

const PropertyName NOTE_TYPE            = "notetype";
const PropertyName NOTE_DOTS            = "notedots";

const PropertyName MARK_COUNT		= "marks";

PropertyName getMarkPropertyName(int markNo)
{
    static std::vector<PropertyName> firstFive;

    if (firstFive.empty()) {
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

    markPropertyName << "mark" << (markNo + 1);

    return markPropertyName.str();
}

const PropertyName TIED_BACKWARD	= "tiedback";
const PropertyName TIED_FORWARD		= "tiedforward";
const PropertyName TIE_IS_ABOVE		= "tieabove";

// capitalised for back-compatibility (used to be in NotationProperties)
const PropertyName HEIGHT_ON_STAFF      = "HeightOnStaff";
const PropertyName NOTE_STYLE		= "NoteStyle";
const PropertyName BEAMED		= "Beamed";

const PropertyName BEAMED_GROUP_ID               = "groupid";
const PropertyName BEAMED_GROUP_TYPE		 = "grouptype";

const PropertyName BEAMED_GROUP_TUPLET_BASE	 = "tupletbase";
const PropertyName BEAMED_GROUP_TUPLED_COUNT	 = "tupledcount";
const PropertyName BEAMED_GROUP_UNTUPLED_COUNT	 = "untupledcount";

// persistent, but mixed-case anyway
const PropertyName IS_GRACE_NOTE                 = "IsGraceNote";

// obsolete
const PropertyName HAS_GRACE_NOTES               = "HasGraceNotes";

// non-persistent
const PropertyName MAY_HAVE_GRACE_NOTES          = "MayHaveGraceNotes";

const std::string GROUP_TYPE_BEAMED		 = "beamed";
const std::string GROUP_TYPE_TUPLED		 = "tupled";
const std::string GROUP_TYPE_GRACE		 = "grace";

const PropertyName TRIGGER_EXPAND                = "trigger_expand";
const PropertyName TRIGGER_EXPANSION_DEPTH       = "trigger_expansion_depth";
const PropertyName TRIGGER_SEGMENT_ID            = "triggersegmentid";
const PropertyName TRIGGER_SEGMENT_RETUNE        = "triggersegmentretune";
const PropertyName TRIGGER_SEGMENT_ADJUST_TIMES  = "triggersegmentadjusttimes";

const std::string TRIGGER_SEGMENT_ADJUST_NONE = "none";
const std::string TRIGGER_SEGMENT_ADJUST_SQUISH = "squish";
const std::string TRIGGER_SEGMENT_ADJUST_SYNC_START = "syncstart";
const std::string TRIGGER_SEGMENT_ADJUST_SYNC_END = "syncend";

const PropertyName RECORDED_CHANNEL		 = "recordedchannel";
const PropertyName RECORDED_PORT		 = "recordedport";

const PropertyName DISPLACED_X                   = "displacedx";
const PropertyName DISPLACED_Y                   = "displacedy";

const PropertyName INVISIBLE                     = "invisible";

const PropertyName TMP         = "temporary"; /// TODO : TMP -> REPEATING
const PropertyName LINKED_SEGMENT_IGNORE_UPDATE  = "linkedsegmentignoreupdate";

}

}

