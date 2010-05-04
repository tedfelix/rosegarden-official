/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2010 the Rosegarden development team.

    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.

    Idea obtained from an article by David Boddie in Qt Quarterly.
*/

#ifndef SHORTCUTSDIALOG_H
#define SHORTCUTSDIALOG_H

#include <qdialog.h>
#include <qlist.h>
//Added by qt3to4:
#include <Q3ValueList>

class QAction;
class Q3Table;
class Q3TableItem;
class QWidget;

namespace Rosegarden
{

class ShortcutsDialog : public QDialog
{
    Q_OBJECT

public:
    ShortcutsDialog(QObjectList *shortcutsList, QWidget *parent = 0);

protected slots:
    void accept();

private slots:
    void recordShortcut(int row, int column);
    void validateShortcut(int row, int column);

private:
    QString m_oldAccelText;
    Q3Table *m_shortcutsTable;
    Q3ValueList<QAction*> m_shortcutsList;
};

}
#endif

