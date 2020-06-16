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
    setIcon(IconLoader::load(m_iconName));

    if (m_wantsMemory) { 
        RG_DEBUG << "CheckButton: recall for: " << iconName;
        // memory recall
        QSettings settings;
        settings.beginGroup(CheckButtonConfigGroup);
        setChecked(settings.value(m_iconName, true).toBool());
        settings.endGroup();
    } else {
        // buttons without memory default to checked
        setChecked(true);

        // and do not display themselves any differently when they're checked,
        // since it's just the colored bit being in a strange state that
        // disconcerts the eye, and the functionality is actually pretty
        // reasonable (compared to the one row of buttons I actually hooked up
        // to some convoluted state monitoring gobbledegook, which I
        // subsequently dumped, because it was massively complicated without
        // really improving the user experience any)
        QString localStyle("border: 1px solid #AAAAAA; border-radius: 3px; background-color: qlineargradient(spread:pad, x1:0, y1:1, x2:0, y2:0, stop:0 #999999, stop:1 #DDDDDD); color: #000000; padding: 0 5px 0 5px;");
        setStyleSheet(localStyle);
    }

    connect(this, SIGNAL(toggled(bool)), this, SLOT(toggle(bool)));

    // thinking out loud...
    // since I'm subclassing anyway, I might add the ability to make a master
    // toggle button that keeps track of children and switches them all on or
    // off at once, else I'll have 8+ switch this when that switches slots with
    // lots of things to keep track of manually  <-- did that...  it sucked
}

CheckButton::~CheckButton()
{
}

void
CheckButton::toggle(bool state)
{
    if (m_wantsMemory) {
        // Memory storage
        RG_DEBUG << "CheckButton, storing value of " << m_iconName << " as: " << state;
        QSettings settings;
        settings.beginGroup(CheckButtonConfigGroup);
        settings.setValue(m_iconName, state);
        settings.endGroup();
    }
}


}

