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

#include <rosedebug.h>

#include <cassert>

CollapsingFrame::CollapsingFrame(QString label, QWidget *parent, const char *name) :
    QFrame(parent, name),
    m_widget(0),
    m_collapsed(false)
{
    m_layout = new QGridLayout(this, 3, 3, 0, 0);

    m_toggleButton = new QToolButton(this);
    m_toggleButton->setTextLabel(label);
    m_toggleButton->setUsesTextLabel(true);
    m_toggleButton->setUsesBigPixmap(false);
//    m_toggleButton->setToggleButton(true);
    m_toggleButton->setTextPosition(QToolButton::BesideIcon);
    m_toggleButton->setAutoRaise(true);
    m_toggleButton->setOn(true);
    
    QFont font(m_toggleButton->font());
    font.setBold(true);
    m_toggleButton->setFont(font);

    QString pixmapDir = KGlobal::dirs()->findResource("appdata", "pixmaps/");
    QPixmap pixmap(pixmapDir + "/misc/arrow-expanded.png");
    m_toggleButton->setIconSet(pixmap);
//    m_toggleButton->setPixmap(pixmap);

    connect(m_toggleButton, SIGNAL(clicked()), this, SLOT(toggle()));

    m_layout->addMultiCellWidget(m_toggleButton, 0, 0, 0, 2);
//    m_toggleButton->setFlat(true);
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
}

void
CollapsingFrame::toggle()
{
    if (m_collapsed) m_widget->show();
    else m_widget->hide();
    m_collapsed = !m_collapsed;

    QString pixmapDir = KGlobal::dirs()->findResource("appdata", "pixmaps/");
    QPixmap pixmap;

    if (m_collapsed) {
	pixmap = QPixmap(pixmapDir + "/misc/arrow-contracted.png");
    } else {
	pixmap = QPixmap(pixmapDir + "/misc/arrow-expanded.png");
    }

    m_toggleButton->setIconSet(pixmap);
}

#include "collapsingframe.moc"

