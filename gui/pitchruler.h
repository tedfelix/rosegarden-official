// -*- c-basic-offset: 4 -*-

/*
    Rosegarden-4
    A sequencer and musical notation editor.

    This program is Copyright 2000-2006
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

#ifndef PITCHRULER_H
#define PITCHRULER_H

#include <qwidget.h>

class PitchRuler : public QWidget
{
    Q_OBJECT
public:
    PitchRuler(QWidget *parent);

    virtual QSize sizeHint() const;
    virtual QSize minimumSizeHint() const;

    virtual void drawHoverNote(int evPitch) = 0;

signals:

    /**
     * A pitch has been clicked.
     * y is the simple event y-coordinate.
     * If the user is in the middle of dragging, repeating will be set.
     */
    void keyPressed(unsigned int y, bool repeating);

    /**
     * A pitch has been clicked with the selection modifier pressed.
     * y is the simple event y-coordinate.
     * If the user is in the middle of dragging, repeating will be set.
     */
    void keySelected(unsigned int y, bool repeating);

    /**
     * Emitted when the mouse cursor moves to a different pitch when
     * not clicking or selecting.
     * y is the simple event y-coordinate.
     */
    void hoveredOverKeyChanged(unsigned int y);
};

#endif
