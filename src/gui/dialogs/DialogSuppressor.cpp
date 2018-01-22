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

#include "DialogSuppressor.h"

#include <QDialog>
#include <QWidget>
#include <QLayout>
#include <QCheckBox>
#include <QDialogButtonBox>
#include <QSettings>
#include <QLabel>

#include <iostream>
#include "misc/ConfigGroups.h"

namespace Rosegarden
{
  
void
SuppressionTarget::slotSuppressionToggled(bool on)
{
    std::cerr << "SuppressionTarget::slotSuppressionToggled" << std::endl;

    QCheckBox *cb = dynamic_cast<QCheckBox *>(sender());
    if (!cb) return;

    std::cerr << "checked = " << on << std::endl;

    QSettings settings;
    settings.beginGroup(DialogSuppressorConfigGroup);
    settings.setValue(m_key, on);
    settings.endGroup();
}

bool
DialogSuppressor::shouldSuppress(QDialog *dialog, QString settingsKey)
{
    if (isSuppressed(settingsKey)) return true;
    
    QList<QDialogButtonBox *> bbl = dialog->findChildren<QDialogButtonBox *>();
    if (bbl.empty()) {
        std::cerr << "DialogSuppressor::shouldSuppress: Dialog does not contain a button box, nothing to hook into" << std::endl;
        return false;
    }
    QDialogButtonBox *bb = bbl[bbl.size()-1];
    QWidget *p = bb->parentWidget();
    if (!p) {
        std::cerr << "DialogSuppressor::shouldSuppress: Dialog's button box has no parent widget!?" << std::endl;
        return false;
    }        
    QLayout *layout = p->layout();
    if (!p) {
        std::cerr << "DialogSuppressor::shouldSuppress: Dialog's button box's parent widget has no layout!?" << std::endl;
        return false;
    }        
    QWidget *impostor = new QWidget;
    QVBoxLayout *il = new QVBoxLayout;
    impostor->setLayout(il);
    layout->removeWidget(bb);
    bb->setParent(impostor);
    QString errorStr = QObject::tr("Do not show this warning again");
    QCheckBox *cb = new QCheckBox(errorStr);
    SuppressionTarget *target = new SuppressionTarget(settingsKey);
    target->setParent(cb);
    QObject::connect(cb, SIGNAL(toggled(bool)), target, SLOT(slotSuppressionToggled(bool)));
    il->addWidget(cb);
    il->addWidget(bb);
    QGridLayout *grid = dynamic_cast<QGridLayout *>(layout);
    if (grid) {
        grid->addWidget(impostor, grid->rowCount()-1, 0, -1, -1);
    } else {
        layout->addWidget(impostor);
    }
    return false;
}

bool
DialogSuppressor::isSuppressed(QString settingsKey)
{
    QSettings settings;
    settings.beginGroup(DialogSuppressorConfigGroup);
    bool suppress = settings.value(settingsKey, false).toBool();
    settings.endGroup();
    return suppress;
}

}

