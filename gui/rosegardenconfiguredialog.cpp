// -*- c-basic-offset: 4 -*-

/*
    Rosegarden-4
    A sequencer and musical notation editor.

    This program is Copyright 2000-2005
        Guillaume Laurent   <glaurent@telegraph-road.org>,
        Chris Cannam        <cannam@all-day-breakfast.com>,
        Richard Bown        <bownie@bownie.com>

    The moral right of the authors to claim authorship of this work
    has been asserted.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

/*
 * Code borrowed from KDE KMail : configuredialog*.h
 * Copyright (C) 2000 The KMail Development Team
 */

#include "Composition.h"
#include "Configuration.h"
#include "RealTime.h"
#include "MidiDevice.h"

#include "SoundDriver.h"

#include <algorithm>

#include <qspinbox.h>
#include <qslider.h>
#include <qcombobox.h>
#include <qcheckbox.h>
#include <qlayout.h>
#include <qtabwidget.h>
#include <qlabel.h>
#include <kfiledialog.h>
#include <qpushbutton.h>
#include <qradiobutton.h>
#include <qlineedit.h>
#include <qtooltip.h>
#include <qvbox.h>
#include <qstringlist.h>
#include <qtable.h>
#include <qheader.h>
#include <klineeditdlg.h>

#include <kdeversion.h>
#include <kcombobox.h>
#include <klistview.h>
#include <klocale.h>
#include <kiconloader.h>
#include <kmessagebox.h>
#include <kprocess.h>
#include <kcolordialog.h>
#include <kdiskfreesp.h>
#if KDE_VERSION >= KDE_MAKE_VERSION(3,2,0)
#include <kfontrequester.h>
#else
#include "kde32_kfontrequester.h"
#endif
#include "constants.h"
#include "colours.h"
#include "rosestrings.h"
#include "rosegardenconfiguredialog.h"
#include "rosegardenconfigurationpage.h"
#include "notationhlayout.h"
#include "notationstrings.h"
#include "notationview.h"
#include "matrixview.h"
#include "notestyle.h"
#include "notefont.h"
#include "rosegardenguidoc.h"
#include "rosedebug.h"
#include "notefontviewer.h"
#include "sequencemanager.h"
#include "notefont.h"
#include "matrixtool.h"
#include "notationtool.h"
#include "segmenttool.h"
#include "editcommands.h"
#include "studiocontrol.h"
#include "widgets.h"
#include "colourwidgets.h"
#include "audiopluginmanager.h"
#include "segmentcommands.h"
#include "rgapplication.h"

namespace Rosegarden
{



//------------------------------------------------------------

static inline QPixmap loadIcon(const char *name)
{
  return KGlobal::instance()->iconLoader()
    ->loadIcon(QString::fromLatin1(name), KIcon::NoGroup, KIcon::SizeMedium);
}

ConfigureDialogBase::ConfigureDialogBase(QWidget *parent,
					 QString label,
                                         const char *name):
    KDialogBase(IconList, label ? label : i18n("Configure"), Help|Apply|Ok|Cancel,
                Ok, parent, name, true) // modal
{
    setWFlags(WDestructiveClose);
}

ConfigureDialogBase::~ConfigureDialogBase()
{
}

void
ConfigureDialogBase::slotApply()
{
    for(configurationpages::iterator i = m_configurationPages.begin();
        i != m_configurationPages.end(); ++i)
        (*i)->apply();
}


void
ConfigureDialogBase::slotActivateApply()
{
//     ApplyButton->setDisabled(false);
}

void
ConfigureDialogBase::slotOk()
{
    slotApply();
    accept();
}

void
ConfigureDialogBase::slotCancelOrClose()
{
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

//------------------------------------------------------------

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
	return;
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
	return;
    }
}


}

#include "rosegardenconfiguredialog.moc"
