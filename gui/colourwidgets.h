
// -*- c-basic-offset: 4 -*-

/*
    Rosegarden-4
    A sequencer and musical notation editor.

    This program is Copyright 2000-2003
        Guillaume Laurent   <glaurent@telegraph-road.org>,
        Chris Cannam        <cannam@all-day-breakfast.com>,
        Richard Bown        <bownie@bownie.com>

    Portions of this file Copyright 2003
        Mark Hymers         <markh@linuxfromscratch.org>

    The moral right of the authors to claim authorship of this work
    has been asserted.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#ifndef _COLOURWIDGETS_H_
#define _COLOURWIDGETS_H_

#include <vector>

#include <qtable.h>
#include <qsignalmapper.h>
#include <qcolor.h>

#include "ColourMap.h"


class RosegardenColourTable : public QTable
{
    Q_OBJECT

public:
    typedef std::map<unsigned int, unsigned int, std::less<unsigned int> > ColourList;
    RosegardenColourTable(QWidget *parent, Rosegarden::ColourMap &input, ColourList &list);
    void populate_table(Rosegarden::ColourMap &input, ColourList &list);
    

signals:
    void entryTextChanged(unsigned int, QString);
    void entryColourChanged(unsigned int, QColor);

public slots:
    void slotEditEntry (int, int);

protected:
    std::vector<QColor> m_colours;

};

class RosegardenColourTableItem : public QTableItem
{
public:
    RosegardenColourTableItem(QTable *t, const QColor &input)
     : QTableItem(t, QTableItem::Never, ""),
       currentColour(input) {}
    void setColor(QColor &input);
    void paint(QPainter *p, const QColorGroup &cg, const QRect &cr, bool selected);

protected:
    QColor currentColour;
};


#endif // _COLOURWIDGETS_H_
