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


#include "TabbedConfigurationPage.h"

#include "ConfigurationPage.h"
#include "document/RosegardenGUIDoc.h"
#include <kconfig.h>
#include <kdialog.h>
#include <QString>
#include <QTabWidget>
#include <QWidget>
#include <QLayout>


namespace Rosegarden
{

TabbedConfigurationPage::TabbedConfigurationPage(RosegardenGUIDoc *doc,
                                                 QWidget *parent,
                                                 const char *name)
    : ConfigurationPage(doc, parent, name)
{
    init();
}

TabbedConfigurationPage::TabbedConfigurationPage(QSettings cfg,
                                                 QWidget *parent,
                                                 const char *name)
    : ConfigurationPage(cfg, parent, name)
{
    init();
}

TabbedConfigurationPage::TabbedConfigurationPage(RosegardenGUIDoc *doc,
                                                 QSettings cfg,
                                                 QWidget *parent,
                                                 const char *name)
    : ConfigurationPage(doc, cfg, parent, name)
{
    init();
}

void TabbedConfigurationPage::init()
{
    QVBoxLayout *vlay = new QVBoxLayout(this, 0, KDialog::spacingHint());
    m_tabWidget = new QTabWidget(this);
    vlay->addWidget(m_tabWidget);
}

void TabbedConfigurationPage::addTab(QWidget *tab, const QString &title)
{
    m_tabWidget->addTab(tab, title);
}

}
#include "TabbedConfigurationPage.moc"
