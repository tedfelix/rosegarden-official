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

#ifndef PIANOKEYBOARD_H
#define PIANOKEYBOARD_H

#include <vector>

#include <qwidget.h>

class PianoKeyboard : public QWidget
{
    Q_OBJECT

public:
    PianoKeyboard(QSize keySize, QWidget *parent,
                  const char* name = 0, WFlags f = 0);

    virtual QSize sizeHint() const;
    virtual QSize minimumSizeHint() const;

signals:
    void notePressed(int pitch);

    /**
     * Emitted when the mouse cursor moves to a different key
     *
     * \a noteName contains the MIDI name of the corresponding note
     */
    void hoveredOverNoteChanged(const QString &noteName);

protected:
    // ActiveItem interface
//     virtual void mousePressEvent       (QMouseEvent*);
//     virtual void mouseReleaseEvent     (QMouseEvent*);
//     virtual void mouseDoubleClickEvent (QMouseEvent*);
//     virtual void mouseMoveEvent        (QMouseEvent*);

    virtual void paintEvent(QPaintEvent*);

    void computeKeyPos();

    //--------------- Data members ---------------------------------
    QSize m_keySize;
    QSize m_blackKeySize;
    unsigned int m_nbKeys;

    std::vector<unsigned int> m_whiteKeyPos;
    std::vector<unsigned int> m_blackKeyPos;
};


#endif
