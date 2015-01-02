/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2015 the Rosegarden development team.

    Portions of this file Copyright 2003
        Mark Hymers         <markh@linuxfromscratch.org>

    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#ifndef RG_COLOURTABLE_H
#define RG_COLOURTABLE_H

#include <map>
#include <QTableWidget>
#include <vector>


class QWidget;
class ColourList;


namespace Rosegarden
{

class ColourMap;


class ColourTable : public QTableWidget
{
    Q_OBJECT

public:
    typedef std::map<unsigned int, unsigned int, std::less<unsigned int> > ColourList;
    ColourTable(QWidget *parent, ColourMap &input, ColourList &list);
    void populate_table(ColourMap &input, ColourList &list);
    

signals:
    void entryTextChanged(unsigned int, QString);
    void entryColourChanged(unsigned int, QColor);

public slots:
    void slotEditEntry (int, int);

protected:
    std::vector<QColor> m_colours;

};


}

#endif
