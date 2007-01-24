/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
 
    This program is Copyright 2000-2007
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


#include "TimeSignatureDialog.h"
#include <kapplication.h>

#include <klocale.h>
#include "document/ConfigGroups.h"
#include "base/Composition.h"
#include "base/NotationTypes.h"
#include "gui/widgets/TimeWidget.h"
#include "gui/widgets/BigArrowButton.h"
#include <kconfig.h>
#include <kdialogbase.h>
#include <qbuttongroup.h>
#include <qcheckbox.h>
#include <qfont.h>
#include <qgroupbox.h>
#include <qhbox.h>
#include <qlabel.h>
#include <qobject.h>
#include <qradiobutton.h>
#include <qstring.h>
#include <qvbox.h>
#include <qwidget.h>


namespace Rosegarden
{

TimeSignatureDialog::TimeSignatureDialog(QWidget *parent,
        Composition *composition,
        timeT insertionTime,
        TimeSignature sig,
        bool timeEditable,
        QString explanatoryText) :
        KDialogBase(parent, 0, true, i18n("Time Signature"), Ok | Cancel | Help),
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
    static QFont *timeSigFont = 0;

    if (timeSigFont == 0) {
        timeSigFont = new QFont("new century schoolbook", 8, QFont::Bold);
        timeSigFont->setPixelSize(20);
    }

    QVBox *vbox = makeVBoxMainWidget();
    QGroupBox *groupBox = new QGroupBox
                          (1, Horizontal, i18n("Time signature"), vbox);
    QHBox *numBox = new QHBox(groupBox);
    QHBox *denomBox = new QHBox(groupBox);

    QLabel *explanatoryLabel = 0;
    if (explanatoryText) {
        explanatoryLabel = new QLabel(explanatoryText, groupBox);
    }

    BigArrowButton *numDown = new BigArrowButton(numBox, Qt::LeftArrow);
    BigArrowButton *denomDown = new BigArrowButton(denomBox, Qt::LeftArrow);

    m_numLabel = new QLabel
                 (QString("%1").arg(m_timeSignature.getNumerator()), numBox);
    m_denomLabel = new QLabel
                   (QString("%1").arg(m_timeSignature.getDenominator()), denomBox);

    m_numLabel->setAlignment(AlignHCenter | AlignVCenter);
    m_denomLabel->setAlignment(AlignHCenter | AlignVCenter);

    m_numLabel->setFont(*timeSigFont);
    m_denomLabel->setFont(*timeSigFont);

    BigArrowButton *numUp = new BigArrowButton(numBox, Qt::RightArrow);
    BigArrowButton *denomUp = new BigArrowButton(denomBox, Qt::RightArrow);

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

        groupBox = new QButtonGroup(1, Horizontal, i18n("Scope"), vbox);

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
                              (i18n("Start measure %1 here").arg(barNo + 2), groupBox);

            if (!atStartOfBar) {
                m_startOfBarButton = new QRadioButton
                                     (i18n("Change time from start of measure %1")
                                      .arg(barNo + 1), groupBox);
                m_startOfBarButton->setChecked(true);
            } else {
                m_asGivenButton->setChecked(true);
            }
        } else {
            new QLabel(i18n("Time change will take effect at the start of measure %1.")
                       .arg(barNo + 1), groupBox);
        }
    }

    groupBox = new QGroupBox(1, Horizontal, i18n("Options"), vbox);
    KConfig *config = kapp->config();
    config->setGroup(GeneralOptionsConfigGroup);

    m_hideSignatureButton = new QCheckBox
                            (i18n("Make the time signature hidden"), groupBox);
    m_hideSignatureButton->setChecked
    (config->readBoolEntry("timesigdialogmakehidden", false));

    m_hideBarsButton = new QCheckBox
                       (i18n("Make subsequent bar lines hidden"), groupBox);
    m_hideBarsButton->setChecked
    (config->readBoolEntry("timesigdialogmakehiddenbars", false));

    m_commonTimeButton = new QCheckBox
                         (i18n("Show as common time"), groupBox);
    m_commonTimeButton->setChecked
    (config->readBoolEntry("timesigdialogshowcommon", true));

    m_normalizeRestsButton = new QCheckBox
                             (i18n("Correct the durations of following measures"), groupBox);
    m_normalizeRestsButton->setChecked
    (config->readBoolEntry("timesigdialognormalize", true));

    QObject::connect(m_hideSignatureButton, SIGNAL(clicked()), this,
                     SLOT(slotUpdateCommonTimeButton()));
    slotUpdateCommonTimeButton();
    m_explanatoryLabel = explanatoryLabel;

    setHelp("time-signature");
}

TimeSignature
TimeSignatureDialog::getTimeSignature() const
{
    KConfig *config = kapp->config();
    config->setGroup(GeneralOptionsConfigGroup);

    config->writeEntry("timesigdialogmakehidden", m_hideSignatureButton->isChecked());
    config->writeEntry("timesigdialogmakehiddenbars", m_hideBarsButton->isChecked());
    config->writeEntry("timesigdialogshowcommon", m_commonTimeButton->isChecked());
    config->writeEntry("timesigdialognormalize", m_normalizeRestsButton->isChecked());

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
