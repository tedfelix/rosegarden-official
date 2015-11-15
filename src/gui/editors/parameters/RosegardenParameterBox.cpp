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

#define RG_MODULE_STRING "[RosegardenParameterBox]"

#include "RosegardenParameterBox.h"

#include "RosegardenParameterArea.h"
#include "misc/Debug.h"
#include <QTabWidget>
#include <QFont>
#include <QFrame>
#include <QScrollArea>
#include <QString>
#include <QWidget>
#include <QVBoxLayout>
#include <QStackedWidget>


namespace Rosegarden
{

RosegardenParameterBox::RosegardenParameterBox(const QString &shortLabel,
        const QString &longLabel,
        QWidget *parent) :
        QFrame(parent),
        m_shortLabel(shortLabel),
        m_longLabel(longLabel),
        m_mode(LANDSCAPE_MODE)
{
    init();
}

void RosegardenParameterBox::init()
{
    QFont plainFont;
    plainFont.setPointSize(plainFont.pointSize() * 90 / 100);
    plainFont.setBold(false);
    m_font = plainFont;

    RG_DEBUG << "RosegardenParameterBox::init: " << this << ": m_font size is " << m_font.pixelSize() << "px (" << m_font.pointSize() << "pt)" << endl;

    QFont boldFont;
    boldFont.setPointSize(int(boldFont.pointSize() * 9.5 / 10.0 + 0.5));
    if (boldFont.pixelSize() > 14)
        boldFont.setPixelSize(14);
    boldFont.setBold(true);

    setFont(boldFont);
}

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

}
