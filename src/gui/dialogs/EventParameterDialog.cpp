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

#define RG_MODULE_STRING "[EventParameterDialog]"

#include "EventParameterDialog.h"

#include "misc/Debug.h"
#include "misc/Strings.h"
#include "base/PropertyName.h"

#include <QComboBox>
#include <QDialogButtonBox>
#include <QLabel>
#include <QSpinBox>
#include <QString>
#include <QWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGroupBox>


namespace Rosegarden
{

    /******** Nested class EventParameterDialog::ParamWidget ********/

EventParameterDialog::ParamWidget::ParamWidget(QLayout *parent)
{
    // ??? Derive from QWidget?  Might be clearer.
    QWidget *box = new QWidget;
    parent->addWidget(box);

    QHBoxLayout *boxLayout = new QHBoxLayout;

    // Label
    m_label = new QLabel(tr("Value"));
    boxLayout->addWidget(m_label);

    // SpinBox
    m_spinBox = new QSpinBox;
    boxLayout->addWidget(m_spinBox);

    box->setLayout(boxLayout);
}

int
EventParameterDialog::ParamWidget::getValue() const
{
    return m_spinBox->value();
}

void
EventParameterDialog::ParamWidget::showByArgs(
        const ParameterPattern::SliderSpec *args)
{
    m_label->setText(args->m_label);
    m_spinBox->setMinimum(args->m_minValue);
    m_spinBox->setMaximum(args->m_maxValue);
    m_spinBox->setValue(args->m_defaultValue);

    m_label->show();
    m_spinBox->show();
}

void
EventParameterDialog::ParamWidget::hide()
{
    m_label->hide();
    m_spinBox->hide();
}

    /******** Main class EventParameterDialog ********/

EventParameterDialog::EventParameterDialog(
        QWidget *parent,
        const QString &name,
        SelectionSituation *situation,
        const ParameterPattern::ParameterPatternVec *patterns) :
    QDialog(parent),
    m_situation(situation),
    m_patterns(patterns),
    m_NbParameters(0)
{
    setModal(true);
    setWindowTitle(name);
    setSizePolicy(QSizePolicy(QSizePolicy::Maximum, QSizePolicy::Maximum));

    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(0);

    QLabel *explainLabel = new QLabel;
    QString propertyName = m_situation->getPropertyNameQString();
    QString text = tr("Set the %1 property of the event selection:")
                   .arg(propertyName);
    explainLabel->setText(text);
    mainLayout->addWidget(explainLabel);

    
    QWidget *patternBox = new QWidget;
    QHBoxLayout *patternBoxLayout = new QHBoxLayout;
    patternBox->setLayout(patternBoxLayout);
    mainLayout->addWidget(patternBox);
    
    QLabel *child_10 = new QLabel(tr("Pattern"));
    m_patternCombo = new QComboBox;
    patternBoxLayout->addWidget(child_10);
    patternBoxLayout->addWidget(m_patternCombo);

    initPatternCombo();

    connect(m_patternCombo, SIGNAL(activated(int)),
            this, SLOT(slotPatternSelected(int)));

    // Instead of looping for 2 we just call twice.
    m_paramVec.push_back(ParamWidget(mainLayout));
    m_paramVec.push_back(ParamWidget(mainLayout));

    slotPatternSelected(0);

    QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    mainLayout->addWidget(buttonBox);

    connect(buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
}

void
EventParameterDialog::initPatternCombo()
{
    typedef ParameterPattern::ParameterPatternVec::const_iterator iterator;
    QString propertyName = m_situation->getPropertyNameQString();

    for (iterator i = m_patterns->begin();
         i != m_patterns->end();
         ++i) {
        QString text = (*i)->getText(propertyName);
        m_patternCombo->addItem(text);
    }
}

void
EventParameterDialog::slotPatternSelected(int value)
{
    ParameterPattern::SliderSpecVector sliderArgs =
        getPattern(value)->getSliderSpec(m_situation);
    typedef ParameterPattern::SliderSpecVector::const_iterator
        ArgIterator;

    // We don't try to handle more than 2 parameters.
    if (sliderArgs.size() > 2) { return; }
    m_NbParameters = sliderArgs.size();
    ParamWidgetVec::iterator widgetBox = m_paramVec.begin();
    for (ArgIterator i = sliderArgs.begin();
         i != sliderArgs.end();
         ++i, ++widgetBox) {
        widgetBox->showByArgs(&*i);
    }

    // If not all widgets are being used, hide the rest.
    for (;widgetBox != m_paramVec.end(); ++widgetBox)
    { widgetBox->hide(); }

    adjustSize();
}

ParameterPattern::BareParams
EventParameterDialog::getBareParams()
{
    ParameterPattern::BareParams result;
    for (int i = 0; i < m_NbParameters; ++i) {
        ParamWidget &widgetBox = m_paramVec[i];
        result.push_back(widgetBox.getValue());
    }
    return result;
}

ParameterPattern::Result
EventParameterDialog::getResult()
{
    const int patternIndex = m_patternCombo->currentIndex();
    return 
        ParameterPattern::Result(m_situation,
                                 getPattern(patternIndex),
                                 getBareParams());
}


}
