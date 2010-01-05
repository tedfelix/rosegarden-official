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
*/


#include "DocumentConfigureDialog.h"
#include "ConfigureDialogBase.h"
#include "document/RosegardenDocument.h"
#include "gui/configuration/AudioPropertiesPage.h"
#include "gui/configuration/ColourConfigurationPage.h"
#include "gui/configuration/DocumentMetaConfigurationPage.h"
#include "gui/configuration/GeneralConfigurationPage.h"
#include "gui/general/IconLoader.h"

#include <QLayout>
#include <QString>
#include <QWidget>


namespace Rosegarden
{

DocumentConfigureDialog::DocumentConfigureDialog(RosegardenDocument *doc,
        QWidget *parent,
        const char *name)
    : ConfigureDialogBase(parent, tr("Document Properties"), name )//, QMessageBox::StandardButtons buttons )
{
//    QWidget *pageWidget = 0;
//    QVBoxLayout *vlay = 0;
//    ConfigurationPage* page = 0;
    QWidget *page = 0;
    // Document Meta Page
    //
    IconLoader il;
//    pageWidget = addPage(DocumentMetaConfigurationPage::iconLabel(),
//                         DocumentMetaConfigurationPage::title(),
//                         il.load( DocumentMetaConfigurationPage::iconName()) );
//    vlay = new QVBoxLayout(pageWidget); //, 0, spacingHint());
    page = new DocumentMetaConfigurationPage(doc, this);
    addPage(DocumentMetaConfigurationPage::iconLabel(),DocumentMetaConfigurationPage::title(),il.loadPixmap( DocumentMetaConfigurationPage::iconName()),page);
//    vlay->addWidget(page);
//    page->setPageIndex(pageIndex(pageWidget));
//    m_tabWidget->setCurrentIndex( m_tabWidget->indexOf(pageWidget) );
    m_configurationPages.push_back((ConfigurationPage *)page);

    // Audio Page
    //
//    pageWidget = addPage(AudioPropertiesPage::iconLabel(),
//                         AudioPropertiesPage::title(),
//                         il.load(AudioPropertiesPage::iconName()));
//    vlay = new QVBoxLayout(pageWidget); //, 0, spacingHint());
    page = new AudioPropertiesPage(doc, this);
    addPage(AudioPropertiesPage::iconLabel(),AudioPropertiesPage::title(),il.loadPixmap(AudioPropertiesPage::iconName()),page);
//    vlay->addWidget(page);
    //page->setPageIndex(pageIndex(pageWidget));
//    m_tabWidget->setCurrentIndex( m_tabWidget->indexOf(pageWidget) );
    m_configurationPages.push_back((ConfigurationPage *)page);

//&&&
//
// I've been looking into the color configuration bits, and beyond mere style
// issues, these are quite thoroughly busted.  I don't want to take the approach
// of just tossing something aside merely because it is challenging to fix, but
// in this case I think we may well be justified in removing the color table
// editor.  It was rarely, if ever used, and it is very likely that anybody who
// actually cares about segment colors already has all the colors they could
// ever want pre-defined since I added the gigantic (400+) list of named colors
// to the default studio some time back.  This isn't a drawing application, and
// people won't likely care if they can't have the exact shade of red they want.
/*
    // Colour Page
//    pageWidget = addPage(ColourConfigurationPage::iconLabel(),
//                         ColourConfigurationPage::title(),
//                         il.load(ColourConfigurationPage::iconName()));

//    vlay = new QVBoxLayout(pageWidget); //, 0, spacingHint());
    page = new ColourConfigurationPage(doc, this);
    addPage(ColourConfigurationPage::iconLabel(),ColourConfigurationPage::title(),il.loadPixmap(ColourConfigurationPage::iconName()),page);
//    vlay->addWidget(page);
    //page->setPageIndex(pageIndex(pageWidget));
//    m_tabWidget->setCurrentIndex( m_tabWidget->indexOf(pageWidget) );
    m_configurationPages.push_back((ConfigurationPage *)page); */

    // resize(minimumSizeHint());
}

void
DocumentConfigureDialog::showAudioPage()
{
    int index = 0;

    for (configurationpages::iterator i = m_configurationPages.begin();
            i != m_configurationPages.end(); ++i) {

        AudioPropertiesPage *page =
            dynamic_cast<AudioPropertiesPage *>(*i);

        if (!page) {
            ++index;
            continue;
        }

        //showPage(index);
//        m_tabWidget->setCurrentIndex( index );
        return ;
    }
}

/* hjj: WHAT TO DO WITH THIS ?
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
*/

}
