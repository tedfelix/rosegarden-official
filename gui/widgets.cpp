// -*- c-basic-offset: 4 -*-

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

#include <iostream>

#include <qfontdatabase.h>
#include <qtimer.h>
#include <qapplication.h>
#include <qcursor.h>

#include "widgets.h"
#include "rosedebug.h"

void
RosegardenComboBox::wheelEvent(QWheelEvent *e)
{
    e->accept();

    int value = e->delta();

    if (m_reverse)
         value = -value;
       
    if (value < 0)
    {
        if (currentItem() < count() - 1)
        {
            setCurrentItem(currentItem() + 1);
            emit propagate(currentItem());
        }
    }
    else
    {
        if (currentItem() > 0)
        {
            setCurrentItem(currentItem() - 1);
            emit propagate(currentItem());
        }
    }
}


void
RosegardenTristateCheckBox::mouseReleaseEvent(QMouseEvent *)
{
}

RosegardenTristateCheckBox::~RosegardenTristateCheckBox()
{
}

RosegardenSpinBox::RosegardenSpinBox(QWidget *parent, const char *name):
    QSpinBox(parent, name), m_doubleValue(0)
{
}

QString
RosegardenSpinBox::mapValueToText(int value)
{
    QString doubleStr;

    // Assume we want to show the precision
    //
    if ((int)m_doubleValue != value)
        m_doubleValue = (double) value;

    doubleStr.sprintf("%4.6f", m_doubleValue);

    // clear any special value
    //setSpecialValueText("");

    return doubleStr;
}

int
RosegardenSpinBox::mapTextToValue(bool *ok)
{
    double number = text().toDouble();

    if (number)
    {
        m_doubleValue = number;
        return ((int)number);
    }

    return 120; // default
}


RosegardenParameterBox::RosegardenParameterBox(QString label,
					       QWidget *parent,
					       const char *name) :
    QGroupBox(label, parent, name)
{

/*
    QFontDatabase db;
    QValueList<int> sizes(db.smoothSizes(m_font.family(),
					 db.styleString(m_font)));

    kdDebug(KDEBUG_AREA) << "Family: " << m_font.family()
			 << ", style: " << db.styleString(m_font) << endl;
    
    int size = -1;
    int plainSize = m_font.pointSize();

    for (QValueList<int>::Iterator it = sizes.begin();
	 it != sizes.end(); ++it) {

	kdDebug(KDEBUG_AREA) << "Found size " << *it << endl;

	// find largest size no more than 90% of the plain size
	// and at least 9pt, assuming they're in ascending order
	if (*it >= plainSize) break;
	else if (*it >= 9 && *it <= (plainSize*9)/10) size = *it;
    }

    kdDebug(KDEBUG_AREA) << "Default font: " << plainSize
			 << ", my font: " << size << endl;
    if (size > 0) {
	m_font.setPointSize(size);
    } else {
	m_font.setPointSize(plainSize * 9 / 10);
    }
*/

    QFont plainFont;
    plainFont.setPointSize(plainFont.pointSize() * 9 / 10);
    m_font = plainFont;

    QFont boldFont;
    boldFont.setPointSize(int(boldFont.pointSize() * 9.5 / 10.0 + 0.5));
    boldFont.setBold(true);

    setFont(boldFont);
}

RosegardenProgressDialog::RosegardenProgressDialog(QApplication *app,
                                                   QWidget *creator,
                                                   const char *name,
                                                   bool modal,
                                                   WFlags f):
    QProgressDialog(creator, name, modal, f),
    Rosegarden::Progress(100), // default to percent
    m_app(app)
{
    QTimer *timer = new QTimer(this);
    connect(timer, SIGNAL(timeout()), this, SLOT(slotShowMyself()));
    timer->start(500, TRUE); // half a second

    // set the cursor
    QApplication::setOverrideCursor(QCursor(Qt::waitCursor));
}


RosegardenProgressDialog::RosegardenProgressDialog(
                QApplication *app,
                const QString &labelText,
                const QString &cancelButtonText,
                int totalSteps,
                QWidget *creator,
                const char *name,
                bool modal,
                WFlags f):
        QProgressDialog(labelText,
                        cancelButtonText,
                        totalSteps,
                        creator,
                        name,
                        modal,
                        f),
        Rosegarden::Progress(100), // default to percent
        m_app(app)
{
    QTimer *timer = new QTimer(this);
    connect(timer, SIGNAL(timeout()), this, SLOT(slowShowMyself()));
    timer->start(500, TRUE); // half a second

    QApplication::setOverrideCursor(QCursor(Qt::waitCursor));
}

RosegardenProgressDialog::~RosegardenProgressDialog()
{
    QApplication::restoreOverrideCursor();
}

void
RosegardenProgressDialog::set(int value)
{
    if (value > m_max)
        m_value = m_max;

    setProgress(value);
}

void
RosegardenProgressDialog::process()
{
    if (m_app)
        m_app->processEvents(50);
}

void
RosegardenProgressDialog::slotShowMyself()
{
    show();
    QApplication::setOverrideCursor(QCursor(Qt::waitCursor));
}
