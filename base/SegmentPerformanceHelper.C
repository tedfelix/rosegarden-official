
#include "TrackPerformanceHelper.h"

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
    int pitch = e->get<Int>("pitch");

    for (;;) {
        i = track().findContiguousNext(i);
        e = *i;

        timeT t2 = e->getAbsoluteTime();
        
        if (t2 > t + d) break;
        else if (t2 < t + d || e->get<Int>("pitch") != pitch) continue;

        if (!e->get<Bool>(Note::TiedBackwardPropertyName, tiedBack) ||
            !tiedBack) break;

        d += e->getDuration();
        if (!e->get<Bool>(Note:: TiedForwardPropertyName, tiedForward) ||
            !tiedForward) return d;
    }

    return d;
}

}
