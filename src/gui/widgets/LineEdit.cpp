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

#include "LineEdit.h"

#include <QLineEdit>
#include <QToolTip>


namespace Rosegarden
{

LineEdit::LineEdit(QWidget *parent) :
        QLineEdit(parent)
{
    // This just gets more and more ugly.  Even here, no selector is specific
    // enough to avoid the colors getting broken, so we have to set the
    // background in a stylesheet with no selector and no brackets to get the
    // background color to work.  We also have to specify the foreground color
    // here, it turns out, to make it work everywhere, as it only comes in from
    // the external stylesheet about half the time, and white text on a white
    // background isn't very useful.
    QString localStyle = "QWidget {background-color: #FFFFFF; color: #000000;} QToolTip {background-color: #FFFBD4; color: #000000;}";
    setStyleSheet(localStyle);
    setToolTip("I am a LineEdit, not a QLineEdit");
}

LineEdit::LineEdit(const QString& string, QWidget *parent) :
        QLineEdit(string, parent)
{
    // Leave everything but the background to the external stylesheet
    QString localStyle = "background-color: #FFFFFF;";
    setStyleSheet(localStyle);
}

LineEdit::~LineEdit()
{
}

}

