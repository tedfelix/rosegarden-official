/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.

    This program is Copyright 2000-2006
        Guillaume Laurent   <glaurent@telegraph-road.org>,
        Chris Cannam        <cannam@all-day-breakfast.com>,
        Richard Bown        <richard.bown@ferventsoftware.com>

    The moral rights of Guillaume Laurent, Chris Cannam, and Richard
    Bown to claim authorship of this work have been asserted.

    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#ifndef _RG_CONTROLSELECTOR_H_
#define _RG_CONTROLSELECTOR_H_

#include "ControlRuler.h"

class QCanvasRectangle;

namespace Rosegarden {

        
/**
 * Selector tool for the ControlRuler
 *
 * Allow the user to select several ControlItems so he can change them
 * all at the same time
 */
class ControlSelector : public QObject
{
public:
    ControlSelector(ControlRuler* parent);
    virtual ~ControlSelector() {};
    
    virtual void handleMouseButtonPress(QMouseEvent *e);
    virtual void handleMouseButtonRelease(QMouseEvent *e);
    virtual void handleMouseMove(QMouseEvent *e, int deltaX, int deltaY);

    QCanvasRectangle* getSelectionRectangle() { return m_ruler->getSelectionRectangle(); }
protected:
    //--------------- Data members ---------------------------------

    ControlRuler* m_ruler;
};

}

#endif /*CONTROLSELECTOR_H_*/
