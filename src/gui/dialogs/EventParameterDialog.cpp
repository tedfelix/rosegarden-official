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


#include "EventParameterDialog.h"

#include <klocale.h>
#include "misc/Debug.h"
#include "misc/Strings.h"
#include "base/PropertyName.h"
#include <QComboBox>
#include <QDialog>
#include <QDialogButtonBox>
#include <QLabel>
#include <QString>
#include <QWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>


namespace Rosegarden
{

EventParameterDialog::EventParameterDialog(
    QDialogButtonBox::QWidget *parent,
    const QString &name,
    const PropertyName &property,
    int startValue):
        QDialog(parent),
        m_property(property)
{
    setModal(true);
    setWindowTitle(name);

    QGridLayout *metagrid = new QGridLayout;
    setLayout(metagrid);
	
    
	QWidget *topBox = new QWidget(this);
	metagrid->addWidget(topBox, 0, 0);
	QVBoxLayout *topBoxLayout = new QVBoxLayout;
	topBox->setLayout(topBoxLayout);
    

    QLabel *explainLabel = new QLabel( topBox );
    topBoxLayout->addWidget(explainLabel);
    QString text = i18n("Set the %1 property of the event selection:", 
                   strtoqstr(property));
    explainLabel->setText(text);

	
	QWidget *patternBox = new QWidget(this);
	metagrid->addWidget(patternBox, 0, 0);
	QVBoxLayout *patternBoxLayout = new QVBoxLayout;
	patternBox->setLayout(topBoxLayout);
	
	QLabel *child_10 = new QLabel(i18n("Pattern"), patternBox );
    patternBoxLayout->addWidget(child_10);
    m_patternCombo = new QComboBox( patternBox );
    patternBoxLayout->addWidget(m_patternCombo);

    // create options
    // 0 flat
    text = i18n("Flat - set %1 to value", strtoqstr(property));
    m_patternCombo->addItem(text);

    // 1 alternating
    text = i18n("Alternating - set %1 to max and min on alternate events", strtoqstr(property));
    m_patternCombo->addItem(text);

    // 2 crescendo
    text = i18n("Crescendo - set %1 rising from min to max", strtoqstr(property));
    m_patternCombo->addItem(text);

    // 3 diminuendo
    text = i18n("Diminuendo - set %1 falling from max to min", strtoqstr(property));
    m_patternCombo->addItem(text);

    // 4 ringing
    text = i18n("Ringing - set %1 alternating from max to min with both dying to zero", strtoqstr(property));
    m_patternCombo->addItem(text);

    connect(m_patternCombo, SIGNAL(activated(int)),
            this, SLOT(slotPatternSelected(int)));

	QWidget *value1Box = new QWidget( topBox );
	topBoxLayout->addWidget(value1Box);
//    topBox->setLayout(topBoxLayout);
    QHBoxLayout *value1BoxLayout = new QHBoxLayout;
    m_value1Label = new QLabel(i18n("Value"), value1Box );
    value1BoxLayout->addWidget(m_value1Label);
    m_value1Combo = new QComboBox( value1Box );
    value1BoxLayout->addWidget(m_value1Combo);
    value1Box->setLayout(value1BoxLayout);

	m_value2Label = new QLabel(i18n("Value"), topBox );
	topBoxLayout->addWidget(m_value2Label);
	m_value2Combo = new QComboBox( topBox );
	topBoxLayout->addWidget(m_value2Combo);
	topBox->setLayout(topBoxLayout);

    for (unsigned int i = 0; i < 128; i++) {
        m_value1Combo->addItem(QString("%1").arg(i));
        m_value2Combo->addItem(QString("%1").arg(i));
    }
    m_value1Combo->setCurrentIndex(127);

    slotPatternSelected(0);

    // start value
    m_value1Combo->setCurrentIndex(startValue);
    m_value2Combo->setCurrentIndex(startValue);

    QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    metagrid->addWidget(buttonBox, 1, 0);
    metagrid->setRowStretch(0, 10);
    connect(buttonBox, SIGNAL(accepted()), this, SLOT(accept()));
    connect(buttonBox, SIGNAL(rejected()), this, SLOT(reject()));
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
    return PropertyPattern(m_patternCombo->currentIndex());
}

int
EventParameterDialog::getValue1()
{
    return m_value1Combo->currentIndex();
}

int
EventParameterDialog::getValue2()
{
    return m_value2Combo->currentIndex();
}

}
#include "EventParameterDialog.moc"
