// -*- c-basic-offset: 4 -*-

/*
    Rosegarden-4 v0.1
    A sequencer and musical notation editor.

    This program is Copyright 2000-2002
        Guillaume Laurent   <glaurent@telegraph-road.org>,
        Chris Cannam        <cannam@all-day-breakfast.com>,
        Richard Bown        <bownie@bownie.com>

    The moral right of the authors to claim authorship of this work
    has been asserted.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

/*
 * Code borrowed from KDE KMail : configuredialog*.h
 * Copyright (C) 2000 The KMail Development Team
 */

#include <qspinbox.h>
#include <qslider.h>
#include <qcombobox.h>
#include <qlayout.h>
#include <qtabwidget.h>
#include <qlabel.h>
#include <qfiledialog.h>
#include <qpushbutton.h>

#include <klocale.h>
#include <kiconloader.h>

#include "rosestrings.h"
#include "rosegardenconfiguredialog.h"
#include "rosegardenguidoc.h"
#include "Composition.h"
#include "Configuration.h"
#include "RealTime.h"
#include "rosedebug.h"

namespace Rosegarden
{

//
//
// basic ConfigurationPage (inherit pages from this)
//
//

TabbedConfigurationPage::TabbedConfigurationPage(RosegardenGUIDoc *doc,
                                                 QWidget *parent,
                                                 const char *name)
  : ConfigurationPage(doc, parent, name)
{
  QVBoxLayout *vlay = new QVBoxLayout(this, 0, KDialog::spacingHint());
  m_tabWidget = new QTabWidget(this);
  vlay->addWidget(m_tabWidget);
}

void TabbedConfigurationPage::addTab(QWidget *tab, const QString &title)
{
  m_tabWidget->addTab(tab, title);
}

//------------------------------------------------------------

class GeneralConfigurationPage : public TabbedConfigurationPage
{
public:
    GeneralConfigurationPage(RosegardenGUIDoc *doc,
                             QWidget *parent=0, const char *name=0);

    virtual void apply();

    static QString iconLabel() { return i18n("General"); }
    static QString title() { return i18n("General Configuration"); }

    int getCountInSpin()    { return m_countIn->value(); }
    int getDblClickClient() { return m_client->currentItem(); }
    
protected:
    QComboBox* m_client;
    QSpinBox* m_countIn;
};

GeneralConfigurationPage::GeneralConfigurationPage(RosegardenGUIDoc *doc,
                                                   QWidget *parent, const char *name)
    : TabbedConfigurationPage(doc, parent, name),
      m_client(0),
      m_countIn(0)
{
    Rosegarden::Composition &comp = doc->getComposition();
    Rosegarden::Configuration &config = doc->getConfiguration();

    QFrame *frame = new QFrame(m_tabWidget);
    QGridLayout *layout = new QGridLayout(frame, 2, 2,
                                          10, 5);

    layout->addWidget(new QLabel(i18n("Double click on segment opens..."), frame), 0, 0);
    layout->addWidget(new QLabel(i18n("Number of count-in bars when recording"), frame), 1, 0);

    m_client = new QComboBox(frame);
    m_client->insertItem(i18n("Notation"));
    m_client->insertItem(i18n("Matrix"));
    m_client->setCurrentItem(config.getDoubleClickClient());

    layout->addWidget(m_client, 0, 1);

    m_countIn = new QSpinBox(frame);
    m_countIn->setValue(comp.getCountInBars());
    m_countIn->setMaxValue(10);
    m_countIn->setMinValue(0);

    layout->addWidget(m_countIn, 1, 1);

    addTab(frame, i18n("General"));
}

void GeneralConfigurationPage::apply()
{
    Rosegarden::Composition &comp = m_doc->getComposition();
    Rosegarden::Configuration &config = m_doc->getConfiguration();

    int countIn = getCountInSpin();
    comp.setCountInBars(countIn);

    int client = getDblClickClient();
    config.setDoubleClickClient(
                                (Rosegarden::Configuration::DoubleClickClient)client);

}


class PlaybackConfigurationPage : public TabbedConfigurationPage
{
public:
    PlaybackConfigurationPage(RosegardenGUIDoc *doc,
                              QWidget *parent=0, const char *name=0);

    virtual void apply();

    static QString iconLabel() { return i18n("Playback"); }
    static QString title()     { return i18n("Sequencer and Playback"); }

    int getReadAheadValue() { return m_readAhead->value(); }
    int getPlaybackValue()  { return m_playback->value(); }

protected:
    QSlider* m_readAhead;
    QSlider* m_playback;
};

PlaybackConfigurationPage::PlaybackConfigurationPage(RosegardenGUIDoc *doc,
                                                     QWidget *parent,
                                                     const char *name)
    : TabbedConfigurationPage(doc, parent, name),
      m_readAhead(0),
      m_playback(0)
{
    Rosegarden::Configuration &config = doc->getConfiguration();

    QFrame *frame = new QFrame(m_tabWidget);
    QGridLayout *layout = new QGridLayout(frame, 3, 2,
                                          10, 5);

    layout->addMultiCellWidget(new QLabel(i18n("Higher latency improves playback quality on slower systems\nbut reduces overall sequencer response."), frame),
                               0, 0,
                               0, 1);

    layout->addWidget(new QLabel(i18n("Read ahead (in ms)"), frame), 1, 0);
    layout->addWidget(new QLabel(i18n("Playback (in ms)"), frame), 2, 0);

    m_readAhead = new QSlider(Horizontal, frame);
    m_readAhead->setMinValue(20);
    m_readAhead->setMaxValue(80);
    m_readAhead->setValue(config.getReadAhead().usec / 1000);
    m_readAhead->setTickmarks(QSlider::Below);
    layout->addWidget(m_readAhead, 1, 1);

    m_playback = new QSlider(Horizontal, frame);
    m_playback->setMinValue(20);
    m_playback->setMaxValue(500);
    m_playback->setValue(config.getPlaybackLatency().usec / 1000);
    m_playback->setTickmarks(QSlider::Below);
    layout->addWidget(m_playback, 2, 1);

    addTab(frame, i18n("Latency"));
}

void PlaybackConfigurationPage::apply()
{
    Rosegarden::Configuration &config = m_doc->getConfiguration();

    int readAhead = getReadAheadValue();
    config.setReadAhead((RealTime(0, (readAhead * 1000))));

    int playback = getPlaybackValue();
    config.setPlaybackLatency((RealTime(0, (playback * 1000))));
}

AudioConfigurationPage::AudioConfigurationPage(RosegardenGUIDoc *doc,
                                               QWidget *parent,
                                               const char *name)
    : TabbedConfigurationPage(doc, parent, name),
    m_doc(doc),
    m_newDirectory("")
{
    Rosegarden::Configuration &config = doc->getConfiguration();
    Rosegarden::AudioFileManager &afm = doc->getAudioFileManager();

    QFrame *frame = new QFrame(m_tabWidget);
    QGridLayout *layout = new QGridLayout(frame, 3, 2,
                                          10, 5);
    layout->addWidget(new QLabel(i18n("Audio file path:"), frame), 1, 0);

    m_changePathButton =
        new QPushButton(i18n(QString(afm.getAudioPath().c_str())),
                        frame);

    layout->addWidget(m_changePathButton, 1, 1);

    connect(m_changePathButton, SIGNAL(released()),
            SLOT(slotFileDialog()));

    addTab(frame, i18n("Modify audio path"));
}

void
AudioConfigurationPage::slotFileDialog()
{
    Rosegarden::AudioFileManager &afm = m_doc->getAudioFileManager();

    QFileDialog *fileDialog = new QFileDialog(this, "file dialog", TRUE);
    fileDialog->setMode(QFileDialog::DirectoryOnly);
    fileDialog->setDir(QString(afm.getAudioPath().c_str()));

    connect(fileDialog, SIGNAL(fileSelected(const QString&)),
            SLOT(slotFileSelected(const QString&)));

    connect(fileDialog, SIGNAL(destroyed()),
            SLOT(slotDirectoryDialogClosed()));

    if (fileDialog->exec() == QDialog::Accepted)
    {
        m_newDirectory = fileDialog->selectedFile();
        m_changePathButton->setText(m_newDirectory);
    }
    delete fileDialog;
}

void
AudioConfigurationPage::apply()
{
    Rosegarden::AudioFileManager &afm = m_doc->getAudioFileManager();

    if (!m_newDirectory.isNull())
    {
        afm.setAudioPath(std::string(m_newDirectory.data()));
    }
}


//------------------------------------------------------------
static inline QPixmap loadIcon( const char * name ) {
  return KGlobal::instance()->iconLoader()
    ->loadIcon( QString::fromLatin1(name), KIcon::NoGroup, KIcon::SizeMedium );
}

RosegardenConfigureDialog::RosegardenConfigureDialog(RosegardenGUIDoc *doc,
                                                     QWidget *parent,
                                                     const char *name):
    KDialogBase(IconList, i18n("Configure"), Help|Apply|Ok|Cancel,
                Ok, parent, name, true), // modal
    m_doc(doc),
    m_generalConfigurationPage(0),
    m_playbackConfigurationPage(0)
{
  QWidget *page;
  QVBoxLayout *vlay;

  // General Page
  //
  page = addPage(GeneralConfigurationPage::iconLabel(),
                 GeneralConfigurationPage::title(),
                 loadIcon(GeneralConfigurationPage::iconName()));
  vlay = new QVBoxLayout(page, 0, spacingHint());
  m_generalConfigurationPage = new GeneralConfigurationPage(m_doc, page);
  vlay->addWidget(m_generalConfigurationPage);
  m_generalConfigurationPage->setPageIndex(pageIndex(page));

  // Playback Page
  //
  page = addPage(PlaybackConfigurationPage::iconLabel(),
                 PlaybackConfigurationPage::title(),
                 loadIcon(PlaybackConfigurationPage::iconName()));
  vlay = new QVBoxLayout(page, 0, spacingHint());
  m_playbackConfigurationPage = new PlaybackConfigurationPage(m_doc, page);
  vlay->addWidget(m_playbackConfigurationPage);
  m_playbackConfigurationPage->setPageIndex(pageIndex(page));

  // Audio Page
  //
  page = addPage(AudioConfigurationPage::iconLabel(),
                 AudioConfigurationPage::title(),
                 loadIcon(AudioConfigurationPage::iconName()));
  vlay = new QVBoxLayout(page, 0, spacingHint());
  m_audioConfigurationPage = new AudioConfigurationPage(m_doc, page);
  vlay->addWidget(m_audioConfigurationPage);
  m_audioConfigurationPage->setPageIndex(pageIndex(page));

}

RosegardenConfigureDialog::~RosegardenConfigureDialog()
{
}

void
RosegardenConfigureDialog::slotApply()
{
    m_generalConfigurationPage->apply();
    m_playbackConfigurationPage->apply();
    m_audioConfigurationPage->apply();
}


void
RosegardenConfigureDialog::slotActivateApply()
{
//     ApplyButton->setDisabled(false);
}

void
RosegardenConfigureDialog::slotOk()
{
    kdDebug(KDEBUG_AREA) << "RosegardenConfigureDialog::slotOK()\n";

    slotApply();
    accept();
}

void
RosegardenConfigureDialog::slotCancelOrClose()
{
}


}
