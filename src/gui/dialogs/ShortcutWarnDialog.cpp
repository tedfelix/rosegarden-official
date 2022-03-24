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
#define RG_NO_DEBUG_PRINT

#include "ShortcutWarnDialog.h"

#include "misc/Debug.h"
#include "gui/general/ActionData.h"

#include <QDialogButtonBox>
#include <QGridLayout>
#include <QLabel>
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

    int row = 0;
    foreach(auto pair, ddata) {
        const ActionData::DuplicateDataForKey& ddatak = pair.second;
        QLabel *ecLabel = new QLabel(ddatak.editContext);
        QLabel *eaLabel = new QLabel(ddatak.editActionText);
        layout->addWidget(ecLabel, row, 0);
        layout->addWidget(eaLabel, row, 1);
        row++;
        foreach(auto dp, ddatak.duplicateMap) {
            const QKeySequence& ks = (dp).first;
            const ActionData::KeyDuplicates& kdups = (dp).second;
            QLabel* ksl = new QLabel(ks.toString(QKeySequence::NativeText));
            QLabel* elabel = new QLabel(tr("set Shortcut"));
            layout->addWidget(ksl, row, 0, 1, 2);
            layout->addWidget(elabel, row, 2);
            row++;

            foreach(auto kdup, kdups) {
                QLabel* cLabel = new QLabel(kdup.context);
                QLabel* aLabel = new QLabel(kdup.actionText);
                QLabel* rLabel = new QLabel(tr("remove Shortcut"));
                layout->addWidget(cLabel, row, 0);
                layout->addWidget(aLabel, row, 1);
                layout->addWidget(rLabel, row, 2);
                row++;
            }
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
