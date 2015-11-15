/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2015 the Rosegarden development team.
 
    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.
 
    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/


#include "PluginPushButton.h"

#include <QPushButton>


namespace Rosegarden
{

void
PluginPushButton::setState(State state)
{
    // It's most sensible to do this with stylesheets now, since there are
    // problems using the various ColorRoles of QPalette with stylesheets,
    // especially for backgrounds.  The best thing to do would be to implement
    // properties and tie styles to the properties.  That's how LMMS does
    // everything I've done with these local stylesheet hacks.  But I figure
    // I'll save reworking and tidying all of this for some future release, and
    // just continue in the same haphazard fashion that got us this far for now.
    
    QString tipStyle(" QToolTip {color: black;}");

    QString localStyle;

    switch (state) {
        case Bypassed:
            localStyle = ("QPushButton::enabled {color: #C0C000; background: qlineargradient(spread:pad, x1:0, y1:1, x2:0, y2:0, stop:0 #C03600, stop:1 #C07C00);}");
            break;
        case Active:
            localStyle = ("QPushButton::enabled {color: yellow; background: qlineargradient(spread:pad, x1:0, y1:1, x2:0, y2:0, stop:0 #FF4500, stop:1 #FFA500);} QPushButton:hover {background: qlineargradient(spread:pad, x1:0, y1:1, x2:0, y2:0, stop:0 #FF6700, stop:1 #FFC700);}");
            break;
        case Normal:
        default:
            localStyle = ("QPushButton::enabled {background-color: qlineargradient(spread:pad, x1:0, y1:1, x2:0, y2:0, stop:0 #999999, stop:1 #DDDDDD);  color: #000000;} QPushButton::!enabled {background-color: qlineargradient(spread:pad, x1:0, y1:1, x2:0, y2:0, stop:0 #999999, stop:1 #DDDDDD); color: #000000;} QPushButton:hover {background-color: #CCDFFF; color: #000000;} QPushButton::checked, QPushButton::pressed {background-color: qlineargradient(spread:pad, x1:0, y1:1, x2:0, y2:0, stop:0 #E0E0E0, stop:1 #EEEEEE);}");
    }

    this->setStyleSheet(localStyle + tipStyle);
}

PluginPushButton::~PluginPushButton()
{
}

}
