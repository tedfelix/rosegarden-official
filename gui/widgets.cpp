// -*- c-basic-offset: 4 -*-

/*
    Rosegarden-4 v0.2
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
                                                   bool modal,
                                                   WFlags f):
    QProgressDialog(creator, name, modal, f),
    Rosegarden::Progress(100), // default to percent
    m_timeoutSet(clock()),
    m_firstTimeout(true),
    m_shown(false)
{
    setCaption(i18n("Processing..."));
}


RosegardenProgressDialog::RosegardenProgressDialog(
                const QString &labelText,
                const QString &cancelButtonText,
                int totalSteps,
                QWidget *creator,
                const char *name,
                bool modal,
                WFlags f) :
    QProgressDialog(labelText,
		    cancelButtonText,
		    totalSteps,
		    creator,
		    name,
		    modal,
		    f | WDestructiveClose),
    Rosegarden::Progress(totalSteps),
    m_timeoutSet(clock()),
    m_firstTimeout(true),
    m_shown(false)
{
    setCaption(i18n("Processing..."));
}

void
RosegardenProgressDialog::setOperationName(std::string name)
{
    setLabelText(strtoqstr(name));
}

void
RosegardenProgressDialog::setCompleted(int value)
{
    if (value > m_max)
        m_value = m_max;
    else
	m_value = value; //???

    if (m_shown) setProgress(value);
}

void
RosegardenProgressDialog::processEvents()
{
    if (m_shown) {
	kapp->processEvents(50);
	update();
    } else {

	if (m_timeoutSet != 0) {

	    clock_t now = clock();
	    int msElapsed = (now - m_timeoutSet) * 1000 / CLOCKS_PER_SEC;
	    
	    if (msElapsed > 500) {
		if (!m_firstTimeout || ((m_value * 4) < (m_max * 3))) {
		    slotShowMyself();
		} else {
		    m_firstTimeout = false;
		    m_timeoutSet = now;
		}
	    }
	}
    }
}

void
RosegardenProgressDialog::slotShowMyself()
{
    setProgress(m_value);
    show();
    m_shown = true;
}

void
RosegardenProgressDialog::done()
{
    close();
}

//----------------------------------------

RosegardenProgressBar::RosegardenProgressBar(int totalSteps,
					     bool useDelay,
					     QWidget *creator,
					     const char *name,
					     WFlags f) :
    QProgressBar(totalSteps, creator, name, f),
    Rosegarden::Progress(totalSteps),
    m_timeoutSet(0),
    m_shown(true),
    m_useDelay(useDelay),
    m_changedCursor(false)
{
}

RosegardenProgressBar::~RosegardenProgressBar()
{
}

void
RosegardenProgressBar::setCompleted(int value)
{
    if (m_value == 0 && value < m_max/2 && m_useDelay) {
	m_shown = false;
	m_timeoutSet = clock();
    }

    if (value > m_max)
        m_value = m_max;
    else
	m_value = value; //???

    if (m_shown) setProgress(value);
}    

void
RosegardenProgressBar::processEvents()
{
    if (m_timeoutSet != 0) {
	clock_t now = clock();

	if ((now - m_timeoutSet) * 1000 / CLOCKS_PER_SEC > 300) {

	    // we don't really want to call processEvents here because
	    // it's likely to break stuff (we don't have the option of
	    // being a modal dialog because we aren't a dialog so we don't
	    // want to permit editing-type events to happen)

	    setProgress(m_value);
	    m_shown = true;
	    m_timeoutSet = 0;

	    if (!m_changedCursor) {
		QApplication::setOverrideCursor(QCursor(Qt::waitCursor));
		m_changedCursor = true;
	    }
	}
    }

    if (m_shown) update();

    // kapp->processEvents(50); - enabled this if installing as event filter
}

void
RosegardenProgressBar::done()
{
    reset();
    if (m_changedCursor) {
	QApplication::restoreOverrideCursor();
	m_changedCursor = false;
    }
    m_shown = true;
    m_value = 0;
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

        return QProgressBar::eventFilter(watched, e);
}

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

//   ------------- PluginButton ----------------
//
PluginButton::PluginButton(QWidget *parent, int index):
    QPushButton(parent), m_index(index)
{
}

void
PluginButton::mouseReleaseEvent(QMouseEvent * /*e*/)
{
    emit released(m_index);
    setDown(false);
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

    paint.drawLine(hyp, hyp, x, y);

    // Draw the new position
    //
    angle = (0.2 * M_PI) // offset 
                   + (1.6 * M_PI * (double(m_position - m_minValue) /
                     (double(m_maxValue) - double(m_minValue))));

    x = hyp - 0.8 * hyp * sin(angle);
    y = hyp + 0.8 * hyp * cos(angle);

    paint.setPen(kapp->palette().color(QPalette::Active, QColorGroup::Dark));
    paint.drawLine(hyp, hyp, x, y);

    m_lastPosition = m_position;
} 

void
RosegardenRotary::setPosition(float position)
{
    m_position = position;
    drawPosition();
}


