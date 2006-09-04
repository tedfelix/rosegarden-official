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

#include "collapsingframe.h"

#include <qlayout.h>
#include <qlabel.h>
#include <qpushbutton.h>
#include <qtoolbutton.h>
#include <qobjectlist.h>
#include <qiconset.h>
#include <qpixmap.h>

#include <kglobal.h>
#include <kstddirs.h>
#include <kconfig.h>
#include <kapplication.h>

#include <rosedebug.h>

#include <cassert>

CollapsingFrame::CollapsingFrame(QString label, QWidget *parent, const char *n) :
    QFrame(parent, n),
    m_widget(0),
    m_collapsed(false)
{
    m_layout = new QGridLayout(this, 3, 3, 0, 0);

    m_toggleButton = new QToolButton(this);
    m_toggleButton->setTextLabel(label);
    m_toggleButton->setUsesTextLabel(true);
    m_toggleButton->setUsesBigPixmap(false);
    m_toggleButton->setTextPosition(QToolButton::BesideIcon);
    m_toggleButton->setAutoRaise(true);

    QFont font(m_toggleButton->font());
    font.setBold(true);
    m_toggleButton->setFont(font);

    QString pixmapDir = KGlobal::dirs()->findResource("appdata", "pixmaps/");
    QPixmap pixmap(pixmapDir + "/misc/arrow-expanded.png");
    m_toggleButton->setIconSet(pixmap);

    connect(m_toggleButton, SIGNAL(clicked()), this, SLOT(toggle()));

    m_layout->addMultiCellWidget(m_toggleButton, 0, 0, 0, 2);
}

CollapsingFrame::~CollapsingFrame()
{
}

void
CollapsingFrame::setWidget(QWidget *widget)
{
    assert(!m_widget);
    m_widget = widget;
    m_layout->addWidget(widget, 1, 1);

    bool expanded = true;
    if (name(0)) {
	KConfig *config = kapp->config();
	QString groupTemp = config->group();
	config->setGroup("CollapsingFrame");
	expanded = config->readBoolEntry(name(), true);
	config->setGroup(groupTemp);
    }
    if (expanded != !m_collapsed) toggle();
}

void
CollapsingFrame::toggle()
{
    m_collapsed = !m_collapsed;

    m_widget->setShown(!m_collapsed);

    QString pixmapDir = KGlobal::dirs()->findResource("appdata", "pixmaps/");
    QPixmap pixmap;

    if (m_collapsed) {
	pixmap = QPixmap(pixmapDir + "/misc/arrow-contracted.png");
    } else {
	pixmap = QPixmap(pixmapDir + "/misc/arrow-expanded.png");
    }

    if (name(0)) {
	KConfig *config = kapp->config();
	QString groupTemp = config->group();
	config->setGroup("CollapsingFrame");
	config->writeEntry(name(), !m_collapsed);
	config->setGroup(groupTemp);
    }

    m_toggleButton->setIconSet(pixmap);
}

#include "collapsingframe.moc"

