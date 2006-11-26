/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
 
    This program is Copyright 2000-2006
        Guillaume Laurent   <glaurent@telegraph-road.org>,
        Chris Cannam        <cannam@all-day-breakfast.com>,
        Richard Bown        <richard.bown@ferventsoftware.com>
 
    The moral rights of Guillaume Laurent, Chris Cannam, and Richard
    Bown to claim authorship of this work have been asserted.
 
    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.
 
    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/


#include "CompositionLengthDialog.h"

#include <klocale.h>
#include "base/Composition.h"
#include <kdialogbase.h>
#include <qhbox.h>
#include <qlabel.h>
#include <qspinbox.h>
#include <qvbox.h>
#include <qwidget.h>


namespace Rosegarden
{

CompositionLengthDialog::CompositionLengthDialog(
    QWidget *parent,
    Composition *composition):
        KDialogBase(parent, 0, true, i18n("Change Composition Length"),
                    Ok | Cancel),
        m_composition(composition)
{
    QVBox *vBox = makeVBoxMainWidget();

    new QLabel(i18n("Set the Start and End bar markers for this Composition"),
               vBox);

    QHBox *startBox = new QHBox(vBox);
    new QLabel(i18n("Start Bar"), startBox);
    m_startMarkerSpinBox = new QSpinBox(startBox);
    m_startMarkerSpinBox->setMinValue( -10);
    m_startMarkerSpinBox->setMaxValue(10000);
    m_startMarkerSpinBox->setValue(
        m_composition->getBarNumber(m_composition->getStartMarker()) + 1);

    QHBox *endBox = new QHBox(vBox);
    new QLabel(i18n("End Bar"), endBox);
    m_endMarkerSpinBox = new QSpinBox(endBox);
    m_endMarkerSpinBox->setMinValue( -10);
    m_endMarkerSpinBox->setMaxValue(10000);
    m_endMarkerSpinBox->setValue(
        m_composition->getBarNumber(m_composition->getEndMarker()));

}

timeT
CompositionLengthDialog::getStartMarker()
{
    return m_composition->getBarStart(m_startMarkerSpinBox->value() - 1);
}

timeT
CompositionLengthDialog::getEndMarker()
{
    return m_composition->getBarStart(m_endMarkerSpinBox->value());
}

}
#include "CompositionLengthDialog.moc"
