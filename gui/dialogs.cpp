/*
    Rosegarden-4 v0.1
    A sequencer and musical notation editor.

    This program is Copyright 2000-2002
        Guillaume Laurent   <glaurent@telegraph-road.org>,
        Chris Cannam        <cannam@all-day-breakfast.com>,
        Richard Bown        <bownie@bownie.com>

    The moral right of the authors to claim authorship of this work
    has been asserted.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#include "dialogs.h"

#include <qwidget.h>
#include <qlineedit.h>
#include <qlabel.h>
#include <qhbox.h>
#include <qvbox.h>
#include <qlayout.h>

#include <klocale.h>
#include <karrowbutton.h>

using Rosegarden::TimeSignature;


SimpleTextDialog::SimpleTextDialog(QWidget *parent, int maxLength) :
    KDialogBase(parent, 0, true, i18n("Text"), Ok | Cancel)
{
    QHBox *w = makeHBoxMainWidget();
    new QLabel(i18n("Text:"), w);
    m_lineEdit = new QLineEdit(w);
    if (maxLength > 0) m_lineEdit->setMaxLength(maxLength);
    m_lineEdit->setFocus();
}

std::string
SimpleTextDialog::getText() const
{
    return m_lineEdit->text().latin1();
}


class BigArrowButton : public KArrowButton
{
public:
    BigArrowButton(QWidget *parent = 0, Qt::ArrowType arrow = Qt::UpArrow,
		   const char *name = 0) :
	KArrowButton(parent, arrow, name) { }
    virtual ~BigArrowButton() { } 

    virtual QSize sizeHint() const {
	return QSize(20, 20);
    }
};
    

TimeSignatureDialog::TimeSignatureDialog(QWidget *parent,
					 Rosegarden::TimeSignature sig) :
    KDialogBase(parent, 0, true, i18n("Time Signature"), Ok | Cancel),
    m_timeSignature(sig)
{
    static QFont *timeSigFont = 0;

    if (timeSigFont == 0) {
	timeSigFont = new QFont("new century schoolbook", 8, QFont::Bold);
	timeSigFont->setPixelSize(20);
    }

    QVBox *vbox = makeVBoxMainWidget();
    QHBox *numBox = new QHBox(vbox);
    QHBox *denomBox = new QHBox(vbox);

    BigArrowButton *numDown   = new BigArrowButton(numBox, Qt::LeftArrow);
    BigArrowButton *denomDown = new BigArrowButton(denomBox, Qt::LeftArrow);

    m_numLabel   = new QLabel
	(QString("%1").arg(m_timeSignature.getNumerator()), numBox);
    m_denomLabel = new QLabel
	(QString("%1").arg(m_timeSignature.getDenominator()), denomBox);

    m_numLabel->setAlignment(AlignHCenter | AlignVCenter);
    m_denomLabel->setAlignment(AlignHCenter | AlignVCenter);

    m_numLabel->setFont(*timeSigFont);
    m_denomLabel->setFont(*timeSigFont);

    BigArrowButton *numUp     = new BigArrowButton(numBox, Qt::RightArrow);
    BigArrowButton *denomUp   = new BigArrowButton(denomBox, Qt::RightArrow);

    QObject::connect(numDown,   SIGNAL(pressed()), this, SLOT(slotNumDown()));
    QObject::connect(numUp,     SIGNAL(pressed()), this, SLOT(slotNumUp()));
    QObject::connect(denomDown, SIGNAL(pressed()), this, SLOT(slotDenomDown()));
    QObject::connect(denomUp,   SIGNAL(pressed()), this, SLOT(slotDenomUp()));
}

void
TimeSignatureDialog::slotNumDown()
{
    int n = m_timeSignature.getNumerator();
    if (--n >= 1) {
	m_timeSignature = TimeSignature(n, m_timeSignature.getDenominator());
	m_numLabel->setText(QString("%1").arg(n));
    }
}

void
TimeSignatureDialog::slotNumUp()
{
    int n = m_timeSignature.getNumerator();
    if (++n <= 99) {
	m_timeSignature = TimeSignature(n, m_timeSignature.getDenominator());
	m_numLabel->setText(QString("%1").arg(n));
    }
}

void
TimeSignatureDialog::slotDenomDown()
{
    int n = m_timeSignature.getDenominator();
    if ((n /= 2) >= 1) {
	m_timeSignature = TimeSignature(m_timeSignature.getNumerator(), n);
	m_denomLabel->setText(QString("%1").arg(n));
    }
}

void
TimeSignatureDialog::slotDenomUp()
{
    int n = m_timeSignature.getDenominator();
    if ((n *= 2) <= 64) {
	m_timeSignature = TimeSignature(m_timeSignature.getNumerator(), n);
	m_denomLabel->setText(QString("%1").arg(n));
    }
}


