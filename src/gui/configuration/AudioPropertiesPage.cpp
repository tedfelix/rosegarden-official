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
#include "gui/widgets/FileDialog.h"
#include "document/RosegardenDocument.h"
#include "sequencer/RosegardenSequencer.h"

#include <QFrame>
#include <QGridLayout>
#include <QLabel>
#include <QMessageBox>
#include <QPushButton>
#include <QTabWidget>

#include <sys/statvfs.h>


namespace Rosegarden
{


AudioPropertiesPage::AudioPropertiesPage(QWidget *parent) :
    TabbedConfigurationPage(parent)
{
    QFrame *frame = new QFrame(m_tabWidget);
    frame->setContentsMargins(10, 10, 10, 10);

    QGridLayout *layout = new QGridLayout(frame);
    layout->setSpacing(5);

    // Audio file path
    layout->addWidget(new QLabel(tr("Audio file path:"), frame), 0, 0);

    AudioFileManager &afm = m_doc->getAudioFileManager();
    m_path = new QLabel(afm.getAudioPath(), frame);
    layout->addWidget(m_path, 0, 1);

    m_changePathButton = new QPushButton(tr("Choose..."), frame);
    layout->addWidget(m_changePathButton, 0, 2);

    // Disk space remaining
    m_diskSpace = new QLabel(frame);
    layout->addWidget(new QLabel(tr("Disk space remaining:"), frame), 1, 0);
    layout->addWidget(m_diskSpace, 1, 1, 1, 2);

    // Recording time
    m_minutesAtStereo = new QLabel(frame);
    layout->addWidget(new QLabel(tr("Recording time:"),
                                 frame),
                      2, 0);

    layout->addWidget(m_minutesAtStereo, 2, 1, 1, 2);

    layout->setRowStretch(3, 2);
    frame->setLayout(layout);

    updateWidgets();

    connect(m_changePathButton, &QAbstractButton::released,
            this, &AudioPropertiesPage::slotChoosePath);

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

    // Disk space remaining

    struct statvfs buf;

    if (statvfs(m_path->text().toLocal8Bit().data(), &buf)) {
        RG_WARNING << "statvfs() failed.  errno:" << errno;
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
AudioPropertiesPage::slotChoosePath()
{
    while (true)
    {
        QString selectedPath = FileDialog::getExistingDirectory(
                this, tr("Choose Audio File Path"), m_path->text());

        if (selectedPath.isEmpty())
            return;

        QFileInfo fileInfo(selectedPath);

        // If the directory cannot be written to, let the user try again.
        if (!fileInfo.isWritable()) {
            QMessageBox::information(
                    this,
                    tr("Choose Audio File Path"),
                    tr("Selected path cannot be written to.  Please try another."));

            // Try again.
            continue;
        }

        // ??? Should we support relative paths (./audio) so that it is
        //     safe to move/copy the rg project directory?
        //     Actually, the handling for the audiopath tag in
        //     RoseXmlHandler assumes "./" if there is no ~ or / at
        //     the beginning of the path.

        m_path->setText(selectedPath);

        updateWidgets();

        return;
    }
}

void
AudioPropertiesPage::apply()
{
    AudioFileManager &audioFileManager = m_doc->getAudioFileManager();

    const QString newPath = m_path->text();

    // If there's been a change...
    if (newPath != audioFileManager.getAudioPath()) {

        bool moveFiles = false;

        if (!audioFileManager.empty()) {
            QMessageBox::StandardButton result = QMessageBox::question(
                    this,
                    tr("Choose Audio Path"),
                    tr("Would you like to move the document's audio files to the new path?  Please note that this will force a save of the file."));

            moveFiles = (result == QMessageBox::Yes);
        }

        // Update the AudioFileManager.
        audioFileManager.setAudioPath(newPath, moveFiles);

        // Moving the files will force a save.  Only needed if files
        // aren't moving.
        if (!moveFiles)
            m_doc->slotDocumentModified();
    }
}


}
