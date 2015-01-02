
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

#ifndef RG_COMPOSITIONRECT_H
#define RG_COMPOSITIONRECT_H

#include <QBrush>
#include <QColor>
#include <QPen>
#include <QRect>
#include <QString>
#include <QVector>


class QSize;
class QPoint;


namespace Rosegarden
{

class CompositionRect : public QRect
{
public:
    typedef QVector<int> repeatmarks;

    friend bool operator<(const CompositionRect&, const CompositionRect&);

    CompositionRect() : QRect(), m_selected(false),
                        m_needUpdate(false), m_brush(DefaultBrushColor), m_pen(DefaultPenColor) {};
    CompositionRect(const QRect& r) : QRect(r), m_resized(false), m_selected(false),
                                      m_needUpdate(false), m_brush(DefaultBrushColor), m_pen(DefaultPenColor), m_z(0) {};
    CompositionRect(const QPoint & topLeft, const QPoint & bottomRight)
        : QRect(topLeft, bottomRight), m_resized(false), m_selected(false),
          m_needUpdate(false), m_brush(DefaultBrushColor), m_pen(DefaultPenColor), m_z(0) {};
    CompositionRect(const QPoint & topLeft, const QSize & size)
        : QRect(topLeft, size), m_resized(false), m_selected(false),
          m_needUpdate(false), m_brush(DefaultBrushColor), m_pen(DefaultPenColor), m_z(0) {};
    CompositionRect(int left, int top, int width, int height)
        : QRect(left, top, width, height), m_resized(false), m_selected(false),
          m_needUpdate(false), m_brush(DefaultBrushColor), m_pen(DefaultPenColor), m_z(0) {};

    void setResized(bool s)       { m_resized = s; }
    bool isResized() const        { return m_resized; }
    void setSelected(bool s)      { m_selected = s; }
    bool isSelected() const       { return m_selected; }
    bool needsFullUpdate() const  { return m_needUpdate; }
    void setNeedsFullUpdate(bool s) { m_needUpdate = s; }

    void setZ(int z) { m_z = z; }
    int z() const { return m_z; }
    
    // brush, pen draw info
    void setBrush(QBrush b)       { m_brush = b; }
    QBrush getBrush() const       { return m_brush; }
    void setPen(QPen b)           { m_pen = b; }
    QPen getPen() const           { return m_pen; }

    // repeating segments
    void                setRepeatMarks(const repeatmarks& rm) { m_repeatMarks = rm; }
    const repeatmarks&  getRepeatMarks() const                { return m_repeatMarks; }
    bool                isRepeating() const                   { return m_repeatMarks.size() > 0; }
    int                 getBaseWidth() const                  { return m_baseWidth; }
    void                setBaseWidth(int bw)                  { m_baseWidth = bw; }
    QString             getLabel() const                      { return m_label; }
    void                setLabel(QString l)                   { m_label = l; }

    static const QColor DefaultPenColor;
    static const QColor DefaultBrushColor;

protected:
    bool        m_resized;
    bool        m_selected;
    bool        m_needUpdate;
    QBrush      m_brush;
    QPen        m_pen;
    repeatmarks m_repeatMarks;
    int         m_baseWidth;
    QString     m_label;
    int         m_z;
};


}

#endif
