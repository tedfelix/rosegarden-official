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

#ifndef ROSEGARDENCANVASVIEW_H
#define ROSEGARDENCANVASVIEW_H

#include <qcanvas.h>

class QScrollBar;

/**
 * A QCanvasView with an auxiliary horiz. scrollbar
 */
class RosegardenCanvasView : public QCanvasView
{
    Q_OBJECT
public:
    RosegardenCanvasView(QScrollBar*, QCanvas*,
                         QWidget* parent=0, const char* name=0, WFlags f=0);

    virtual void polish();

public slots:
    /// Update the RosegardenCanvasView after a change of content
    virtual void slotUpdate();

protected:
    QScrollBar* m_horizontalScrollBar;
};

#endif

