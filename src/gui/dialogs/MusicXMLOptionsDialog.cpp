/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2018 the Rosegarden development team.

    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#include "MusicXMLOptionsDialog.h"

#include "misc/ConfigGroups.h"
#include "document/RosegardenDocument.h"
#include "gui/configuration/HeadersConfigurationPage.h"
#include "misc/Strings.h"
#include "misc/Debug.h"

#include <QApplication>
#include <QCheckBox>
#include <QComboBox>
#include <QDialog>
#include <QDialogButtonBox>
#include <QUrl>
#include <QDesktopServices>
#include <QFrame>
#include <QGridLayout>
#include <QGroupBox>
#include <QLabel>
#include <QLayout>
#include <QSettings>
#include <QString>
#include <QTabWidget>
#include <QToolTip>
#include <QVBoxLayout>
#include <QWidget>
#include <QLocale>

#include <iostream>


namespace Rosegarden
{

MusicXMLOptionsDialog::MusicXMLOptionsDialog(QWidget *parent,
                                             RosegardenDocument *doc,
                                             QString windowCaption,
                                             QString /* heading */) :
                       QDialog(parent),
                       m_doc(doc)
{
    setModal(true);
    setWindowTitle((windowCaption == "" ? tr("MusicXML Export") : windowCaption));

    QGridLayout *metaGridLayout = new QGridLayout;

    QWidget *mainbox = new QWidget(this);
    QVBoxLayout *mainboxLayout = new QVBoxLayout;
    metaGridLayout->addWidget(mainbox, 0, 0);

    //
    // Arrange options in "Layout" and "Headers" tabs.
    //

    QTabWidget *tabWidget = new QTabWidget(mainbox);
    mainboxLayout->addWidget(tabWidget);

    QFrame *layoutFrame = new QFrame();
    tabWidget->addTab(layoutFrame, tr("Layout"));

    layoutFrame->setContentsMargins(5, 5, 5, 5);
    QGridLayout *layoutGrid = new QGridLayout;
    layoutGrid->setSpacing(5);

    m_headersPage = new HeadersConfigurationPage(this, m_doc);
    tabWidget->addTab(m_headersPage, tr("Headers"));

    //
    // MusicXML export: Basic options
    //

    QGroupBox *basicOptionsBox = new QGroupBox(tr("Basic options"), layoutFrame);
    QVBoxLayout *basicOptionsBoxLayout = new QVBoxLayout;

    layoutGrid->addWidget(basicOptionsBox, 0, 0);
    QFrame *frameBasic = new QFrame(basicOptionsBox);
    frameBasic->setContentsMargins(10, 10, 10, 10);
    QGridLayout *layoutBasic = new QGridLayout;
    layoutBasic->setSpacing(5);
    basicOptionsBoxLayout->addWidget(frameBasic);

    frameBasic->setMinimumWidth(250);

    layoutBasic->addWidget(new QLabel(tr("Export content"), frameBasic), 0, 0);
    m_mxmlExportSelection = new QComboBox(frameBasic);
    m_mxmlExportSelection->setToolTip(tr("<qt>Choose which tracks or segments to export</qt>"));
    m_mxmlExportSelection->addItem(tr("All tracks"));
    m_mxmlExportSelection->addItem(tr("Non-muted tracks"));
    m_mxmlExportSelection->addItem(tr("Selected track"));
    m_mxmlExportSelection->addItem(tr("Selected segments"));
    layoutBasic->addWidget(m_mxmlExportSelection, 0, 1);

    m_mxmlExportStaffGroup = new QCheckBox(tr("Export track staff brackets"), frameBasic);
    layoutBasic->addWidget(m_mxmlExportStaffGroup, 1, 0, 1, 2);
    m_mxmlExportStaffGroup->setToolTip(tr("<qt>Track staff brackets are found in the <b>Track Parameters</b> box, and may be used to group staffs in various ways</qt>"));

    //
    // MusicXML export: Advanced options
    //

    QGroupBox *specificOptionsBox = new QGroupBox(tr("Advanced/Experimental options"), layoutFrame);
    QVBoxLayout *specificOptionsBoxLayout = new QVBoxLayout;
    layoutGrid->addWidget(specificOptionsBox, 2, 0);

    QFrame *frameAdvanced = new QFrame(specificOptionsBox);
    frameAdvanced->setContentsMargins(10, 10, 10, 10);
    QGridLayout *layoutAdvanced = new QGridLayout;
    layoutAdvanced->setSpacing(5);
    specificOptionsBoxLayout->addWidget(frameAdvanced);

    // Music XML version/
    layoutAdvanced->addWidget(new QLabel(tr("Compatibility level"), frameBasic), 0, 0);
    m_mxmlVersion = new QComboBox(frameAdvanced);
    m_mxmlVersion->setToolTip(tr("<qt>Set the MusicXML version you want export</qt>"));
    m_mxmlVersion->addItem("MusicXML 1.1");
    m_mxmlVersion->addItem("MusicXML 2.0");
    m_mxmlVersion->setSizeAdjustPolicy(QComboBox::AdjustToContents);
    layoutAdvanced->addWidget(m_mxmlVersion, 0, 1);

    // part-wise or score-wise score.
    layoutAdvanced->addWidget(new QLabel(tr("MusicXML type"), frameBasic), 1, 0);
    m_mxmlDTDType = new QComboBox(frameBasic);
    m_mxmlDTDType->setToolTip(tr("<qt><p>Choose the format of the MusicXML file.</p><p>The &quot;partwise&quot; format contains &quot;part&quot; elements that contain &quot;measure&quot; elements.  The &quot;timewise&quot; format reverses this ordering.</p></qt>"));
    m_mxmlDTDType->addItem("partwise");
    m_mxmlDTDType->addItem("timewise");
    m_mxmlDTDType->setMinimumWidth(150);
    layoutAdvanced->addWidget(m_mxmlDTDType, 1, 1);

    layoutAdvanced->addWidget(new QLabel(tr("Export grand staff instrument"), frameAdvanced), 2, 0);
    m_mxmlMultiStave = new QComboBox(frameAdvanced);
    m_mxmlMultiStave->addItem(tr("None"));
    m_mxmlMultiStave->addItem("{[--  --]}");
    m_mxmlMultiStave->addItem("{--  --}");
    layoutAdvanced->addWidget(m_mxmlMultiStave, 2, 1);
    m_mxmlMultiStave->setToolTip(tr("<qt>Choose which bracket will create a grand staff system</qt>"));

    m_mxmlExportPercussion = new QComboBox(frameAdvanced);
    m_mxmlExportPercussion->addItem(tr("Don't export percussion tracks"));
    m_mxmlExportPercussion->addItem("Export percussion as notes");
    m_mxmlExportPercussion->addItem("Export percussion as percussion (experimental)");

    layoutAdvanced->addWidget(m_mxmlExportPercussion, 3, 0, 1, 2);
    m_mxmlExportPercussion->setToolTip(
        tr("<qt><p>Exporting percussion is still experimental.</p>Percussion can be exported &quot;as notes&quot; or &quot;as percussion.&quot;  When exporting &quot;as notes&quot; a percussion track is handled as a normal track.</p><p>If a track is exported &quot;as percussion&quot; it will be exported as a MusicXML percussion part. Since Rosegarden doesn't have percussion notation, MusicXML Export tries to convert the percussion track to percussion notation. This option is still <b>experimental</b>.</p></qt>"));

    m_mxmlUseOctaveShift = new QCheckBox(tr("Use \"<octave-shift>\" for transposing instruments"),
                                         frameAdvanced);
    layoutAdvanced->addWidget(m_mxmlUseOctaveShift, 4, 0, 1, 2);
    m_mxmlUseOctaveShift->setToolTip(
        tr("<qt><p>Some transposing instruments (like a tenor or baritone sax) transpose over more than one octave.  For such large transpositions some tools require an &quot;&lt;octave-shift&gt;&quot; element while other tools do not support this element at all.</p><p>When importing the MusicXML file into another tool, check transposing instruments carefully in both concert and notated pitch. When this is not correct toggling this option might help.</p></qt>"));

    layoutGrid->setRowStretch(4, 10);

    basicOptionsBox->setLayout(basicOptionsBoxLayout);
    specificOptionsBox->setLayout(specificOptionsBoxLayout);

    layoutFrame->setLayout(layoutGrid);

    frameAdvanced->setLayout(layoutAdvanced);
    frameBasic->setLayout(layoutBasic);

    mainbox->setLayout(mainboxLayout);

    QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Apply |
                                                       QDialogButtonBox::Ok |
                                                       QDialogButtonBox::Cancel |
                                                       QDialogButtonBox::Help);
    metaGridLayout->addWidget(buttonBox, 1, 0);
    metaGridLayout->setRowStretch(0, 10);

    setLayout(metaGridLayout);


    connect(buttonBox, SIGNAL(accepted()), this, SLOT(accept()));
    connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
    connect(buttonBox, &QDialogButtonBox::helpRequested, this, &MusicXMLOptionsDialog::help);

    populateDefaultValues();

    resize(minimumSizeHint());
}

void
MusicXMLOptionsDialog::help()
{
    // TRANSLATORS: if the manual is translated into your language, you can
    // change the two-letter language code in this URL to point to your language
    // version, eg. "http://rosegardenmusic.com/wiki/doc:manual-es" for the
    // Spanish version. If your language doesn't yet have a translation, feel
    // free to create one.
    QString helpURL = tr("http://rosegardenmusic.com/wiki/doc:manual-musicxmlexportoptions-en");
    QDesktopServices::openUrl(QUrl(helpURL));
}


void
MusicXMLOptionsDialog::populateDefaultValues()
{
    QSettings settings;
    settings.beginGroup(MusicXMLExportConfigGroup);

    m_mxmlVersion->setCurrentIndex(settings.value("mxmlversion", 0).toUInt());
    m_mxmlDTDType->setCurrentIndex(settings.value("mxmldtdtype", 0).toUInt());
    m_mxmlExportSelection->setCurrentIndex(
            settings.value("mxmlexportselection", 1).toUInt());
    m_mxmlExportStaffGroup->setChecked(settings.value(
            "mxmlexportstaffgroup", "true").toBool());
    m_mxmlExportPercussion->setCurrentIndex(settings.value(
            "mxmlexportpercussion", 0).toUInt());
    m_mxmlUseOctaveShift->setChecked(settings.value(
            "mxmluseoctaveshift", "true").toBool());
    m_mxmlMultiStave->setCurrentIndex(settings.value("mxmlmultistave", 0).toUInt());

    settings.endGroup();
}


void
MusicXMLOptionsDialog::slotApply()
{
    QSettings settings;
    settings.beginGroup(MusicXMLExportConfigGroup);

    settings.setValue("mxmlversion", m_mxmlVersion->currentIndex());
    settings.setValue("mxmldtdtype", m_mxmlDTDType->currentIndex());
    settings.setValue("mxmlexportselection", m_mxmlExportSelection->currentIndex());
    settings.setValue("mxmlmultistave", m_mxmlMultiStave->currentIndex());
    settings.setValue("mxmlexportstaffgroup", m_mxmlExportStaffGroup->isChecked());
    settings.setValue("mxmlexportpercussion", m_mxmlExportPercussion->currentIndex());
    settings.setValue("mxmluseoctaveshift", m_mxmlUseOctaveShift->isChecked());

    settings.endGroup();

    //! NOTE Does this sets the "document changed" flag???
    //! apparantly! Why??????
    //
    // If you edit the headers, those are stored in the document, so the
    // document might have changed when you have something to apply.  This could
    // probably be handled in a more fine-grained and robust manner. -dmm
    m_headersPage->apply();
}

void
MusicXMLOptionsDialog::accept()
{
    slotApply();
    QDialog::accept();
}

}
