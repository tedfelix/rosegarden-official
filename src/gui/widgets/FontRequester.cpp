/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2016 the Rosegarden development team.

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

#include <QLabel>
#include <QPushButton>
#include <QGridLayout>
#include <QFontDialog>
#include <QSettings>

namespace Rosegarden
{
 
FontRequester::FontRequester(QWidget *parent) :
    QWidget(parent)
{
    QSettings settings;
    settings.beginGroup(GeneralOptionsConfigGroup);
    bool Thorn = settings.value("use_thorn_style", true).toBool();
    settings.endGroup();

    // These buttons wanted white backgrounds.  I'm not even bothering to try to
    // fix problems like this via the stylesheet anymore.  It would probably
    // just have unintended side effects and lead to tail chasing anyway.
    QString localStyle("QPushButton::enabled { border: 1px solid #AAAAAA;  border-radius: 3px; background-color: qlineargradient(spread:pad, x1:0, y1:1, x2:0, y2:0, stop:0 #999999, stop:1 #DDDDDD);  color: #000000; padding: 0 5px 0 5px; } QPushButton::!enabled { border: 1px solid #808080; border-radius: 3px; background-color: qlineargradient(spread:pad, x1:0, y1:1, x2:0, y2:0, stop:0 #999999, stop:1 #DDDDDD); color: #000000; padding: 0 5px 0 5px; } QPushButton:hover { border: 1px solid #AAAAAA; border-radius: 3px; background-color: #CCDFFF; color: #000000; } QPushButton::checked, QPushButton::pressed { border: 1px solid #E0E0E0; border-radius: 3px; background-color: qlineargradient(spread:pad, x1:0, y1:1, x2:0, y2:0, stop:0 #E0E0E0, stop:1 #EEEEEE); }");

    QGridLayout *layout = new QGridLayout();
    setLayout(layout);
    
    m_label = new QLabel();
    m_label->setFrameStyle(QFrame::Panel | QFrame::Sunken);
    m_label->setLineWidth(2);
    layout->addWidget(m_label, 0, 0);

    QPushButton *button = new QPushButton(tr("Choose..."));
    if (Thorn) button->setStyleSheet(localStyle);
    layout->addWidget(button, 0, 1);

    layout->setColumnStretch(0, 20);

    connect(button, SIGNAL(clicked()), this, SLOT(slotChoose()));
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


