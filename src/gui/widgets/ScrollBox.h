
/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2021 the Rosegarden development team.

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

#ifndef RG_SCROLLBOX_H
#define RG_SCROLLBOX_H

#include <QFrame>
#include <QPixmap>
#include <QPoint>
#include <QSize>


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

    ScrollBox(QWidget *parent = nullptr,
              SizeMode mode = FixWidth,
              const char *name = nullptr);

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
    void mousePressEvent(QMouseEvent *) override;
    void mouseMoveEvent(QMouseEvent *) override;
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
