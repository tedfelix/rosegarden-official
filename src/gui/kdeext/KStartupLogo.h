// -*- c-basic-offset: 4 -*-

/*
    Rosegarden
    A sequencer and musical notation editor.
    Copyright 2000-2008 the Rosegarden development team.
    See the AUTHORS file for more details.

    This file contains code borrowed from KDevelop 2.0
    Copyright (c) The KDevelop Development Team.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#ifndef KSTARTUPLOGO_H
#define KSTARTUPLOGO_H

#include <qwidget.h>
#include <qpixmap.h>

class KStartupLogo : public QWidget
{
    Q_OBJECT

public:
    static KStartupLogo* getInstance();

    static void hideIfStillThere();
    
    void setHideEnabled(bool enabled) { m_readyToHide = enabled; };
    void setShowTip(bool showTip) { m_showTip = showTip; };

public slots:
    void slotShowStatusMessage(QString);
    virtual void close();

protected:

    KStartupLogo(QWidget *parent=0, const char *name=0);
    ~KStartupLogo();
    
    virtual void paintEvent(QPaintEvent*);
    virtual void mousePressEvent( QMouseEvent*);

    bool m_readyToHide;
    bool m_showTip;

    QPixmap m_pixmap;

    static KStartupLogo* m_instance;
    static bool m_wasClosed;
    QString m_statusMessage;
};

#endif





