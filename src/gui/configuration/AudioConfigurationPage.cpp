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


#include "AudioConfigurationPage.h"

#include "sound/Midi.h"
#include "sound/SoundDriver.h"
#include "document/ConfigGroups.h"
#include "base/MidiProgram.h"
#include "base/Studio.h"
#include "ConfigurationPage.h"
#include "document/RosegardenGUIDoc.h"
#include "gui/dialogs/ShowSequencerStatusDialog.h"
#include "gui/seqmanager/SequenceManager.h"
#include "gui/application/RosegardenApplication.h"
#include "gui/studio/StudioControl.h"
#include "sound/MappedEvent.h"
#include "TabbedConfigurationPage.h"
#include "misc/Strings.h"
#include <QComboBox>
#include <kconfig.h>
#include <kfiledialog.h>
#include <QCheckBox>
#include <QComboBox>
#include <QByteArray>
#include <QDataStream>
#include <QFrame>
#include <QLabel>
#include <QLineEdit>
#include <QObject>
#include <QPushButton>
#include <QLayout>
#include <QSlider>
#include <QSpinBox>
#include <QString>
#include <QStringList>
#include <QTabWidget>
#include <QToolTip>
#include <QWidget>
#include <QMessageBox>


namespace Rosegarden
{

AudioConfigurationPage::AudioConfigurationPage(
    RosegardenGUIDoc *doc,
    QSettings cfg,
    QWidget *parent,
    const char *name):
    TabbedConfigurationPage(cfg, parent, name),
    m_externalAudioEditorPath(0)
{
    // set the document in the super class
    m_doc = doc;

    QSettings m_cfg;

    m_cfg.beginGroup( SequencerOptionsConfigGroup );

    // 

    // FIX-manually-(GW), add:

    // m_cfg.endGroup();		// corresponding to: m_cfg.beginGroup( SequencerOptionsConfigGroup );

    //  


    QFrame *frame = new QFrame(m_tabWidget);
    QGridLayout *layout = new QGridLayout(frame, 7, 2, 10, 5);

    QLabel *label = 0;

    int row = 0;

    QSettings m_cfg;

    m_cfg.beginGroup( GeneralOptionsConfigGroup );

    // 

    // FIX-manually-(GW), add:

    // m_cfg.endGroup();		// corresponding to: m_cfg.beginGroup( GeneralOptionsConfigGroup );

    //  


    layout->setRowSpacing(row, 15);
    ++row;

    layout->addWidget(new QLabel(i18n("Audio preview scale"),
                                 frame), row, 0);

    m_previewStyle = new QComboBox(frame);
    m_previewStyle->addItem(i18n("Linear - easier to see loud peaks"));
    m_previewStyle->addItem(i18n("Meter scaling - easier to see quiet activity"));
    m_previewStyle->setCurrentIndex( m_cfg.value("audiopreviewstyle", 1).toUInt() );
    layout->addWidget(m_previewStyle, row, 1, row- row+1, 2);
    ++row;

#ifdef HAVE_LIBJACK
    QSettings m_cfg;
    m_cfg.beginGroup( SequencerOptionsConfigGroup );
    // 
    // FIX-manually-(GW), add:
    // m_cfg.endGroup();		// corresponding to: m_cfg.beginGroup( SequencerOptionsConfigGroup );
    //  


    label = new QLabel(i18n("Record audio files as"), frame);
    m_audioRecFormat = new QComboBox(frame);
    m_audioRecFormat->addItem(i18n("16-bit PCM WAV format (smaller files)"));
    m_audioRecFormat->addItem(i18n("32-bit float WAV format (higher quality)"));
    m_audioRecFormat->setCurrentIndex( m_cfg.value("audiorecordfileformat", 1).toUInt() );
    layout->addWidget(label, row, 0);
    layout->addWidget(m_audioRecFormat, row, 1, row- row+1, 2);
    ++row;
#endif

    QSettings m_cfg;

    m_cfg.beginGroup( GeneralOptionsConfigGroup );

    // 

    // FIX-manually-(GW), add:

    // m_cfg.endGroup();		// corresponding to: m_cfg.beginGroup( GeneralOptionsConfigGroup );

    //  


    layout->addWidget(new QLabel(i18n("External audio editor"), frame),
                      row, 0);

    QString defaultAudioEditor = getBestAvailableAudioEditor();

    std::cerr << "defaultAudioEditor = " << defaultAudioEditor << std::endl;

    QString externalAudioEditor = m_cfg.value("externalaudioeditor",
                                  defaultAudioEditor) ;

    if (externalAudioEditor == "") {
        externalAudioEditor = defaultAudioEditor;
        m_cfg.setValue("externalaudioeditor", externalAudioEditor);
    }

    m_externalAudioEditorPath = new QLineEdit(externalAudioEditor, frame);
//    m_externalAudioEditorPath->setMinimumWidth(150);
    layout->addWidget(m_externalAudioEditorPath, row, 1);
    
    QPushButton *changePathButton =
        new QPushButton(i18n("Choose..."), frame);

    layout->addWidget(changePathButton, row, 2);
    connect(changePathButton, SIGNAL(clicked()), SLOT(slotFileDialog()));
    ++row;

    QSettings m_cfg;

    m_cfg.beginGroup( SequencerOptionsConfigGroup );

    // 

    // FIX-manually-(GW), add:

    // m_cfg.endGroup();		// corresponding to: m_cfg.beginGroup( SequencerOptionsConfigGroup );

    //  


    layout->addWidget(new QLabel(i18n("Create JACK outputs"), frame),
                      row, 0);
//    ++row;

#ifdef HAVE_LIBJACK
    m_createFaderOuts = new QCheckBox(i18n("for individual audio instruments"), frame);
    m_createFaderOuts->setChecked( qStrToBool( m_cfg.value("audiofaderouts", "false" ) ) );

//    layout->addWidget(label, row, 0, Qt::AlignRight);
    layout->addWidget(m_createFaderOuts, row, 1);
    ++row;

    m_createSubmasterOuts = new QCheckBox(i18n("for submasters"), frame);
    m_createSubmasterOuts->setChecked( qStrToBool( m_cfg.value("audiosubmasterouts", "                                      false" ) ) );

//    layout->addWidget(label, row, 0, Qt::AlignRight);
    layout->addWidget(m_createSubmasterOuts, row, 1);
    ++row;
#endif

    layout->setRowStretch(row, 10);

    addTab(frame, i18n("General"));

    // --------------------- Startup control ----------------------
    //
#ifdef HAVE_LIBJACK
#define OFFER_JACK_START_OPTION 1
#ifdef OFFER_JACK_START_OPTION

    frame = new QFrame(m_tabWidget);
    layout = new QGridLayout(frame, 8, 4, 10, 5);

    row = 0;

    layout->setRowSpacing(row, 15);
    ++row;

    label = new QLabel(i18n("Rosegarden can start the JACK audio daemon (jackd) for you automatically if it isn't already running when Rosegarden starts.\n\nThis is recommended for beginners and those who use Rosegarden as their main audio application, but it might not be to the liking of advanced users.\n\nIf you want to start JACK automatically, make sure the command includes a full path where necessary as well as any command-line arguments you want to use.\n\nFor example: /usr/local/bin/jackd -d alsa -d hw -r44100 -p 2048 -n 2\n\n"), frame);
    label->setAlignment(Qt::TextWordWrap);

    layout->addWidget(label, row, 0, row- row+1, 3- 1);
    ++row;

    // JACK control things
    //
    bool startJack = qStrToBool( m_cfg.value("jackstart", "false" ) ) ;
    m_startJack = new QCheckBox(frame);
    m_startJack->setChecked(startJack);

    layout->addWidget(new QLabel(i18n("Start JACK when Rosegarden starts"), frame), 2, 0);

    layout->addWidget(m_startJack, row, 1);
    ++row;

    layout->addWidget(new QLabel(i18n("JACK command"), frame),
                      row, 0);

    QString jackPath = m_cfg.value("jackcommand",
                                        // "/usr/local/bin/jackd -d alsa -d hw -r 44100 -p 2048 -n 2") ;
                                        "/usr/bin/qjackctl -s");
    m_jackPath = new QLineEdit(jackPath, frame);

    layout->addWidget(m_jackPath, row, 1, row- row+1, 3);
    ++row;

    layout->setRowStretch(row, 10);

    addTab(frame, i18n("JACK Startup"));

#endif // OFFER_JACK_START_OPTION
#endif // HAVE_LIBJACK

}

void
AudioConfigurationPage::slotFileDialog()
{
    QString path = KFileDialog::getOpenFileName(QString::null, QString::null, this, i18n("External audio editor path"));
    m_externalAudioEditorPath->setText(path);
}

void
AudioConfigurationPage::apply()
{
    QSettings m_cfg;
    m_cfg.beginGroup( SequencerOptionsConfigGroup );
    // 
    // FIX-manually-(GW), add:
    // m_cfg.endGroup();		// corresponding to: m_cfg.beginGroup( SequencerOptionsConfigGroup );
    //  


#ifdef HAVE_LIBJACK
#ifdef OFFER_JACK_START_OPTION
    // Jack control
    //
    m_cfg.setValue("jackstart", m_startJack->isChecked());
    m_cfg.setValue("jackcommand", m_jackPath->text());
#endif // OFFER_JACK_START_OPTION

    // Jack audio inputs
    //
    m_cfg.setValue("audiofaderouts", m_createFaderOuts->isChecked());
    m_cfg.setValue("audiosubmasterouts", m_createSubmasterOuts->isChecked());
    m_cfg.setValue("audiorecordfileformat", m_audioRecFormat->currentIndex());
#endif

    QSettings m_cfg;

    m_cfg.beginGroup( GeneralOptionsConfigGroup );

    // 

    // FIX-manually-(GW), add:

    // m_cfg.endGroup();		// corresponding to: m_cfg.beginGroup( GeneralOptionsConfigGroup );

    //  


    int previewstyle = m_previewStyle->currentIndex();
    m_cfg.setValue("audiopreviewstyle", previewstyle);

    QString externalAudioEditor = getExternalAudioEditor();

    QStringList extlist = externalAudioEditor.split(" ", QString::SkipEmptyParts);
    QString extpath = "";
    if (extlist.size() > 0) extpath = extlist[0];

    if (extpath != "") {
        QFileInfo info(extpath);
        if (!info.exists() || !info.isExecutable()) {
            QMessageBox::critical(0, i18n("External audio editor \"%1\" not found or not executable", extpath));
            m_cfg.setValue("externalaudioeditor", "");
        } else {
            m_cfg.setValue("externalaudioeditor", externalAudioEditor);
        }
    } else {
        m_cfg.setValue("externalaudioeditor", "");
    }
}

QString
AudioConfigurationPage::getBestAvailableAudioEditor()
{
    static QString result = "";
    static bool haveResult = false;

    if (haveResult) return result;

    QString path;
    const char *cpath = getenv("PATH");
    if (cpath) path = cpath;
    else path = "/usr/bin:/bin";

    QStringList pathList = path.split(":", QString::SkipEmptyParts);

    const char *candidates[] = {
        "mhwaveedit",
        "rezound",
        "audacity"
    };

    for (int i = 0;
         i < sizeof(candidates)/sizeof(candidates[0]) && result == "";
         i++) {

        QString n(candidates[i]);

        for (int j = 0;
             j < pathList.size() && result == "";
             j++) {

            QDir dir(pathList[j]);
            QString fp(dir.filePath(n));
            QFileInfo fi(fp);

            if (fi.exists() && fi.isExecutable()) {
                if (n == "rezound") {
                    result = QString("%1 --audio-method=jack").arg(fp);
                } else {
                    result = fp;
                }
            }
        }
    }

    haveResult = true;
    return result;
}

}
#include "AudioConfigurationPage.moc"

