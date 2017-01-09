/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A sequencer and musical notation editor.
    Copyright 2000-2017 the Rosegarden development team.
    See the AUTHORS file for more details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#include "Sets.h"

#include "Event.h"
#include "base/BaseProperties.h"
#include "Quantizer.h"

namespace Rosegarden {

template <>
Event *
AbstractSet<Event, Segment>::getAsEvent(const Segment::iterator &i)
{
    return *i;
}

template <>
Event *
AbstractSet<Event, CompositionTimeSliceAdapter>::getAsEvent(const CompositionTimeSliceAdapter::iterator &i)
{
    return *i;
}

/*
 * This ridiculous shit appears to be necessary to please gcc.
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

extern void
setMaybe__Int(Event *e, const PropertyName &name, long value)
{
    e->setMaybe<Int>(name, value);
}

extern void
setMaybe__String(Event *e, const PropertyName &name, const std::string &value)
{
    e->setMaybe<String>(name, value);
}

}

