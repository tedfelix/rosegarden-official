/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
 
    This program is Copyright 2000-2006
        Guillaume Laurent   <glaurent@telegraph-road.org>,
        Chris Cannam        <cannam@all-day-breakfast.com>,
        Richard Bown        <richard.bown@ferventsoftware.com>
 
    The moral rights of Guillaume Laurent, Chris Cannam, and Richard
    Bown to claim authorship of this work have been asserted.
 
    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.
 
    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/


#include "ConfigureDialog.h"

#include <klocale.h>
#include "ConfigureDialogBase.h"
#include "document/RosegardenGUIDoc.h"
#include "gui/configuration/GeneralConfigurationPage.h"
#include "gui/configuration/NotationConfigurationPage.h"
#include "gui/configuration/SequencerConfigurationPage.h"
#include <kconfig.h>
#include <kdialogbase.h>
#include <qstring.h>
#include <qwidget.h>


namespace Rosegarden
{

ConfigureDialog::ConfigureDialog(RosegardenGUIDoc *doc,
                                 KConfig* cfg,
                                 QWidget *parent,
                                 const char *name)
        : ConfigureDialogBase(parent, i18n("Configure Rosegarden"), name)
{
    QWidget *pageWidget = 0;
    QVBoxLayout *vlay = 0;
    ConfigurationPage* page = 0;

    // General Page
    //
    pageWidget = addPage(GeneralConfigurationPage::iconLabel(),
                         GeneralConfigurationPage::title(),
                         loadIcon(GeneralConfigurationPage::iconName()));
    vlay = new QVBoxLayout(pageWidget, 0, spacingHint());
    page = new GeneralConfigurationPage(doc, cfg, pageWidget);
    vlay->addWidget(page);
    page->setPageIndex(pageIndex(pageWidget));
    m_configurationPages.push_back(page);

    connect(page, SIGNAL(updateAutoSaveInterval(unsigned int)),
            this, SIGNAL(updateAutoSaveInterval(unsigned int)));
    connect(page, SIGNAL(updateSidebarStyle(unsigned int)),
            this, SIGNAL(updateSidebarStyle(unsigned int)));

    // Sequencer Page
    //
    pageWidget = addPage(SequencerConfigurationPage::iconLabel(),
                         SequencerConfigurationPage::title(),
                         loadIcon(SequencerConfigurationPage::iconName()));
    vlay = new QVBoxLayout(pageWidget, 0, spacingHint());
    page = new SequencerConfigurationPage(doc, cfg, pageWidget);
    vlay->addWidget(page);
    page->setPageIndex(pageIndex(pageWidget));
    m_configurationPages.push_back(page);

    // Notation Page
    pageWidget = addPage(NotationConfigurationPage::iconLabel(),
                         NotationConfigurationPage::title(),
                         loadIcon(NotationConfigurationPage::iconName()));
    vlay = new QVBoxLayout(pageWidget, 0, spacingHint());
    page = new NotationConfigurationPage(cfg, pageWidget);
    vlay->addWidget(page);
    page->setPageIndex(pageIndex(pageWidget));
    m_configurationPages.push_back(page);
    /*
        // Matrix Page
        pageWidget = addPage(MatrixConfigurationPage::iconLabel(),
                             MatrixConfigurationPage::title(),
                             loadIcon(MatrixConfigurationPage::iconName()));
        vlay = new QVBoxLayout(pageWidget, 0, spacingHint());
        page = new MatrixConfigurationPage(cfg, pageWidget);
        vlay->addWidget(page);
        page->setPageIndex(pageIndex(pageWidget));
        m_configurationPages.push_back(page);
     
        // Latency Page
        //
        pageWidget = addPage(LatencyConfigurationPage::iconLabel(),
                             LatencyConfigurationPage::title(),
                             loadIcon(LatencyConfigurationPage::iconName()));
        vlay = new QVBoxLayout(pageWidget, 0, spacingHint());
        page = new LatencyConfigurationPage(doc, cfg, pageWidget);
        vlay->addWidget(page);
        page->setPageIndex(pageIndex(pageWidget));
        m_configurationPages.push_back(page);
    */
}

}
