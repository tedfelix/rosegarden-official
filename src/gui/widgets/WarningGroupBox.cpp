/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2021 the Rosegarden development team.
 
    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.
 
    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#include "WarningGroupBox.h"

#include <QGroupBox>


namespace Rosegarden
{

WarningGroupBox::WarningGroupBox(QWidget *parent) :
        QGroupBox(parent)
{
    QString localStyle = "QGroupBox {background-color: #EF9F9F; border: 2px solid red; color: #FFFFFF;} QLabel {color: #000000; background-color: #EF9F9F;} QToolTip {background-color: #FFFBD4; color: #000000;}";
    setStyleSheet(localStyle);
}

WarningGroupBox::~WarningGroupBox()
{
}

}

