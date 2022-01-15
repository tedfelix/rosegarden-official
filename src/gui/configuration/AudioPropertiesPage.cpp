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
#include <QPushButton>
#include <QTabWidget>

#include <stdint.h>
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
            this, &AudioPropertiesPage::slotChangePath);

    addTab(frame, tr("Modify audio path"));
}

void
AudioPropertiesPage::updateWidgets()
{
#if 0
    // Windoze version that needs rewriting for the newer compiler.

    ULARGE_INTEGER available, total, totalFree;
    if (GetDiskFreeSpaceExA(m_path->text().toLocal8Bit().data(),
                            &available, &total, &totalFree)) {
        __int64 a = available.QuadPart;
        __int64 t = total.QuadPart;
        __int64 u = 0;
        if (t > a) u = t - a;
        updateWidgets2(t / 1024, a / 1024);
    } else {
        std::cerr << "WARNING: GetDiskFreeSpaceEx failed: error code "
                  << GetLastError() << std::endl;
    }
#endif

    // Disk space remaining

    struct statvfs buf;

    if (statvfs(m_path->text().toLocal8Bit().data(), &buf)) {
        RG_WARNING << "statvfs() failed";
        m_diskSpace->setText("XXXX");
        m_minutesAtStereo->setText("XXXX");
        return;
    }

    // Use 64 bit math to handle gigantic drives in the future.
    const uint64_t available = (uint64_t)buf.f_bavail * (uint64_t)buf.f_bsize;
    const uint64_t total = (uint64_t)buf.f_blocks * (uint64_t)buf.f_bsize;

    const uint64_t mebibytes = 1024 * 1024;

    m_diskSpace->setText(tr("%1 MiB out of %2 MiB (%3% used)").
            arg(available / mebibytes).
            arg(total / mebibytes).
            arg(100 - lround((double)available / (double)total * 100.0) ));

    // Recording time

    int sampleRate = RosegardenSequencer::getInstance()->getSampleRate();

    // No sample rate?  Pick something typical.
    if (sampleRate == 0)
        sampleRate = 48000;

    constexpr int numberOfChannels = 2;
    constexpr int bytesPerSample = 2;  // 16-bits
    constexpr double secondsToMinutes = 1/60.0;

    const double stereoMins =
            double(available / sampleRate / numberOfChannels / bytesPerSample) *
            secondsToMinutes;
    const QString minsStr = QString::asprintf("%8.1f", stereoMins);

    m_minutesAtStereo->setText(
            QString("%1 %2 %3Hz 16-bit stereo").arg(minsStr).
                                  arg(tr("minutes at")).
                                  arg(sampleRate));
}

void
AudioPropertiesPage::slotChangePath()
{
    // ??? We need to also support directories that do not exist.
    QString selectedDirectory = FileDialog::getExistingDirectory(
            this, tr("Audio Recording Path"), m_path->text());

    if (selectedDirectory.isEmpty())
        return;

    // ??? If the directory doesn't exist, prompt to create it.

    // ??? If the directory cannot be created or written to, let the
    //     user know and leave.

    // ??? We also need to support relative paths (./audio) so that it is
    //     safe to move/copy the rg project directory.

    m_path->setText(selectedDirectory);

    updateWidgets();
}

void
AudioPropertiesPage::apply()
{
    AudioFileManager &afm = m_doc->getAudioFileManager();

    const QString newDir = m_path->text();

    // If there's been a change, update the AudioFileManager.
    if (newDir != afm.getAudioPath()) {
        afm.setAudioPath(newDir);
        m_doc->slotDocumentModified();
    }
}


}
