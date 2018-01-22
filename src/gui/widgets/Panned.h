/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2018 the Rosegarden development team.
 
    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.
 
    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#ifndef RG_PANNED_H
#define RG_PANNED_H

#include <QGraphicsView>

class QWheelEvent;
class QEvent;

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
    void pannedContentsScrolled();
    void mouseLeaves();

public slots:
    void slotSetPannedRect(QRectF);
    void slotEmulateWheelEvent(QWheelEvent *ev);

    void slotShowPositionPointer(float x); // scene coord; full height
    void slotShowPositionPointer(QPointF top, float height); // scene coords
    void slotHidePositionPointer();
    void slotEnsurePositionPointerInView(bool page); // if visible

protected:
    QRectF m_pannedRect;
    QPointF m_pointerTop;
    float m_pointerHeight;
    bool m_pointerVisible;

    virtual void paintEvent(QPaintEvent *);
    virtual void resizeEvent(QResizeEvent *);
    virtual void drawForeground(QPainter *, const QRectF &);
    virtual void wheelEvent(QWheelEvent *);
    virtual void leaveEvent(QEvent *);
};

}

#endif

