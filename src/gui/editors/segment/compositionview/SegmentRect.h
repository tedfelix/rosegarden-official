
/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2015 the Rosegarden development team.

    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#ifndef RG_SEGMENTRECT_H
#define RG_SEGMENTRECT_H

#include <QBrush>
#include <QColor>
#include <QPen>
#include <QRect>
#include <QString>

#include <vector>


class QSize;
class QPoint;


namespace Rosegarden
{

/// A segment rectangle.
/**
 * ??? This class shouldn't derive from QRect.  QRect doesn't provide a
 *     virtual dtor.  Therefore, if you have a QRect pointer to a SegmentRect,
 *     and you delete through that pointer, the SegmentRect's dtor will not be
 *     called.  This class should have a QRect member and provide functions
 *     that delegate to corresponding QRect functions for those that need
 *     them.
 */
class SegmentRect : public QRect  // ??? Deriving from QRect is dangerous.
{
public:
    SegmentRect() :
        QRect(),
        selected(false),
        brush(DefaultBrushColor),
        pen(DefaultPenColor),
        repeatMarks(),
        baseWidth(0),
        label()
    { }

    SegmentRect(const QRect &r) :
        QRect(r),
        selected(false),
        brush(DefaultBrushColor),
        pen(DefaultPenColor),
        repeatMarks(),
        baseWidth(0),
        label()
    { }

    SegmentRect(const QPoint &topLeft, const QSize &size) :
        QRect(topLeft, size),
        selected(false),
        brush(DefaultBrushColor),
        pen(DefaultPenColor),
        repeatMarks(),
        baseWidth(0),
        label()
    { }

    bool selected;

    QBrush brush;
    static const QColor DefaultBrushColor;

    QPen pen;
    static const QColor DefaultPenColor;

    typedef std::vector<int> RepeatMarks;
    RepeatMarks repeatMarks;
    bool isRepeating() const  { return !repeatMarks.empty(); }

    int baseWidth;

    QString label;

    /// Like QRect::intersected(), but also combines the brush colors.
    SegmentRect intersected(const SegmentRect &other) const;
};


}

#endif
