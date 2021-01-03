/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2021 the Rosegarden development team.

    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#ifndef RG_COLOURTABLEITEM_H
#define RG_COLOURTABLEITEM_H

#include <QColor>
#include <QTableWidget>
#include <QTableWidgetItem>

namespace Rosegarden
{


class ColourTableItem : public QTableWidgetItem
{
public:
    ColourTableItem(QTableWidget *t, const QColor &input);

    void setColor(QColor &input);

protected:
    QColor currentColour;
    QTableWidgetItem *m_ti;
};


}

#endif
