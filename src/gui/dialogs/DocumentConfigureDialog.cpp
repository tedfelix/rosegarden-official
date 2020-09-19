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

#define RG_MODULE_STRING "[DocumentConfigureDialog]"

#include "DocumentConfigureDialog.h"

#include "ConfigureDialogBase.h"
#include "document/RosegardenDocument.h"
#include "gui/configuration/AudioPropertiesPage.h"
#include "gui/configuration/ColourConfigurationPage.h"
#include "gui/configuration/DocumentMetaConfigurationPage.h"
#include "gui/configuration/GeneralConfigurationPage.h"
#include "gui/general/IconLoader.h"
#include "misc/Debug.h"

#include <QLayout>
#include <QString>
#include <QWidget>


namespace Rosegarden
{

DocumentConfigureDialog::DocumentConfigureDialog(QWidget *parent,
                                                 const char *name) :
    ConfigureDialogBase(parent, tr("Document Properties"), name)
{
    // Document Meta Page
    //
    DocumentMetaConfigurationPage *documentMetaConfigurationPage =
            new DocumentMetaConfigurationPage(this);
    connect(documentMetaConfigurationPage,
                &DocumentMetaConfigurationPage::modified,
            this, &DocumentConfigureDialog::slotActivateApply);
    addPage(DocumentMetaConfigurationPage::iconLabel(),
            DocumentMetaConfigurationPage::title(),
            IconLoader::loadPixmap( DocumentMetaConfigurationPage::iconName()),
            documentMetaConfigurationPage);
    m_configurationPages.push_back(
            static_cast<TabbedConfigurationPage *>(documentMetaConfigurationPage));

    // Audio Page
    //
    AudioPropertiesPage *audioPropertiesPage = new AudioPropertiesPage(this);
    connect(audioPropertiesPage, &AudioPropertiesPage::modified,
            this, &DocumentConfigureDialog::slotActivateApply);
    addPage(AudioPropertiesPage::iconLabel(),
            AudioPropertiesPage::title(),
            IconLoader::loadPixmap(AudioPropertiesPage::iconName()),
            audioPropertiesPage);
    m_configurationPages.push_back(
            static_cast<TabbedConfigurationPage *>(audioPropertiesPage));
}

void
DocumentConfigureDialog::showAudioPage()
{
    int index = 0;

    for (ConfigurationPages::iterator i = m_configurationPages.begin();
            i != m_configurationPages.end(); ++i) {

        AudioPropertiesPage *page =
            dynamic_cast<AudioPropertiesPage *>(*i);

        if (!page) {
            ++index;
            continue;
        }

        setPageByIndex(index);
        return ;
    }
}


}
