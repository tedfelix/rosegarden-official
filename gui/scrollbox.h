// -*- c-basic-offset: 4 -*-

/*
    Rosegarden-4
    A sequencer and musical notation editor.

    This program is Copyright 2000-2003
        Guillaume Laurent   <glaurent@telegraph-road.org>,
        Chris Cannam        <cannam@all-day-breakfast.com>,
        Richard Bown        <bownie@bownie.com>

    This file is based on code from KGhostView, Copyright 1997-2002
        Markkhu Hihnala     <mah@ee.oulu.fi>
        and the KGhostView authors.

    The moral right of the authors to claim authorship of this work
    has been asserted.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#ifndef __SCROLLBOX_H__
#define __SCROLLBOX_H__

#include <qframe.h>
#include <qimage.h>
#include <kdialog.h>

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

class ScrollBoxDialog : public KDialog
{
    Q_OBJECT

public:
    ScrollBoxDialog(QWidget *parent = 0,
		    ScrollBox::SizeMode mode = ScrollBox::FixWidth,
		    const char *name = 0,
		    WFlags flags = 0);
    ~ScrollBoxDialog();

    ScrollBox *scrollbox() { return m_scrollbox; }
    void setPageSize(const QSize&);
    
protected:
    virtual void closeEvent(QCloseEvent * e);

signals:
    void closed();

private:
    ScrollBox *m_scrollbox;
};


#endif

