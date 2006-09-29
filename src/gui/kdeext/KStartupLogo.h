// -*- c-basic-offset: 4 -*-

/*
    Rosegarden-4
    A sequencer and musical notation editor.

    This program is Copyright 2000-2006
        Guillaume Laurent   <glaurent@telegraph-road.org>,
        Chris Cannam        <cannam@all-day-breakfast.com>,
        Richard Bown        <bownie@bownie.com>

    This file contains code borrowed from KDevelop 2.0
    Copyright (c) The KDevelop Development Team.

    The moral right of the authors to claim authorship of this work
    has been asserted.

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

public slots:
    void slotShowStatusMessage(QString);
    virtual void close();

protected:

    KStartupLogo(QWidget *parent=0, const char *name=0);
    ~KStartupLogo();
    
    virtual void paintEvent(QPaintEvent*);
    virtual void mousePressEvent( QMouseEvent*);

    bool m_readyToHide;

    QPixmap m_pixmap;

    static KStartupLogo* m_instance;
    static bool m_wasClosed;
    QString m_statusMessage;
};

#endif





