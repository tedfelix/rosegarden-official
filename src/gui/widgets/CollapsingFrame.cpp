/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2008 the Rosegarden development team.
 
    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.
 
    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/


#include "CollapsingFrame.h"
#include <QApplication>
#include <kstandarddirs.h>
#include <QSettings>
#include <kglobal.h>
#include <QFont>
#include <QFrame>
#include <QLayout>
#include <QPixmap>
#include <QString>
#include <QToolButton>
#include <QWidget>
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

    m_layout->addWidget(m_toggleButton, 0, 0, 0- 0+1, 2- 1);
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
        m_layout->addWidget(widget, 1, 0, 0+1, 2- 1);
    } else {
        m_layout->addWidget(widget, 1, 1);
    }

    bool expanded = true;
    if (name(0)) {
        QSettings config ; // was: confq4
        QString groupTemp = config->group();
        QSettings config;
        config.beginGroup( "CollapsingFrame" );
        // 
        // FIX-manually-(GW), add:
        // config.endGroup();		// corresponding to: config.beginGroup( "CollapsingFrame" );
        //  

        expanded = qStrToBool( config.value(name(, "" ) ) , true);
        QSettings config;
        config.beginGroup( groupTemp );
        // 
        // FIX-manually-(GW), add:
        // config.endGroup();		// corresponding to: config.beginGroup( groupTemp );
        //  

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
        QSettings config ; // was: confq4
        QString groupTemp = config->group();
        QSettings config;
        config.beginGroup( "CollapsingFrame" );
        // 
        // FIX-manually-(GW), add:
        // config.endGroup();		// corresponding to: config.beginGroup( "CollapsingFrame" );
        //  

        config.setValue(name(), !m_collapsed);
        QSettings config;
        config.beginGroup( groupTemp );
        // 
        // FIX-manually-(GW), add:
        // config.endGroup();		// corresponding to: config.beginGroup( groupTemp );
        //  

    }

    m_toggleButton->setIconSet(pixmap);

    m_toggleButton->setMaximumHeight(h);
}

}
#include "CollapsingFrame.moc"
