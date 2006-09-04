// -*- c-basic-offset: 4 -*-

/*
    Rosegarden-4
    A sequencer and musical notation editor.

    This program is Copyright 2000-2006
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

#ifndef _COLLAPSING_FRAME_H_
#define _COLLAPSING_FRAME_H_

#include <qframe.h>

class QToolButton;
class QGridLayout;

class CollapsingFrame : public QFrame
{
    Q_OBJECT

public:
    CollapsingFrame(QString label, QWidget *parent = 0, const char *name = 0);
    virtual ~CollapsingFrame();

    /// This frame contains a single other widget.  Set it here.
    void setWidget(QWidget *w);

public slots:
    void toggle();

protected:
    QGridLayout *m_layout;
    QToolButton *m_toggleButton;
    QWidget *m_widget;
    bool m_collapsed;
};


#endif

