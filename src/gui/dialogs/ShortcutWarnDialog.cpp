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

#include "misc/Debug.h"
#include "gui/general/ActionData.h"

#include <QDialogButtonBox>
#include <QGridLayout>
#include <QLabel>
#include <QPushButton>
#include <QFrame>
#include <QScrollArea>

namespace Rosegarden
{

    ShortcutWarnDialog::ShortcutWarnDialog(ActionData::DuplicateData ddata)
{
    setModal(true);
    setWindowTitle(tr("Shortcut Conflicts"));
    QHBoxLayout *hLayout = new QHBoxLayout;
    setLayout(hLayout);

    QFrame* scrollFrame = new QFrame;
    // make room for scrollbar
    scrollFrame->setContentsMargins(0, 0, 20, 0);

    QScrollArea* scrollArea = new QScrollArea;
    scrollArea->setWidgetResizable(true);
    scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    scrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    scrollArea->setWidget(scrollFrame);
    hLayout->addWidget(scrollArea);

    QGridLayout* layout = new QGridLayout;
    scrollFrame->setLayout(layout);

    m_duplicateButtons.editKey = ddata.editKey;
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
        ebutton->setCheckable(true);
        KeyDuplicateButtons keyDupButtons;
        keyDupButtons.editButton = ebutton;
        layout->addWidget(ksl, row, 0, 1, 2);
        layout->addWidget(ebutton, row, 2);
        row++;

        foreach(auto kdup, kdups) {
            QLabel* cLabel = new QLabel(kdup.context);
            QLabel* aLabel = new QLabel(kdup.actionText);
            QPushButton *button = new QPushButton(tr("remove Shortcut"));
            button->setCheckable(true);
            KeyDuplicateButton kdb;
            kdb.key = kdup.key;
            kdb.button = button;
            keyDupButtons.buttonList.push_back(kdb);
            layout->addWidget(cLabel, row, 0);
            layout->addWidget(aLabel, row, 1);
            layout->addWidget(button, row, 2);
            row++;
        }
        m_duplicateButtons.duplicateButtonMap[ks] = keyDupButtons;

        QFrame* line = new QFrame();
        line->setFrameShape(QFrame::HLine);
        line->setFrameShadow(QFrame::Sunken);
        layout->addWidget(line, row, 0, 1, 3);
        row++;
    }

    QDialogButtonBox *buttonBox =
        new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    QObject::connect(buttonBox, &QDialogButtonBox::accepted,
                     this, &ShortcutWarnDialog::OKclicked);
    QObject::connect(buttonBox, &QDialogButtonBox::rejected,
                     this, &QDialog::reject);
    layout->addWidget(buttonBox, row, 0, 1, 3);
}

ShortcutWarnDialog::~ShortcutWarnDialog()
{
}

void ShortcutWarnDialog::OKclicked()
{
    // do what the user wants
    RG_DEBUG << "OKclicked";
    ActionData* adata = ActionData::getInstance();
    foreach(auto pair, m_duplicateButtons.duplicateButtonMap) {
        const QKeySequence& ks = pair.first;
        const KeyDuplicateButtons& keyDupButtons = pair.second;
        RG_DEBUG << "editButton checked:" <<
            keyDupButtons.editButton->isChecked();
        if (keyDupButtons.editButton->isChecked()) {
            RG_DEBUG << "setting shortcut" << ks <<
                "for" << m_duplicateButtons.editKey;
            adata->addUserShortcut(m_duplicateButtons.editKey, ks);
        }
        foreach(auto kdb, keyDupButtons.buttonList) {
            if (kdb.button->isChecked()) {
                RG_DEBUG << "removing shorcut" << ks <<
                    "from" << kdb.key;
                adata->removeUserShortcut(kdb.key, ks);
            }
        }
    }

    accept();
}

}
