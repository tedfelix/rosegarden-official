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

#define RG_MODULE_STRING "[ShortcutWarnDialog]"
//#define RG_NO_DEBUG_PRINT

#include "ShortcutWarnDialog.h"

#include <QDialogButtonBox>
#include <QGridLayout>
#include <QLabel>
#include <QPushButton>
#include <QFrame>

namespace Rosegarden
{

    ShortcutWarnDialog::ShortcutWarnDialog(ActionData::DuplicateData ddata)
{
    setModal(true);
    setWindowTitle(tr("Shortcut warnings"));
    
    QGridLayout* layout = new QGridLayout;
    setLayout(layout);

    int row = 0;
    QLabel *ecLabel = new QLabel(ddata.editContext);
    QLabel *eaLabel = new QLabel(ddata.editActionText);
    layout->addWidget(ecLabel, row, 0);
    layout->addWidget(eaLabel, row, 1);
    row++;
    foreach(auto dp, ddata.duplicateMap) {
        const QKeySequence& ks = (dp).first;
        const ActionData::KeyDuplicates& kdups = (dp).second;
        QLabel* ksl = new QLabel(ks.toString(QKeySequence::NativeText));
        QPushButton *ebutton = new QPushButton(tr("set Shortcut"));
        m_duplicateButtons.editKey = ddata.editKey;
        m_duplicateButtons.editButton = ebutton;
        ebutton->setCheckable(true);
        layout->addWidget(ksl, row, 0, 1, 2);
        layout->addWidget(ebutton, row, 2);
        row++;
        
        foreach(auto kdup, kdups) {
            QLabel* cLabel = new QLabel(kdup.context);
            QLabel* aLabel = new QLabel(kdup.actionText);
            QPushButton *button = new QPushButton(tr("remove Shortcut"));
            KeyDuplicateButton kdb;
            kdb.key = kdup.key;
            kdb.button = button;
            m_duplicateButtons.duplicateButtonMap[ks].push_back(kdb);
            button->setCheckable(true);
            layout->addWidget(cLabel, row, 0);
            layout->addWidget(aLabel, row, 1);
            layout->addWidget(button, row, 2);
            row++;
        }

        QFrame* line = new QFrame();
        line->setFrameShape(QFrame::HLine);
        line->setFrameShadow(QFrame::Sunken);
        layout->addWidget(line, row, 0, 1, 3);
        row++;
    }
    
    QDialogButtonBox *buttonBox =
        new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    QObject::connect(buttonBox, &QDialogButtonBox::accepted,
                     this, &QDialog::accept);
    QObject::connect(buttonBox, &QDialogButtonBox::rejected,
                     this, &QDialog::reject);
    layout->addWidget(buttonBox, row, 0, 1, 3);
}

ShortcutWarnDialog::~ShortcutWarnDialog()
{
}

}
