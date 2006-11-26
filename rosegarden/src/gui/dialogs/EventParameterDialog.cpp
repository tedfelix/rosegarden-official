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


#include "EventParameterDialog.h"

#include <klocale.h>
#include "misc/Debug.h"
#include "misc/Strings.h"
#include "base/PropertyName.h"
#include <kcombobox.h>
#include <kdialogbase.h>
#include <qhbox.h>
#include <qlabel.h>
#include <qstring.h>
#include <qvbox.h>
#include <qwidget.h>


namespace Rosegarden
{

EventParameterDialog::EventParameterDialog(
    QWidget *parent,
    const QString &name,
    const PropertyName &property,
    int startValue):
        KDialogBase(parent, 0, true, name, Ok | Cancel),
        m_property(property)
{
    QVBox *vBox = makeVBoxMainWidget();

    QHBox *topBox = new QHBox(vBox);
    QLabel *explainLabel = new QLabel(topBox);
    QString text = i18n("Set the %1 property of the event selection:").
                   arg(strtoqstr(property));
    explainLabel->setText(text);

    QHBox *patternBox = new QHBox(vBox);
    new QLabel(i18n("Pattern"), patternBox);
    m_patternCombo = new KComboBox(patternBox);

    // create options
    // 0 flat
    text = i18n("Flat - set %1 to value").arg(strtoqstr(property));
    m_patternCombo->insertItem(text);

    // 1 alternating
    text = i18n("Alternating - set %1 to max and min on alternate events").arg(strtoqstr(property));
    m_patternCombo->insertItem(text);

    // 2 crescendo
    text = i18n("Crescendo - set %1 rising from min to max").arg(strtoqstr(property));
    m_patternCombo->insertItem(text);

    // 3 diminuendo
    text = i18n("Diminuendo - set %1 falling from max to min").arg(strtoqstr(property));
    m_patternCombo->insertItem(text);

    // 4 ringing
    text = i18n("Ringing - set %1 alternating from max to min with both dying to zero").arg(strtoqstr(property));
    m_patternCombo->insertItem(text);

    connect(m_patternCombo, SIGNAL(activated(int)),
            this, SLOT(slotPatternSelected(int)));

    QHBox *value1Box = new QHBox(vBox);
    m_value1Label = new QLabel(i18n("Value"), value1Box);
    m_value1Combo = new KComboBox(value1Box);

    QHBox *value2Box = new QHBox(vBox);
    m_value2Label = new QLabel(i18n("Value"), value2Box);
    m_value2Combo = new KComboBox(value2Box);

    for (unsigned int i = 0; i < 128; i++) {
        m_value1Combo->insertItem(QString("%1").arg(i));
        m_value2Combo->insertItem(QString("%1").arg(i));
    }
    m_value1Combo->setCurrentItem(127);

    slotPatternSelected(0);

    // start value
    m_value1Combo->setCurrentItem(startValue);
    m_value2Combo->setCurrentItem(startValue);

}

void
EventParameterDialog::slotPatternSelected(int value)
{
    switch (value) {
    case 0:  // flat
        m_value1Label->setText(i18n("Value"));
        m_value1Label->show();
        m_value1Combo->show();
        m_value2Label->hide();
        m_value2Combo->hide();
        break;

    case 1:  // alternating
        m_value1Label->setText(i18n("First Value"));
        m_value2Label->setText(i18n("Second Value"));
        m_value1Label->show();
        m_value1Combo->show();
        m_value2Label->show();
        m_value2Combo->show();
        break;

    case 2:  // crescendo
        m_value1Label->setText(i18n("Low Value"));
        m_value2Label->setText(i18n("High Value"));
        m_value1Label->show();
        m_value1Combo->show();
        m_value2Label->show();
        m_value2Combo->show();
        break;

    case 3:  // decrescendo
        m_value1Label->setText(i18n("High Value"));
        m_value2Label->setText(i18n("Low Value"));
        m_value1Label->show();
        m_value1Combo->show();
        m_value2Label->show();
        m_value2Combo->show();
        break;

    case 4:  // ringing
        m_value1Label->setText(i18n("First Value"));
        m_value2Label->setText(i18n("Second Value"));
        m_value1Label->show();
        m_value1Combo->show();
        m_value2Label->show();
        m_value2Combo->show();
        break;

    default:
        RG_DEBUG << "EventParameterDialog::slotPatternSelected - "
        << "unrecognised pattern number" << endl;
        break;
    }

}

PropertyPattern
EventParameterDialog::getPattern()
{
    return PropertyPattern(m_patternCombo->currentItem());
}

int
EventParameterDialog::getValue1()
{
    return m_value1Combo->currentItem();
}

int
EventParameterDialog::getValue2()
{
    return m_value2Combo->currentItem();
}

}
#include "EventParameterDialog.moc"
