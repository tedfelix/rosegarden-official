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

#ifndef RG_NOTEITEM_H
#define RG_NOTEITEM_H

#include <QGraphicsItem>

#include "NotePixmapParameters.h"

namespace Rosegarden
{

struct NoteItemDimensions
{
    int noteBodyWidth, noteBodyHeight;
    int left, right, above, below;
    int borderX, borderY;
    QPoint stemStart, stemEnd;
};

class NotePixmapFactory;
class NoteStyle;

class NoteItem : public QGraphicsItem
{
public:
    NoteItem(const NotePixmapParameters &params,
             NoteStyle *style,
             bool selected,
             bool shaded,
	     NotePixmapFactory *factory,
	     QGraphicsItem *parent = nullptr);
    ~NoteItem() override;

    QRectF boundingRect() const override;
    QPointF offset() const;
    QPixmap makePixmap() const;

    void paint(QPainter *painter,
               const QStyleOptionGraphicsItem *option,
               QWidget *widget) override;

    enum DrawMode {
        DrawNormal,
        DrawLarge,
        DrawSmall,
        DrawTiny
    };

protected:
    NotePixmapParameters m_parameters;
    NoteStyle *m_style;
    bool m_selected;
    bool m_shaded;
    NotePixmapFactory *m_factory;
    mutable NoteItemDimensions m_dimensions;
    mutable bool m_haveDimensions;
    mutable QPoint m_offset;
    mutable QSize m_size;
    
    void getDimensions() const;
};

}

#endif
