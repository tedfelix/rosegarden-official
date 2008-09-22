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


#include "DocumentConfigureDialog.h"
#include "ConfigureDialogBase.h"
#include "document/RosegardenGUIDoc.h"
#include "gui/configuration/AudioPropertiesPage.h"
#include "gui/configuration/ColourConfigurationPage.h"
#include "gui/configuration/DocumentMetaConfigurationPage.h"
#include "gui/configuration/GeneralConfigurationPage.h"
#include "gui/general/IconLoader.h"

#include <QLayout>
#include <QString>
#include <QWidget>

//#include <kdialogbase.h>
#include <klocale.h>
//#include <kstandarddirs.h>


namespace Rosegarden
{
/*
// may not be required, use gui/general/IconLoader.h instead	
static QPixmap loadIcon(const char *name)
{
    QString pixmapDir = KGlobal::dirs()->findResource("appdata", "pixmaps/");
    QString fileBase = pixmapDir + "/misc/";
    fileBase += name;
    if (QFile(fileBase + ".png").exists()) {
        return QPixmap(fileBase + ".png");
    } else if (QFile(fileBase + ".xpm").exists()) {
        return QPixmap(fileBase + ".xpm");
    }

    QPixmap pmap = KGlobal::instance()->iconLoader()
        ->loadIcon(QString::fromLatin1(name), KIcon::NoGroup, KIcon::SizeMedium);
//	IconLoader il;
//	il.load( QString::fromLatin1(name) );
    return pmap;
}
*/

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
	IconLoader il;
    pageWidget = addPage(DocumentMetaConfigurationPage::iconLabel(),
                         DocumentMetaConfigurationPage::title(),
                         il.load( DocumentMetaConfigurationPage::iconName()) );
    vlay = new QVBoxLayout(pageWidget, 0, spacingHint());
    page = new DocumentMetaConfigurationPage(doc, pageWidget);
    vlay->addWidget(page);
    page->setPageIndex(pageIndex(pageWidget));
    m_configurationPages.push_back(page);

    // Audio Page
    //
    pageWidget = addPage(AudioPropertiesPage::iconLabel(),
                         AudioPropertiesPage::title(),
                         il.load(AudioPropertiesPage::iconName()));
    vlay = new QVBoxLayout(pageWidget, 0, spacingHint());
    page = new AudioPropertiesPage(doc, pageWidget);
    vlay->addWidget(page);
    page->setPageIndex(pageIndex(pageWidget));
    m_configurationPages.push_back(page);

    // Colour Page
    pageWidget = addPage(ColourConfigurationPage::iconLabel(),
                         ColourConfigurationPage::title(),
                         il.load(ColourConfigurationPage::iconName()));

    vlay = new QVBoxLayout(pageWidget, 0, spacingHint());
    page = new ColourConfigurationPage(doc, pageWidget);
    vlay->addWidget(page);
    page->setPageIndex(pageIndex(pageWidget));
    m_configurationPages.push_back(page);

    resize(minimumSize());
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

        showPage(index);
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
