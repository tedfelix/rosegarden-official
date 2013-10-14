/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2013 the Rosegarden development team.
 
    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.
 
    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#include "CheckButton.h"
#include "gui/general/IconLoader.h"

#include <QToolButton>

namespace Rosegarden
{


CheckButton::CheckButton(QString icon, QWidget *parent) :
    QToolButton(parent)
{
    // Buttons are black on black without adjusting the style, so we use a local
    // stylesheet.  This should probably obey m_Thorn, but we have plenty of
    // other hard coded style that doesn't, so I'm not bothering with all that.
    QString localStyle(
        "QToolButton,                                                                                                 "
        "QToolButton::enabled,                                                                                        "
        "{                                                                                                            "
        "    color: #FFFFFF;                                                                                          "
        "    background-color: transparent;                                                                           "
        "    border: 1px solid transparent;                                                                           "
        "    border-radius: 2px;                                                                                      "
        "}                                                                                                            "
        "                                                                                                             "
        "QToolButton::pressed,                                                                                        "
        "QToolButton::checked,                                                                                        "
        "{                                                                                                            "
        "    border: 1px solid #AAAAAA;                                                                               "
        "    border-radius: 2px;                                                                                      "
        "    background-color: qlineargradient(spread:pad, x1:0, y1:1, x2:0, y2:0, stop:0 #E0E0E0, stop:1 #EEEEEE);   "
        "}                                                                                                            "
        "                                                                                                             "
        "QToolButton::enabled:hover,                                                                                  "
        "{                                                                                                            "
        "    border: 1px solid #AAAAAA;                                                                               "
        "    border-radius: 2px;                                                                                      "
        "    background-color: #CCDFFF;                                                                               "
        "}                                                                                                            "
        "                                                                                                             "
        "QToolButton::!enabled,                                                                                       "
        "{                                                                                                            "
        "    color: #FFFFFF;                                                                                          "
        "    background-color: transparent;                                                                           "
        "}                                                                                                            "
    );
    setStyleSheet(localStyle);

    setToolButtonStyle(Qt::ToolButtonIconOnly);
    setCheckable(true);
    setIcon(IconLoader().loadPixmap(icon));

    // thinking out loud...
    //
    // since I'm subclassing anyway, I might add the ability to make a master
    // toggle button that keeps track of children and switches them all on or
    // off at once, else I'll have 8+ switch this when that switches slots with
    // lots of things to keep track of manually
}

CheckButton::~CheckButton()
{
}


}

#include "CheckButton.moc"
