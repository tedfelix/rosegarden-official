// -*- c-basic-offset: 4 -*-

// -*- c-file-style:  "bsd" -*-
#ifndef _TIMED_VECTOR_H_
#define _TIMED_VECTOR_H_

#include "FastVector.h"

/** We have a choice of timing mechanisms here.  We want to be able to
    calculate the time of each element in the list.  We'd ideally like
    to avoid having to change all the timings when one changes; we'd
    like the timing information preferably to be associated with an
    element rather than its position in the list, so that we can just
    stream elements out on their own for a complete representation
    without having to explicitly associate the timings with the
    individual elements.

    The options are:

  1 Store an absolute time with each element.  I wouldn't want to put
    these _in_ the element structures, as I'd prefer elements to be as
    far as possible self-contained and transportable from one place in
    a segment to another without having to be rewritten for the new
    timing.

  2 Store the time that has elapsed since the start of the previous
    element.  This is also information that would not make the element
    quite self-contained, and in the usual case where elements follow
    one after another with no pause it would mean that the duration of
    each element is effectively stored twice in different places.

  3 Store the time that has elapsed since the _end_ of the previous
    element.  This is the approach of RG3a.  This means the usual case
    sees the values all set to zero, which is quite attractive, but it
    does lead to some rather counterintuitive negative times for
    chords and overlapping elements.

  4 Store the time that will elapse between the start of this element
    and the start of the following one.  In other words each element
    has two stored durations: the real duration of the sound, and its
    effective duration until the following element starts.  This has
    its attractions, not least being conceptually comprehensible.

  5 Store the time that will elapse between the _end_ of this element
    and the start of the following one.  From the code perspective
    this is probably much the same as the option 4.

    In any case, I anticipate storing duration as a "special" property
    that every element has natively and that doesn't have to go into
    the same property list within the element structure as the normal
    properties do -- looking up durations should be fast and should
    always succeed.  Options 4 and 5 would work best if the
    "effective" duration was also stored this way, but perhaps
    optionally -- i.e. only if it differed from the "real" duration.

    At the moment I'm favouring option 5, although it will mean a bit
    of a change from the existing RG3a code.  (Not as much as the
    otherwise almost equivalent option 4 would though.)

*/


#endif

