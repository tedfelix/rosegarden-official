/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2016 the Rosegarden development team.
 
    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.
 
    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/


#include "AudioPropertiesPage.h"
#include "misc/Strings.h"
#include "ConfigurationPage.h"
#include "document/RosegardenDocument.h"
#include "sequencer/RosegardenSequencer.h"
#include "gui/studio/AudioPluginManager.h"
#include "gui/general/FileSource.h"
#include "sound/AudioFileManager.h"
#include "TabbedConfigurationPage.h"
#include "gui/widgets/FileDialog.h"

#include <QSettings>
#include <QFile>
#include <QByteArray>
#include <QDataStream>
#include <QFrame>
#include <QLabel>
#include <QPushButton>
#include <QString>
#include <QTabWidget>
#include <QWidget>
#include <QLayout>

#include <stdint.h>

#ifndef WIN32
#include <sys/statvfs.h>
#endif



namespace Rosegarden
{

AudioPropertiesPage::AudioPropertiesPage(RosegardenDocument *doc,  QWidget *parent)
        : TabbedConfigurationPage(doc, parent)
{
    AudioFileManager &afm = doc->getAudioFileManager();

    QFrame *frame = new QFrame(m_tabWidget);
    frame->setContentsMargins(10, 10, 10, 10);
    QGridLayout *layout = new QGridLayout(frame);
    layout->setSpacing(5);
    layout->addWidget(new QLabel(tr("Audio file path:"), frame), 0, 0);
    m_path = new QLabel(afm.getAudioPath(), frame);
    layout->addWidget(m_path, 0, 1);

    m_changePathButton =
        new QPushButton(tr("Choose..."), frame);

    layout->addWidget(m_changePathButton, 0, 2);

    m_diskSpace = new QLabel(frame);
    layout->addWidget(new QLabel(tr("Disk space remaining:"), frame), 1, 0);
    layout->addWidget(m_diskSpace, 1, 1);

    m_minutesAtStereo = new QLabel(frame);
    layout->addWidget(
        new QLabel(tr("Equivalent minutes of 16-bit stereo:"),
                   frame), 2, 0);

    layout->addWidget(m_minutesAtStereo, 2, 1, Qt::AlignCenter);

    layout->setRowStretch(3, 2);
    frame->setLayout(layout);

    calculateStats();

    connect(m_changePathButton, SIGNAL(released()),
            SLOT(slotFileDialog()));

    addTab(frame, tr("Modify audio path"));
}

void
AudioPropertiesPage::calculateStats()
{
#ifdef WIN32
    ULARGE_INTEGER available, total, totalFree;
    if (GetDiskFreeSpaceExA(m_path->text().toLocal8Bit().data(),
                            &available, &total, &totalFree)) {
        __int64 a = available.QuadPart;
        __int64 t = total.QuadPart;
        __int64 u = 0;
        if (t > a) u = t - a;
        slotFoundMountPoint(m_path->text(), t / 1024, u / 1024, a / 1024);
    } else {
        std::cerr << "WARNING: GetDiskFreeSpaceEx failed: error code "
                  << GetLastError() << std::endl;
    }
#else
    struct statvfs buf;
    if (!statvfs(m_path->text().toLocal8Bit().data(), &buf)) {
        // do the multiplies and divides in this order to reduce the
        // likelihood of arithmetic overflow
        std::cerr << "statvfs(" << m_path->text().toLocal8Bit().data() << ") says available: " << buf.f_bavail << ", total: " << buf.f_blocks << ", block size: " << buf.f_bsize << std::endl;
        uint64_t available = ((buf.f_bavail / 1024) * buf.f_bsize);
        uint64_t total = ((buf.f_blocks / 1024) * buf.f_bsize);
        uint64_t used = 0;
        if (total > available) used = total - available;
        slotFoundMountPoint(m_path->text(), total, used, available);
    } else {
        perror("statvfs failed");
    }
#endif
}

void
AudioPropertiesPage::slotFoundMountPoint(const QString&,
        unsigned long kBSize,
        unsigned long /*kBUsed*/,
        unsigned long kBAvail )
{
    m_diskSpace->setText(tr("%1 kB out of %2 kB (%3% kB used)")
                          //KIO::convertSizeFromKB
              .arg(kBAvail)
                          //KIO::convertSizeFromKB
              .arg(kBSize)
                          .arg(100 - (int)(100.0 * kBAvail / kBSize) ));


    // AudioPluginManager *apm = m_doc->getPluginManager();

    int sampleRate = RosegardenSequencer::getInstance()->getSampleRate();

    // Work out total bytes and divide this by the sample rate times the
    // number of channels (2) times the number of bytes per sample (2)
    // times 60 seconds.
    //
    float stereoMins = ( float(kBAvail) * 1024.0 ) /
                       ( float(sampleRate) * 2.0 * 2.0 * 60.0 );
    QString minsStr;
    minsStr.sprintf("%8.1f", stereoMins);

    m_minutesAtStereo->
    setText(QString("%1 %2 %3Hz").arg(minsStr)
            .arg(tr("minutes at"))
            .arg(sampleRate));
}

void
AudioPropertiesPage::slotFileDialog()
{
    AudioFileManager &afm = m_doc->getAudioFileManager();

    QString selectedDirectory = FileDialog::getExistingDirectory(this, tr("Audio Recording Path"), m_path->text());

    if (!selectedDirectory.isEmpty()) {
        m_path->setText(selectedDirectory);
    }
    calculateStats();
}

void
AudioPropertiesPage::apply()
{
    AudioFileManager &afm = m_doc->getAudioFileManager();
    QString oldDir = afm.getAudioPath();
    QString newDir = m_path->text();

    if (newDir != oldDir) {
        afm.setAudioPath(newDir);
        m_doc->slotDocumentModified();
    }
}

}
