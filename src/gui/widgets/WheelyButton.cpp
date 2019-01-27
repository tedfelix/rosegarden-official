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

#define RG_MODULE_STRING "[WheelyButton]"

#include "WheelyButton.h"

#include "misc/Debug.h"

#include <QPushButton>
#include <QWidget>
#include <QFont>


namespace Rosegarden
{


    // These things are only used by AudioRouteMenu, and it uses these to
    // provide the compact version of its interface, which is only used by the
    // audio mixer window.  This ctor is implemented to force the buttons to use
    // a smaller font, so they fit in better on the audio mixer and the buttons
    // don't say "aste" instead of "Master" and so on.  It would have been
    // cleaner to provide some mechanism for the audio mixer to get its audio
    // route menu's wheely buttons and only change them local to that context,
    // but in practice, this will work too, and it's cheap.
    WheelyButton::WheelyButton(QWidget *w) : QPushButton(w)
    {
        QFont font;
        font.setPointSize(6);
        setFont(font);
    }

    void WheelyButton::wheelEvent(QWheelEvent *e)
    {
        // We'll handle this.  Don't pass to parent.
        e->accept();

        if (e->angleDelta().y() > 0)
            emit wheel(true);
        else if (e->angleDelta().y() < 0)
            emit wheel(false);
    }


}
