/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2024 the Rosegarden development team.

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
                                         const TimeSignature& defaultSig,
                                         bool timeEditable,
                                         QString explanatoryText) :
        QDialog(parent),
        m_composition(composition),
        m_timeSignature(defaultSig),
        m_time(insertionTime),
        m_numLabel(nullptr),
        m_denomLabel(nullptr),
        m_explanatoryLabel(nullptr),
        m_commonTimeButton(nullptr),
        m_hideSignatureButton(nullptr),
        m_normalizeRestsButton(nullptr),
        m_asGivenButton(nullptr),
        m_startOfBarButton(nullptr),
        m_timeEditor(nullptr)
{
    setModal(true);
    setWindowTitle(tr("Time Signature"));
    setObjectName("MinorDialog");

    static QFont *timeSigFont = nullptr;

    if (timeSigFont == nullptr) {
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

    QLabel *explanatoryLabel = nullptr;
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

    QObject::connect(numDown, &QAbstractButton::clicked, this, &TimeSignatureDialog::slotNumDown);
    QObject::connect(numUp, &QAbstractButton::clicked, this, &TimeSignatureDialog::slotNumUp);
    QObject::connect(denomDown, &QAbstractButton::clicked, this, &TimeSignatureDialog::slotDenomDown);
    QObject::connect(denomUp, &QAbstractButton::clicked, this, &TimeSignatureDialog::slotDenomUp);

    if (timeEditable) {

        m_timeEditor = new TimeWidget
                       (tr("Time where signature takes effect"),
                        this,
                        composition,
                        m_time,
                        true);
        vboxLayout->addWidget(m_timeEditor);
        m_asGivenButton = nullptr;
        m_startOfBarButton = nullptr;

    } else {

        m_timeEditor = nullptr;

        groupBox = new QGroupBox(tr("Scope"));
        groupBoxLayout = new QVBoxLayout;
        vboxLayout->addWidget(groupBox);

        int barNo = composition->getBarNumber(m_time);
        bool atStartOfBar = (m_time == composition->getBarStart(barNo));

        if (!atStartOfBar) {

            QString scopeText;

            scopeText = QString
                (tr("Insertion point is in the middle of measure %1."))
                .arg(barNo + 1);

            groupBoxLayout->addWidget(new QLabel(scopeText));
            m_asGivenButton = new QRadioButton
                (tr("Start measure %1 here").arg(barNo + 2), groupBox);
            groupBoxLayout->addWidget(m_asGivenButton);
            m_startOfBarButton = new QRadioButton
                (tr("Change time from start of measure %1")
                 .arg(barNo + 1), groupBox);
            groupBoxLayout->addWidget(m_startOfBarButton);
            m_startOfBarButton->setChecked(true);
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

    QObject::connect(m_hideSignatureButton, &QAbstractButton::clicked, this,
                     &TimeSignatureDialog::slotUpdateCommonTimeButton);
    slotUpdateCommonTimeButton();
    m_explanatoryLabel = explanatoryLabel;

    settings.endGroup();

    QDialogButtonBox *buttonBox = new QDialogButtonBox(  QDialogButtonBox::Ok
                                                       | QDialogButtonBox::Cancel
                                                       | QDialogButtonBox::Help);

    vboxLayout->addWidget(buttonBox);

    connect(buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
    connect(buttonBox, &QDialogButtonBox::helpRequested, this, &TimeSignatureDialog::slotHelpRequested);

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
