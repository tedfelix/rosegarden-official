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

#include "TrackLabelDialog.h"
#include "gui/widgets/LineEdit.h"
#include "misc/ConfigGroups.h"

#include <QDialog>
#include <QVBoxLayout>
#include <QLabel>
#include <QDialogButtonBox>


namespace Rosegarden
{


TrackLabelDialog::TrackLabelDialog(QWidget *parent,
                                   const QString &title,       
                                   const QString &primaryLabel,
                                   const QString &primaryContents,
                                   const QString &primaryTooltip,
                                   const QString &secondaryLabel,
                                   const QString &secondaryContents,
                                   const QString &secondaryTooltip) :
            QDialog(parent)
{
    setModal(true);
    setWindowTitle(title);

    QVBoxLayout *layout = new QVBoxLayout;

    QLabel *primary = new QLabel(primaryLabel);
    m_primaryText = new LineEdit(primaryContents);
    m_primaryText->setToolTip(primaryTooltip);
    m_primaryText->selectAll();

    QLabel *secondary = new QLabel(secondaryLabel);
    m_secondaryText = new LineEdit(secondaryContents);
    m_secondaryText->setToolTip(secondaryTooltip);

    QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    connect(buttonBox, SIGNAL(accepted()), this, SLOT(accept()));
    connect(buttonBox, SIGNAL(rejected()), this, SLOT(reject()));

    layout->addWidget(primary);
    layout->addWidget(m_primaryText);
    layout->addWidget(secondary);
    layout->addWidget(m_secondaryText);
    layout->addWidget(buttonBox);
    setLayout(layout);
}

QString
TrackLabelDialog::getPrimaryText()
{
    return m_primaryText->text();
}

QString
TrackLabelDialog::getSecondaryText()
{
    return m_secondaryText->text();
}


} // namespace

