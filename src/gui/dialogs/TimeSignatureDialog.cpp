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


#include "TimeSignatureDialog.h"
#include <QApplication>

#include <klocale.h>
#include "document/ConfigGroups.h"
#include "base/Composition.h"
#include "base/NotationTypes.h"
#include "gui/widgets/TimeWidget.h"
#include "gui/widgets/BigArrowButton.h"
#include <kconfig.h>
#include <QDialog>
#include <QDialogButtonBox>
#include <QGroupBox>
#include <QCheckBox>
#include <QFont>
#include <QGroupBox>
#include <QLabel>
#include <QObject>
#include <QRadioButton>
#include <QString>
#include <QWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>


namespace Rosegarden
{

TimeSignatureDialog::TimeSignatureDialog(QDialogButtonBox::QWidget *parent,
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
    setWindowTitle(i18n("Time Signature"));

#warning Dialog needs QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel | QDialogButtonBox::Help)

    static QFont *timeSigFont = 0;

    if (timeSigFont == 0) {
        timeSigFont = new QFont("new century schoolbook", 8, QFont::Bold);
        timeSigFont->setPixelSize(20);
    }

    QVBox *vbox = makeVBoxMainWidget();
    QGroupBox *groupBox = new QGroupBox
                          (1, Horizontal, i18n("Time signature"), vbox);
    QWidget *denomBox = new QWidget(groupBox);
    QHBoxLayout *denomBoxLayout = new QHBoxLayout;

    QLabel *explanatoryLabel = 0;
    if (!explanatoryText.isEmpty()) {
        explanatoryLabel = new QLabel(explanatoryText, groupBox);
    }

    BigArrowButton *numDown = new BigArrowButton( numBox , Qt::LeftArrow);
    numBoxLayout->addWidget(numDown);
    BigArrowButton *denomDown = new BigArrowButton( denomBox , Qt::LeftArrow);
    denomBoxLayout->addWidget(denomDown);

    m_numLabel = new QLabel(QString("%1").arg(m_timeSignature.getNumerator()), numBox );
    numBoxLayout->addWidget(m_numLabel);
    m_denomLabel = new QLabel(QString("%1").arg(m_timeSignature.getDenominator()), denomBox );
    denomBoxLayout->addWidget(m_denomLabel);

    m_numLabel->setAlignment(AlignHCenter | AlignVCenter);
    m_denomLabel->setAlignment(AlignHCenter | AlignVCenter);

    m_numLabel->setFont(*timeSigFont);
    m_denomLabel->setFont(*timeSigFont);

    BigArrowButton *numUp = new BigArrowButton( numBox , Qt::RightArrow);
    numBoxLayout->addWidget(numUp);
    numBox->setLayout(numBoxLayout);
    BigArrowButton *denomUp = new BigArrowButton( denomBox , Qt::RightArrow);
    denomBoxLayout->addWidget(denomUp);
    denomBox->setLayout(denomBoxLayout);

    QObject::connect(numDown, SIGNAL(clicked()), this, SLOT(slotNumDown()));
    QObject::connect(numUp, SIGNAL(clicked()), this, SLOT(slotNumUp()));
    QObject::connect(denomDown, SIGNAL(clicked()), this, SLOT(slotDenomDown()));
    QObject::connect(denomUp, SIGNAL(clicked()), this, SLOT(slotDenomUp()));

    if (timeEditable) {

        m_timeEditor = new TimeWidget
                       (i18n("Time where signature takes effect"),
                        vbox,
                        composition,
                        m_time,
                        true);

        m_asGivenButton = 0;
        m_startOfBarButton = 0;

    } else {

        m_timeEditor = 0;

        groupBox = new QGroupBox(1, Horizontal, i18n("Scope"), vbox);

        int barNo = composition->getBarNumber(m_time);
        bool atStartOfBar = (m_time == composition->getBarStart(barNo));

        if (!atStartOfBar) {

            QString scopeText;

            if (barNo != 0 || !atStartOfBar) {
                if (atStartOfBar) {
                    scopeText = QString
                                (i18n("Insertion point is at start of measure %1."))
                                .arg(barNo + 1);
                } else {
                    scopeText = QString
                                (i18n("Insertion point is in the middle of measure %1."))
                                .arg(barNo + 1);
                }
            } else {
                scopeText = QString
                            (i18n("Insertion point is at start of composition."));
            }

            new QLabel(scopeText, groupBox);
            m_asGivenButton = new QRadioButton
                              (i18n("Start measure %1 here", barNo + 2), groupBox);

            if (!atStartOfBar) {
                m_startOfBarButton = new QRadioButton
                                     (i18n("Change time from start of measure %1",
                                       barNo + 1), groupBox);
                m_startOfBarButton->setChecked(true);
            } else {
                m_asGivenButton->setChecked(true);
            }
        } else {
            new QLabel(i18n("Time change will take effect at the start of measure %1.",
                        barNo + 1), groupBox);
        }
    }

    groupBox = new QGroupBox(1, Horizontal, i18n("Options"), vbox);
    QSettings config;
    config.beginGroup( GeneralOptionsConfigGroup );
    // 
    // FIX-manually-(GW), add:
    // config.endGroup();		// corresponding to: config.beginGroup( GeneralOptionsConfigGroup );
    //  


    m_hideSignatureButton = new QCheckBox
                            (i18n("Hide the time signature"), groupBox);
    m_hideSignatureButton->setChecked
    ( qStrToBool( config.value("timesigdialogmakehidden", "false" ) ) );

    m_hideBarsButton = new QCheckBox
                       (i18n("Hide the affected bar lines"), groupBox);
    m_hideBarsButton->setChecked
    ( qStrToBool( config.value("timesigdialogmakehiddenbars", "false" ) ) );

    m_commonTimeButton = new QCheckBox
                         (i18n("Show as common time"), groupBox);
    m_commonTimeButton->setChecked
    ( qStrToBool( config.value("timesigdialogshowcommon", "true" ) ) );

    m_normalizeRestsButton = new QCheckBox
                             (i18n("Correct the durations of following measures"), groupBox);
    m_normalizeRestsButton->setChecked
    ( qStrToBool( config.value("timesigdialognormalize", "true" ) ) );

    QObject::connect(m_hideSignatureButton, SIGNAL(clicked()), this,
                     SLOT(slotUpdateCommonTimeButton()));
    slotUpdateCommonTimeButton();
    m_explanatoryLabel = explanatoryLabel;

    //setHelp("time-signature");
}

TimeSignature
TimeSignatureDialog::getTimeSignature() const
{
    QSettings config;
    config.beginGroup( GeneralOptionsConfigGroup );
    // 
    // FIX-manually-(GW), add:
    // config.endGroup();		// corresponding to: config.beginGroup( GeneralOptionsConfigGroup );
    //  


    config.setValue("timesigdialogmakehidden", m_hideSignatureButton->isChecked());
    config.setValue("timesigdialogmakehiddenbars", m_hideBarsButton->isChecked());
    config.setValue("timesigdialogshowcommon", m_commonTimeButton->isChecked());
    config.setValue("timesigdialognormalize", m_normalizeRestsButton->isChecked());

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
                m_commonTimeButton->setText(i18n("Display as common time"));
                m_commonTimeButton->setEnabled(true);
                return ;
            } else if (m_timeSignature.getNumerator() == 2) {
                m_commonTimeButton->setText(i18n("Display as cut common time"));
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

}
#include "TimeSignatureDialog.moc"
