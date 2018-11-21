/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A sequencer and musical notation editor.
    Copyright 2000-2018 the Rosegarden development team.
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

#include <QWidget>
#include <QPixmap>

#include <rosegardenprivate_export.h>

namespace Rosegarden
{

class ROSEGARDENPRIVATE_EXPORT StartupLogo : public QWidget
{
    Q_OBJECT

public:
    static StartupLogo* getInstance();

    static void hideIfStillThere();
    
    void setHideEnabled(bool enabled) { m_readyToHide = enabled; };
    void setShowTip(bool showTip) { m_showTip = showTip; };

public slots:
    void slotShowStatusMessage(QString);
    virtual void close();

protected:

    StartupLogo(QWidget *parent=nullptr);
    ~StartupLogo();
    
    void paintEvent(QPaintEvent*) override;
    void mousePressEvent( QMouseEvent*) override;

    bool m_readyToHide;
    bool m_showTip;

    QPixmap m_pixmap;

    static StartupLogo* m_instance;
    static bool m_wasClosed;
    QString m_statusMessage;
};

}

#endif
