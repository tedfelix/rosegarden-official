/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2015 the Rosegarden development team.
 
    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.
 
    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/


#ifndef RG_ICONSTACKEDWIDGET_H
#define RG_ICONSTACKEDWIDGET_H

#include "IconButton.h"

#include <QWidget>
#include <QHBoxLayout>
#include <QStackedWidget>
#include <QVBoxLayout>

#include <vector>

class IconStackedWidget : public QWidget
{
    Q_OBJECT
public:
    IconStackedWidget(QWidget *parent = 0);
    void addPage(const QString& iconLabel, QWidget *page, const QPixmap& icon);
    typedef std::vector<IconButton *> iconbuttons;
    void setPageByIndex(int index);
    
public slots:
    void slotPageSelect();
    
protected:
    iconbuttons m_iconButtons;
    int m_buttonHeight;
    int m_buttonWidth;
    QFrame * m_iconPanel;
    
private:
    QVBoxLayout * m_iconLayout;
    QStackedWidget * m_pagePanel;
    QHBoxLayout * m_layout;
    QColor m_backgroundColor;
};

#endif
