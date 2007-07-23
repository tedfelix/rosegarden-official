
/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.

    This program is Copyright 2000-2007
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

#ifndef _RG_GUITAR_CHORD_INSERTER_H_
#define _RG_GUITAR_CHORD_INSERTER_H_

#include "NotationTool.h"
#include <qstring.h>
#include "base/Event.h"


class QMouseEvent;


namespace Rosegarden
{

class ViewElement;
class Staff;
class NotationView;
class GuitarChordSelectorDialog;

/**
 * This tool will insert guitar chord on mouse click events
*/
class GuitarChordInserter : public NotationTool
{
    Q_OBJECT

    friend class NotationToolBox;

public:

    virtual void handleLeftButtonPress(timeT t,
                                       int height,
                                       int staffNo,
                                       QMouseEvent* e,
                                       ViewElement *element);

/*
    virtual void handleMouseDoubleClick(timeT,
                                        int height, int staffNo,
                                        QMouseEvent*,
                                        ViewElement* el);
*/

    static const QString ToolName;

protected slots:
    void slotGuitarChordSelected();
    void slotEraseSelected();
    void slotSelectSelected();

protected:
    GuitarChordSelectorDialog* m_guitarChordSelector;

    GuitarChordInserter(NotationView*);

private:
    void handleSelectedGuitarChord (ViewElement* element,
                                  Staff *staff);

    void createNewGuitarChord (ViewElement* element,
                             Staff *staff,
                             QMouseEvent* e);

    bool processDialog (Staff *staff,
                        timeT& insertionTime);
};


}

#endif
