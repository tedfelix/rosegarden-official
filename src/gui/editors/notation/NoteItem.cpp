/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2024 the Rosegarden development team.

    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#include "NoteItem.h"

#include "NotePixmapFactory.h"
#include "misc/Debug.h"
#include "base/Profiler.h"

#include <QPainter>

namespace Rosegarden
{

NoteItemDimensions::NoteItemDimensions() :
    noteBodyWidth(0),
    noteBodyHeight(0),
    left(0),
    right(0),
    above(0),
    below(),
    borderX(0),
    borderY(0)
{}

NoteItem::NoteItem(const NotePixmapParameters &params,
                   QSharedPointer<NoteStyle> style,
                   bool selected,
                   bool shaded,
		   NotePixmapFactory *factory,
		   QGraphicsItem *parent) :
    QGraphicsItem(parent),
    m_parameters(params),
    m_style(style),
    m_selected(selected),
    m_shaded(shaded),
    m_factory(factory),
    m_haveDimensions(false)
{
}

NoteItem::~NoteItem()
{
}

QRectF
NoteItem::boundingRect() const
{
    if (!m_haveDimensions) {
	getDimensions();
    }

    return QRectF(m_offset, m_size);
}

void
NoteItem::paint(QPainter *painter,
		const QStyleOptionGraphicsItem */* option */,
		QWidget */* widget */)
{
    if (!m_haveDimensions) {
	getDimensions();
    }

    Profiler profiler("NoteItem::paint");

    QTransform t = painter->worldTransform();

//    NOTATION_DEBUG << "note: transform " << t << " differs from last transform " << m_lastTransform << ", or is not a small transform";

    bool tiny = (t.m11() < 0.15 || t.m22() < 0.15);

    // bug 1484 - the following 5 lines are now commented out

    //if (!tiny) {
    //    QRectF rect = boundingRect();
    //    QRectF target = t.mapRect(rect);
    //    tiny = (target.width() < 10 && target.height() < 10);
    //}

    DrawMode mode;
    if (tiny) {
        mode = DrawTiny;
    } else if (t.m11() < 1.0) {
        mode = DrawSmall;
    } else if (t.m11() > 1.0) {
        mode = DrawLarge;
    } else {
        mode = DrawNormal;
    }

    painter->save();
    if (mode == DrawLarge) {
        painter->setRenderHint(QPainter::Antialiasing, true);
    } else {
        painter->setRenderHint(QPainter::Antialiasing, false);
    }
    m_factory->setNoteStyle(m_style);
    m_factory->setSelected(m_selected);
    m_factory->setShaded(m_shaded);
    m_factory->drawNoteForItem(m_parameters, m_dimensions, mode, painter);
    painter->restore();
}

void
NoteItem::getDimensions() const
{
    Profiler profiler("NoteItem::getDimensions");

    m_factory->getNoteDimensions(m_parameters, m_dimensions);
    m_offset = QPoint(-m_dimensions.left,
                      -m_dimensions.above - m_dimensions.noteBodyHeight / 2);
    m_size = QSize(m_dimensions.noteBodyWidth + m_dimensions.left + m_dimensions.right,
                   m_dimensions.noteBodyHeight + m_dimensions.above + m_dimensions.below);
    m_haveDimensions = true;
}

QPointF
NoteItem::offset() const
{
    if (!m_haveDimensions) {
	getDimensions();
    }

    return m_offset;
}

QPixmap
NoteItem::makePixmap() const
{
    m_factory->setNoteStyle(m_style);
    m_factory->setSelected(m_selected);
    m_factory->setShaded(m_shaded);

    QGraphicsPixmapItem *pi = m_factory->makeNotePixmapItem(m_parameters);
    QPixmap pixmap = pi->pixmap();
    delete pi;

    return pixmap;
}


}
