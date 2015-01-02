/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2015 the Rosegarden development team.
 
    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.
 
    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/


#include "GeneralConfigurationPage.h"
#include "TabbedConfigurationPage.h"
#include "ConfigurationPage.h"

#include "misc/Debug.h"
#include "misc/Strings.h"
#include "misc/ConfigGroups.h"
#include "document/RosegardenDocument.h"
#include "gui/editors/eventlist/EventView.h"
#include "gui/editors/parameters/RosegardenParameterArea.h"
#include "gui/studio/StudioControl.h"
#include "gui/dialogs/ShowSequencerStatusDialog.h"
#include "gui/seqmanager/SequenceManager.h"
#include "sound/SoundDriver.h"

#include <QComboBox>
#include <QSettings>
#include <QFileDialog>
#include <QMessageBox>
#include <QCheckBox>
#include <QComboBox>
#include <QFileInfo>
#include <QFrame>
#include <QLabel>
#include <QPushButton>
#include <QSpinBox>
#include <QString>
#include <QTabWidget>
#include <QWidget>
#include <QVBoxLayout>
#include <QLayout>


namespace Rosegarden
{

GeneralConfigurationPage::GeneralConfigurationPage(RosegardenDocument *doc,
        QWidget *parent)
        : TabbedConfigurationPage(parent),
        m_doc(doc),
        m_client(0),
        m_countIn(0),
        m_nameStyle(0),
        m_appendLabel(0)
{
    QSettings settings;
    settings.beginGroup(GeneralOptionsConfigGroup);

    QFrame *frame;
    QGridLayout *layout;
    QLabel *label = 0;
    int row = 0;

    //
    // "Behavior" tab
    //
    frame = new QFrame(m_tabWidget);
    frame->setContentsMargins(10, 10, 10, 10);
    layout = new QGridLayout(frame);
    layout->setSpacing(5);

    layout->setRowMinimumHeight(row, 15);
    ++row;

    QLabel *gsLabel = new QLabel(tr("Graphics performance"));
    layout->addWidget(gsLabel, row, 0);

    m_graphicsSystem = new QComboBox(frame);
    connect(m_graphicsSystem, SIGNAL(activated(int)), this, SLOT(slotModified()));
    m_graphicsSystem->addItem(tr("Fast"));   // raster
    m_graphicsSystem->addItem(tr("Safe"));   // native
    // I decided to give up on this after all.  I only know of one person for
    // whom OpenGL actively crashes, but dealing with that possibility in terms
    // of the wording of user-visible warnings and advisories is an annoying
    // PITA, so I've dumped this for the time being, and limited this to only
    // two options.  I've also changed "normal" to be the new "fast" to
    // encourage users to pick that one.  Stupid users, always foiling the
    // best-intentioned plans with their bumbling.  Alas.
//    m_graphicsSystem->addItem(tr("Unstable"));   // opengl
    m_lastGraphicsSystemIndex = settings.value("graphics_system", Raster).toUInt();
    m_graphicsSystem->setCurrentIndex(m_lastGraphicsSystemIndex);
    layout->addWidget(m_graphicsSystem, row, 1, 1, 2);
    ++row;

    QString graphicsSystemTip(tr("<qt><p>Qt offers you the choice of three graphics systems. The fast (raster) graphics system offers the best tradeoff between performance and stability, but may cause problems for some users.  If you experience frequent crashes, or distorted graphics, you should try the safe (native) graphics system instead.</p></qt>"));
    gsLabel->setToolTip(graphicsSystemTip);
    m_graphicsSystem->setToolTip(graphicsSystemTip);

    layout->addWidget(new QLabel(tr("Double-click opens segment in"),
                                 frame), row, 0);

    m_client = new QComboBox(frame);
    connect(m_client, SIGNAL(activated(int)), this, SLOT(slotModified()));
    m_client->addItem(tr("Notation editor"));
    m_client->addItem(tr("Matrix editor"));
    m_client->addItem(tr("Event List editor"));
    m_client->setCurrentIndex(settings.value("doubleclickclient", NotationView).toUInt());

    layout->addWidget(m_client, row, 1, 1, 2);
    ++row;

    layout->addWidget(new QLabel(tr("Number of count-in measures when recording"),
                                 frame), row, 0);

    m_countIn = new QSpinBox(frame);
    connect(m_countIn, SIGNAL(valueChanged(int)), this, SLOT(slotModified()));
    m_countIn->setValue(settings.value("countinbars", 0).toUInt());
    m_countIn->setMaximum(10);
    m_countIn->setMinimum(0);
    layout->addWidget(m_countIn, row, 1, row- row+1, 2);
    ++row;

    layout->addWidget(new QLabel(tr("Auto-save interval"), frame), row, 0);
    
    m_autoSave = new QComboBox(frame);
    connect(m_autoSave, SIGNAL(activated(int)), this, SLOT(slotModified()));
    m_autoSave->addItem(tr("Every 30 seconds"));
    m_autoSave->addItem(tr("Every minute"));
    m_autoSave->addItem(tr("Every five minutes"));
    m_autoSave->addItem(tr("Every half an hour"));
    m_autoSave->addItem(tr("Never"));

    bool doAutoSave = settings.value("autosave", true).toBool();
    int autoSaveInterval = settings.value("autosaveinterval", 300).toUInt() ;
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

    label = new QLabel(tr("Append suffixes to segment labels"), frame);
    layout->addWidget(label, row, 0);

    m_appendLabel = new QCheckBox(frame);
    connect(m_appendLabel, SIGNAL(stateChanged(int)), this, SLOT(slotModified()));
    // I traditionally had these turned off, and when they reappeared, I found
    // them incredibly annoying, so I'm making false the default:
    m_appendLabel->setChecked(settings.value("appendlabel", false).toBool());
    layout->addWidget(m_appendLabel, row, 1, row- row+1, 2);
    row++;
    settings.endGroup();

    // JACK Transport
    //
#ifdef HAVE_LIBJACK
    settings.beginGroup(SequencerOptionsConfigGroup);

    label = new QLabel(tr("Use JACK transport"), frame);
    layout->addWidget(label, row, 0);

    m_jackTransport = new QCheckBox(frame);
    connect(m_jackTransport, SIGNAL(stateChanged(int)), this, SLOT(slotModified()));
    layout->addWidget(m_jackTransport, row, 1, row- row+1, 2);

//    m_jackTransport->addItem(tr("Ignore JACK transport"));
//    m_jackTransport->addItem(tr("Sync"));

    /*!!! Removed as not yet implemented
        m_jackTransport->addItem(tr("Sync, and offer timebase master"));
    */

    // bool jackMaster = settings.value("jackmaster", false).toBool();
    bool jackTransport = settings.value("jacktransport", false).toBool();
/*
    if (jackTransport)
        m_jackTransport->setCurrentIndex(1);
    else
        m_jackTransport->setCurrentIndex(0);
*/
    m_jackTransport->setChecked(jackTransport);

    ++row;

    settings.endGroup();
#endif
    settings.beginGroup(GeneralOptionsConfigGroup);

    layout->setRowMinimumHeight(row, 20);
    ++row;

    layout->addWidget(new QLabel(tr("Sequencer status"), frame), row, 0);

    QString status(tr("Unknown"));
    SequenceManager *mgr = doc->getSequenceManager();
    if (mgr) {
        int driverStatus = mgr->getSoundDriverStatus() & (AUDIO_OK | MIDI_OK);
        switch (driverStatus) {
        case AUDIO_OK:
            status = tr("No MIDI, audio OK");
            break;
        case MIDI_OK:
            status = tr("MIDI OK, no audio");
            break;
        case AUDIO_OK | MIDI_OK:
            status = tr("MIDI OK, audio OK");
            break;
        default:
            status = tr("No driver");
            break;
        }
    }

    layout->addWidget(new QLabel(status, frame), row, 1);

    QPushButton *showStatusButton = new QPushButton(tr("Details..."),
                                    frame);
    QObject::connect(showStatusButton, SIGNAL(clicked()),
                     this, SLOT(slotShowStatus()));
    layout->addWidget(showStatusButton, row, 2, Qt::AlignRight);
    ++row;

    layout->setRowStretch(row, 10);

    addTab(frame, tr("Behavior"));

    //
    // "Presentation" tab
    //
    frame = new QFrame(m_tabWidget);
    frame->setContentsMargins(10, 10, 10, 10);
    layout = new QGridLayout(frame);
    layout->setSpacing(5);

    row = 0;

    layout->setRowMinimumHeight(row, 15);
    ++row;

    layout->addWidget(new QLabel(tr("Use Thorn style"),
                                 frame), row, 0);

    m_Thorn = new QCheckBox;
    connect(m_Thorn, SIGNAL(stateChanged(int)), this, SLOT(slotModified()));
    layout->addWidget(m_Thorn, row, 1, 1, 3);
    m_Thorn->setToolTip(tr("<qt>When checked, Rosegarden will use the Thorn look and feel, otherwise default system preferences will be used the next time Rosegarden starts.</qt>"));
    m_Thorn->setChecked(settings.value("use_thorn_style", true).toBool());
    ++row;

    layout->addWidget(new QLabel(tr("Note name style"),
                                 frame), row, 0);

    m_nameStyle = new QComboBox(frame);
    connect(m_nameStyle, SIGNAL(activated(int)), this, SLOT(slotModified()));
    m_nameStyle->addItem(tr("Always use US names (e.g. quarter, 8th)"));
    m_nameStyle->addItem(tr("Localized (where available)"));
    m_nameStyle->setCurrentIndex(settings.value("notenamestyle", Local).toUInt());
    layout->addWidget(m_nameStyle, row, 1, 1, 3);
    ++row;
/*
    layout->addWidget(new QLabel(tr("Show tool context help in status bar"), frame), row, 0);

    m_toolContextHelp = new QCheckBox(frame);
    layout->addWidget(m_toolContextHelp, row, 1);
    m_toolContextHelp->setChecked(settings->readBoolEntry
                                  ("toolcontexthelp", true));
    ++row;
*/

    layout->addWidget(new QLabel(tr("Show textured background on"), frame), row, 0);

    m_backgroundTextures = new QCheckBox(tr("Main window"), frame);
    connect(m_backgroundTextures, SIGNAL(stateChanged(int)), this, SLOT(slotModified()));
    layout->addWidget(m_backgroundTextures, row, 1);

    m_notationBackgroundTextures = new QCheckBox(tr("Notation"), frame);
    connect(m_notationBackgroundTextures, SIGNAL(stateChanged(int)), this, SLOT(slotModified()));
    layout->addWidget(m_notationBackgroundTextures, row, 2);

    m_backgroundTextures->setChecked(settings.value("backgroundtextures", true).toBool());

    settings.endGroup();

    settings.beginGroup(NotationViewConfigGroup);
    m_notationBackgroundTextures->setChecked(settings.value("backgroundtextures", true).toBool());
    settings.endGroup();

    ++row;

    layout->addWidget(new QLabel(tr("Show full path in window titles")), row, 0);

    m_longTitles = new QCheckBox;
    connect(m_longTitles, SIGNAL(stateChanged(int)), this, SLOT(slotModified()));
    layout->addWidget(m_longTitles, row, 1);

    settings.beginGroup(GeneralOptionsConfigGroup);
    m_longTitles->setChecked(settings.value("long_window_titles", false).toBool());
    settings.endGroup();

    ++row;
    layout->setRowStretch(row, 10);

    addTab(frame, tr("Presentation"));


    //
    // "External Applications" tab
    //

    frame = new QFrame(m_tabWidget);
    frame->setContentsMargins(10, 10, 10, 10);
    layout = new QGridLayout(frame);
    layout->setSpacing(5);

    row = 0;

    layout->setRowMinimumHeight(row, 15);
    ++row;

    QLabel *explanation = new QLabel(tr("<qt>Rosegarden relies on external applications to provide certain features.  Each selected application must be installed and available on your path.  When choosing an application to use, please ensure that it can run from a \"run command\" box (typically <b>Alt+F2</b>) which should allow Rosegarden to make use of it when necessary.<br></qt>"), frame);
    explanation->setWordWrap(true);
    layout->addWidget(explanation, row, 0, 1, 4);
    ++row;

    layout->addWidget(new QLabel(tr("PDF viewer"),
                      frame), row, 0);

    m_pdfViewer = new QComboBox(frame);
    connect(m_pdfViewer, SIGNAL(activated(int)), this, SLOT(slotModified()));
    m_pdfViewer->addItem(tr("Okular (KDE 4.x)"));
    m_pdfViewer->addItem(tr("Evince (GNOME)"));
    m_pdfViewer->addItem(tr("Adobe Acrobat Reader (non-free)"));
    m_pdfViewer->addItem(tr("MuPDF"));
    m_pdfViewer->addItem(tr("ePDFView"));
    m_pdfViewer->addItem(tr("xdg-open (recommended)"));
    m_pdfViewer->setToolTip(tr("Used to preview generated LilyPond output"));

    layout->addWidget(m_pdfViewer, row, 1, 1, 3);
    ++row;

    layout->addWidget(new QLabel(tr("Command-line file printing utility"),
                      frame), row, 0);

    m_filePrinter = new QComboBox(frame);
    connect(m_filePrinter, SIGNAL(activated(int)), this, SLOT(slotModified()));
    m_filePrinter->addItem(tr("Gtk-LP (GNOME)"),1);
    m_filePrinter->addItem(tr("lpr (no GUI)"),2);
    m_filePrinter->addItem(tr("lp (no GUI)"),3);
    m_filePrinter->addItem(tr("HPLIP (Qt 4)"),4);
    m_filePrinter->setToolTip(tr("Used to print generated LilyPond output without previewing it"));

    layout->addWidget(m_filePrinter, row, 1, 1, 3);
    ++row;

    settings.beginGroup(ExternalApplicationsConfigGroup);
    m_pdfViewer->setCurrentIndex(settings.value("pdfviewer", xdgOpen).toUInt());

    // now that I'm actually on KDE 4.2, I see no more KPrinter.  I'll default
    // to Lpr instead.
    m_filePrinter->setCurrentIndex(settings.value("fileprinter", Lpr).toUInt());
    settings.endGroup();

    ++row;



    layout->setRowStretch(row, 10);

    addTab(frame, tr("External Applications"));

}

void
GeneralConfigurationPage::slotShowStatus()
{
    ShowSequencerStatusDialog dialog(this);
    dialog.exec();
}

void GeneralConfigurationPage::apply()
{
    QSettings settings;

    // tacking the newest external applications settings up here at the top so
    // they're easier to keep track of while putting this together--feel free to
    // relocate this code if this offends your sensibilities
    settings.beginGroup(ExternalApplicationsConfigGroup);

    int pdfViewerIndex = getPdfViewer();
    settings.setValue("pdfviewer", pdfViewerIndex);

    int filePrinterIndex = getFilePrinter();
    settings.setValue("fileprinter", filePrinterIndex);

    settings.endGroup();


    settings.beginGroup(GeneralOptionsConfigGroup);

    int countIn = getCountInSpin();
    settings.setValue("countinbars", countIn);

    int graphicsSystem = getGraphicsSystem();
    settings.setValue("graphics_system", graphicsSystem);
    bool graphicsSystemChanged = false;
    // !!! This comparison is suspicious.  Need to clarify the role,
    // if any, of negative values.
    if (graphicsSystem != m_lastGraphicsSystemIndex) graphicsSystemChanged = true;

    int client = getDblClickClient();
    settings.setValue("doubleclickclient", client);

    int namestyle = getNoteNameStyle();
    settings.setValue("notenamestyle", namestyle);
    
    // bool texturesChanged = false;
    bool mainTextureChanged = false;

    if (settings.value("backgroundtextures", true).toBool() !=
        m_backgroundTextures->isChecked()) {
        // texturesChanged = true;
        mainTextureChanged = true;
        settings.endGroup();
    } else {
        settings.endGroup();
        settings.beginGroup(NotationViewConfigGroup);
        if (settings.value("backgroundtextures", true).toBool() !=
            m_notationBackgroundTextures->isChecked()) {
            // texturesChanged = true;
        }
        settings.endGroup();
    }

    settings.beginGroup(GeneralOptionsConfigGroup);

    settings.setValue("backgroundtextures", m_backgroundTextures->isChecked());
    settings.endGroup();

    settings.beginGroup(NotationViewConfigGroup);

    settings.setValue("backgroundtextures", m_notationBackgroundTextures->isChecked());
    settings.endGroup();

    settings.beginGroup(GeneralOptionsConfigGroup);

    bool thornChanged = false;
    if (settings.value("use_thorn_style", true).toBool()
        != m_Thorn->isChecked()) {
        thornChanged = true;
    }

    std::cerr << "NB. use_thorn_style = " <<
        settings.value("use_thorn_style", true).toBool()
              << ", m_Thorn->isChecked() = " << m_Thorn->isChecked() << std::endl;

    settings.setValue("use_thorn_style", m_Thorn->isChecked());

    settings.setValue("long_window_titles", m_longTitles->isChecked());

    unsigned int interval = 0;

    if (m_autoSave->currentIndex() == 4) {
        settings.setValue("autosave", false);
    } else {
        settings.setValue("autosave", true);
        if (m_autoSave->currentIndex() == 0) {
            interval = 30;
        } else if (m_autoSave->currentIndex() == 1) {
            interval = 60;
        } else if (m_autoSave->currentIndex() == 2) {
            interval = 300;
        } else {
            interval = 1800;
        }
        settings.setValue("autosaveinterval", interval);
        emit updateAutoSaveInterval(interval);
    }

    bool appendLabel = getAppendLabel();
    settings.setValue("appendlabel", appendLabel);

    settings.endGroup();

#ifdef HAVE_LIBJACK
    settings.beginGroup(SequencerOptionsConfigGroup);

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
    settings.setValue("jacktransport", jackTransport);
    settings.setValue("jackmaster", jackMaster);

    // Now send it
    //
    MappedEvent mEjackValue(MidiInstrumentBase,  // InstrumentId
                            MappedEvent::SystemJackTransport,
                            MidiByte(jackValue));

    StudioControl::sendMappedEvent(mEjackValue);

    settings.endGroup();
#endif // HAVE_LIBJACK

    if (mainTextureChanged) {
        QMessageBox::information(this, tr("Rosegarden"), tr("Changes to the textured background in the main window will not take effect until you restart Rosegarden."));
    }

    if (graphicsSystemChanged) {
        QMessageBox::information(this, tr("Rosegarden"), tr("You must restart Rosegarden for the graphics system change to take effect."));
    }

    if (thornChanged) {
        QMessageBox::information(this, tr("Rosegarden"), tr("You must restart Rosegarden for the presentation change to take effect."));
    }
}


}
#include "GeneralConfigurationPage.moc"
