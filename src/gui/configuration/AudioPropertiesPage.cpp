/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2022 the Rosegarden development team.

    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#define RG_MODULE_STRING "[AudioPropertiesPage]"

#include "AudioPropertiesPage.h"

#include "sound/AudioFileManager.h"
#include "misc/Debug.h"
#include "gui/widgets/LineEdit.h"
#include "document/RosegardenDocument.h"
#include "sequencer/RosegardenSequencer.h"

#include <QComboBox>
#include <QDir>
#include <QFileInfo>
#include <QFrame>
#include <QGridLayout>
#include <QLabel>
#include <QMessageBox>
#include <QTabWidget>

#include <sys/statvfs.h>


namespace Rosegarden
{


AudioPropertiesPage::AudioPropertiesPage(QWidget *parent) :
    TabbedConfigurationPage(parent),
    m_docAbsFilePath(RosegardenDocument::currentDocument->getAbsFilePath())
{
    QFileInfo fileInfo(m_docAbsFilePath);
    m_documentNameDir = "./" + fileInfo.completeBaseName();

    AudioFileManager &afm = m_doc->getAudioFileManager();
    m_relativeAudioPath = afm.getRelativeAudioPath();

    QFrame *frame = new QFrame(m_tabWidget);
    frame->setContentsMargins(10, 10, 10, 10);

    QGridLayout *layout = new QGridLayout(frame);
    layout->setSpacing(5);

    int row = 0;

    // Audio file location
    layout->addWidget(new QLabel(tr("Audio file location:"), frame),
                      row, 0);

    // If the document has been saved, put up the combobox.
    if (m_docAbsFilePath != "") {
        m_audioFileLocation = new QComboBox(frame);
        connect(m_audioFileLocation,
                    static_cast<void(QComboBox::*)(int)>(&QComboBox::activated),
                this, &AudioPropertiesPage::slotModified);
        // Make sure these match the names in AudioFileLocationDialog.
        // See AudioFileLocationDialog::Location enum.
        m_audioFileLocation->addItem(tr("Audio directory (%1)").arg("./audio"));
        m_audioFileLocation->addItem(tr("Document name directory (%1)").arg(m_documentNameDir));
        m_audioFileLocation->addItem(tr("Document directory (.)"));
        m_audioFileLocation->addItem(tr("Central repository (%1)").arg("~/rosegarden-audio"));
        m_audioFileLocation->addItem(tr("Custom audio file location (specify below)"));
        layout->addWidget(m_audioFileLocation, row, 1);
    } else {
        m_audioFileLocation = nullptr;
        layout->addWidget(new QLabel(tr("Save document first."), frame),
                          row, 1);
    }

    ++row;

    // Custom audio file location
    layout->addWidget(new QLabel(tr("Custom audio file location:"), frame),
                      row, 0);

    // If the document has been saved, put up the combobox.
    if (m_docAbsFilePath != "") {
        m_customAudioLocation = new LineEdit(frame);
        // If we haven't saved yet, disable this.
        // ??? Or should we hide it and show an explanatory label here?
        if (m_docAbsFilePath == "")
            m_customAudioLocation->setEnabled(false);
        connect(m_customAudioLocation, &QLineEdit::textChanged,
                this, &AudioPropertiesPage::slotModified);
        layout->addWidget(m_customAudioLocation, row, 1);
    } else {
        m_customAudioLocation = nullptr;
        layout->addWidget(new QLabel(tr("Save document first."), frame),
                          row, 1);
    }

    ++row;

    // Disk space remaining
    m_diskSpace = new QLabel(frame);
    layout->addWidget(new QLabel(tr("Disk space remaining:"), frame), row, 0);
    layout->addWidget(m_diskSpace, row, 1, 1, 2);

    ++row;

    // Recording time
    m_minutesAtStereo = new QLabel(frame);
    layout->addWidget(new QLabel(tr("Recording time:"),
                                 frame),
                      row, 0);

    layout->addWidget(m_minutesAtStereo, row, 1, 1, 2);

    ++row;

    layout->setRowStretch(row, 2);

    updateWidgets();

    addTab(frame, tr("Modify audio path"));
}

void
AudioPropertiesPage::updateWidgets()
{
#if 0
    // Windoze version that needs rewriting for the newer compiler.

    ULARGE_INTEGER available1, total1, totalFree1;
    if (GetDiskFreeSpaceExA(m_path->text().toLocal8Bit().data(),
                            &available, &total, &totalFree)) {
        unsigned long long available = available1.QuadPart;
        unsigned long long total = total1.QuadPart;
    } else {
        std::cerr << "WARNING: GetDiskFreeSpaceEx failed: error code "
                  << GetLastError() << std::endl;
    }
#endif

    // If we can change the path...
    if (m_audioFileLocation  &&  m_customAudioLocation) {
        // ??? Why do this on every update?  Why not move to ctor?
        if (m_relativeAudioPath.isEmpty())
            m_relativeAudioPath = ".";
        // ??? Why do this on every update?  Why not move to ctor?
        if (m_relativeAudioPath.endsWith('/'))
            m_relativeAudioPath.chop(1);

        m_customAudioLocation->setText("");
        // See AudioFileLocationDialog::Location enum.
        if (m_relativeAudioPath == "./audio") {
            m_audioFileLocation->setCurrentIndex(0);
        } else if (m_relativeAudioPath == m_documentNameDir) {
            m_audioFileLocation->setCurrentIndex(1);
        } else if (m_relativeAudioPath == ".") {
            m_audioFileLocation->setCurrentIndex(2);
        } else if (m_relativeAudioPath == "~/rosegarden-audio") {
            m_audioFileLocation->setCurrentIndex(3);
        } else {
            m_audioFileLocation->setCurrentIndex(4);
            m_customAudioLocation->setText(m_relativeAudioPath);
        }
    }

    // Disk space remaining

    struct statvfs buf;

    // ??? Go with the drive the document is on.  Or do an AFM::toAbsolute()
    //     on this path and use that.
    if (statvfs(m_docAbsFilePath.toLocal8Bit().data(), &buf)) {
        RG_WARNING << "statvfs(" << m_docAbsFilePath << ") failed.  errno:" << errno;
        m_diskSpace->setText("----");
        m_minutesAtStereo->setText("----");
        return;
    }

    // Use 64-bit math to handle gigantic drives in the future.
    const unsigned long long available =
            (unsigned long long)buf.f_bavail * (unsigned long long)buf.f_bsize;
    const unsigned long long total =
            (unsigned long long)buf.f_blocks * (unsigned long long)buf.f_bsize;

    const unsigned long long mebibytes = 1024 * 1024;

    const double percentUsed =
            100 - (long double)available / (long double)total * 100;

    m_diskSpace->setText(tr("%1 MiB out of %2 MiB (%3% used)").
            arg(QLocale().toString(double(available / mebibytes), 'f', 0)).
            arg(QLocale().toString(double(total / mebibytes), 'f', 0)).
            arg(QLocale().toString(percentUsed, 'f', 1)));

    // Recording time

    unsigned int sampleRate =
            RosegardenSequencer::getInstance()->getSampleRate();

    // No sample rate?  Pick something typical.
    if (sampleRate == 0)
        sampleRate = 48000;

    constexpr int numberOfChannels = 2;
    constexpr int bytesPerSample = 2;  // 16-bits
    constexpr double secondsToMinutes = 1/60.0;

    const double stereoMins =
            double(available / sampleRate / numberOfChannels / bytesPerSample) *
            secondsToMinutes;

    m_minutesAtStereo->setText(tr("%1 minutes at %3Hz 16-bit stereo").
            arg(QLocale().toString(stereoMins, 'f', 1)).
            arg(QLocale().toString((double)sampleRate, 'f', 0)));
}

void
AudioPropertiesPage::apply()
{
    // If we can't change the path, bail.
    if (!m_audioFileLocation  ||  !m_customAudioLocation)
        return;

    AudioFileManager &audioFileManager = m_doc->getAudioFileManager();

    QString newPath;

    // See AudioFileLocationDialog::Location enum.
    switch (m_audioFileLocation->currentIndex()) {
    case 0: // AudioFileLocationDialog::AudioDir
        newPath = "./audio";
        break;
    case 1: // AudioFileLocationDialog::DocumentNameDir
        newPath = m_documentNameDir;
        break;
    case 2: // AudioFileLocationDialog::DocumentDir
        newPath = ".";
        break;
    case 3: //AudioFileLocationDialog::CentralDir
        newPath = "~/rosegarden-audio";
        break;
    case 4: //AudioFileLocationDialog::CustomDir
        newPath = m_customAudioLocation->text();
        break;
    }

    // If there's been a change...
    if (newPath != m_relativeAudioPath) {

        bool moveFiles = false;

        // If there are audio files...
        if (!audioFileManager.empty()) {
            QMessageBox::information(
                    this,
                    tr("Change Audio Path"),
                    tr("Document's audio files will now be moved to the new location.<br />Please note that this will force a save of the file."));

            moveFiles = true;
        }
        // Update the AudioFileManager.
        audioFileManager.setRelativeAudioPath(newPath, moveFiles);

        // Avoid showing the prompt when they save.
        // Since we don't store this flag in the document, they can
        // still save, exit, open, create audio, save and get the prompt.
        // It might even be surprising if they have the defaults set to
        // something different along with "don't show".  If someone
        // complains, we can add the flag to the document.
        audioFileManager.setAudioLocationConfirmed();

        // Moving the files will force a save.  Set the document modified
        // flag only if files aren't moving.
        if (!moveFiles)
            m_doc->slotDocumentModified();

    }
}


}
