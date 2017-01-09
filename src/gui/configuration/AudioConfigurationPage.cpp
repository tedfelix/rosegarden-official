/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2017 the Rosegarden development team.
 
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
#include "misc/ConfigGroups.h"
#include "base/MidiProgram.h"
#include "base/Studio.h"
#include "ConfigurationPage.h"
#include "document/RosegardenDocument.h"
#include "gui/dialogs/ShowSequencerStatusDialog.h"
#include "gui/seqmanager/SequenceManager.h"
#include "gui/application/RosegardenApplication.h"
#include "gui/studio/StudioControl.h"
#include "sound/MappedEvent.h"
#include "TabbedConfigurationPage.h"
#include "misc/Strings.h"
#include "misc/Debug.h"
#include "gui/widgets/LineEdit.h"
#include "gui/widgets/FileDialog.h"

#include <QComboBox>
#include <QSettings>
#include <QCheckBox>
#include <QComboBox>
#include <QByteArray>
#include <QDataStream>
#include <QFrame>
#include <QLabel>
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
    RosegardenDocument *doc,
    QWidget *parent):
    TabbedConfigurationPage(parent),
    m_externalAudioEditorPath(0)
{
    // set the document in the super class
    m_doc = doc;

    QSettings settings;

    QFrame *frame = new QFrame(m_tabWidget);
    frame->setContentsMargins(10, 10, 10, 10);
    QGridLayout *layout = new QGridLayout(frame);
    layout->setSpacing(5);

    int row = 0;

    settings.beginGroup( GeneralOptionsConfigGroup );

    layout->setRowMinimumHeight(row, 15);
    ++row;

    layout->addWidget(new QLabel(tr("Audio preview scale"),
                                 frame), row, 0);

    m_previewStyle = new QComboBox(frame);
    connect(m_previewStyle, SIGNAL(activated(int)), this, SLOT(slotModified()));
    m_previewStyle->addItem(tr("Linear - easier to see loud peaks"));
    m_previewStyle->addItem(tr("Meter scaling - easier to see quiet activity"));
    m_previewStyle->setCurrentIndex( settings.value("audiopreviewstyle", 1).toUInt() );
    layout->addWidget(m_previewStyle, row, 1, row- row+1, 2);
    ++row;

    settings.endGroup();

#ifdef HAVE_LIBJACK
    settings.beginGroup( SequencerOptionsConfigGroup );

    QLabel *label = new QLabel(tr("Record audio files as"), frame);
    m_audioRecFormat = new QComboBox(frame);
    connect(m_audioRecFormat, SIGNAL(activated(int)), this, SLOT(slotModified()));
    m_audioRecFormat->addItem(tr("16-bit PCM WAV format (smaller files)"));
    m_audioRecFormat->addItem(tr("32-bit float WAV format (higher quality)"));
    m_audioRecFormat->setCurrentIndex( settings.value("audiorecordfileformat", 1).toUInt() );
    layout->addWidget(label, row, 0);
    layout->addWidget(m_audioRecFormat, row, 1, row- row+1, 2);
    ++row;

    settings.endGroup();

#endif
    settings.beginGroup( GeneralOptionsConfigGroup );

    layout->addWidget(new QLabel(tr("External audio editor"), frame),
                      row, 0);

    QString defaultAudioEditor = getBestAvailableAudioEditor();

    std::cerr << "defaultAudioEditor = " << defaultAudioEditor << std::endl;

    QString externalAudioEditor = settings.value("externalaudioeditor",
                                  defaultAudioEditor).toString() ;

    if (externalAudioEditor == "") {
        externalAudioEditor = defaultAudioEditor;
        settings.setValue("externalaudioeditor", externalAudioEditor);
    }

    m_externalAudioEditorPath = new LineEdit(externalAudioEditor, frame);
    connect(m_externalAudioEditorPath, SIGNAL(textChanged(const QString &)), this, SLOT(slotModified()));
//    m_externalAudioEditorPath->setMinimumWidth(150);
    layout->addWidget(m_externalAudioEditorPath, row, 1);
    
    QPushButton *changePathButton =
        new QPushButton(tr("Choose..."), frame);

    layout->addWidget(changePathButton, row, 2);
    connect(changePathButton, SIGNAL(clicked()), SLOT(slotFileDialog()));
    ++row;

    settings.endGroup();

    layout->addWidget(new QLabel(tr("Create JACK outputs"), frame),
                      row, 0);
//    ++row;

#ifdef HAVE_LIBJACK
    settings.beginGroup( SequencerOptionsConfigGroup );

    m_createFaderOuts = new QCheckBox(tr("for individual audio instruments"), frame);
    connect(m_createFaderOuts, SIGNAL(stateChanged(int)), this, SLOT(slotModified()));
    m_createFaderOuts->setChecked( qStrToBool( settings.value("audiofaderouts", "false" ) ) );

//    layout->addWidget(label, row, 0, Qt::AlignRight);
    layout->addWidget(m_createFaderOuts, row, 1);
    ++row;

    m_createSubmasterOuts = new QCheckBox(tr("for submasters"), frame);
    connect(m_createSubmasterOuts, SIGNAL(stateChanged(int)), this, SLOT(slotModified()));
    m_createSubmasterOuts->setChecked( qStrToBool( settings.value("audiosubmasterouts", "false" ) ) );

//    layout->addWidget(label, row, 0, Qt::AlignRight);
    layout->addWidget(m_createSubmasterOuts, row, 1);
    ++row;

    settings.endGroup();
#endif

    ++row;
    layout->addWidget(new QLabel(tr("Make default JACK connections for"),frame),
                      row, 0);

#ifdef HAVE_LIBJACK
    settings.beginGroup(SequencerOptionsConfigGroup);

    m_connectDefaultAudioOutputs = new QCheckBox(tr("audio outputs"));
    connect(m_connectDefaultAudioOutputs, SIGNAL(stateChanged(int)), this, SLOT(slotModified()));
    m_connectDefaultAudioOutputs->setChecked(qStrToBool(settings.value("connect_default_jack_outputs", "true")));
    layout->addWidget(m_connectDefaultAudioOutputs, row, 1);
    ++row;

    m_connectDefaultAudioInputs = new QCheckBox(tr("audio inputs"));
    connect(m_connectDefaultAudioInputs, SIGNAL(stateChanged(int)), this, SLOT(slotModified()));
    m_connectDefaultAudioInputs->setChecked(qStrToBool(settings.value("connect_default_jack_inputs", "true")));
    layout->addWidget(m_connectDefaultAudioInputs, row, 1);
    ++row;

    layout->addWidget(new QLabel(tr("Start JACK automatically"),frame), row, 0);
    m_autoStartJackServer = new QCheckBox();
    connect(m_autoStartJackServer, SIGNAL(stateChanged(int)), this, SLOT(slotModified()));
    m_autoStartJackServer->setChecked(settings.value("autostartjack", "true").toBool());
    layout->addWidget(m_autoStartJackServer, row, 1);
    ++row;

    settings.endGroup();
#endif

    layout->setRowStretch(row, 10);

    addTab(frame, tr("General"));
}

void
AudioConfigurationPage::slotFileDialog()
{
    QString path = FileDialog::getOpenFileName(this, tr("External audio editor path"), QDir::currentPath() );

    m_externalAudioEditorPath->setText(path);
}

void
AudioConfigurationPage::apply()
{
    QSettings settings;
    settings.beginGroup( SequencerOptionsConfigGroup );

#ifdef HAVE_LIBJACK
    // Jack audio inputs
    //
    settings.setValue("audiofaderouts", m_createFaderOuts->isChecked());
    settings.setValue("audiosubmasterouts", m_createSubmasterOuts->isChecked());
    settings.setValue("audiorecordfileformat", m_audioRecFormat->currentIndex());
    settings.setValue("connect_default_jack_outputs", m_connectDefaultAudioOutputs->isChecked());
    settings.setValue("connect_default_jack_inputs", m_connectDefaultAudioInputs->isChecked());
    settings.setValue("autostartjack", m_autoStartJackServer->isChecked());
#endif

    settings.endGroup();
    settings.beginGroup( GeneralOptionsConfigGroup );

    int previewstyle = m_previewStyle->currentIndex();
    settings.setValue("audiopreviewstyle", previewstyle);

    QString externalAudioEditor = getExternalAudioEditor();

    QStringList extlist = externalAudioEditor.split(" ", QString::SkipEmptyParts);
    QString extpath = "";
    if (extlist.size() > 0) extpath = extlist[0];

    if (extpath != "") {
        QFileInfo info(extpath);
        if (!info.exists() || !info.isExecutable()) {
            QMessageBox::critical(0, tr("Rosegarden"), tr("External audio editor \"%1\" not found or not executable").arg(extpath));
            settings.setValue("externalaudioeditor", "");
        } else {
            settings.setValue("externalaudioeditor", externalAudioEditor);
        }
    } else {
        settings.setValue("externalaudioeditor", "");
    }
    settings.endGroup();
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

    for (size_t i = 0;
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

