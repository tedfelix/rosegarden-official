// -*- c-basic-offset: 4 -*-

/*
    Rosegarden-4
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
#include <unistd.h>
#include <math.h>

#include <kapp.h>
#include <klocale.h>

#include <qfontdatabase.h>
#include <qtimer.h>
#include <qapplication.h>
#include <qcursor.h>
#include <qpainter.h>
#include <qtooltip.h>

#include "widgets.h"
#include "rosedebug.h"
#include "rosestrings.h"

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
RosegardenSpinBox::mapTextToValue(bool * /*ok*/)
{
    double number = text().toDouble();

    if (number)
    {
        m_doubleValue = number;
        return ((int)number);
    }

    return 120; // default
}


RosegardenParameterBox::RosegardenParameterBox(int strips,
                                               Orientation orientation,
                                               QString label,
					       QWidget *parent,
					       const char *name) :
    QGroupBox(strips, orientation, label, parent, name)
{
    init();
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

    RG_DEBUG << "Family: " << m_font.family()
			 << ", style: " << db.styleString(m_font) << endl;
    
    int size = -1;
    int plainSize = m_font.pointSize();

    for (QValueList<int>::Iterator it = sizes.begin();
	 it != sizes.end(); ++it) {

	RG_DEBUG << "Found size " << *it << endl;

	// find largest size no more than 90% of the plain size
	// and at least 9pt, assuming they're in ascending order
	if (*it >= plainSize) break;
	else if (*it >= 9 && *it <= (plainSize*9)/10) size = *it;
    }

    RG_DEBUG << "Default font: " << plainSize
			 << ", my font: " << size << endl;
    if (size > 0) {
	m_font.setPointSize(size);
    } else {
	m_font.setPointSize(plainSize * 9 / 10);
    }
*/
    init();
}

void RosegardenParameterBox::init()
{
    QFont plainFont;
    plainFont.setPointSize(plainFont.pointSize() * 9 / 10);
    m_font = plainFont;

    QFont boldFont;
    boldFont.setPointSize(int(boldFont.pointSize() * 9.5 / 10.0 + 0.5));
    boldFont.setBold(true);

    setFont(boldFont);
}


RosegardenProgressDialog::RosegardenProgressDialog(QWidget *creator,
                                                   const char *name,
                                                   bool modal):
    KProgressDialog(creator, name,
                    i18n("Processing..."), QString::null, modal),
    m_wasVisible(false),
    m_frozen(false)
{
    setCaption(i18n("Processing..."));
    RG_DEBUG << "RosegardenProgressDialog::RosegardenProgressDialog - "
             << labelText() << endl;

    connect(progressBar(), SIGNAL(percentageChanged (int)),
            this,          SLOT(slotCheckShow(int)));
    
    m_chrono.start();

    CurrentProgressDialog::set(this);
}


RosegardenProgressDialog::RosegardenProgressDialog(
                const QString &labelText,
                int totalSteps,
                QWidget *creator,
                const char *name,
                bool modal) :
    KProgressDialog(creator,
		    name,
                    i18n("Processing..."),
                    labelText,
		    modal),
    m_wasVisible(false),
    m_frozen(false)
{
    progressBar()->setTotalSteps(totalSteps);
    RG_DEBUG << "RosegardenProgressDialog::RosegardenProgressDialog - "
             << labelText << endl;

    connect(progressBar(), SIGNAL(percentageChanged (int)),
            this,          SLOT(slotCheckShow(int)));

    m_chrono.start();

    CurrentProgressDialog::set(this);
}

void
RosegardenProgressDialog::slotSetOperationName(QString name)
{
//     RG_DEBUG << "RosegardenProgressDialog::slotSetOperationName("
//              << name << ") visible : " << isVisible() << endl;
    
    setLabel(name);
}

void RosegardenProgressDialog::slotCancel()
{
    RG_DEBUG << "RosegardenProgressDialog::slotCancel()\n";
    KProgressDialog::slotCancel();
    emit operationCancelled();
}

void RosegardenProgressDialog::slotCheckShow(int)
{
//     RG_DEBUG << "RosegardenProgressDialog::slotCheckShow() : "
//              << m_chrono.elapsed() << " - " << minimumDuration()
//              << endl;

    if (!isVisible() &&
        !m_frozen &&
        m_chrono.elapsed() > minimumDuration()) {
        show();
    }
}

void RosegardenProgressDialog::slotFreeze()
{
    RG_DEBUG << "RosegardenProgressDialog::slotFreeze()\n";

    m_wasVisible = isVisible();
    if (isVisible()) hide();

    mShowTimer->stop();
    m_frozen = true;
}

void RosegardenProgressDialog::slotThaw()
{
    RG_DEBUG << "RosegardenProgressDialog::slotThaw()\n";

    if (m_wasVisible) show();

    // Restart timer
    mShowTimer->start(minimumDuration());
    m_frozen = false;
    m_chrono.restart();
}



//----------------------------------------

RosegardenProgressBar::RosegardenProgressBar(int totalSteps,
					     bool useDelay,
					     QWidget *creator,
					     const char *name,
					     WFlags f) :
    KProgress(totalSteps, creator, name, f)
{
}

bool
RosegardenProgressBar::eventFilter(QObject *watched, QEvent *e)
{
    if (e->type() == QEvent::MouseButtonPress    ||
        e->type() == QEvent::MouseButtonRelease  ||
        e->type() == QEvent::MouseButtonDblClick ||
        e->type() == QEvent::KeyPress            ||
        e->type() == QEvent::KeyRelease          ||
        e->type() == QEvent::DragEnter           ||
        e->type() == QEvent::DragMove            ||
        e->type() == QEvent::DragLeave           ||
        e->type() == QEvent::Drop                ||
        e->type() == QEvent::DragResponse)

        return true;

    else

        return KProgress::eventFilter(watched, e);
}

//----------------------------------------

CurrentProgressDialog* CurrentProgressDialog::getInstance()
{
    if (!m_instance)
        m_instance = new CurrentProgressDialog(0);

    return m_instance;
}


RosegardenProgressDialog*
CurrentProgressDialog::get()
{
    return m_currentProgressDialog;
}

void
CurrentProgressDialog::set(RosegardenProgressDialog* d)
{
    if (m_currentProgressDialog)
        m_currentProgressDialog->disconnect(getInstance());

    m_currentProgressDialog = d;

    // this lets the progress dialog deregister itself when it's deleted
    connect(d, SIGNAL(destroyed()),
            getInstance(), SLOT(slotCurrentProgressDialogDestroyed()));
}

void CurrentProgressDialog::freeze()
{
    if (m_currentProgressDialog)
        m_currentProgressDialog->slotFreeze();
}

void CurrentProgressDialog::thaw()
{
    if (m_currentProgressDialog)
        m_currentProgressDialog->slotThaw();
}

void CurrentProgressDialog::slotCurrentProgressDialogDestroyed()
{
    m_currentProgressDialog = 0;
}

CurrentProgressDialog* CurrentProgressDialog::m_instance = 0;
RosegardenProgressDialog* CurrentProgressDialog::m_currentProgressDialog = 0;

//----------------------------------------

RosegardenFader::RosegardenFader(QWidget *parent):
    QSlider(Qt::Vertical, parent)
{
    connect(this, SIGNAL(valueChanged(int)),
            this, SLOT(slotValueChanged(int)));

}

// We invert the value - so that it appear the top of the fader
// is our maximum and vice versa.  For the moment we only catch
// and re-emit this signal - so beware.
//
void
RosegardenFader::slotValueChanged(int value)
{
    int adjValue = maxValue() - value;
    if (adjValue < 0) adjValue = 0;

    emit faderChanged(adjValue);
}

void 
RosegardenFader::setFader(int value)
{
    emit faderChanged(value);

    value = maxValue() - value;

    if (value > maxValue()) value = maxValue();
    if (value < minValue()) value = minValue();

    setValue(value);
}

// ------------------ RosegardenRotary -----------------
//
//

RosegardenRotary::RosegardenRotary(QWidget *parent,
                                   float minValue,
                                   float maxValue,
                                   float step,
                                   float pageStep,
                                   float initialPosition,
                                   int size):
    QWidget(parent),
    m_minValue(minValue),
    m_maxValue(maxValue),
    m_step(step),
    m_pageStep(pageStep),
    m_size(size),
    m_lastPosition(initialPosition),
    m_position(initialPosition),
    m_buttonPressed(false),
    m_lastY(0),
    m_lastX(0),
    m_knobColour(0, 0, 0)
{
    setFixedSize(size, size);
    QToolTip::add(this, i18n("Click n' Drag - up and down or left to right"));
}


void
RosegardenRotary::setKnobColour(const QColor &colour)
{
    m_knobColour = colour;
    repaint();
}

void
RosegardenRotary::paintEvent(QPaintEvent *e)
{
    QPainter paint(this);

    paint.setClipRegion(e->region());
    paint.setClipRect(e->rect().normalize());

    paint.setPen(kapp->palette().color(QPalette::Active, QColorGroup::Dark));

    if (m_knobColour != Qt::black)
        paint.setBrush(m_knobColour);
    else
        paint.setBrush(
                kapp->palette().color(QPalette::Active, QColorGroup::Base));

    paint.drawEllipse(0, 0, m_size, m_size);

    drawPosition();
}


void
RosegardenRotary::mousePressEvent(QMouseEvent *e)
{
    if (e->button() == LeftButton)
    {
        m_buttonPressed = true;
        m_lastY = e->y();
        m_lastX = e->x();
    }
}

void
RosegardenRotary::mouseReleaseEvent(QMouseEvent *e)
{
    if (e->button() == LeftButton)
    {
        m_buttonPressed = false;
        m_lastY = 0;
        m_lastX = 0;
    }
}

void
RosegardenRotary::mouseMoveEvent(QMouseEvent *e)
{
    if (m_buttonPressed)
    {
        // Dragging by x or y axis when clicked modifies value
        //
        float newValue = m_position +
            (m_lastY - float(e->y()) + float(e->x()) - m_lastX) * m_step;

        if (newValue > m_maxValue)
            m_position = m_maxValue;
        else
        if (newValue < m_minValue)
            m_position = m_minValue;
        else
            m_position = newValue;

        // don't update if there's nothing to update
        if (m_lastPosition == m_position) return;

        m_lastY = e->y();
        m_lastX = e->x();

        drawPosition();

        emit valueChanged(m_position);
    }
}

void
RosegardenRotary::wheelEvent(QWheelEvent *e)
{
    if (e->delta() > 0)
        m_position -= m_pageStep;
    else 
        m_position += m_pageStep;

    if (m_position > m_maxValue)
        m_position = m_maxValue;

    if (m_position < m_minValue)
        m_position = m_minValue;

    drawPosition();

    emit valueChanged(m_position);
}

// Draw the position line - note that we adjust for the minimum value
// so that the knobs always work "vertically"
//
void
RosegardenRotary::drawPosition()
{
    QPainter paint(this);

    double hyp = double(m_size) / 2.0;

    // Undraw the previous line
    //
    double angle = (0.2 * M_PI) // offset 
                   + (1.6 * M_PI * (double(m_lastPosition - m_minValue) /
                     (double(m_maxValue) - double(m_minValue))));

    double x = hyp - 0.8 * hyp * sin(angle);
    double y = hyp + 0.8 * hyp * cos(angle);

    if (m_knobColour != Qt::black)
        paint.setPen(m_knobColour);
    else
        paint.setPen
            (kapp->palette().color(QPalette::Active, QColorGroup::Base));

    paint.drawLine(int(hyp), int(hyp), int(x), int(y));

    // Draw the new position
    //
    angle = (0.2 * M_PI) // offset 
                   + (1.6 * M_PI * (double(m_position - m_minValue) /
                     (double(m_maxValue) - double(m_minValue))));

    x = hyp - 0.8 * hyp * sin(angle);
    y = hyp + 0.8 * hyp * cos(angle);

    paint.setPen(kapp->palette().color(QPalette::Active, QColorGroup::Dark));
    paint.drawLine(int(hyp), int(hyp), int(x), int(y));

    m_lastPosition = m_position;
} 

void
RosegardenRotary::setPosition(float position)
{
    m_position = position;
    drawPosition();
}


