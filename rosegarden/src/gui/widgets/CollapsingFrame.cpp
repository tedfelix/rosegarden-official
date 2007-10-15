/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
 
    This program is Copyright 2000-2007
        Guillaume Laurent   <glaurent@telegraph-road.org>,
        Chris Cannam        <cannam@all-day-breakfast.com>,
        Richard Bown        <richard.bown@ferventsoftware.com>
 
    The moral rights of Guillaume Laurent, Chris Cannam, and Richard
    Bown to claim authorship of this work have been asserted.
 
    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.
 
    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/


#include "CollapsingFrame.h"
#include <kapplication.h>
#include <kstddirs.h>
#include <kconfig.h>
#include <kglobal.h>
#include <qfont.h>
#include <qframe.h>
#include <qlayout.h>
#include <qpixmap.h>
#include <qstring.h>
#include <qtoolbutton.h>
#include <qwidget.h>
#include <cassert>


namespace Rosegarden
{

CollapsingFrame::CollapsingFrame(QString label, QWidget *parent, const char *n) :
        QFrame(parent, n),
        m_widget(0),
        m_fill(false),
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
{}

void
CollapsingFrame::setWidgetFill(bool fill)
{
    m_fill = fill;
}

QFont
CollapsingFrame::font() const
{
    return m_toggleButton->font();
}

void
CollapsingFrame::setFont(QFont font)
{
    m_toggleButton->setFont(font);
}

void
CollapsingFrame::setWidget(QWidget *widget)
{
    assert(!m_widget);
    m_widget = widget;
    if (m_fill) {
        m_layout->addMultiCellWidget(widget, 1, 1, 0, 2);
    } else {
        m_layout->addWidget(widget, 1, 1);
    }

    bool expanded = true;
    if (name(0)) {
        KConfig *config = kapp->config();
        QString groupTemp = config->group();
        config->setGroup("CollapsingFrame");
        expanded = config->readBoolEntry(name(), true);
        config->setGroup(groupTemp);
    }
    if (expanded != !m_collapsed)
        toggle();
}

void
CollapsingFrame::toggle()
{
    int h = m_toggleButton->height();

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

    m_toggleButton->setMaximumHeight(h);
}

}
#include "CollapsingFrame.moc"
