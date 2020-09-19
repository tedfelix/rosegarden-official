
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

#ifndef RG_COLOURCONFIGURATIONPAGE_H
#define RG_COLOURCONFIGURATIONPAGE_H

#include "base/ColourMap.h"
#include "gui/widgets/ColourTable.h"
#include "TabbedConfigurationPage.h"
#include <QString>


class QWidget;


namespace Rosegarden
{

class RosegardenDocument;


/**
 * Colour Configuration Page
 *
 * (document-wide settings)
 */
class ColourConfigurationPage : public TabbedConfigurationPage
{
    Q_OBJECT

public:
    ColourConfigurationPage(QWidget *parent);
    void apply() override;

    void populate_table();

    static QString iconLabel() { return tr("Color"); }
    static QString title()     { return tr("Color Settings"); }
    static QString iconName()  { return "colorize"; }

signals:
    void docColoursChanged();

protected slots:
    void slotAddNew();
    void slotDelete();
    void slotTextChanged(unsigned int, QString);
    void slotColourChanged(unsigned int, QColor);

protected:
    ColourTable *m_colourtable;

    ColourMap m_map;
    ColourTable::ColourList m_listmap;

};

// -----------  SequencerConfigurationage -----------------
//


}

#endif
