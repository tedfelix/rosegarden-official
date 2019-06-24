
/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2018 the Rosegarden development team.

    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#ifndef RG_TRISTATECHECKBOX_H
#define RG_TRISTATECHECKBOX_H

#include <QCheckBox>


class QWidget;
class QMouseEvent;


namespace Rosegarden
{


/**
 * A QCheckBox which starts out Tristate by default and allows us to
 * click only between on and off and only to *show* the
 * Qt::PartiallyChecked state.
 *
 * This is only used by the SegmentParameterBox.
 *
 * ??? I think we need to take a look at QCheckBox::nextCheckState() which
 *     appears to be the proper way to implement a Tristate that doesn't
 *     allow the user to select PartiallyChecked.  Then get rid of the
 *     bogus mouseReleaseEvent().
 */
class TristateCheckBox : public QCheckBox
{
Q_OBJECT
public:
    explicit TristateCheckBox(QWidget *parent=nullptr) :
        QCheckBox(parent)
    {
        setTristate(true);
    }

    ~TristateCheckBox() override  { }

protected:
    // Don't emit when the button is released.
    // ??? Why?
    void mouseReleaseEvent(QMouseEvent *) override  { }

private:
};


}

#endif
