// -*- c-basic-offset: 4 -*-


#include "TrackPerformanceHelper.h"
#include "TrackNotationHelper.h" //!!! for tupled stuff; should move from TrackNotationHelper to somewhere more global
#include <iostream>

namespace Rosegarden 
{
using std::cerr;
using std::endl;
using std::string;

TrackPerformanceHelper::~TrackPerformanceHelper() { }


timeT
TrackPerformanceHelper::getSoundingDuration(iterator i)
{
    Event *e = *i;
    timeT d = getDurationWithTupling(e);

    if (d == 0 || !e->isa(Note::EventType)) return d;

    bool tiedBack = false, tiedForward = false;
    e->get<Bool>(Note::TiedBackwardPropertyName, tiedBack);
    e->get<Bool>(Note:: TiedForwardPropertyName, tiedForward);

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

        if (!e->get<Bool>(Note::TiedBackwardPropertyName, tiedBack) ||
            !tiedBack) break;

        d += getDurationWithTupling(e);
        if (!e->get<Bool>(Note::TiedForwardPropertyName, tiedForward) ||
            !tiedForward) return d;
    }

    return d;
}


timeT TrackPerformanceHelper::getDurationWithTupling(Event *e)
{
    timeT d = e->getDuration();

    long tupledLength;
    if (e->get<Int>(TrackNotationHelper::BeamedGroupTupledLengthPropertyName,
		    tupledLength)) {

	long untupledLength;
	if (e->get<Int>
	    (TrackNotationHelper::BeamedGroupUntupledLengthPropertyName,
	     untupledLength)) {
	    return (d * tupledLength) / untupledLength;
	} else {
	    cerr << "TrackPerformanceHelper::getDurationWithTupling: WARNING: "
		 << "Found tupled length without untupled length property"
		 << endl;
	}
    }

    return d;
}


}
