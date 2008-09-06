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


#include "GeneralConfigurationPage.h"

#include "document/ConfigGroups.h"
#include "ConfigurationPage.h"
#include "document/RosegardenGUIDoc.h"
#include "gui/editors/eventlist/EventView.h"
#include "gui/editors/parameters/RosegardenParameterArea.h"
#include "gui/studio/StudioControl.h"
#include "gui/dialogs/ShowSequencerStatusDialog.h"
#include "gui/seqmanager/SequenceManager.h"
#include "sound/SoundDriver.h"
#include "TabbedConfigurationPage.h"
#include <QComboBox>
#include <kconfig.h>
#include <kfiledialog.h>
#include <kmessagebox.h>
#include <QCheckBox>
#include <QComboBox>
#include <QFileInfo>
#include <QFrame>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QSpinBox>
#include <QString>
#include <QTabWidget>
#include <QWidget>
#include <QVBoxLayout>
#include <QLayout>


namespace Rosegarden
{

GeneralConfigurationPage::GeneralConfigurationPage(RosegardenGUIDoc *doc,
        KConfig *cfg,
        QWidget *parent, const char *name)
        : TabbedConfigurationPage(cfg, parent, name),
        m_doc(doc),
        m_client(0),
        m_countIn(0),
        m_nameStyle(0),
        m_appendLabel(0)
{
    m_cfg->setGroup(GeneralOptionsConfigGroup);

    QFrame *frame;
    QGridLayout *layout;
    QLabel *label = 0;
    int row = 0;

    //
    // "Behavior" tab
    //
    frame = new QFrame(m_tabWidget);
    layout = new QGridLayout(frame,
                             6, 2,  // nbrow, nbcol
                             10, 5);

    layout->setRowSpacing(row, 15);
    ++row;

    layout->addWidget(new QLabel(i18n("Double-click opens segment in"),
                                 frame), row, 0);

    m_client = new QComboBox(frame);
    m_client->addItem(i18n("Notation editor"));
    m_client->addItem(i18n("Matrix editor"));
    m_client->addItem(i18n("Event List editor"));
    m_client->setCurrentIndex(m_cfg->readUnsignedNumEntry("doubleclickclient", NotationView));

    layout->addWidget(m_client, row, 1, row- row+1, 2);
    ++row;

    layout->addWidget(new QLabel(i18n("Number of count-in measures when recording"),
                                 frame), row, 0);

    m_countIn = new QSpinBox(frame);
    m_countIn->setValue(m_cfg->readUnsignedNumEntry("countinbars", 0));
    m_countIn->setMaximum(10);
    m_countIn->setMinimum(0);
    layout->addWidget(m_countIn, row, 1, row- row+1, 2);
    ++row;

    layout->addWidget(new QLabel(i18n("Auto-save interval"), frame), row, 0);
    
    m_autoSave = new QComboBox(frame);
    m_autoSave->addItem(i18n("Every 30 seconds"));
    m_autoSave->addItem(i18n("Every minute"));
    m_autoSave->addItem(i18n("Every five minutes"));
    m_autoSave->addItem(i18n("Every half an hour"));
    m_autoSave->addItem(i18n("Never"));

    bool doAutoSave = m_cfg->readBoolEntry("autosave", true);
    int autoSaveInterval = m_cfg->readUnsignedNumEntry("autosaveinterval", 300);
    if (!doAutoSave || autoSaveInterval == 0) {
        m_autoSave->setCurrentIndex(4); // off
    } else if (autoSaveInterval < 45) {
        m_autoSave->setCurrentIndex(0);
    } else if (autoSaveInterval < 150) {
        m_autoSave->setCurrentIndex(1);
    } else if (autoSaveInterval < 900) {
        m_autoSave->setCurrentIndex(2);
    } else {
        m_autoSave->setCurrentIndex(3);
    }

    layout->addWidget(m_autoSave, row, 1, row- row+1, 2);
    ++row;

    label = new QLabel(i18n("Append suffixes to segment labels"), frame);
    layout->addWidget(label, row, 0);

    m_appendLabel = new QCheckBox(frame);
    m_appendLabel->setChecked(m_cfg->readBoolEntry("appendlabel", true));
    layout->addWidget(m_appendLabel, row, 1, row- row+1, 2);
    row++;

    // JACK Transport
    //
#ifdef HAVE_LIBJACK
    m_cfg->setGroup(SequencerOptionsConfigGroup);

    label = new QLabel(i18n("Use JACK transport"), frame);
    layout->addWidget(label, row, 0);

    m_jackTransport = new QCheckBox(frame);
    layout->addWidget(m_jackTransport, row, 1, row- row+1, 2);

//    m_jackTransport->addItem(i18n("Ignore JACK transport"));
//    m_jackTransport->addItem(i18n("Sync"));

    /*!!! Removed as not yet implemented
        m_jackTransport->addItem(i18n("Sync, and offer timebase master"));
    */

    bool jackMaster = m_cfg->readBoolEntry("jackmaster", false);
    bool jackTransport = m_cfg->readBoolEntry("jacktransport", false);
/*
    if (jackTransport)
        m_jackTransport->setCurrentIndex(1);
    else
        m_jackTransport->setCurrentIndex(0);
*/
    m_jackTransport->setChecked(jackTransport);

    ++row;

    m_cfg->setGroup(GeneralOptionsConfigGroup);
#endif

    layout->setRowSpacing(row, 20);
    ++row;

    layout->addWidget(new QLabel(i18n("Sequencer status"), frame), row, 0);

    QString status(i18n("Unknown"));
    SequenceManager *mgr = doc->getSequenceManager();
    if (mgr) {
        int driverStatus = mgr->getSoundDriverStatus() & (AUDIO_OK | MIDI_OK);
        switch (driverStatus) {
        case AUDIO_OK:
            status = i18n("No MIDI, audio OK");
            break;
        case MIDI_OK:
            status = i18n("MIDI OK, no audio");
            break;
        case AUDIO_OK | MIDI_OK:
            status = i18n("MIDI OK, audio OK");
            break;
        default:
            status = i18n("No driver");
            break;
        }
    }

    layout->addWidget(new QLabel(status, frame), row, 1);

    QPushButton *showStatusButton = new QPushButton(i18n("Details..."),
                                    frame);
    QObject::connect(showStatusButton, SIGNAL(clicked()),
                     this, SLOT(slotShowStatus()));
    layout->addWidget(showStatusButton, row, 2, Qt::AlignRight);
    ++row;

    layout->setRowStretch(row, 10);

    addTab(frame, i18n("Behavior"));

    //
    // "Appearance" tab
    //
    frame = new QFrame(m_tabWidget);
    layout = new QGridLayout(frame,
                                          7, 4,  // nbrow, nbcol -- one extra row improves layout
                                          10, 5);

    row = 0;

    layout->setRowSpacing(row, 15);
    ++row;

    layout->addWidget(new QLabel(i18n("Side-bar parameter box layout"),
                                 frame), row, 0);

    m_sidebarStyle = new QComboBox(frame);
    m_sidebarStyle->addItem(i18n("Vertically stacked"),
                               RosegardenParameterArea::CLASSIC_STYLE);
    m_sidebarStyle->addItem(i18n("Tabbed"),
                               RosegardenParameterArea::TAB_BOX_STYLE);

    m_sidebarStyle->setCurrentIndex(m_cfg->readUnsignedNumEntry("sidebarstyle",
                                   0));
    layout->addWidget(m_sidebarStyle, row, 1, row- row+1, 3);
    ++row;

    layout->addWidget(new QLabel(i18n("Note name style"),
                                 frame), row, 0);

    m_nameStyle = new QComboBox(frame);
    m_nameStyle->addItem(i18n("Always use US names (e.g. quarter, 8th)"));
    m_nameStyle->addItem(i18n("Localized (where available)"));
    m_nameStyle->setCurrentIndex(m_cfg->readUnsignedNumEntry("notenamestyle", Local));
    layout->addWidget(m_nameStyle, row, 1, row- row+1, 3);
    ++row;
/*
    layout->addWidget(new QLabel(i18n("Show tool context help in status bar"), frame), row, 0);

    m_toolContextHelp = new QCheckBox(frame);
    layout->addWidget(m_toolContextHelp, row, 1);
    m_toolContextHelp->setChecked(m_cfg->readBoolEntry
                                  ("toolcontexthelp", true));
    ++row;
*/

    layout->addWidget(new QLabel(i18n("Show textured background on"), frame), row, 0);

    m_backgroundTextures = new QCheckBox(i18n("Main window"), frame);
    layout->addWidget(m_backgroundTextures, row, 1);

    m_matrixBackgroundTextures = new QCheckBox(i18n("Matrix"), frame);
    layout->addWidget(m_matrixBackgroundTextures, row, 2);

    m_notationBackgroundTextures = new QCheckBox(i18n("Notation"), frame);
    layout->addWidget(m_notationBackgroundTextures, row, 3);

    m_backgroundTextures->setChecked(m_cfg->readBoolEntry
                                     ("backgroundtextures", true));

    m_cfg->setGroup(MatrixViewConfigGroup);
    m_matrixBackgroundTextures->setChecked(m_cfg->readBoolEntry
                                           ("backgroundtextures-1.6-plus", true));
    m_cfg->setGroup(NotationViewConfigGroup);
    m_notationBackgroundTextures->setChecked(m_cfg->readBoolEntry
                                             ("backgroundtextures", true));
    m_cfg->setGroup(GeneralOptionsConfigGroup);
    ++row;

    layout->addWidget(new QLabel(i18n("Use bundled Klearlook theme"), frame), row, 0);
    m_globalStyle = new QComboBox(frame);
    m_globalStyle->addItem(i18n("Never"));
    m_globalStyle->addItem(i18n("When not running under KDE"));
    m_globalStyle->addItem(i18n("Always"));
    m_globalStyle->setCurrentIndex(m_cfg->readUnsignedNumEntry("Install Own Theme", 1));
    layout->addWidget(m_globalStyle, row, 1, row- row+1, 3);

    ++row;

    layout->setRowStretch(row, 10);

    addTab(frame, i18n("Presentation"));

}

void
GeneralConfigurationPage::slotShowStatus()
{
    ShowSequencerStatusDialog dialog(this);
    dialog.exec();
}

void GeneralConfigurationPage::apply()
{
    m_cfg->setGroup(GeneralOptionsConfigGroup);

    int countIn = getCountInSpin();
    m_cfg->writeEntry("countinbars", countIn);

    int client = getDblClickClient();
    m_cfg->writeEntry("doubleclickclient", client);

    int globalstyle = m_globalStyle->currentIndex();
    m_cfg->writeEntry("Install Own Theme", globalstyle);

    int namestyle = getNoteNameStyle();
    m_cfg->writeEntry("notenamestyle", namestyle);
/*
    m_cfg->writeEntry("toolcontexthelp", m_toolContextHelp->isChecked());
*/
    bool texturesChanged = false;
    bool mainTextureChanged = false;
    m_cfg->setGroup(GeneralOptionsConfigGroup);

    if (m_cfg->readBoolEntry("backgroundtextures", true) !=
        m_backgroundTextures->isChecked()) {
        texturesChanged = true;
        mainTextureChanged = true;
    } else {
        m_cfg->setGroup(MatrixViewConfigGroup);
        if (m_cfg->readBoolEntry("backgroundtextures-1.6-plus", false) !=
            m_matrixBackgroundTextures->isChecked()) {
            texturesChanged = true;
        } else {
            m_cfg->setGroup(NotationViewConfigGroup);
            if (m_cfg->readBoolEntry("backgroundtextures", true) !=
                m_notationBackgroundTextures->isChecked()) {
                texturesChanged = true;
            }
        }
    }

    m_cfg->setGroup(GeneralOptionsConfigGroup);
    m_cfg->writeEntry("backgroundtextures", m_backgroundTextures->isChecked());

    m_cfg->setGroup(MatrixViewConfigGroup);
    m_cfg->writeEntry("backgroundtextures-1.6-plus", m_matrixBackgroundTextures->isChecked());

    m_cfg->setGroup(NotationViewConfigGroup);
    m_cfg->writeEntry("backgroundtextures", m_notationBackgroundTextures->isChecked());

    m_cfg->setGroup(GeneralOptionsConfigGroup);

    int sidebarStyle = m_sidebarStyle->currentIndex();
    m_cfg->writeEntry("sidebarstyle", sidebarStyle);
    emit updateSidebarStyle(sidebarStyle);

    unsigned int interval = 0;

    if (m_autoSave->currentIndex() == 4) {
        m_cfg->writeEntry("autosave", false);
    } else {
        m_cfg->writeEntry("autosave", true);
        if (m_autoSave->currentIndex() == 0) {
            interval = 30;
        } else if (m_autoSave->currentIndex() == 1) {
            interval = 60;
        } else if (m_autoSave->currentIndex() == 2) {
            interval = 300;
        } else {
            interval = 1800;
        }
        m_cfg->writeEntry("autosaveinterval", interval);
        emit updateAutoSaveInterval(interval);
    }

    bool appendLabel = getAppendLabel();
    m_cfg->writeEntry("appendlabel", appendLabel);

#ifdef HAVE_LIBJACK
    m_cfg->setGroup(SequencerOptionsConfigGroup);

    // Write the JACK entry
    //
/*
    int jackValue = m_jackTransport->currentIndex();
    bool jackTransport, jackMaster;

    switch (jackValue) {
    case 2:
        jackTransport = true;
        jackMaster = true;
        break;

    case 1:
        jackTransport = true;
        jackMaster = false;
        break;

    default:
        jackValue = 0;

    case 0:
        jackTransport = false;
        jackMaster = false;
        break;
    }
*/

    bool jackTransport = m_jackTransport->isChecked();
    bool jackMaster = false;

    int jackValue = 0; // 0 -> nothing, 1 -> sync, 2 -> master
    if (jackTransport) jackValue = 1;

    // Write the items
    //
    m_cfg->writeEntry("jacktransport", jackTransport);
    m_cfg->writeEntry("jackmaster", jackMaster);

    // Now send it
    //
    MappedEvent mEjackValue(MidiInstrumentBase,  // InstrumentId
                            MappedEvent::SystemJackTransport,
                            MidiByte(jackValue));

    StudioControl::sendMappedEvent(mEjackValue);
#endif // HAVE_LIBJACK

    if (mainTextureChanged) {
        KMessageBox::information(this, i18n("Changes to the textured background in the main window will not take effect until you restart Rosegarden."));
    }

}

}
#include "GeneralConfigurationPage.moc"
