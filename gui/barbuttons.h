// -*- c-basic-offset: 4 -*-

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

#ifndef _BARBUTTONS_H_
#define _BARBUTTONS_H_

#include <qvbox.h>
//#include "FastVector.h"
#include "rosegardenguidoc.h"

namespace Rosegarden {
    class RulerScale;
}
class LoopRuler;
class RosegardenGUIDoc;
class BarButtonsWidget;

class BarButtons : public QVBox
{
    Q_OBJECT

public:
    BarButtons(RosegardenGUIDoc* doc,
	       Rosegarden::RulerScale *rulerScale,
               int buttonHeight,
	       bool invert = false,
               QWidget* parent = 0,
               const char* name = 0,
               WFlags f=0);

    LoopRuler* getLoopRuler() { return m_loopRuler; }
    
    void setMinimumWidth(int width);

public slots:
    void slotScrollHoriz(int x);

private:
    //--------------- Data members ---------------------------------
    bool m_invert;
    int m_loopRulerHeight;
    int m_offset;
    int m_currentXOffset;
    
    RosegardenGUIDoc *m_doc;
    Rosegarden::RulerScale *m_rulerScale;

    BarButtonsWidget *m_hButtonBar;
    LoopRuler *m_loopRuler;
};

#endif // _BARBUTTONS_H_
