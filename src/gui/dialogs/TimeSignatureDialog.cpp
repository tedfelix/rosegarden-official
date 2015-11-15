/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2015 the Rosegarden development team.
 
    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.
 
    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/


#include "TimeSignatureDialog.h"

#include "misc/ConfigGroups.h"
#include "base/Composition.h"
#include "base/NotationTypes.h"
#include "gui/widgets/TimeWidget.h"
#include "gui/widgets/BigArrowButton.h"
#include "misc/Strings.h"

#include <QApplication>
#include <QSettings>
#include <QDialog>
#include <QDialogButtonBox>
#include <QGroupBox>
#include <QCheckBox>
#include <QFont>
#include <QLabel>
#include <QObject>
#include <QRadioButton>
#include <QString>
#include <QWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QUrl>
#include <QDesktopServices>


namespace Rosegarden
{

TimeSignatureDialog::TimeSignatureDialog(QWidget *parent,
                                         Composition *composition,
                                         timeT insertionTime,
                                         TimeSignature sig,
                                         bool timeEditable,
                                         QString explanatoryText) :
        QDialog(parent),
        m_composition(composition),
        m_timeSignature(sig),
        m_time(insertionTime),
        m_numLabel(0),
        m_denomLabel(0),
        m_explanatoryLabel(0),
        m_commonTimeButton(0),
        m_hideSignatureButton(0),
        m_normalizeRestsButton(0),
        m_asGivenButton(0),
        m_startOfBarButton(0),
        m_timeEditor(0)
{
    setModal(true);
    setWindowTitle(tr("Time Signature"));
    setObjectName("MinorDialog");

    static QFont *timeSigFont = 0;

    if (timeSigFont == 0) {
        timeSigFont = new QFont("new century schoolbook", 8, QFont::Bold);
        timeSigFont->setPixelSize(20);
    }

    QVBoxLayout *vboxLayout = new QVBoxLayout;
    setLayout(vboxLayout);

    QGroupBox *groupBox = new QGroupBox(tr("Time signature"));
    QVBoxLayout *groupBoxLayout = new QVBoxLayout;
    groupBoxLayout->setSpacing(0);
    vboxLayout->addWidget(groupBox);

    QWidget *numBox = new QWidget;
    QHBoxLayout *numBoxLayout = new QHBoxLayout;
    groupBoxLayout->addWidget(numBox);

    QWidget *denomBox = new QWidget;
    QHBoxLayout *denomBoxLayout = new QHBoxLayout;
    groupBoxLayout->addWidget(denomBox);

    QLabel *explanatoryLabel = 0;
    if (!explanatoryText.isEmpty()) {
        explanatoryLabel = new QLabel(explanatoryText, groupBox);
        groupBoxLayout->addWidget(explanatoryLabel);
    }
    groupBox->setLayout(groupBoxLayout);

    BigArrowButton *numDown = new BigArrowButton(Qt::LeftArrow);
    numBoxLayout->addWidget(numDown);
    BigArrowButton *denomDown = new BigArrowButton(Qt::LeftArrow);
    denomBoxLayout->addWidget(denomDown);

    m_numLabel = new QLabel(QString("%1").arg(m_timeSignature.getNumerator()), numBox );
    numBoxLayout->addWidget(m_numLabel);
    m_denomLabel = new QLabel(QString("%1").arg(m_timeSignature.getDenominator()), denomBox );
    denomBoxLayout->addWidget(m_denomLabel);

    m_numLabel->setAlignment(Qt::AlignCenter);
    m_denomLabel->setAlignment(Qt::AlignCenter);

    m_numLabel->setFont(*timeSigFont);
    m_denomLabel->setFont(*timeSigFont);

    BigArrowButton *numUp = new BigArrowButton(Qt::RightArrow);
    numBoxLayout->addWidget(numUp);
    numBox->setLayout(numBoxLayout);
    BigArrowButton *denomUp = new BigArrowButton(Qt::RightArrow);
    denomBoxLayout->addWidget(denomUp);
    denomBox->setLayout(denomBoxLayout);

    QObject::connect(numDown, SIGNAL(clicked()), this, SLOT(slotNumDown()));
    QObject::connect(numUp, SIGNAL(clicked()), this, SLOT(slotNumUp()));
    QObject::connect(denomDown, SIGNAL(clicked()), this, SLOT(slotDenomDown()));
    QObject::connect(denomUp, SIGNAL(clicked()), this, SLOT(slotDenomUp()));

    if (timeEditable) {

        m_timeEditor = new TimeWidget
                       (tr("Time where signature takes effect"),
                        this,
                        composition,
                        m_time,
                        true);
        vboxLayout->addWidget(m_timeEditor);
        m_asGivenButton = 0;
        m_startOfBarButton = 0;

    } else {

        m_timeEditor = 0;

        groupBox = new QGroupBox(tr("Scope"));
        groupBoxLayout = new QVBoxLayout;
        vboxLayout->addWidget(groupBox);

        int barNo = composition->getBarNumber(m_time);
        bool atStartOfBar = (m_time == composition->getBarStart(barNo));

        if (!atStartOfBar) {

            QString scopeText;

            if (barNo != 0 || !atStartOfBar) {
                if (atStartOfBar) {
                    scopeText = QString
                                (tr("Insertion point is at start of measure %1."))
                                .arg(barNo + 1);
                } else {
                    scopeText = QString
                                (tr("Insertion point is in the middle of measure %1."))
                                .arg(barNo + 1);
                }
            } else {
                scopeText = QString
                            (tr("Insertion point is at start of composition."));
            }

            groupBoxLayout->addWidget(new QLabel(scopeText));
            m_asGivenButton = new QRadioButton
                              (tr("Start measure %1 here").arg(barNo + 2), groupBox);
            groupBoxLayout->addWidget(m_asGivenButton);
            if (!atStartOfBar) {
                m_startOfBarButton = new QRadioButton
                                     (tr("Change time from start of measure %1")
                                       .arg(barNo + 1), groupBox);
                groupBoxLayout->addWidget(m_startOfBarButton);
                m_startOfBarButton->setChecked(true);
            } else {
                m_asGivenButton->setChecked(true);
            }
        } else {
            groupBoxLayout->addWidget(
                new QLabel(tr("Time change will take effect at the start of"
                                " measure %1.").arg(barNo + 1)));
        }
    }
    groupBox->setLayout(groupBoxLayout);

    groupBox = new QGroupBox(tr("Options"));
    groupBoxLayout = new QVBoxLayout;
    vboxLayout->addWidget(groupBox);
    QSettings settings;
    settings.beginGroup( GeneralOptionsConfigGroup );

    m_hideSignatureButton = new QCheckBox(tr("Hide the time signature"));
    groupBoxLayout->addWidget(m_hideSignatureButton);
    m_hideSignatureButton->setChecked
    ( qStrToBool( settings.value("timesigdialogmakehidden", "false" ) ) );

    m_hideBarsButton = new QCheckBox(tr("Hide the affected bar lines"));
    groupBoxLayout->addWidget(m_hideBarsButton);
    m_hideBarsButton->setChecked
    ( qStrToBool( settings.value("timesigdialogmakehiddenbars", "false" ) ) );

    m_commonTimeButton = new QCheckBox(tr("Show as common time"));
    groupBoxLayout->addWidget(m_commonTimeButton);
    m_commonTimeButton->setChecked
    ( qStrToBool( settings.value("timesigdialogshowcommon", "true" ) ) );

    m_normalizeRestsButton = new QCheckBox
                             (tr("Correct the durations of following measures"));
    groupBoxLayout->addWidget(m_normalizeRestsButton);
    m_normalizeRestsButton->setChecked
    ( qStrToBool( settings.value("timesigdialognormalize", "true" ) ) );

    groupBox->setLayout(groupBoxLayout);

    QObject::connect(m_hideSignatureButton, SIGNAL(clicked()), this,
                     SLOT(slotUpdateCommonTimeButton()));
    slotUpdateCommonTimeButton();
    m_explanatoryLabel = explanatoryLabel;

    settings.endGroup();

    QDialogButtonBox *buttonBox = new QDialogButtonBox(  QDialogButtonBox::Ok
                                                       | QDialogButtonBox::Cancel
                                                       | QDialogButtonBox::Help);

    vboxLayout->addWidget(buttonBox);

    connect(buttonBox, SIGNAL(accepted()), this, SLOT(accept()));
    connect(buttonBox, SIGNAL(rejected()), this, SLOT(reject()));
    connect(buttonBox, SIGNAL(helpRequested()), this, SLOT(slotHelpRequested()));

}

TimeSignature
TimeSignatureDialog::getTimeSignature() const
{
    QSettings settings;
    settings.beginGroup( GeneralOptionsConfigGroup );

    settings.setValue("timesigdialogmakehidden", m_hideSignatureButton->isChecked());
    settings.setValue("timesigdialogmakehiddenbars", m_hideBarsButton->isChecked());
    settings.setValue("timesigdialogshowcommon", m_commonTimeButton->isChecked());
    settings.setValue("timesigdialognormalize", m_normalizeRestsButton->isChecked());

    TimeSignature ts(m_timeSignature.getNumerator(),
                     m_timeSignature.getDenominator(),
                     (m_commonTimeButton &&
                      m_commonTimeButton->isEnabled() &&
                      m_commonTimeButton->isChecked()),
                     (m_hideSignatureButton &&
                      m_hideSignatureButton->isEnabled() &&
                      m_hideSignatureButton->isChecked()),
                     (m_hideBarsButton &&
                      m_hideBarsButton->isEnabled() &&
                      m_hideBarsButton->isChecked()));

    settings.endGroup();

    return ts;
}

void
TimeSignatureDialog::slotNumDown()
{
    int n = m_timeSignature.getNumerator();
    if (--n >= 1) {
        m_timeSignature = TimeSignature(n, m_timeSignature.getDenominator());
        m_numLabel->setText(QString("%1").arg(n));
    }
    slotUpdateCommonTimeButton();
}

void
TimeSignatureDialog::slotNumUp()
{
    int n = m_timeSignature.getNumerator();
    if (++n <= 99) {
        m_timeSignature = TimeSignature(n, m_timeSignature.getDenominator());
        m_numLabel->setText(QString("%1").arg(n));
    }
    slotUpdateCommonTimeButton();
}

void
TimeSignatureDialog::slotDenomDown()
{
    int n = m_timeSignature.getDenominator();
    if ((n /= 2) >= 1) {
        m_timeSignature = TimeSignature(m_timeSignature.getNumerator(), n);
        m_denomLabel->setText(QString("%1").arg(n));
    }
    slotUpdateCommonTimeButton();
}

void
TimeSignatureDialog::slotDenomUp()
{
    int n = m_timeSignature.getDenominator();
    if ((n *= 2) <= 64) {
        m_timeSignature = TimeSignature(m_timeSignature.getNumerator(), n);
        m_denomLabel->setText(QString("%1").arg(n));
    }
    slotUpdateCommonTimeButton();
}

void
TimeSignatureDialog::slotUpdateCommonTimeButton()
{
    if (m_explanatoryLabel)
        m_explanatoryLabel->hide();
    if (!m_hideSignatureButton || !m_hideSignatureButton->isChecked()) {
        if (m_timeSignature.getDenominator() == m_timeSignature.getNumerator()) {
            if (m_timeSignature.getNumerator() == 4) {
                m_commonTimeButton->setText(tr("Display as common time"));
                m_commonTimeButton->setEnabled(true);
                return ;
            } else if (m_timeSignature.getNumerator() == 2) {
                m_commonTimeButton->setText(tr("Display as cut common time"));
                m_commonTimeButton->setEnabled(true);
                return ;
            }
        }
    }
    m_commonTimeButton->setEnabled(false);
}

timeT
TimeSignatureDialog::getTime() const
{
    if (m_timeEditor) {
        return m_timeEditor->getTime();
    } else if (m_asGivenButton && m_asGivenButton->isChecked()) {
        return m_time;
    } else if (m_startOfBarButton && m_startOfBarButton->isChecked()) {
        int barNo = m_composition->getBarNumber(m_time);
        return m_composition->getBarStart(barNo);
    } else {
        return m_time;
    }
}

bool
TimeSignatureDialog::shouldNormalizeRests() const
{
    return (m_normalizeRestsButton && m_normalizeRestsButton->isEnabled() &&
            m_normalizeRestsButton->isChecked());
}


void
TimeSignatureDialog::slotHelpRequested()
{
    // TRANSLATORS: if the manual is translated into your language, you can
    // change the two-letter language code in this URL to point to your language
    // version, eg. "http://rosegardenmusic.com/wiki/doc:timeSignatureDialog-es" for the
    // Spanish version. If your language doesn't yet have a translation, feel
    // free to create one.
    QString helpURL = tr("http://rosegardenmusic.com/wiki/doc:timeSignatureDialog-en");
    QDesktopServices::openUrl(QUrl(helpURL));
}
}
