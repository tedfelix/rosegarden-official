// -*- c-basic-offset: 4 -*-

/*
    Rosegarden-4
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

#ifndef QDEFERSCROLLVIEW_H
#define QDEFERSCROLLVIEW_H

#include <qscrollview.h>

/**
 * A QScrollView which defers vertical scrolling (through mouse wheel)
 * elsewhere, typically another QScrollView, so that both can be kept
 * in sync. The master scrollview will connect its vertical scrollbar
 * to the slave view so the scrollbar will act on both views.
 *
 * The slave scrollview will defer its scrolling to the master by
 * having the gotWheelEvent() signal connected to a slot in the master
 * scrollview, which will simply process the wheel event as if it had
 * received it itself.
 *
 * @see TrackEditor
 * @see SegmentCanvas
 * @see TrackEditor::m_trackButtonScroll
 */
class QDeferScrollView : public QScrollView
{
    Q_OBJECT
public:
    QDeferScrollView(QWidget* parent=0, const char *name=0, WFlags f=0);

signals:
    void gotWheelEvent(QWheelEvent*);

protected:
    virtual void contentsWheelEvent(QWheelEvent*);
    
};

#endif
