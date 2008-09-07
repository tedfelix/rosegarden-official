/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2008 the Rosegarden development team.
 
    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.
 
    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/


#include "ColourTable.h"

#include <klocale.h>
#include "misc/Strings.h"
#include "base/ColourMap.h"
#include "ColourTableItem.h"
#include "gui/general/GUIPalette.h"
#include <QColorDialog>
#include <QInputDialog>
#include <QColor>
#include <QPoint>
#include <QString>
#include <Q3Table>
#include <QWidget>
#include <QLineEdit>


namespace Rosegarden
{

ColourTable::ColourTable
(QWidget *parent, ColourMap &input, ColourList &list)
        : Q3Table(1, 2, parent, "RColourTable")
{
    setSorting(FALSE);
    setSelectionMode(Q3Table::SingleRow);
    horizontalHeader()->setLabel(0, i18n("Name"));
    horizontalHeader()->setLabel(1, i18n("Color"));
    populate_table(input, list);
    connect(this, SIGNAL(doubleClicked(int, int, int, const QPoint&)),
            SLOT(slotEditEntry(int, int)));

}

void
ColourTable::slotEditEntry(int row, int col)
{

    switch (col) {
    case 0: {
            if (row == 0)
                return ;
            bool ok = false;
            //@@@ QInputDialog replaces KLineEditDlg.  This is not a clean swap,
            // and this is untested
            QString newName = QInputDialog::getText(i18n("Modify Color Name"), i18n("Enter new name"),
                                                    QLineEdit::Normal, item(row, col)->text(), &ok);

            if ((ok == true) && (!newName.isEmpty())) {
                emit entryTextChanged(row, newName);
                return ;
            }
        }
        break;
    case 1: {
            QColor temp = m_colours[row];

            //@@@ KColorDialog to QColorDialog is a really weird conversion.
            // This syntax seems to do the equivalent of the old job.  It
            // returns a QColor.  Test that result for isValid() to see if the
            // dialog was accepted (else it was cancelled.)  This builds now,
            // and I'm pretty sure this is the right thing, but I'm flagging it
            // as a future source of mysterious bugs.
            QColor result = QColorDialog::getColor(temp);

            if (result.isValid()) {
                emit entryColourChanged(row, temp);
                return ;
            }
        }
        break;
    default:  // Should never happen
        break;
    }

}

void
ColourTable::populate_table(ColourMap &input, ColourList &list)
{
    m_colours.reserve(input.size());
    setNumRows(input.size());

    QString name;

    unsigned int i = 0;

    for (RCMap::const_iterator it = input.begin(); it != input.end(); ++it) {
        if (it->second.second == std::string(""))
            name = i18n("Default Color");
        else
            name = strtoqstr(it->second.second);

        Q3TableItem *text = new Q3TableItem(
                               dynamic_cast<Q3Table*>(this),
                               Q3TableItem::Never, name);

        setItem(i, 0, text);

        list[i] = it->first;
        m_colours[i] = GUIPalette::convertColour(it->second.first);

        ColourTableItem *temp = new ColourTableItem(this, m_colours[i]);
        setItem(i, 1, temp);

        verticalHeader()->setLabel(i, QString::number(it->first));

        ++i;
    }

}

}
#include "ColourTable.moc"
