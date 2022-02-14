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
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QFrame>

namespace Rosegarden
{

    ShortcutWarnDialog::ShortcutWarnDialog(ActionData::DuplicateData ddata)
{
    setModal(true);
    setWindowTitle(tr("Shortcut warnings"));
    
    QVBoxLayout *layout = new QVBoxLayout;
    setLayout(layout);

    foreach(auto dp, ddata) {
        const QKeySequence& ks = (dp).first;
        const ActionData::KeyDuplicates& kdups = (dp).second;
        QLabel* ksl = new QLabel(ks.toString(QKeySequence::NativeText));
        layout->addWidget(ksl);

        foreach(auto kdup, kdups) {
            QHBoxLayout* hlayout = new QHBoxLayout;
            layout->addLayout(hlayout);
            QLabel* cLabel = new QLabel(kdup.context);
            QLabel* aLabel = new QLabel(kdup.actionText);
            hlayout->addWidget(cLabel);
            hlayout->addWidget(aLabel);
        }

        QFrame* line = new QFrame();
        line->setFrameShape(QFrame::HLine);
        line->setFrameShadow(QFrame::Sunken);
        layout->addWidget(line);
    }
    
    QDialogButtonBox *buttonBox =
        new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    QObject::connect(buttonBox, &QDialogButtonBox::accepted,
                     this, &QDialog::accept);
    QObject::connect(buttonBox, &QDialogButtonBox::rejected,
                     this, &QDialog::reject);
    layout->addWidget(buttonBox);
}

ShortcutWarnDialog::~ShortcutWarnDialog()
{
}

}
