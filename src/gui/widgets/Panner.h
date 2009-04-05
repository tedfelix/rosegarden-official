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

#ifndef _RG_PANNER_H_
#define _RG_PANNER_H_

#include <QGraphicsView>

namespace Rosegarden
{

class Panner : public QGraphicsView
{
    Q_OBJECT

public:
    Panner();
    virtual ~Panner() { }

signals:
    void pannedRectChanged(QRectF);

public slots:
    void slotSetPannedRect(QRectF);

protected:
    class PannerViewport : public QWidget
    {
    public:
        PannerViewport(Panner *p) : QWidget(p), m_p(p) { }
    protected:
        void paintEvent(QPaintEvent *);
        Panner *m_p;
    };

    friend class PannerViewport;
    QRectF m_pannedRect;

    void moveTo(QPoint);

    virtual void mousePressEvent(QMouseEvent *e);
    virtual void mouseMoveEvent(QMouseEvent *e);
    virtual void mouseReleaseEvent(QMouseEvent *e);
    virtual void mouseDoubleClickEvent(QMouseEvent *e);

    virtual void resizeEvent(QResizeEvent *);
};

}

#endif

