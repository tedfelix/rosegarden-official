// -*- c-basic-offset: 4 -*-

/*
    Rosegarden-4
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
#include <qfontmetrics.h>

#include "kstartuplogo.h"
#include <kapp.h>
#include <kstddirs.h>

#include "rosedebug.h"

#include <unistd.h>

KStartupLogo::KStartupLogo(QWidget * parent, const char *name)
    : QWidget(parent,name,
	      WStyle_Dialog  | WStyle_StaysOnTop | WStyle_NoBorderEx | WStyle_Customize | WDestructiveClose),
    m_readyToHide(false)
{
    QString pixmapFile = locate("appdata", "pixmaps/splash.png");
    if (!pixmapFile) return;
    m_pixmap.load(pixmapFile);
    setBackgroundPixmap(m_pixmap);
    setGeometry(QApplication::desktop()->width()/2-m_pixmap.width()/2,
                QApplication::desktop()->height()/2-m_pixmap.height()/2,
                m_pixmap.width(),m_pixmap.height());
}

KStartupLogo::~KStartupLogo()
{
    m_wasClosed = true;
    m_instance = 0;
}

void KStartupLogo::paintEvent(QPaintEvent*)
{
    // Print version number
    QPainter paint(this);

    QFont defaultFont;
    defaultFont.setPixelSize(12);
    paint.setFont(defaultFont);

    QFontMetrics metrics(defaultFont);
    int width = metrics.width(m_statusMessage) + 6;
    if (width > 200) width = 200;

    int y = m_pixmap.height() - 12;

    paint.setPen(QColor(206,214,163));
    paint.setBrush(QColor(206,214,163));
    paint.drawRect(QRect(m_pixmap.width() - 220, m_pixmap.height() - 43,
			 220, (y + 8) - (m_pixmap.height() - 43)));

    paint.setPen(Qt::black);
    paint.setBrush(Qt::black);

    QString version(VERSION);
    int sepIdx = version.find("-");

    paint.drawText(m_pixmap.width() - 54,
                   m_pixmap.height() - 28,
                   QString("R") + version.left(sepIdx) +
                   QString(" v") + version.mid(sepIdx+1));

    paint.drawText(m_pixmap.width() - (width + 10), y, m_statusMessage);
}

void KStartupLogo::slotShowStatusMessage(const QString &message)
{
    m_statusMessage = message;
    paintEvent(0);
    QApplication::flushX();
}


void KStartupLogo::mousePressEvent( QMouseEvent*)
{
    // for the haters of raising startlogos
    if (m_readyToHide)
        hide(); // don't close, main() sets up a QTimer for that
}

KStartupLogo* KStartupLogo::getInstance()
{
    if (m_wasClosed) return 0;
    
    if (!m_instance) m_instance = new KStartupLogo;

    return m_instance;
}

void KStartupLogo::hideIfStillThere()
{
    if (m_instance) m_instance->hide();
    // don't close, main() sets up a QTimer for that
}


KStartupLogo* KStartupLogo::m_instance = 0;
bool KStartupLogo::m_wasClosed = false;
