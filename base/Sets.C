// -*- c-basic-offset: 4 -*-

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

#include "Sets.h"

#include "Event.h"
#include "BaseProperties.h"
#include "Quantizer.h"

namespace Rosegarden {

template <>
Event *
GenericSet<Event, Segment>::getAsEvent(const Segment::iterator &i)
{
    return *i;
}

/*
 * This ridiculous shit appears to be necessary to please gcc-2.95.2.
 * Compiler bug?  My own misunderstanding of some huge crock of crap
 * in the C++ standard?  No idea.  If you know, tell me.  Anyway, as
 * it stands I can't get any calls to get<> or set<> from the Set or
 * Chord methods to compile -- the compiler appears to parse the
 * opening < of the template arguments as an operator<.  Hence this.
 */

extern long
get__Int(Event *e, const PropertyName &name)
{
    return e->get<Int>(name);
}

extern bool
get__Bool(Event *e, const PropertyName &name)
{
    return e->get<Bool>(name);
}

extern std::string
get__String(Event *e, const PropertyName &name)
{
    return e->get<String>(name);
}

extern bool
get__Int(Event *e, const PropertyName &name, long &ref)
{
    return e->get<Int>(name, ref);
}

extern bool
get__Bool(Event *e, const PropertyName &name, bool &ref)
{
    return e->get<Bool>(name, ref);
}

extern bool
get__String(Event *e, const PropertyName &name, std::string &ref)
{
    return e->get<String>(name, ref);
}

extern bool
isPersistent__Bool(Event *e, const PropertyName &name)
{
    return e->isPersistent<Bool>(name);
}

}

