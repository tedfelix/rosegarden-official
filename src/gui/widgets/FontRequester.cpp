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

#include "FontRequester.h"
#include "misc/ConfigGroups.h"
#include "gui/general/ThornStyle.h"

#include <QLabel>
#include <QPushButton>
#include <QGridLayout>
#include <QFontDialog>

namespace Rosegarden
{
 
FontRequester::FontRequester(QWidget *parent) :
    QWidget(parent)
{
    QGridLayout *layout = new QGridLayout(this);

    m_label = new QLabel();
    m_label->setAutoFillBackground(true);
    m_label->setFrameStyle(QFrame::Panel | QFrame::Sunken);
    m_label->setLineWidth(2);
    layout->addWidget(m_label, 0, 0);

    if (ThornStyle::isEnabled()) {
        QPalette pal = m_label->palette();
        pal.setColor(QPalette::Window, Qt::white);
        pal.setColor(QPalette::WindowText, Qt::black);
        m_label->setPalette(pal);
    }

    QPushButton *button = new QPushButton(tr("Choose..."));
    layout->addWidget(button, 0, 1);

    layout->setColumnStretch(0, 20);

    connect(button, &QAbstractButton::clicked, this, &FontRequester::slotChoose);
}

FontRequester::~FontRequester()
{
}

void
FontRequester::setFont(QFont font)
{
    m_label->setFont(font);
    font = m_label->font();
    m_label->setText(tr("%1 %2").arg(font.family()).arg(font.pointSize()));
}

QFont
FontRequester::getFont() const
{
    return m_label->font();
}

void
FontRequester::slotChoose()
{
    bool ok = false;
    //$$$ We're going to have to subclass QFontDialog to fix the "Sample" style,
    // but I'm not going to bother with that at this time.
    QFont newFont = QFontDialog::getFont(&ok, getFont());
    if (ok) {
        setFont(newFont);
        emit fontChanged(getFont());
    }
}

}


