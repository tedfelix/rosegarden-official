/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2021 the Rosegarden development team.

    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#include "ControlRulerTabBar.h"

#include "misc/Debug.h"
#include "gui/general/IconLoader.h"

#include <QTabBar>
#include <QString>
#include <QPainter>
#include <QMouseEvent>

namespace Rosegarden
{

ControlRulerTabBar::ControlRulerTabBar():QTabBar()
{
    m_closeIcon = QPixmap(IconLoader::loadPixmap("tab-close"));
}

void ControlRulerTabBar::paintEvent(QPaintEvent *event)
{
    QTabBar::paintEvent(event);
    
    QPainter painter(this);

    for (std::vector<QRect*>::iterator it = m_closeButtons.begin(); it != m_closeButtons.end(); ++it) {
        painter.drawPixmap((*it)->topLeft(), m_closeIcon);
    }
}

void ControlRulerTabBar::mousePressEvent(QMouseEvent *event)
{
    if (event->buttons() & Qt::LeftButton) {
        int index = 0;
        for (std::vector<QRect*>::iterator it = m_closeButtons.begin(); it != m_closeButtons.end(); ++it) {
            if ((*it)->contains(event->pos())) {
                emit tabCloseRequest(index);
                return;
            }
            index++;
        }
    }
    
    QTabBar::mousePressEvent(event);
}

void ControlRulerTabBar::tabLayoutChange()
{
    for (std::vector<QRect*>::iterator it = m_closeButtons.begin(); it != m_closeButtons.end(); ++it) {
        delete(*it);
    }
    m_closeButtons.clear();
    
    QRect *newButton, rect;
    for (int index = 0; index < count(); ++index) {
        rect = tabRect(index);
        // ??? MEMORY LEAK (confirmed)
        newButton = new QRect(rect.right()-hMargin-m_closeIcon.width(),
                rect.top()+(rect.height()-m_closeIcon.height())/2,
                m_closeIcon.width(),
                m_closeIcon.height());
        m_closeButtons.push_back(newButton);
    }
}

int ControlRulerTabBar::addTab(const QString &text)
{
    // Append some white space to the tab name to make room
    // for our close icon
    QString str = text;
    str.append("        ");

    int newindex = QTabBar::addTab(str);
    
    return newindex;
}

}

