// -*- c-basic-offset: 4 -*-

/*
    Rosegarden-4 v0.1
    A sequencer and musical notation editor.

    This program is Copyright 2000-2002
        Guillaume Laurent   <glaurent@telegraph-road.org>,
        Chris Cannam        <cannam@all-day-breakfast.com>,
        Richard Bown        <bownie@bownie.com>

    The moral right of the authors to claim authorship of this work
    has been asserted.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

// This file contains code borrowed from KDevelop 2.0
// (c) The KDevelop Development Team

#ifndef KSTARTUPLOGO_H
#define KSTARTUPLOGO_H

#include <qwidget.h>
#include <qpixmap.h>

/**
  *@author ??
  */

class KStartupLogo : public QWidget
{
    Q_OBJECT
public:

    KStartupLogo(QWidget *parent=0, const char *name=0);
    ~KStartupLogo();

    void setHideEnabled(bool bEnabled) { m_bReadyToHide = bEnabled; };

protected:


    virtual void paintEvent(QPaintEvent*);
    virtual void mousePressEvent( QMouseEvent*);

    bool m_bReadyToHide;

    QPixmap m_pixmap;
};

#endif





