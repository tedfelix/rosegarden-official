
#include "TrackPerformanceHelper.h"
#include <iostream>

namespace Rosegarden 
{
using std::cerr;
using std::endl;
using std::string;

TrackPerformanceHelper::~TrackPerformanceHelper() { }


timeT TrackPerformanceHelper::getSoundingDuration(iterator i)
{
    Event *e = *i;
    timeT d = e->getDuration();

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

        d += e->getDuration();
        if (!e->get<Bool>(Note::TiedForwardPropertyName, tiedForward) ||
            !tiedForward) return d;
    }

    return d;
}

}
