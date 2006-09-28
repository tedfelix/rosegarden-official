
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

#ifndef _RG_ROSEGARDENTRISTATECHECKBOX_H_
#define _RG_ROSEGARDENTRISTATECHECKBOX_H_

#include <qcheckbox.h>


class QWidget;
class QMouseEvent;


namespace Rosegarden
{



/** Create out own check box which is always Tristate 
 * and allows us to click only between on and off
 * and only to _show_ the third ("Some") state 
 */
class TristateCheckBox : public QCheckBox
{
Q_OBJECT
public:
    TristateCheckBox(QWidget *parent=0,
                               const char *name=0):QCheckBox(parent, name)
        { setTristate(true) ;}

    virtual ~TristateCheckBox();

protected:
    // don't emit when the button is released
    virtual void mouseReleaseEvent(QMouseEvent *);

private:
};


// A label that emits a double click signal and provides scroll wheel information.
//
//

}

#endif
