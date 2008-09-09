// -*- c-basic-offset: 4 -*-

/*
    Rosegarden
    A sequencer and musical notation editor.
    Copyright 2000-2008 the Rosegarden development team.
    See the AUTHORS file for more details.

    This file contains code borrowed from KDevelop 2.0
    Copyright (c) The KDevelop Development Team.
 
    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#include <unistd.h>
#include <QApplication>

#include <QPainter>
#include <QFontMetrics>

#include <kapp.h>
#include <kstandarddirs.h>
#include <kconfig.h>
#include <ktip.h>

#include "KStartupLogo.h"
#include "misc/Debug.h"

KStartupLogo::KStartupLogo(QWidget * parent, const char *name)
        : QWidget(parent, name,
                  WStyle_Customize |
#if QT_VERSION >= 0x030100
                  WStyle_Splash
#else
                  WStyle_NoBorder | WStyle_StaysOnTop | WStyle_Tool | WX11BypassWM | WWinOwnDC
#endif
                 ),
	  m_readyToHide(false),
	  m_showTip(true)
{
    QString pixmapFile = locate("appdata", "pixmaps/splash.png");
    if (!pixmapFile)
        return ;
    m_pixmap.load(pixmapFile);
    setBackgroundPixmap(m_pixmap);
    setGeometry(QApplication::desktop()->width() / 2 - m_pixmap.width() / 2,
                QApplication::desktop()->height() / 2 - m_pixmap.height() / 2,
                m_pixmap.width(), m_pixmap.height());
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
    if (width > 200)
        width = 200;

    int y = m_pixmap.height() - 12;

    // grep me: splash color
    //    QColor bg(49, 94, 19); // color for 2006 splash
    QColor bg(19, 19, 19);  // color for the 2008 splash
    paint.setPen(bg);
    paint.setBrush(bg);
    paint.drawRect(QRect(m_pixmap.width() - 220, m_pixmap.height() - 43,
                         220, (y + 8) - (m_pixmap.height() - 43)));

    //    paint.setPen(QColor(Qt::black));
    //    paint.setBrush(QColor(Qt::black));
    paint.setPen(QColor(Qt::white));
    paint.setBrush(QColor(Qt::white));

    //QString version(VERSION);
    //int sepIdx = version.find("-");
    QString versionLabel(VERSION);
    //QString("R%1 v%2").arg(version.left(sepIdx)).arg(version.mid(sepIdx + 1));
    int versionWidth = metrics.width(versionLabel);

    paint.drawText(m_pixmap.width() - versionWidth - 18,
                   m_pixmap.height() - 28,
                   versionLabel);

    paint.drawText(m_pixmap.width() - (width + 10), y, m_statusMessage);
}

void KStartupLogo::slotShowStatusMessage(QString message)
{
    m_statusMessage = message;
    paintEvent(0);
    QApplication::flushX();
}

void KStartupLogo::close()
{
    if (!m_wasClosed && isVisible()) {

	if (m_showTip) {
	    RG_DEBUG << "KStartupLogo::close: Showing Tips\n";
	    KTipDialog::showTip(locate("data", "rosegarden/tips"));
	}
    }

    QWidget::close();
}


void KStartupLogo::mousePressEvent(QMouseEvent*)
{
    // for the haters of raising startlogos
    if (m_readyToHide)
        hide(); // don't close, main() sets up a QTimer for that
}

KStartupLogo* KStartupLogo::getInstance()
{
    if (m_wasClosed)
        return 0;

    if (!m_instance)
        m_instance = new KStartupLogo;

    return m_instance;
}

void KStartupLogo::hideIfStillThere()
{
    if (m_instance)
        m_instance->hide();
    // don't close, main() sets up a QTimer for that
}


KStartupLogo* KStartupLogo::m_instance = 0;
bool KStartupLogo::m_wasClosed = false;

#include "KStartupLogo.moc"
