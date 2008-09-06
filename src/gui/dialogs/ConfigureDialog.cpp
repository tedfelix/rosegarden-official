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


#include "ConfigureDialog.h"
#include <QLayout>

#include <klocale.h>
#include "ConfigureDialogBase.h"
#include "document/RosegardenGUIDoc.h"
#include "gui/configuration/GeneralConfigurationPage.h"
#include "gui/configuration/NotationConfigurationPage.h"
#include "gui/configuration/AudioConfigurationPage.h"
#include "gui/configuration/MIDIConfigurationPage.h"
#include <kconfig.h>
#include <kdialogbase.h>
#include <kstandarddirs.h>
#include <QString>
#include <QWidget>


namespace Rosegarden
{

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
    return pmap;
}


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

    pageWidget = addPage(MIDIConfigurationPage::iconLabel(),
                         MIDIConfigurationPage::title(),
                         loadIcon(MIDIConfigurationPage::iconName()));
    vlay = new QVBoxLayout(pageWidget, 0, spacingHint());
    page = new MIDIConfigurationPage(doc, cfg, pageWidget);
    vlay->addWidget(page);
    page->setPageIndex(pageIndex(pageWidget));
    m_configurationPages.push_back(page);

    pageWidget = addPage(AudioConfigurationPage::iconLabel(),
                         AudioConfigurationPage::title(),
                         loadIcon(AudioConfigurationPage::iconName()));
    vlay = new QVBoxLayout(pageWidget, 0, spacingHint());
    page = new AudioConfigurationPage(doc, cfg, pageWidget);
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
}

}
#include "ConfigureDialog.moc"
