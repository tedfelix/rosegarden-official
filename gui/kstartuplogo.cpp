// -*- c-basic-offset: 4 -*-

/*
    Rosegarden-4 v0.1
    A sequencer and musical notation editor.

    This program is Copyright 2000-2002
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

// This file contains code borrowed from KDevelop 2.0
// (c) The KDevelop Development Team

#include "config.h"

#include <qpainter.h>

#include "kstartuplogo.h"
#include <kapp.h>
#include <kstddirs.h>

#include <unistd.h>

KStartupLogo::KStartupLogo(QWidget * parent, const char *name)
    : QWidget(parent,name,
              /* WType_Popup | */ WStyle_Dialog | WStyle_StaysOnTop /* | WStyle_NoBorderEx | WStyle_Customize*/ | WDestructiveClose),
    m_bReadyToHide(false)
{
    m_pixmap.load(locate("appdata", "pixmaps/splash.png"));
    setBackgroundPixmap(m_pixmap);
    setGeometry(QApplication::desktop()->width()/2-m_pixmap.width()/2,
                QApplication::desktop()->height()/2-m_pixmap.height()/2,
                m_pixmap.width(),m_pixmap.height());
}

KStartupLogo::~KStartupLogo()
{
}

void KStartupLogo::paintEvent(QPaintEvent*)
{
    // Print version number
    QPainter paint(this);
    paint.setPen(Qt::black);

    QFont defaultFont;
    defaultFont.setPixelSize(12);
    paint.setFont(defaultFont);

    paint.drawText(m_pixmap.width() - 60,
                   m_pixmap.height() - 10,
                   QString("r4 v") + VERSION);
    raise();
}

void KStartupLogo::mousePressEvent( QMouseEvent*)
{
    // for the haters of raising startlogos
    if (m_bReadyToHide)
        hide();
}
