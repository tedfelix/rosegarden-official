// -*- c-basic-offset: 4 -*-

/*
    Rosegarden-4
    A sequencer and musical notation editor.

    This program is Copyright 2000-2005
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

#ifndef PERCUSSIONPITCHRULER_H
#define PERCUSSIONPITCHRULER_H

#include <vector>

#include <qwidget.h>

#include "pitchruler.h"
#include "MidiProgram.h"

// basically a simpler PianoKeyboard

class PercussionPitchRuler : public PitchRuler
{
    Q_OBJECT
public:
    PercussionPitchRuler(QWidget *parent,
			 const Rosegarden::MidiKeyMapping *mapping,
			 int lineSpacing);

    virtual QSize sizeHint() const;
    virtual QSize minimumSizeHint() const;

    void drawHoverNote(int evPitch);

signals:
    void keyPressed(unsigned int y, bool repeating);
    void keySelected(unsigned int y, bool repeating);
    void hoveredOverKeyChanged(unsigned int y);

protected:
    virtual void paintEvent(QPaintEvent*);
    virtual void mouseMoveEvent(QMouseEvent*);
    virtual void mousePressEvent(QMouseEvent*);
    virtual void mouseReleaseEvent(QMouseEvent*);
    virtual void enterEvent(QEvent *);
    virtual void leaveEvent(QEvent *);

    const Rosegarden::MidiKeyMapping *m_mapping;

    int                       m_width;
    int                       m_lineSpacing;

    bool                      m_mouseDown;
    bool                      m_selecting;

    QWidget                  *m_hoverHighlight;
    int                       m_lastHoverHighlight;
};


#endif
