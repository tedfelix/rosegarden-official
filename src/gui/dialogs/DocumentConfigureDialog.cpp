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


#include "DocumentConfigureDialog.h"

#include <klocale.h>
#include "ConfigureDialogBase.h"
#include "document/RosegardenGUIDoc.h"
#include "gui/configuration/AudioConfigurationPage.h"
#include "gui/configuration/ColourConfigurationPage.h"
#include "gui/configuration/DocumentMetaConfigurationPage.h"
#include <kdialogbase.h>
#include <qstring.h>
#include <qwidget.h>


namespace Rosegarden
{

DocumentConfigureDialog::DocumentConfigureDialog(RosegardenGUIDoc *doc,
        QWidget *parent,
        const char *name)
        : ConfigureDialogBase(parent, i18n("Document Properties"), name)
{
    QWidget *pageWidget = 0;
    QVBoxLayout *vlay = 0;
    ConfigurationPage* page = 0;

    // Document Meta Page
    //
    pageWidget = addPage(DocumentMetaConfigurationPage::iconLabel(),
                         DocumentMetaConfigurationPage::title(),
                         loadIcon(DocumentMetaConfigurationPage::iconName()));
    vlay = new QVBoxLayout(pageWidget, 0, spacingHint());
    page = new DocumentMetaConfigurationPage(doc, pageWidget);
    vlay->addWidget(page);
    page->setPageIndex(pageIndex(pageWidget));
    m_configurationPages.push_back(page);

    // Audio Page
    //
    pageWidget = addPage(AudioConfigurationPage::iconLabel(),
                         AudioConfigurationPage::title(),
                         loadIcon(AudioConfigurationPage::iconName()));
    vlay = new QVBoxLayout(pageWidget, 0, spacingHint());
    page = new AudioConfigurationPage(doc, pageWidget);
    vlay->addWidget(page);
    page->setPageIndex(pageIndex(pageWidget));
    m_configurationPages.push_back(page);

    // Colour Page
    pageWidget = addPage(ColourConfigurationPage::iconLabel(),
                         ColourConfigurationPage::title(),
                         loadIcon(ColourConfigurationPage::iconName()));

    vlay = new QVBoxLayout(pageWidget, 0, spacingHint());
    page = new ColourConfigurationPage(doc, pageWidget);
    vlay->addWidget(page);
    page->setPageIndex(pageIndex(pageWidget));
    m_configurationPages.push_back(page);
}

void
DocumentConfigureDialog::showAudioPage()
{
    int index = 0;

    for (configurationpages::iterator i = m_configurationPages.begin();
            i != m_configurationPages.end(); ++i) {

        AudioConfigurationPage *page =
            dynamic_cast<AudioConfigurationPage *>(*i);

        if (!page) {
            ++index;
            continue;
        }

        showPage(index);
        return ;
    }
}

void
DocumentConfigureDialog::selectMetadata(QString name)
{
    int index = 0;

    for (configurationpages::iterator i = m_configurationPages.begin();
            i != m_configurationPages.end(); ++i) {

        DocumentMetaConfigurationPage *page =
            dynamic_cast<DocumentMetaConfigurationPage *>(*i);

        if (!page) {
            ++index;
            continue;
        }

        page->selectMetadata(name);
        showPage(index);
        return ;
    }
}

}
