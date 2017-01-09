/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2017 the Rosegarden development team.

    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#include "ViewElementAdapter.h"

namespace Rosegarden
{

ViewElementAdapter::ViewElementAdapter(ViewElement* el, const PropertyName& p)
    : m_viewElement(el),
      m_propertyName(p)
{
}

bool ViewElementAdapter::getValue(long& val)
{
    return m_viewElement->event()->get<Rosegarden::Int>(m_propertyName, val);
}

void ViewElementAdapter::setValue(long val)
{
    m_viewElement->event()->set<Rosegarden::Int>(m_propertyName, val);
}

timeT ViewElementAdapter::getTime()
{
    return m_viewElement->getViewAbsoluteTime();
}

timeT ViewElementAdapter::getDuration()
{
    return m_viewElement->getViewDuration();
}

}
