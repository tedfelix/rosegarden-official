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

#include "TabbedConfigurationPage.h"

#include "document/RosegardenDocument.h"
#include "gui/application/RosegardenMainWindow.h"

#include <QString>
#include <QTabWidget>
#include <QVBoxLayout>


namespace Rosegarden
{


TabbedConfigurationPage::TabbedConfigurationPage(QWidget *parent) :
    QWidget(parent)
{
    m_doc = RosegardenDocument::currentDocument;

    QVBoxLayout *layout = new QVBoxLayout(this);
    m_tabWidget = new QTabWidget(this);
    layout->addWidget(m_tabWidget);
}

void TabbedConfigurationPage::addTab(QWidget *tab, const QString &title)
{
    m_tabWidget->addTab(tab, title);
}


}
