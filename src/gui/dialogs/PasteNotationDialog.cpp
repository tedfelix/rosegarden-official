/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2015 the Rosegarden development team.
 
    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.
 
    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/


#include "PasteNotationDialog.h"

#include "commands/edit/PasteEventsCommand.h"
#include "misc/ConfigGroups.h"

#include <QCheckBox>
#include <QDesktopServices>
#include <QDialog>
#include <QDialogButtonBox>
#include <QGroupBox>
#include <QObject>
#include <QRadioButton>
#include <QSettings>
#include <QUrl>
#include <QVBoxLayout>
#include <QWidget>


namespace Rosegarden
{

PasteNotationDialog::PasteNotationDialog(QWidget *parent) :
    QDialog(parent),
    m_defaultType(getSavedPasteType())
{
    //setHelp("nv-paste-types");

    setModal(true);
    setWindowTitle(tr("Paste"));

    QGridLayout *metagrid = new QGridLayout;
    setLayout(metagrid);
    QWidget *vbox = new QWidget(this);
    QVBoxLayout *vboxLayout = new QVBoxLayout;
    metagrid->addWidget(vbox, 0, 0);


    QGroupBox *pasteTypeGroup = new QGroupBox( tr("Paste type"), vbox );
    QVBoxLayout *pasteTypeGroupLayout = new QVBoxLayout;
    vboxLayout->addWidget(pasteTypeGroup);

    PasteEventsCommand::PasteTypeMap pasteTypes =
        PasteEventsCommand::getPasteTypes();

    for (PasteEventsCommand::PasteTypeMap::iterator i = pasteTypes.begin();
            i != pasteTypes.end(); ++i) {

        QRadioButton *button = new QRadioButton(i->second, pasteTypeGroup);
        pasteTypeGroupLayout->addWidget(button);
        button->setChecked(m_defaultType == i->first);
        QObject::connect(button, SIGNAL(clicked()),
                         this, SLOT(slotPasteTypeChanged()));

        m_pasteTypeButtons.push_back(button);
    }
    pasteTypeGroup->setLayout(pasteTypeGroupLayout);


    QGroupBox *setAsDefaultGroup = new QGroupBox( tr("Options"), vbox );
    QVBoxLayout *setAsDefaultGroupLayout = new QVBoxLayout;
    vboxLayout->addWidget(setAsDefaultGroup);
    vbox->setLayout(vboxLayout);

    m_setAsDefaultButton = new QCheckBox
                           (tr("Make this the default paste type"), setAsDefaultGroup);
    setAsDefaultGroupLayout->addWidget(m_setAsDefaultButton);
    m_setAsDefaultButton->setChecked(true);
    setAsDefaultGroup->setLayout(setAsDefaultGroupLayout);

    QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel | QDialogButtonBox::Help );
    metagrid->addWidget(buttonBox, 1, 0);
    metagrid->setRowStretch(0, 10);
    connect(buttonBox, SIGNAL(accepted()), this, SLOT(accept()));
    connect(buttonBox, SIGNAL(rejected()), this, SLOT(reject()));
    connect(buttonBox, SIGNAL(helpRequested()), this, SLOT(slotHelpRequested()));
}

PasteEventsCommand::PasteType
PasteNotationDialog::getSavedPasteType()
{
    QSettings settings;
    settings.beginGroup(NotationViewConfigGroup);
    PasteEventsCommand::PasteType type =
        (PasteEventsCommand::PasteType)
        settings.value("pastetype",
                       PasteEventsCommand::Restricted).toUInt();
    settings.endGroup();
    return type;
}

PasteEventsCommand::PasteType
PasteNotationDialog::getPasteType() const
{
    for (unsigned int i = 0; i < m_pasteTypeButtons.size(); ++i) {
        if (m_pasteTypeButtons[i]->isChecked()) {
            return (PasteEventsCommand::PasteType)i;
        }
    }

    return PasteEventsCommand::Restricted;
}

bool
PasteNotationDialog::setAsDefault() const
{
    return m_setAsDefaultButton->isChecked();
}

void
PasteNotationDialog::slotPasteTypeChanged()
{
    m_setAsDefaultButton->setChecked(m_defaultType == getPasteType());
}

void
PasteNotationDialog::accept()
{
    if (setAsDefault()) {
        QSettings settings;
        settings.beginGroup(NotationViewConfigGroup);
        settings.setValue("pastetype", getPasteType());
        settings.endGroup();
    }
    QDialog::accept();
}

void
PasteNotationDialog::slotHelpRequested()
{
    // TRANSLATORS: if the manual is translated into your language, you can
    // change the two-letter language code in this URL to point to your language
    // version, eg. "http://rosegardenmusic.com/wiki/doc:pasteNotationDialog-es" for the
    // Spanish version. If your language doesn't yet have a translation, feel
    // free to create one.
    QString helpURL = tr("http://rosegardenmusic.com/wiki/doc:pasteNotationDialog-en");
    QDesktopServices::openUrl(QUrl(helpURL));
}
}
