// -*- c-basic-offset: 4 -*-

/*
    Rosegarden-4
    A sequencer and musical notation editor.

    This program is Copyright 2000-2003
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

#include <unistd.h>

#include "config.h"

#include <qpainter.h>
#include <qfontmetrics.h>

#include <kapp.h>
#include <kstddirs.h>
#include <kconfig.h>
#include <ktip.h>

#include "kstartuplogo.h"
#include "constants.h"
#include "rosedebug.h"

KStartupLogo::KStartupLogo(QWidget * parent, const char *name)
    : QWidget(parent,name,
	      WStyle_Customize |
#if QT_VERSION >= 0x030100
              WStyle_Splash
#else
              WStyle_NoBorder | WStyle_StaysOnTop | WStyle_Tool | WX11BypassWM | WWinOwnDC
#endif
),
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
    QString versionLabel =
	QString("R%1 v%2").arg(version.left(sepIdx)).arg(version.mid(sepIdx + 1));
    int versionWidth = metrics.width(versionLabel);

    paint.drawText(m_pixmap.width() - versionWidth - 18,
                   m_pixmap.height() - 28,
		   versionLabel);

    paint.drawText(m_pixmap.width() - (width + 10), y, m_statusMessage);
}

void KStartupLogo::slotShowStatusMessage(const QString &message)
{
    m_statusMessage = message;
    paintEvent(0);
    QApplication::flushX();
}

void KStartupLogo::close()
{
    if (!m_wasClosed && isVisible()) {

        RG_DEBUG << "KStartupLogo::close: Showing Tips\n";
        KTipDialog::showTip(locate("data", "rosegarden/tips"));
    }

    QWidget::close();
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
