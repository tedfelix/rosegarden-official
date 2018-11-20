/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2018 the Rosegarden development team.

    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#ifndef RG_VIEWELEMENTADAPTER_H
#define RG_VIEWELEMENTADAPTER_H

#include "ElementAdapter.h"
#include "base/ViewElement.h"

namespace Rosegarden
{

class Event;

class ViewElementAdapter : public ElementAdapter
{
public:
    ViewElementAdapter(ViewElement*, const PropertyName&);

    bool  getValue(long&) override;
    void  setValue(long) override;
    timeT getTime() override;
    timeT getDuration() override;

    Event* getEvent() override { return m_viewElement->event(); }
    ViewElement* getViewElement() { return m_viewElement; }

protected:

    //--------------- Data members ---------------------------------

    ViewElement* m_viewElement;
    const PropertyName& m_propertyName;
};

}

#endif /*VIEWELEMENTADAPTER_H_*/
