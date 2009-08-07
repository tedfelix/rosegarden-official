/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2009 the Rosegarden development team.
 
    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.
 
    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#include "WarningDialog.h"

#include "gui/general/IconLoader.h"

#include <QDialog>
#include <QTabWidget>
#include <QVBoxLayout>
#include <QLabel>
#include <QDialogButtonBox>
#include <QIcon>

#include <iostream>

namespace Rosegarden
{


WarningDialog::WarningDialog() : QDialog()
{
    std::cerr << "WarningDialog::WarningDialog()" << std::endl;

    QVBoxLayout *layout = new QVBoxLayout;
    setLayout(layout);

    m_tabWidget = new QTabWidget;
    layout->addWidget(m_tabWidget);

    QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok);
    layout->addWidget(buttonBox);
    connect(buttonBox, SIGNAL(accepted()), this, SLOT(accept()));

    m_warningIcon = IconLoader().load("warning");

    setWindowTitle(tr("Runtime Problems Detected"));
    setWindowIcon(m_warningIcon);
}

WarningDialog::~WarningDialog()
{
}

void
WarningDialog::addWarning(Message message)
{
    std::cerr << "WarningDialog::addWarning()" << std::endl;

    QWidget *tab = new QWidget;
    QVBoxLayout *layout = new QVBoxLayout;
    tab->setLayout(layout);

    QLabel *text = new QLabel(message.first);
    text->setWordWrap(true);
    layout->addWidget(text);

    QLabel *informativeText = new QLabel(message.second);
    informativeText->setWordWrap(true);
    layout->addWidget(informativeText);

    informativeText->setOpenExternalLinks(true);

    m_tabWidget->addTab(tab, m_warningIcon, "");
}


}

#include "WarningDialog.moc"
