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


#include "GeneralConfigurationPage.h"

#include "document/ConfigGroups.h"
#include "ConfigurationPage.h"
#include "document/RosegardenGUIDoc.h"
#include "gui/editors/eventlist/EventView.h"
#include "gui/editors/parameters/RosegardenParameterArea.h"
#include "TabbedConfigurationPage.h"
#include <kcombobox.h>
#include <kconfig.h>
#include <kfiledialog.h>
#include <kmessagebox.h>
#include <qcheckbox.h>
#include <qcombobox.h>
#include <qfileinfo.h>
#include <qframe.h>
#include <qlabel.h>
#include <qlineedit.h>
#include <qpushbutton.h>
#include <qspinbox.h>
#include <qstring.h>
#include <qtabwidget.h>
#include <qvbox.h>
#include <qwidget.h>


namespace Rosegarden
{

GeneralConfigurationPage::GeneralConfigurationPage(RosegardenGUIDoc *doc,
        KConfig *cfg,
        QWidget *parent, const char *name)
        : TabbedConfigurationPage(cfg, parent, name),
        m_doc(doc),
        m_client(0),
        m_countIn(0),
        m_midiPitchOctave(0),
        m_externalAudioEditorPath(0),
        m_nameStyle(0)
{
    m_cfg->setGroup(GeneralOptionsConfigGroup);

    //
    // "Appearance" tab
    //
    QFrame *frame = new QFrame(m_tabWidget);
    QGridLayout *layout = new QGridLayout(frame,
                                          6, 2,  // nbrow, nbcol -- one extra row improves layout
                                          10, 5);
    layout->addWidget(new QLabel(i18n("Note name style"),
                                 frame), 0, 0);
    layout->addWidget(new QLabel(i18n("Audio preview scale"),
                                 frame), 1, 0);
    layout->addWidget(new QLabel(i18n("Base octave number for MIDI pitch display"),
                                 frame), 2, 0);

    QVBox *box = new QVBox(frame);
    new QLabel(i18n("Use textured backgrounds on canvas areas"), box);
    new QLabel(i18n("    (takes effect only from next restart)"), box);
    layout->addWidget(box, 3, 0);

    layout->addWidget(new QLabel(i18n("Side-bar parameter box layout"),
                                 frame), 4, 0);

    m_nameStyle = new KComboBox(frame);
    m_nameStyle->insertItem(i18n("Always use US names (e.g. quarter, 8th)"));
    m_nameStyle->insertItem(i18n("Localized (where available)"));
    m_nameStyle->setCurrentItem(m_cfg->readUnsignedNumEntry("notenamestyle", Local));
    layout->addWidget(m_nameStyle, 0, 1);

    m_previewStyle = new KComboBox(frame);
    m_previewStyle->insertItem(i18n("Linear - easier to see loud peaks"));
    m_previewStyle->insertItem(i18n("Meter scaling - easier to see quiet activity"));
    m_previewStyle->setCurrentItem(m_cfg->readUnsignedNumEntry("audiopreviewstyle", 1));
    layout->addWidget(m_previewStyle, 1, 1);

    m_midiPitchOctave = new QSpinBox(frame);
    m_midiPitchOctave->setMaxValue(10);
    m_midiPitchOctave->setMinValue( -10);
    m_midiPitchOctave->setValue(m_cfg->readNumEntry("midipitchoctave", -2));

    layout->addWidget(m_midiPitchOctave, 2, 1);

    m_backgroundTextures = new QCheckBox(frame);
    layout->addWidget(m_backgroundTextures, 3, 1);

    m_backgroundTextures->setChecked(m_cfg->readBoolEntry("backgroundtextures",
                                     true));

    m_sidebarStyle = new KComboBox(frame);
    m_sidebarStyle->insertItem(i18n("Vertically stacked"),
                               RosegardenParameterArea::CLASSIC_STYLE);
    m_sidebarStyle->insertItem(i18n("Tabbed"),
                               RosegardenParameterArea::TAB_BOX_STYLE);

    m_sidebarStyle->setCurrentItem(m_cfg->readUnsignedNumEntry("sidebarstyle",
                                   0));
    layout->addWidget(m_sidebarStyle, 4, 1);

    addTab(frame, i18n("Presentation"));

    //
    // "Behavior" tab
    //
    frame = new QFrame(m_tabWidget);
    layout = new QGridLayout(frame,
                             4, 2,  // nbrow, nbcol
                             10, 5);

    layout->addWidget(new QLabel(i18n("Default editor (for double-click on segment)"),
                                 frame), 0, 0);
    layout->addWidget(new QLabel(i18n("Number of count-in measures when recording"),
                                 frame), 1, 0);
    layout->addWidget(new QLabel(i18n("Always use default studio when loading files"),
                                 frame), 2, 0);

    m_client = new KComboBox(frame);
    m_client->insertItem(i18n("Notation"));
    m_client->insertItem(i18n("Matrix"));
    m_client->insertItem(i18n("Event List"));
    m_client->setCurrentItem(m_cfg->readUnsignedNumEntry("doubleclickclient", NotationView));

    layout->addWidget(m_client, 0, 1);

    m_countIn = new QSpinBox(frame);
    m_countIn->setValue(m_cfg->readUnsignedNumEntry("countinbars", 2));
    m_countIn->setMaxValue(10);
    m_countIn->setMinValue(0);
    layout->addWidget(m_countIn, 1, 1);

    m_studio = new QCheckBox(frame);
    m_studio->setChecked(m_cfg->readBoolEntry("alwaysusedefaultstudio", false));
    layout->addWidget(m_studio, 2, 1);

    addTab(frame, i18n("Behavior"));

    //
    // External editor tab
    //
    frame = new QFrame(m_tabWidget);
    layout = new QGridLayout(frame,
                             2, 3,  // nbrow, nbcol
                             10, 5);

    layout->addWidget(new QLabel(i18n("External audio editor"), frame),
                      0, 0);

    QString externalAudioEditor = m_cfg->readEntry("externalaudioeditor",
                                  "audacity");

    m_externalAudioEditorPath = new QLineEdit(externalAudioEditor, frame);
    m_externalAudioEditorPath->setMinimumWidth(200);
    layout->addWidget(m_externalAudioEditorPath, 0, 1);

    QPushButton *changePathButton =
        new QPushButton(i18n("Choose..."), frame);

    layout->addWidget(changePathButton, 0, 2);
    connect(changePathButton, SIGNAL(clicked()), SLOT(slotFileDialog()));


    addTab(frame, i18n("External Editors"));

    //
    // Autosave tab
    //
    frame = new QFrame(m_tabWidget);
    layout = new QGridLayout(frame,
                             3, 2,  // nbrow, nbcol
                             10, 5);

    m_autosave = new QCheckBox(i18n("Enable auto-save"), frame);
    m_autosave->setChecked(m_cfg->readBoolEntry("autosave", true));
    layout->addWidget(m_autosave, 0, 0);

    layout->addWidget(new QLabel(i18n("Auto-save interval (in seconds)"),
                                 frame), 1, 0);
    m_autosaveInterval = new QSpinBox(0, 1200, 10, frame);
    m_autosaveInterval->setValue(m_cfg->readUnsignedNumEntry("autosaveinterval", 60));
    layout->addWidget(m_autosaveInterval, 1, 1);

    addTab(frame, i18n("Auto-save"));

}

void
GeneralConfigurationPage::slotFileDialog()
{
    QString path = KFileDialog::getOpenFileName(QString::null, QString::null, this, i18n("External audio editor path"));
    m_externalAudioEditorPath->setText(path);
}

void GeneralConfigurationPage::apply()
{
    m_cfg->setGroup(GeneralOptionsConfigGroup);

    int countIn = getCountInSpin();
    m_cfg->writeEntry("countinbars", countIn);

    int client = getDblClickClient();
    m_cfg->writeEntry("doubleclickclient", client);

    bool studio = getUseDefaultStudio();
    m_cfg->writeEntry("alwaysusedefaultstudio", studio);

    int octave = m_midiPitchOctave->value();
    m_cfg->writeEntry("midipitchoctave", octave);

    int namestyle = getNoteNameStyle();
    m_cfg->writeEntry("notenamestyle", namestyle);

    int previewstyle = m_previewStyle->currentItem();
    m_cfg->writeEntry("audiopreviewstyle", previewstyle);

    m_cfg->writeEntry("backgroundtextures", m_backgroundTextures->isChecked());

    int sidebarStyle = m_sidebarStyle->currentItem();
    m_cfg->writeEntry("sidebarstyle", sidebarStyle);
    emit updateSidebarStyle(sidebarStyle);

    m_cfg->writeEntry("autosave", m_autosave->isChecked());

    unsigned int autosaveInterval = m_autosaveInterval->value();
    m_cfg->writeEntry("autosaveinterval", autosaveInterval);
    emit updateAutoSaveInterval(autosaveInterval);

    QString externalAudioEditor = getExternalAudioEditor();

    QFileInfo info(externalAudioEditor);
    if (!info.exists() || !info.isExecutable()) {
        /*
        QString errorStr =
             i18n("External editor \"") + externalAudioEditor +
             ("\" not found or not executable.\nReverting to last editor.");
        KMessageBox::error(this, errorStr);

        // revert on gui
        m_externalAudioEditorPath->
            setText(m_cfg->readEntry("externalaudioeditor", ""));
            */
        m_cfg->writeEntry("externalaudioeditor", "");
    } else
        m_cfg->writeEntry("externalaudioeditor", externalAudioEditor);


}

}
