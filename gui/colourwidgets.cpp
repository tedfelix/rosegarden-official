
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

#include <qtable.h>
#include <qinputdialog.h>

#include <kcolordialog.h>
#include <klocale.h>

#include "colours.h"
#include "colourwidgets.h"
#include "rosestrings.h"

/*
 * RosegardenColourTable
 * To be documented.
 */
RosegardenColourTable::RosegardenColourTable
 (QWidget *parent, Rosegarden::ColourMap &input, ColourList &list)
 : QTable(1, 2, parent, "RColourTable")
{
    setSorting(FALSE);
    setSelectionMode(QTable::SingleRow);
    horizontalHeader()->setLabel(0, i18n("Name"));
    horizontalHeader()->setLabel(1, i18n("Colour"));
    populate_table(input, list);
    connect(this, SIGNAL(doubleClicked(int, int, int, const QPoint&)),
            SLOT(slotEditEntry(int, int)));

}

void
RosegardenColourTable::slotEditEntry(int row, int col)
{

    switch(col)
    {
        case 0:
        {
            if (row == 0) return;
            bool ok = false;
            QString newName = QInputDialog::getText(i18n("Modify Colour Name"), i18n("Enter new name"), 
                                                    QLineEdit::Normal, item(row, col)->text(), &ok);

            if ((ok == true) && (!newName.isEmpty()))
            {
                emit entryTextChanged(row, newName);
                return;
            }
        }
            break;
        case 1:
        {
            QColor temp = m_colours[row];
            KColorDialog box(this, "", true);

            int result = box.getColor(temp);

            if (result == KColorDialog::Accepted)
            {
                emit entryColourChanged(row, temp);
                return;
            }
        }
            break;
        default: // Should never happen
            break;
    }

}

void
RosegardenColourTable::populate_table(Rosegarden::ColourMap &input, ColourList &list)
{
    m_colours.reserve(input.size());
    setNumRows(input.size());

    QString name;

    unsigned int i=0;

    for (Rosegarden::RCMap::const_iterator it=input.begin(); it!=input.end(); ++it)
    {
        if (it->second.second == std::string(""))
            name = i18n("Default Colour");
        else
            name = strtoqstr(it->second.second);

        QTableItem *text = new QTableItem(
                dynamic_cast<QTable*>(this),
                QTableItem::Never, name);

        setItem(i, 0, text);

        list[i] = it->first;
        m_colours[i] = RosegardenGUIColours::convertColour(it->second.first);

        RosegardenColourTableItem *temp = new RosegardenColourTableItem(this, m_colours[i]);
        setItem(i, 1, temp);

        verticalHeader()->setLabel(i, QString::number(it->first));

        ++i;
    }

}

/*
 * RosegardenColourTableItem
 * To be documented.
 */
void
RosegardenColourTableItem::setColor(QColor &input)
{
    currentColour = input;
}

void
RosegardenColourTableItem::paint(QPainter *p, const QColorGroup &cg, const QRect &cr, bool selected)
{
    QColorGroup g(cg);
    g.setColor(QColorGroup::Base, currentColour);
    selected = false;
    QTableItem::paint(p, g, cr, selected);
}
