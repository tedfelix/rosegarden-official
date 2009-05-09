/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2009 the Rosegarden development team.
 
    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.
 
    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#ifndef _RG_PANNED_H_
#define _RG_PANNED_H_

#include <QGraphicsView>

class QWheelEvent;

namespace Rosegarden
{

class Panned : public QGraphicsView
{
    Q_OBJECT

public:
    Panned();
    virtual ~Panned() { }

signals:
    void pannedRectChanged(QRectF);
    void wheelEventReceived(QWheelEvent *);
    void pannedContentsScrolled(int, int);

public slots:
    void slotSetPannedRect(QRectF);
    void slotEmulateWheelEvent(QWheelEvent *ev);

protected:
    QRectF m_pannedRect;

    virtual void resizeEvent(QResizeEvent *);
    virtual void drawForeground(QPainter *, const QRectF &);
    virtual void wheelEvent(QWheelEvent *);
    virtual void scrollContentsBy(int dx, int dy);
};

}

#endif

