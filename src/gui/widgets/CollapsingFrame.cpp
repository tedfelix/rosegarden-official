/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2018 the Rosegarden development team.
 
    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.
 
    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/


#include "CollapsingFrame.h"
#include "misc/Strings.h"
#include "misc/ConfigGroups.h"
#include "gui/general/IconLoader.h"

#include <QApplication>
#include <QDir>
#include <QSettings>
#include <QFont>
#include <QFrame>
#include <QLayout>
#include <QPixmap>
#include <QString>
#include <QToolButton>
#include <QWidget>
#include <QtGlobal>

namespace Rosegarden
{

CollapsingFrame::CollapsingFrame(
        QString label, QWidget *parent, const QString &name,
        bool defaultExpanded) :
    QFrame(parent),
    m_widget(0),
    m_fill(false),
    m_collapsed(false)
{
    setObjectName(name);

    // Set up the initial default state if needed.
    QSettings settings;
    settings.beginGroup(CollapsingFrameConfigGroup);
    bool expanded = qStrToBool(settings.value(
            name, defaultExpanded ? "true" : "false"));
    settings.setValue(name, expanded);

    setContentsMargins(0, 0, 0, 0);
    m_layout = new QGridLayout(this);
    m_layout->setMargin(0);
    m_layout->setSpacing(0);

    m_toggleButton = new QToolButton(this);
    m_toggleButton->setText(label);
    m_toggleButton->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    m_toggleButton->setAutoRaise(true);

    QFont font(m_toggleButton->font());
    font.setBold(true);
    m_toggleButton->setFont(font);

    m_toggleButton->setIcon(IconLoader().load("style/arrow-down-small-inverted"));

    connect(m_toggleButton, SIGNAL(clicked()), this, SLOT(toggle()));

    m_layout->addWidget(m_toggleButton, 0, 0, 1, 3);
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
    Q_ASSERT(!m_widget);
    m_widget = widget;
    if (m_fill) {
        m_layout->addWidget(widget, 1, 0, 1, 3);
    } else {
        m_layout->addWidget(widget, 1, 1);
    }

    bool expanded = true;
    if (!objectName().isEmpty()) {
        QSettings settings;
        settings.beginGroup(CollapsingFrameConfigGroup);
        expanded = qStrToBool(settings.value(objectName(), true));
        settings.endGroup();
    }

    if (expanded != !m_collapsed)
        toggle();
}

void
CollapsingFrame::toggle()
{
    int h = m_toggleButton->height();

    m_collapsed = !m_collapsed;

    m_widget->setVisible(!m_collapsed);

    QPixmap pixmap;

    if (m_collapsed) {
        pixmap = IconLoader().loadPixmap("style/arrow-right-small-inverted");
    } else {
        pixmap = IconLoader().loadPixmap("style/arrow-down-small-inverted");
    }

    if (!objectName().isEmpty()) {
        QSettings settings;
        settings.beginGroup(CollapsingFrameConfigGroup);
        settings.setValue(objectName(), !m_collapsed);
        settings.endGroup();
    }

    m_toggleButton->setIcon(pixmap);

    m_toggleButton->setMaximumHeight(h);
}

}
