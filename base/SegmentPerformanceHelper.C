// -*- c-basic-offset: 4 -*-


#include "TrackPerformanceHelper.h"
#include "BaseProperties.h"
#include <iostream>

namespace Rosegarden 
{
using std::cerr;
using std::endl;
using std::string;

using namespace BaseProperties;

TrackPerformanceHelper::~TrackPerformanceHelper() { }


timeT
TrackPerformanceHelper::getSoundingDuration(iterator i)
{
    Event *e = *i;
    timeT d = getDurationWithTupling(e);

    if (d == 0 || !e->isa(Note::EventType)) return d;

    bool tiedBack = false, tiedForward = false;
    e->get<Bool>(TIED_BACKWARD, tiedBack);
    e->get<Bool>(TIED_FORWARD, tiedForward);

    if (tiedBack) return 0;
    else if (!tiedForward) return d;

    timeT t = e->getAbsoluteTime();
    if (!e->has("pitch")) return d;
    int pitch = e->get<Int>("pitch");

    for (;;) {
        i = track().findContiguousNext(i);
   
        // Chris' own fix to stop this fux [rwb]
        //
        if (i == end()) return d;

        e = *i;

        timeT t2 = e->getAbsoluteTime();
        
        if (t2 > t + d) break;
        else if (t2 < t + d || !e->has("pitch") ||
                 e->get<Int>("pitch") != pitch) continue;

        if (!e->get<Bool>(TIED_BACKWARD, tiedBack) ||
            !tiedBack) break;

        d += getDurationWithTupling(e);
        if (!e->get<Bool>(TIED_FORWARD, tiedForward) ||
            !tiedForward) return d;
    }

    return d;
}


timeT TrackPerformanceHelper::getDurationWithTupling(Event *e)
{
    timeT d = e->getDuration();

/*!!! No, we're now storing the performance duration as the event's
  primary duration.  This is fine for triplets (as our base duration
  is divisible by 3) but may not be suitable for other tuplets where
  our performer's resolution is higher than the Rosegarden base
  resolution.  In such a case, this method could use the tupling data
  and the event's nominal-duration property to calculate a more
  precise duration for the performer than the Rosegarden units will
  permit, but to do so we'd have to know the performer's resolution
  and the point at which this method is currently called (when
  creating a MappedEvent) is not a point at which we know that.

    long tupledLength;
    if (e->get<Int>(BEAMED_GROUP_TUPLED_LENGTH, tupledLength)) {

	long untupledLength;
	if (e->get<Int>(BEAMED_GROUP_UNTUPLED_LENGTH, untupledLength)) {
	    return (d * tupledLength) / untupledLength;
	} else {
	    cerr << "TrackPerformanceHelper::getDurationWithTupling: WARNING: "
		 << "Found tupled length without untupled length property"
		 << endl;
	}
    }
*/

    return d;
}


}
