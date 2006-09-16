// -*- c-basic-offset: 4 -*-

/*
    Rosegarden-4
    A sequencer and musical notation editor.

    This program is Copyright 2000-2006
        Guillaume Laurent   <glaurent@telegraph-road.org>,
        Chris Cannam        <cannam@all-day-breakfast.com>,
        Richard Bown        <bownie@bownie.com>

    The moral right of the authors to claim authorship of this work
    has been asserted.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#include "rosegardenparameterbox.h"

RosegardenParameterBox::RosegardenParameterBox(const QString &shortLabel,
					       const QString &longLabel,
					       QWidget *parent,
					       const char *name) :
    QFrame(parent, name),
    m_shortLabel(shortLabel),
    m_longLabel(longLabel),
    m_mode(LANDSCAPE_MODE)
{
    init();
}

void RosegardenParameterBox::init()
{
    QFont plainFont;
    plainFont.setPointSize(plainFont.pointSize() * 95 / 100);
    if (plainFont.pixelSize() > 14) plainFont.setPixelSize(14);
    plainFont.setBold(false);
    m_font = plainFont;

    QFont boldFont;
    boldFont.setPointSize(int(boldFont.pointSize() * 9.5 / 10.0 + 0.5));
    if (boldFont.pixelSize() > 14) boldFont.setPixelSize(14);
    boldFont.setBold(true);

    setFont(boldFont);
}

// Return the string that should be used to label a given parameter box.

QString RosegardenParameterBox::getShortLabel() const
{
    return m_shortLabel;
}

QString RosegardenParameterBox::getLongLabel() const
{
    return m_longLabel;
}

QString RosegardenParameterBox::getPreviousBox(RosegardenParameterArea::Arrangement) const
{
    // No ordering known -- depends on subclasses
    return "";
}
	
#include "rosegardenparameterbox.moc"

