/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2008 the Rosegarden development team.
 
    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.
 
    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/


#include "FloatEdit.h"

#include "gui/widgets/HSpinBox.h"
#include <QDialog>
#include <QDialogButtonBox>
#include <QGroupBox>
#include <QLabel>
#include <QString>
#include <QWidget>
#include <QVBoxLayout>
#include <cmath>

namespace Rosegarden
{

FloatEdit::FloatEdit(QDialogButtonBox::QWidget *parent,
                     const QString &title,
                     const QString &text,
                     float min,
                     float max,
                     float value,
                     float step):
        KDialogBase(parent, "rosegardenFloatEdit", true, title, Ok | Cancel, Ok)
{
    QVBox *vbox = makeVBoxMainWidget();
    QGroupBox *groupBox = new QGroupBox(1, Horizontal, text, vbox);
    QWidget *inVbox = new QWidget(groupBox);
    QVBoxLayout inVboxLayout = new QVBoxLayout;

    // Calculate decimal points according to the step size
    //
    double calDP = log10(step);
    int dps = 0;
    if (calDP < 0.0)
        dps = int( -calDP);
    //std::cout << "CAL DP = " << calDP << ", dps = " << dps << std::endl;

    m_spin = new HSpinBox( inVbox , dps);
    inVboxLayout->addWidget(m_spin);
    inVbox->setLayout(inVboxLayout);
    new QLabel(QString("(min: %1, max: %2)").arg(min).arg(max), inVbox);
}

float
FloatEdit::getValue() const
{
    return m_spin->valuef();
}

}
#include "FloatEdit.moc"
