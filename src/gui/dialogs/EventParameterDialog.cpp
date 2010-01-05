/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2010 the Rosegarden development team.
 
    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.
 
    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/


#include "EventParameterDialog.h"

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
#include <QGroupBox>


namespace Rosegarden
{

EventParameterDialog::EventParameterDialog(
    QWidget *parent,
    const QString &name,
    const PropertyName &property,
    int startValue):
        QDialog(parent),
        m_property(property)
{
    setModal(true);
    setWindowTitle(tr("Rosegarden"));
    setSizePolicy(QSizePolicy(QSizePolicy::Maximum, QSizePolicy::Maximum));

    QVBoxLayout *mainLayout = new QVBoxLayout;
    setLayout(mainLayout);

    QGroupBox *controls = new QGroupBox(name);
    QVBoxLayout *controlsLayout = new QVBoxLayout;
    controlsLayout->setSpacing(0);    
    controls->setLayout(controlsLayout);
    mainLayout->addWidget(controls);
    
    QWidget *topBox = new QWidget;
    QVBoxLayout *topBoxLayout = new QVBoxLayout;
    topBox->setLayout(topBoxLayout);
    controlsLayout->addWidget(topBox);
    

    QLabel *explainLabel = new QLabel;
    QString text = tr("Set the %1 property of the event selection:")
                   .arg(strtoqstr(property));
    explainLabel->setText(text);
    topBoxLayout->addWidget(explainLabel);

    
    QWidget *patternBox = new QWidget;
    QHBoxLayout *patternBoxLayout = new QHBoxLayout;
    patternBox->setLayout(patternBoxLayout);
    controlsLayout->addWidget(patternBox);
    
    QLabel *child_10 = new QLabel(tr("Pattern"));
    m_patternCombo = new QComboBox;
    patternBoxLayout->addWidget(child_10);
    patternBoxLayout->addWidget(m_patternCombo);

    // create options
    // 0 flat
    text = tr("Flat - set %1 to value").arg(strtoqstr(property));
    m_patternCombo->addItem(text);

    // 1 alternating
    text = tr("Alternating - set %1 to max and min on alternate events").arg(strtoqstr(property));
    m_patternCombo->addItem(text);

    // 2 crescendo
    text = tr("Crescendo - set %1 rising from min to max").arg(strtoqstr(property));
    m_patternCombo->addItem(text);

    // 3 diminuendo
    text = tr("Diminuendo - set %1 falling from max to min").arg(strtoqstr(property));
    m_patternCombo->addItem(text);

    // 4 ringing
    text = tr("Ringing - set %1 alternating from max to min with both dying to zero").arg(strtoqstr(property));
    m_patternCombo->addItem(text);

    connect(m_patternCombo, SIGNAL(activated(int)),
            this, SLOT(slotPatternSelected(int)));

    QWidget *value1Box = new QWidget;
    controlsLayout->addWidget(value1Box);
    QHBoxLayout *value1BoxLayout = new QHBoxLayout;
    m_value1Label = new QLabel(tr("Value"));
    value1BoxLayout->addWidget(m_value1Label);
    m_value1Combo = new QComboBox;
    value1BoxLayout->addWidget(m_value1Combo);
    value1Box->setLayout(value1BoxLayout);

    QWidget *value2Box = new QWidget;
    controlsLayout->addWidget(value2Box);
    QHBoxLayout *value2BoxLayout = new QHBoxLayout;
    m_value2Label = new QLabel(tr("Value"));
    value2BoxLayout->addWidget(m_value2Label);
    m_value2Combo = new QComboBox;
    value2BoxLayout->addWidget(m_value2Combo);
    value2Box->setLayout(value2BoxLayout);

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
    mainLayout->addWidget(buttonBox);

    connect(buttonBox, SIGNAL(accepted()), this, SLOT(accept()));
    connect(buttonBox, SIGNAL(rejected()), this, SLOT(reject()));
}

void
EventParameterDialog::slotPatternSelected(int value)
{
    switch (value) {
    case 0:  // flat
        m_value1Label->setText(tr("Value"));
        m_value1Label->show();
        m_value1Combo->show();
        m_value2Label->hide();
        m_value2Combo->hide();
        break;

    case 1:  // alternating
        m_value1Label->setText(tr("First Value"));
        m_value2Label->setText(tr("Second Value"));
        m_value1Label->show();
        m_value1Combo->show();
        m_value2Label->show();
        m_value2Combo->show();
        break;

    case 2:  // crescendo
        m_value1Label->setText(tr("Low Value"));
        m_value2Label->setText(tr("High Value"));
        m_value1Label->show();
        m_value1Combo->show();
        m_value2Label->show();
        m_value2Combo->show();
        break;

    case 3:  // decrescendo
        m_value1Label->setText(tr("High Value"));
        m_value2Label->setText(tr("Low Value"));
        m_value1Label->show();
        m_value1Combo->show();
        m_value2Label->show();
        m_value2Combo->show();
        break;

    case 4:  // ringing
        m_value1Label->setText(tr("First Value"));
        m_value2Label->setText(tr("Second Value"));
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

    adjustSize();
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
