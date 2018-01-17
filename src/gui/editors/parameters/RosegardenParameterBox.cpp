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

#define RG_MODULE_STRING "[RosegardenParameterBox]"

#include "RosegardenParameterBox.h"

#include "misc/Debug.h"


namespace Rosegarden
{


RosegardenParameterBox::RosegardenParameterBox(const QString &label,
                                               QWidget *parent) :
    QFrame(parent),
    m_label(label)
{
    QFont plainFont;
    // Go with 9/10 the default point size.  On mine this goes from 11 to 9.
    plainFont.setPointSize(plainFont.pointSize() * 90 / 100);
    plainFont.setBold(false);
    m_font = plainFont;

    // This font is picked up by the CollapsingFrame's in the
    // TrackParameterBox.
    // ??? Might want to make this more explicit.  Perhaps an m_boldFont and
    //     then have TrackParameterBox pass it to CollapsingFrame?
    QFont boldFont;
    // 95/100 of the default point size.  On mine this goes from 11 to 10.
    boldFont.setPointSize(
            static_cast<int>(boldFont.pointSize() * 9.5 / 10.0 + 0.5));
    boldFont.setBold(true);

    setFont(boldFont);
}


}
