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


#include "LatencyConfigurationPage.h"
#include <QLayout>

#include "document/ConfigGroups.h"
#include "ConfigurationPage.h"
#include "document/RosegardenGUIDoc.h"
#include "TabbedConfigurationPage.h"
#include <kconfig.h>
#include <QFrame>
#include <QLabel>
#include <QPushButton>
#include <QSlider>
#include <QString>
#include <QTabWidget>
#include <QWidget>


namespace Rosegarden
{

LatencyConfigurationPage::LatencyConfigurationPage(RosegardenGUIDoc *doc,
        QSettings cfg,
        QWidget *parent,
        const char *name)
        : TabbedConfigurationPage(doc, cfg, parent, name)
{
    //     Configuration &config = doc->getConfiguration();
    QSettings m_cfg;
    m_cfg.beginGroup( LatencyOptionsConfigGroup );
    // 
    // FIX-manually-(GW), add:
    // m_cfg.endGroup();		// corresponding to: m_cfg.beginGroup( LatencyOptionsConfigGroup );
    //  


#ifdef NOT_DEFINED
#ifdef HAVE_LIBJACK

    frame = new QFrame(m_tabWidget, i18n("JACK latency"));
    layout = new QGridLayout(frame, 6, 5, 10, 10);

    layout->addMultiCellWidget(new QLabel(i18n("Use the \"Fetch JACK latencies\" button to discover the latency values set at\nthe sequencer.  It's recommended that you use the returned values but it's also\npossible to override them manually using the sliders.  Note that if you change\nyour JACK server parameters you should always fetch the latency values again.\nThe latency values will be stored by Rosegarden for use next time."), frame),
                               0, 0,
                               0, 3);

    layout->addWidget(new QLabel(i18n("JACK playback latency (in ms)"), frame), 1, 0);
    layout->addWidget(new QLabel(i18n("JACK record latency (in ms)"), frame), 3, 0);

    m_fetchLatencyValues = new QPushButton(i18n("Fetch JACK latencies"),
                                           frame);

    layout->addWidget(m_fetchLatencyValues, 1, 3);

    connect(m_fetchLatencyValues, SIGNAL(released()),
            SLOT(slotFetchLatencyValues()));

    int jackPlaybackValue = (m_cfg->readLongNumEntry(
                                 "jackplaybacklatencyusec", 0) / 1000) +
                            (m_cfg->readLongNumEntry(
                                 "jackplaybacklatencysec", 0) * 1000);

    m_jackPlayback = new QSlider(Horizontal, frame);
    m_jackPlayback->setTickPosition(QSlider::TicksBelow);
    layout->addWidget(m_jackPlayback, 3, 2, 1, 3- 3);

    QLabel *jackPlaybackLabel = new QLabel(QString("%1").arg(jackPlaybackValue),
                                           frame);
    layout->addWidget(jackPlaybackLabel, 2, 2, Qt::AlignHCenter);
    connect(m_jackPlayback, SIGNAL(valueChanged(int)),
            jackPlaybackLabel, SLOT(setNum(int)));

    m_jackPlayback->setMinimum(0);
    layout->addWidget(new QLabel("0", frame), 3, 1, Qt::AlignRight);

    m_jackPlayback->setMaximum(500);
    layout->addWidget(new QLabel("500", frame), 3, 4, Qt::AlignLeft);

    m_jackPlayback->setValue(jackPlaybackValue);

    int jackRecordValue = (m_cfg->readLongNumEntry(
                               "jackrecordlatencyusec", 0) / 1000) +
                          (m_cfg->readLongNumEntry(
                               "jackrecordlatencysec", 0) * 1000);

    m_jackRecord = new QSlider(Horizontal, frame);
    m_jackRecord->setTickPosition(QSlider::TicksBelow);
    layout->addWidget(m_jackRecord, 5, 2, 1, 3- 3);

    QLabel *jackRecordLabel = new QLabel(QString("%1").arg(jackRecordValue),
                                         frame);
    layout->addWidget(jackRecordLabel, 4, 2, Qt::AlignHCenter);
    connect(m_jackRecord, SIGNAL(valueChanged(int)),
            jackRecordLabel, SLOT(setNum(int)));

    m_jackRecord->setMinimum(0);
    layout->addWidget(new QLabel("0", frame), 5, 1, Qt::AlignRight);

    m_jackRecord->setMaximum(500);
    m_jackRecord->setValue(jackRecordValue);
    layout->addWidget(new QLabel("500", frame), 5, 4, Qt::AlignLeft);

    addTab(frame, i18n("JACK Latency"));
#endif  // HAVE_LIBJACK
#endif // NOT_DEFINED

}

void LatencyConfigurationPage::apply()
{
    QSettings m_cfg;
    m_cfg.beginGroup( LatencyOptionsConfigGroup );
    // 
    // FIX-manually-(GW), add:
    // m_cfg.endGroup();		// corresponding to: m_cfg.beginGroup( LatencyOptionsConfigGroup );
    //  


#ifdef HAVE_LIBJACK

    int jackPlayback = getJACKPlaybackValue();
    m_cfg.setValue("jackplaybacklatencysec", jackPlayback / 1000);
    m_cfg.setValue("jackplaybacklatencyusec", jackPlayback * 1000);

    int jackRecord = getJACKRecordValue();
    m_cfg.setValue("jackrecordlatencysec", jackRecord / 1000);
    m_cfg.setValue("jackrecordlatencyusec", jackRecord * 1000);

#endif  // HAVE_LIBJACK
}

void LatencyConfigurationPage::slotFetchLatencyValues()
{
    int jackPlaybackValue = m_doc->getAudioPlayLatency().msec() +
                            m_doc->getAudioPlayLatency().sec * 1000;

    m_jackPlayback->setValue(jackPlaybackValue);

    int jackRecordValue = m_doc->getAudioRecordLatency().msec() +
                          m_doc->getAudioRecordLatency().sec * 1000;
    m_jackRecord->setValue(jackRecordValue);
}

}
#include "LatencyConfigurationPage.moc"
