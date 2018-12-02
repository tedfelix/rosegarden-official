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

#include "InputDialog.h"
#include "gui/widgets/LineEdit.h"
#include "misc/ConfigGroups.h"

#include <QDialog>
#include <QVBoxLayout>
#include <QLabel>
#include <QDialogButtonBox>
#include <QPushButton>
#include <QGroupBox>


namespace Rosegarden
{

InputDialog::InputDialog(const QString &title, const QString &label,
                         QWidget *parent, QWidget *input, Qt::WindowFlags f)
    : QDialog(parent, f)
{
    // set the window title
    setWindowTitle(tr("Rosegarden"));
    QVBoxLayout *vboxLayout = new QVBoxLayout(this);

    QLabel *t = new QLabel(QString("<qt><h3>%1</h3></qt>").arg(title));
    vboxLayout->addWidget(t);    

    // add the passed label to our layout
/*    QLabel *lbl = new QLabel(label, this);
    vboxLayout->addWidget(lbl);
    vboxLayout->addStretch(1);*/

    // make a group box to hold the controls, for visual consistency with other
    // dialogs
    QGroupBox *gbox = new QGroupBox(this);
    vboxLayout->addWidget(gbox);
    QVBoxLayout *gboxLayout = new QVBoxLayout;
    gbox->setLayout(gboxLayout);
    gboxLayout->addWidget(new QLabel(label));

    // add the passed input widget to our layout, and reparent it
    input->setParent(this);
    gboxLayout->addWidget(input);
    gboxLayout->addStretch(1);

    // I have no idea what this is for, but Qt had it, so we'll keep it in our
    // doctored version --- this is for the accelerator (Alt+letter marked with '&' gives focus to input)
//    lbl->setBuddy(input);

    // Put some clicky buttons and hook them up
    QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Cancel,
                                                       Qt::Horizontal, this);
    QPushButton *okButton = static_cast<QPushButton *>(buttonBox->addButton(QDialogButtonBox::Ok));
    okButton->setDefault(true);
    vboxLayout->addWidget(buttonBox);

    QObject::connect(buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
    QObject::connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);

    // No size grip.  Size grips are stupid looking, and I detest them.
    // Rosegarden has a NO SIZE GRIP policy.
    setSizeGripEnabled(false);
}

InputDialog::~InputDialog()
{
}


QString
InputDialog::getText(QWidget *parent, const QString &title, const QString &label,
                     LineEdit::EchoMode mode, const QString &text,
                     bool *ok, Qt::WindowFlags f)
{
    LineEdit *le = new LineEdit;
    le->setText(text);
    le->setEchoMode(mode);
    le->setFocus();
    le->selectAll();

    InputDialog dlg(title, label, parent, le, f);

    QString result;
    bool accepted = (dlg.exec() == QDialog::Accepted);
    if (ok)
        *ok = accepted;
    if (accepted)
        result = le->text();

    return result;
}

}

