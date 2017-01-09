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

#ifndef RG_PITCHRULER_H
#define RG_PITCHRULER_H

#include <QSize>
#include <QWidget>




namespace Rosegarden
{



class PitchRuler : public QWidget
{
    Q_OBJECT
public:
    PitchRuler(QWidget *parent);

    virtual QSize sizeHint() const;
    virtual QSize minimumSizeHint() const;

    virtual void drawHoverNote(int evPitch) = 0;
    virtual void hideHoverNote() = 0;

signals:

    /**
     * A pitch has been clicked.
     * y is the simple event y-coordinate.
     * If the user is in the middle of dragging, repeating will be set.
     */
    void keyPressed(unsigned int y, bool repeating);

    /**
     * A key has been released on the keyboard.
     *
     * The repeating flag is there to tell the MatrixView not to send
     * the same note again as we're in the middle of a swoosh.
     * MatrixView does the y -> Note calculation.
     */
    void keyReleased(unsigned int y, bool repeating);

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


}

#endif
