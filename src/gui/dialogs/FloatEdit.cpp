/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2025 the Rosegarden development team.
 
    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.
 
    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#define RG_MODULE_STRING "[FloatEdit]"
#define RG_NO_DEBUG_PRINT

#include "FloatEdit.h"

#include <QDialogButtonBox>
#include <QDoubleSpinBox>
#include <QLabel>
#include <QString>
#include <QVBoxLayout>

#include <math.h>  // ceil() and log10()


namespace Rosegarden
{


FloatEdit::FloatEdit(QWidget *parent,
                     const QString &title,
                     const QString &text,
                     float min,
                     float max,
                     float value,
                     float step):
    QDialog(parent)
{
    setModal(true);
    setWindowTitle(title);
    //setObjectName("MinorDialog");

    QVBoxLayout *layout = new QVBoxLayout(this);

    // Text
    if (!text.isEmpty())
        layout->addWidget(new QLabel(text, this));

    // Spin Box

    // Calculate required decimal points according to the step size
    int decimals = 0;
    if (step < 1)
        decimals = ceil(-log10(step));

    //RG_DEBUG << "CAL DP = " << calDP << ", decimals = " << decimals;

    m_spin = new QDoubleSpinBox(this);
    m_spin->setDecimals(decimals);
    // ??? Min and max on a spin box is brutal.  It can render it impossible
    //     to enter values with the keyboard.  Might want to consider an
    //     out of range indicator instead.  See TimeWidget2.
    m_spin->setMinimum(min);
    m_spin->setMaximum(max);
    m_spin->setSingleStep(step);
    m_spin->setValue(value);
    layout->addWidget(m_spin);

    // Range
    layout->addWidget(new QLabel(
            QString("(min: %1, max: %2)").arg(min).arg(max),
            this));

    // Button Box
    QDialogButtonBox *buttonBox = new QDialogButtonBox(
            QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    layout->addWidget(buttonBox);
    connect(buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);

    // Have to show before geometry() will work.
    show();

    // Lock the window size.
    setFixedSize(geometry().size());
}

float
FloatEdit::getValue() const
{
    return m_spin->value();
}


}
