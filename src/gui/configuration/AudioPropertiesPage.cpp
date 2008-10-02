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


#include "AudioPropertiesPage.h"
#include "misc/Strings.h"
#include "ConfigurationPage.h"
#include "document/RosegardenGUIDoc.h"
#include "sequencer/RosegardenSequencer.h"
#include "gui/studio/AudioPluginManager.h"
#include "gui/general/FileSource.h"
#include "sound/AudioFileManager.h"
#include "TabbedConfigurationPage.h"

//#include <kdiskfreesp.h>
//#include <kdiskfreespace.h>	// note: a kde4 include

#include <QSettings>
#include <QFileDialog>
#include <QFile>
#include <QByteArray>
#include <QDataStream>
#include <QDialog>
#include <QFrame>
#include <QLabel>
#include <QPushButton>
#include <QString>
#include <QTabWidget>
#include <QWidget>
#include <QLayout>

#ifndef WIN32
#include <sys/statvfs.h>
#endif



namespace Rosegarden
{

AudioPropertiesPage::AudioPropertiesPage(RosegardenGUIDoc *doc,
        QWidget *parent,
        const char *name)
        : TabbedConfigurationPage(doc, parent, name)
{
    AudioFileManager &afm = doc->getAudioFileManager();

    QFrame *frame = new QFrame(m_tabWidget);
    QGridLayout *layout = new QGridLayout(frame, 4, 3, 10, 5);
    layout->addWidget(new QLabel(i18n("Audio file path:"), frame), 0, 0);
    m_path = new QLabel(QString(afm.getAudioPath().c_str()), frame);
    layout->addWidget(m_path, 0, 1);

    m_changePathButton =
        new QPushButton(i18n("Choose..."), frame);

    layout->addWidget(m_changePathButton, 0, 2);

    m_diskSpace = new QLabel(frame);
    layout->addWidget(new QLabel(i18n("Disk space remaining:"), frame), 1, 0);
    layout->addWidget(m_diskSpace, 1, 1);

    m_minutesAtStereo = new QLabel(frame);
    layout->addWidget(
        new QLabel(i18n("Equivalent minutes of 16-bit stereo:"),
                   frame), 2, 0);

    layout->addWidget(m_minutesAtStereo, 2, 1, Qt::AlignCenter);

    layout->setRowStretch(3, 2);

    calculateStats();

    connect(m_changePathButton, SIGNAL(released()),
            SLOT(slotFileDialog()));

    addTab(frame, i18n("Modify audio path"));
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
    m_diskSpace->setText(i18n("%1 kB out of %2 kB (%3% kB used)",
                          //KIO::convertSizeFromKB
								  (kBAvail),
                          //KIO::convertSizeFromKB
								   (kBSize),
                           100 - (int)(100.0 * kBAvail / kBSize) ));


    AudioPluginManager *apm = m_doc->getPluginManager();

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
            .arg(i18n("minutes at"))
            .arg(sampleRate));
}

void
AudioPropertiesPage::slotFileDialog()
{
    AudioFileManager &afm = m_doc->getAudioFileManager();

    QFileDialog *fileDialog = new QFileDialog(this, QString(afm.getAudioPath().c_str()),
                              "file dialog");
	fileDialog->setFileMode( QFileDialog::Directory );
	
    connect(fileDialog, SIGNAL(fileSelected(const QString&)),
            SLOT(slotFileSelected(const QString&)));

    connect(fileDialog, SIGNAL(destroyed()),
            SLOT(slotDirectoryDialogClosed()));

    if (fileDialog->exec() == QDialog::Accepted) {
        m_path->setText(fileDialog->selectedFile());
        calculateStats();
    }
    delete fileDialog;
}

void
AudioPropertiesPage::apply()
{
    AudioFileManager &afm = m_doc->getAudioFileManager();
    QString newDir = m_path->text();

    if (!newDir.isNull()) {
        afm.setAudioPath(qstrtostr(newDir));
        m_doc->slotDocumentModified();
    }
}

}
#include "AudioPropertiesPage.moc"
