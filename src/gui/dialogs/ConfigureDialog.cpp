/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2021 the Rosegarden development team.
 
    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.
 
    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/


#include "ConfigureDialog.h"
#include "ConfigureDialogBase.h"

#include "document/RosegardenDocument.h"
#include "gui/configuration/GeneralConfigurationPage.h"
#include "gui/configuration/NotationConfigurationPage.h"
#include "gui/configuration/PitchTrackerConfigurationPage.h"
#include "gui/configuration/AudioConfigurationPage.h"
#include "gui/configuration/MIDIConfigurationPage.h"
#include "gui/general/IconLoader.h"

#include <QLayout>
#include <QSettings>
#include <QString>
#include <QWidget>
#include <QTabWidget>

#include <QDir>


namespace Rosegarden
{



ConfigureDialog::ConfigureDialog(RosegardenDocument *doc,
                                 QWidget *parent,
                                 const char *name)
    : ConfigureDialogBase(parent, tr("Rosegarden - Preferences"), name )
{
    
    // General
    GeneralConfigurationPage *generalConfigurationPage =
            new GeneralConfigurationPage(this);
    connect(generalConfigurationPage, &GeneralConfigurationPage::modified,
            this, &ConfigureDialog::slotActivateApply);
    connect(generalConfigurationPage,
                &GeneralConfigurationPage::updateAutoSaveInterval,
            this, &ConfigureDialog::updateAutoSaveInterval);
    addPage(GeneralConfigurationPage::iconLabel(),
            GeneralConfigurationPage::title(),
            IconLoader::loadPixmap(GeneralConfigurationPage::iconName()),
            generalConfigurationPage);
    m_configurationPages.push_back(generalConfigurationPage);

    // MIDI
    MIDIConfigurationPage *midiConfigurationPage =
            new MIDIConfigurationPage(this);
    connect(midiConfigurationPage, &MIDIConfigurationPage::modified,
            this, &ConfigureDialog::slotActivateApply);
    addPage(MIDIConfigurationPage::iconLabel(),
            MIDIConfigurationPage::title(),
            IconLoader::loadPixmap(MIDIConfigurationPage::iconName()),
            midiConfigurationPage);
    m_configurationPages.push_back(midiConfigurationPage);

    // Audio
    AudioConfigurationPage *audioConfigurationPage =
            new AudioConfigurationPage(doc, this);
    connect(audioConfigurationPage, &AudioConfigurationPage::modified,
            this, &ConfigureDialog::slotActivateApply);
    addPage(AudioConfigurationPage::iconLabel(),
            AudioConfigurationPage::title(),
            IconLoader::loadPixmap(AudioConfigurationPage::iconName()),
            audioConfigurationPage);
    m_configurationPages.push_back(audioConfigurationPage);

    // Notation Page
    NotationConfigurationPage *notationConfigurationPage =
            new NotationConfigurationPage(this);
    connect(notationConfigurationPage, &NotationConfigurationPage::modified,
            this, &ConfigureDialog::slotActivateApply);
    addPage(NotationConfigurationPage::iconLabel(),
            NotationConfigurationPage::title(),
            IconLoader::loadPixmap(NotationConfigurationPage::iconName()),
            notationConfigurationPage);
    m_configurationPages.push_back(notationConfigurationPage);

    // Pitch Tracker Page
    PitchTrackerConfigurationPage *pitchTrackerConfigurationPage =
            new PitchTrackerConfigurationPage(this);
    connect(pitchTrackerConfigurationPage,
                &PitchTrackerConfigurationPage::modified,
            this, &ConfigureDialog::slotActivateApply);
    addPage(PitchTrackerConfigurationPage::iconLabel(),
            PitchTrackerConfigurationPage::title(),
            IconLoader::loadPixmap(PitchTrackerConfigurationPage::iconName()),
            pitchTrackerConfigurationPage);
    m_configurationPages.push_back(pitchTrackerConfigurationPage);

}

// I don't remember how this used to work, and I have a feeling there's some
// other broken, parallel, vestigial mechanism I'm ignoring.  Oh well.  This is
// lifted from what worked in DocumentConfigurationPage, and this is the
// implementation that actually works so the notation editor can set Edit ->
// Preferences to the right page.
void
ConfigureDialog::setNotationPage()
{
    int index = 0;

    for (ConfigurationPages::iterator i = m_configurationPages.begin();
            i != m_configurationPages.end(); ++i) {

        NotationConfigurationPage *page =
            dynamic_cast<NotationConfigurationPage *>(*i);

        if (!page) {
            ++index;
            continue;
        }

        setPageByIndex(index);
        return ;
    }
}


}
