
/*
    Rosegarden-4 v0.1
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

#ifndef _NOTATION_RULER_SCALE_H_
#define _NOTATION_RULER_SCALE_H_

#include "rulerscale.h"

class NotationHLayout;
class NotationStaff;
namespace Rosegarden { class Composition; }

class NotationRulerScale : public RulerScale
{
public:
    NotationRulerScale(Rosegarden::Composition *composition);
    virtual ~NotationRulerScale();

    void setLayout(NotationHLayout *layout);
    void setFirstStartingStaff(NotationStaff *firstStartingStaff);
    void setLastFinishingStaff(NotationStaff *lastFinishingStaff);

    virtual int getFirstBarNumber();
    virtual int getLastBarNumber();

    virtual double getBarPosition(int n);
    virtual double getBarWidth(int n);
    virtual double getBeatWidth(int n);
    virtual int getBarForX(double x);
    virtual Rosegarden::timeT getTimeForX(double x);
    virtual double getXForTime(Rosegarden::timeT time);

protected:
    NotationHLayout *m_layout;
    NotationStaff *m_lastFinishingStaff;
    int m_firstBar;

    int getLayoutBarNumber(int n);
};

#endif

