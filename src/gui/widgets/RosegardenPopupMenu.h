/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2016 the Rosegarden development team.

    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#ifndef RG_ROSEGARDENPOPUPMENU_H
#define RG_ROSEGARDENPOPUPMENU_H

#include <QMenu>

namespace Rosegarden {

class RosegardenPopupMenu : public QMenu
{
    // just to make itemHeight public
    //
    // which is no longer possible or relevant in Qt 4, but I'm leaving this
    // header because of the stylesheet hack I added to it during the port
public:
    RosegardenPopupMenu(QWidget *parent) : QMenu(parent) { setStyleSheet("background-color: #EEEEEE;"); }
//    using QMenu::itemHeight;
};


}

#endif /*ROSEGARDENPOPUPMENU_H_*/
