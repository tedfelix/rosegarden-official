
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
        m_selected(false),
        m_needUpdate(false),
        m_brush(DefaultBrushColor),
        m_pen(DefaultPenColor),
        m_repeatMarks(),
        m_baseWidth(0),
        m_label()
    { }

    SegmentRect(const QRect &r) :
        QRect(r),
        m_selected(false),
        m_needUpdate(false),
        m_brush(DefaultBrushColor),
        m_pen(DefaultPenColor),
        m_repeatMarks(),
        m_baseWidth(0),
        m_label()
    { }

    SegmentRect(const QPoint &topLeft, const QSize &size) :
        QRect(topLeft, size),
        m_selected(false),
        m_needUpdate(false),
        m_brush(DefaultBrushColor),
        m_pen(DefaultPenColor),
        m_repeatMarks(),
        m_baseWidth(0),
        m_label()
    { }

    //SegmentRect(const QPoint & topLeft, const QPoint & bottomRight)
    //    : QRect(topLeft, bottomRight), m_resized(false), m_selected(false),
    //      m_needUpdate(false), m_brush(DefaultBrushColor), m_pen(DefaultPenColor), m_z(0) {};
    //SegmentRect(int left, int top, int width, int height)
    //    : QRect(left, top, width, height), m_resized(false), m_selected(false),
    //      m_needUpdate(false), m_brush(DefaultBrushColor), m_pen(DefaultPenColor), m_z(0) {};

    void setSelected(bool selected)  { m_selected = selected; }
    bool isSelected() const  { return m_selected; }

    bool needsFullUpdate() const  { return m_needUpdate; }
    void setNeedsFullUpdate(bool needUpdate)  { m_needUpdate = needUpdate; }
    
    void setBrush(QBrush brush)  { m_brush = brush; }
    QBrush getBrush() const  { return m_brush; }

    void setPen(QPen pen)  { m_pen = pen; }
    QPen getPen() const  { return m_pen; }

    typedef std::vector<int> RepeatMarks;
    void setRepeatMarks(const RepeatMarks &rm)  { m_repeatMarks = rm; }
    const RepeatMarks &getRepeatMarks() const  { return m_repeatMarks; }
    bool isRepeating() const  { return m_repeatMarks.size() > 0; }
    int getBaseWidth() const  { return m_baseWidth; }
    void setBaseWidth(int baseWidth)  { m_baseWidth = baseWidth; }

    QString getLabel() const  { return m_label; }
    void setLabel(QString label)  { m_label = label; }

    static const QColor DefaultPenColor;
    static const QColor DefaultBrushColor;

    SegmentRect intersected(const SegmentRect &other) const
    {
        SegmentRect intersected = QRect::intersected(other);

        // Mix m_brush colors
        const QColor &thisColor = m_brush.color();
        const QColor &otherColor = other.m_brush.color();

        QColor color((thisColor.red()   + otherColor.red())   / 2,
                     (thisColor.green() + otherColor.green()) / 2,
                     (thisColor.blue()  + otherColor.blue())  / 2);

        intersected.setBrush(color);

        // Combine m_selected
        intersected.setSelected((m_selected  ||  other.m_selected));

        return intersected;
    }

//private:
    bool m_selected;
    bool m_needUpdate;
    QBrush m_brush;
    QPen m_pen;
    RepeatMarks m_repeatMarks;
    int m_baseWidth;
    QString m_label;
};


}

#endif
