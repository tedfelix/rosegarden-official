/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2013 the Rosegarden development team.
 
    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.
 
    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#include "CheckButton.h"
#include "gui/general/IconLoader.h"
#include "misc/Debug.h"
#include "misc/ConfigGroups.h"

#include <QPushButton>
#include <QSettings>

namespace Rosegarden
{


CheckButton::CheckButton(QString iconName, bool wantsMemory,  QWidget *parent) :
    QPushButton(parent),
    m_iconName(iconName),
    m_wantsMemory(wantsMemory)
{
    setFixedSize(QSize(32,32));
    setIconSize(QSize(32,32));
    setCheckable(true);
    setIcon(IconLoader().load(m_iconName));

    if (m_wantsMemory) { 
        RG_DEBUG << "CheckButton: recall for: " << iconName << endl;
        // Memory recall
        QSettings settings;
        settings.beginGroup(CheckButtonConfigGroup);
        setChecked(settings.value(m_iconName, true).toBool());
        settings.endGroup();
    }

    connect(this, SIGNAL(toggled(bool)), this, SLOT(toggle(bool)));

    // thinking out loud...
    // since I'm subclassing anyway, I might add the ability to make a master
    // toggle button that keeps track of children and switches them all on or
    // off at once, else I'll have 8+ switch this when that switches slots with
    // lots of things to keep track of manually
    //
    // QButtonGroup looks useful
}

CheckButton::~CheckButton()
{
}

void
CheckButton::toggle(bool state)
{
    if (m_wantsMemory) {
        // Memory storage
        RG_DEBUG << "CheckButton, storing value of " << m_iconName << " as: " << state << endl;
        QSettings settings;
        settings.beginGroup(CheckButtonConfigGroup);
        settings.setValue(m_iconName, state);
        settings.endGroup();
    }
}


}

#include "CheckButton.moc"
