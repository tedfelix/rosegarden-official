
/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.

    This program is Copyright 2000-2008
        Guillaume Laurent   <glaurent@telegraph-road.org>,
        Chris Cannam        <cannam@all-day-breakfast.com>,
        Richard Bown        <richard.bown@ferventsoftware.com>

    The moral rights of Guillaume Laurent, Chris Cannam, and Richard
    Bown to claim authorship of this work have been asserted.

    This file is based on code from KGhostView, Copyright 1997-2002
        Markkhu Hihnala     <mah@ee.oulu.fi>
        and the KGhostView authors.

    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#ifndef _RG_SCROLLBOX_H_
#define _RG_SCROLLBOX_H_

#include <qframe.h>
#include <qpixmap.h>
#include <qpoint.h>
#include <qsize.h>


class QWidget;
class QPainter;
class QMouseEvent;


namespace Rosegarden
{

class ScrollBox: public QFrame
{
    Q_OBJECT

public:
    enum SizeMode { FixWidth, FixHeight };

    ScrollBox(QWidget *parent = 0,
              SizeMode mode = FixWidth,
              const char *name = 0);

public slots:
    void setPageSize(const QSize&);
    void setViewSize(const QSize&);
    void setViewPos(const QPoint&);
    void setViewPos(int x, int y) { setViewPos(QPoint(x, y)); }
    void setViewX(int x);
    void setViewY(int y);
    void setThumbnail(QPixmap img);

signals:
    void valueChanged(const QPoint&);
    void valueChangedRelative(int dx, int dy);
    void button2Pressed();
    void button3Pressed();

protected:
    void mousePressEvent(QMouseEvent *);
    void mouseMoveEvent(QMouseEvent *);
    void drawContents(QPainter *);

private:
    QPoint   m_viewpos;
    QPoint   m_mouse;
    QSize    m_pagesize;
    QSize    m_viewsize;
    SizeMode m_sizeMode;
};


}

#endif
