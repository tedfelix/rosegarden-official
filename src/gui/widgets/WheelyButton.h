/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2008 the Rosegarden development team.

    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#ifndef _RG_WHEELYBUTTON_H_
#define _RG_WHEELYBUTTON_H_

#include <QPushButton>
#include <QWheelEvent>


class QWidget;
class QWheelEvent;


namespace Rosegarden
{



class WheelyButton : public QPushButton
{
    Q_OBJECT

public:
    WheelyButton(QWidget *w) : QPushButton(w) { }
    virtual ~WheelyButton() { }

signals:
    void wheel(bool up);

protected:
    void wheelEvent(QWheelEvent *e) {
        emit wheel(e->delta() > 0);
    }
};



// A specialised menu for selecting audio inputs or outputs, that
// queries the studio and instrument to find out what it should show.
// Available in a "compact" size, which is a push button with a popup
// menu attached, or a regular size which is a combobox.
//

}

#endif
