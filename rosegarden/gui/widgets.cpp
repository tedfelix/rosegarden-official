// -*- c-basic-offset: 4 -*-

/*
    Rosegarden-4
    A sequencer and musical notation editor.

    This program is Copyright 2000-2006
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
#include <cmath>

#include <qfontdatabase.h>
#include <qtimer.h>
#include <qapplication.h>
#include <qcursor.h>
#include <qpainter.h>
#include <qtooltip.h>
#include <qlayout.h>
#include <qtextedit.h>
#include <qlineedit.h>
#include <qbitmap.h>
#include <qeventloop.h>
#include <qimage.h>

#include <klocale.h>
#include <kconfig.h>
#include <kcombobox.h>

#include "rgapplication.h"
#include "widgets.h"
#include "rosedebug.h"
#include "rosestrings.h"
#include "notationstrings.h"
#include "notepixmapfactory.h"
#include "colours.h"
#include "midipitchlabel.h"
#include "commondialogs.h"

#include "Quantizer.h"
#include "Composition.h" // currently for time widget only

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
    double number = qstrtodouble(text());

    if (number)
    {
        m_doubleValue = number;
        return ((int)number);
    }

    return 120; // default
}


RosegardenParameterBox::RosegardenParameterBox(const QString &label,
					       QWidget *parent,
					       const char *name) :
  QFrame(parent, name),
  m_label(label),
  m_mode(LANDSCAPE_MODE)
{
  init();
}

void RosegardenParameterBox::init()
{
    QFont plainFont;
    plainFont.setPointSize(plainFont.pointSize() * 95 / 100);
    if (plainFont.pixelSize() > 14) plainFont.setPixelSize(14);
    plainFont.setBold(false);
    m_font = plainFont;

    QFont boldFont;
    boldFont.setPointSize(int(boldFont.pointSize() * 9.5 / 10.0 + 0.5));
    if (boldFont.pixelSize() > 14) boldFont.setPixelSize(14);
    boldFont.setBold(true);

    setFont(boldFont);
}

// Return the string that should be used to label a given parameter box.

QString RosegardenParameterBox::getLabel() const
{
  return m_label;
}

bool RosegardenProgressDialog::m_modalVisible = false;

RosegardenProgressDialog::RosegardenProgressDialog(QWidget *creator,
                                                   const char *name,
                                                   bool modal):
    KProgressDialog(creator, name,
                    i18n("Processing..."), QString::null, modal),
    m_wasVisible(false),
    m_frozen(false),
    m_modal(modal)
{
    setCaption(i18n("Processing..."));
    RG_DEBUG << "RosegardenProgressDialog::RosegardenProgressDialog type 1 - "
             << labelText() << " - modal : " << modal << endl;

    connect(progressBar(), SIGNAL(percentageChanged (int)),
            this,          SLOT(slotCheckShow(int)));
    
    m_chrono.start();

    CurrentProgressDialog::set(this);

    setMinimumDuration(500); // set a default value for this
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
    m_frozen(false),
    m_modal(modal)
{
    progressBar()->setTotalSteps(totalSteps);

    RG_DEBUG << "RosegardenProgressDialog::RosegardenProgressDialog type 2 - "
             << labelText << " - modal : " << modal << endl;

    connect(progressBar(), SIGNAL(percentageChanged (int)),
            this,          SLOT(slotCheckShow(int)));

    m_chrono.start();

    CurrentProgressDialog::set(this);

    setMinimumDuration(500); // set a default value for this
}

RosegardenProgressDialog::~RosegardenProgressDialog()
{
    m_modalVisible = false;
}

void
RosegardenProgressDialog::polish()
{
    KProgressDialog::polish();

    if (allowCancel())
        setCursor(Qt::ArrowCursor);
    else
        QApplication::setOverrideCursor(QCursor(Qt::waitCursor));
}

void RosegardenProgressDialog::hideEvent(QHideEvent* e)
{
    if (!allowCancel())
        QApplication::restoreOverrideCursor();
    
    KProgressDialog::hideEvent(e);
    m_modalVisible = false;
}

void
RosegardenProgressDialog::slotSetOperationName(QString name)
{
//     RG_DEBUG << "RosegardenProgressDialog::slotSetOperationName("
//              << name << ") visible : " << isVisible() << endl;
    
    setLabel(name);
    // Little trick stolen from QProgressDialog
    // increase resize only, never shrink
    int w = QMAX( isVisible() ? width() : 0, sizeHint().width() );
    int h = QMAX( isVisible() ? height() : 0, sizeHint().height() );
    resize( w, h );
}

void RosegardenProgressDialog::slotCancel()
{
    RG_DEBUG << "RosegardenProgressDialog::slotCancel()\n";
    KProgressDialog::slotCancel();
}

void RosegardenProgressDialog::slotCheckShow(int)
{
//     RG_DEBUG << "RosegardenProgressDialog::slotCheckShow() : "
//              << m_chrono.elapsed() << " - " << minimumDuration()
//              << endl;

    if (!isVisible() &&
        !m_frozen &&
        m_chrono.elapsed() > minimumDuration()) {
        RG_DEBUG << "RosegardenProgressDialog::slotCheckShow() : showing dialog\n";
        show();
	if (m_modal) m_modalVisible = true;
        processEvents();
    }
}

void RosegardenProgressDialog::slotFreeze()
{
    RG_DEBUG << "RosegardenProgressDialog::slotFreeze()\n";

    m_wasVisible = isVisible();
    if (isVisible()) {
	m_modalVisible = false;
	hide();
    }

    // This is also a convenient place to ensure the wait cursor (if
    // currently shown) returns to the original cursor to ensure that
    // the user can respond to whatever's freezing the progress dialog
    QApplication::restoreOverrideCursor();

    mShowTimer->stop();
    m_frozen = true;
}

void RosegardenProgressDialog::slotThaw()
{
    RG_DEBUG << "RosegardenProgressDialog::slotThaw()\n";

    if (m_wasVisible) {
	if (m_modal) m_modalVisible = true;
	show();
    }

    // Restart timer
    mShowTimer->start(minimumDuration());
    m_frozen = false;
    m_chrono.restart();
}

void RosegardenProgressDialog::processEvents()
{
//    RG_DEBUG << "RosegardenProgressDialog::processEvents: modalVisible is "
//	     << m_modalVisible << endl;
    if (m_modalVisible) {
	kapp->processEvents(50);
    } else {
	rgapp->refreshGUI(50);
    }
}


//----------------------------------------

RosegardenProgressBar::RosegardenProgressBar(int totalSteps,
					     bool /*useDelay*/,
					     QWidget *creator,
					     const char *name,
					     WFlags f) :
    KProgress(totalSteps, creator, name, f)
{
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



// ------------------ RosegardenRotary -----------------
//
//
#define ROTARY_MIN (0.25 * M_PI)
#define ROTARY_MAX (1.75 * M_PI)
#define ROTARY_RANGE (ROTARY_MAX - ROTARY_MIN)

static RosegardenTextFloat* _float = 0;
static QTimer *_floatTimer = 0;
RosegardenRotary::PixmapCache RosegardenRotary::m_pixmaps;

RosegardenRotary::RosegardenRotary(QWidget *parent,
                                   float minValue,
                                   float maxValue,
                                   float step,
                                   float pageStep,
                                   float initialPosition,
                                   int size,
				   TickMode ticks,
				   bool snapToTicks,
				   bool centred) :
    QWidget(parent),
    m_minValue(minValue),
    m_maxValue(maxValue),
    m_step(step),
    m_pageStep(pageStep),
    m_size(size),
    m_tickMode(ticks),
    m_snapToTicks(snapToTicks),
    m_centred(centred),
    m_position(initialPosition),
    m_snapPosition(m_position),
    m_initialPosition(initialPosition),
    m_buttonPressed(false),
    m_lastY(0),
    m_lastX(0),
    m_knobColour(0, 0, 0)
{
    setBackgroundMode(Qt::NoBackground);

    if (!_float) _float = new RosegardenTextFloat(this);

    if (!_floatTimer)
    {
        _floatTimer = new QTimer();
    }

    // connect timer
    connect(_floatTimer, SIGNAL(timeout()), this,
            SLOT(slotFloatTimeout()));
    _float->hide();

    QToolTip::add(this,
                  i18n("Click and drag up and down or left and right to modify.\nDouble click to edit value directly."));
    setFixedSize(size, size);

    emit valueChanged(m_snapPosition);
}

// If we destroy any Rotary then destroy the associated float
// so that we recreate on the next 

RosegardenRotary::~RosegardenRotary()
{
    // Remove this connection
    //
    disconnect(_floatTimer, SIGNAL(timeout()), this,
               SLOT(slotFloatTimeout()));

    delete _float;
    _float = 0;
}


void
RosegardenRotary::slotFloatTimeout()
{
    if (_float) _float->hide();
}

void
RosegardenRotary::setKnobColour(const QColor &colour)
{
    m_knobColour = colour;
    repaint();
}

void
RosegardenRotary::paintEvent(QPaintEvent *)
{
    QPainter paint;

    double angle = ROTARY_MIN // offset 
                   + (ROTARY_RANGE *
		      (double(m_snapPosition - m_minValue) /
		       (double(m_maxValue) - double(m_minValue))));
    int degrees = int(angle * 180.0 / M_PI);

//    RG_DEBUG << "degrees: " << degrees << ", size " << m_size << ", pixel " << m_knobColour.pixel() << endl;

    int numTicks = 0;
    switch (m_tickMode) {
    case LimitTicks:
	numTicks = 2;
	break;
    case IntervalTicks:
	numTicks = 5;
	break;
    case PageStepTicks:
	numTicks = 1 + (m_maxValue + 0.0001 - m_minValue) / m_pageStep;
	break;
    case StepTicks:
	numTicks = 1 + (m_maxValue + 0.0001 - m_minValue) / m_step;
	break;
    default: break;
    }

    CacheIndex index(m_size, m_knobColour.pixel(), degrees, numTicks, m_centred);

    if (m_pixmaps.find(index) != m_pixmaps.end()) {
	paint.begin(this);
	paint.drawPixmap(0, 0, m_pixmaps[index]);
	paint.end();
	return;
    }

    int scale = 4;
    int width = m_size * scale;
    QPixmap map(width, width);
    map.fill(paletteBackgroundColor());
    paint.begin(&map);

    QPen pen;
    pen.setColor(kapp->palette().color(QPalette::Active, QColorGroup::Dark));
    pen.setWidth(scale);
    paint.setPen(pen);

    if (m_knobColour != Qt::black) {
        paint.setBrush(m_knobColour);
    } else {
        paint.setBrush(
                kapp->palette().color(QPalette::Active, QColorGroup::Base));
    }

    QColor c(m_knobColour);
    pen.setColor(c);
    paint.setPen(pen);
    
    int indent = width * 0.15 + 1;
    
    paint.drawEllipse(indent, indent, width-2*indent, width-2*indent);
    
    pen.setWidth(2 * scale);
    int pos = indent + (width-2*indent)/8;
    int darkWidth = (width-2*indent) * 2 / 3;
    int darkQuote = (130 * 2 / (darkWidth ? darkWidth : 1)) + 100;
    while (darkWidth) {
	c = c.light(101);
	pen.setColor(c);
	paint.setPen(pen);
	paint.drawEllipse(pos, pos, darkWidth, darkWidth);
	if (!--darkWidth) break;
	paint.drawEllipse(pos, pos, darkWidth, darkWidth);
	if (!--darkWidth) break;
	paint.drawEllipse(pos, pos, darkWidth, darkWidth);
	++pos; --darkWidth;
    }
    
    paint.setBrush(QBrush::NoBrush);
    
    pen.setColor(colorGroup().dark());
    pen.setWidth(scale);
    paint.setPen(pen);

    for (int i = 0; i < numTicks; ++i) {
	int div = numTicks;
	if (div > 1) --div;
	drawTick(paint, ROTARY_MIN + (ROTARY_MAX - ROTARY_MIN) * i / div,
		 width, i != 0 && i != numTicks-1);
    }

    // now the bright metering bit

    pen.setColor(Rosegarden::GUIPalette::getColour(Rosegarden::GUIPalette::RotaryMeter));
    pen.setWidth(indent);
    paint.setPen(pen);

    if (m_centred) {
	paint.drawArc(indent/2, indent/2, width-indent, width-indent,
		      90 * 16, -(degrees - 180) * 16);
    } else {
	paint.drawArc(indent/2, indent/2, width-indent, width-indent,
		      (180 + 45) * 16, -(degrees - 45) * 16);
    }
    
    pen.setWidth(scale);
    paint.setPen(pen);
    
    int shadowAngle = -720;
    c = colorGroup().dark();
    for (int arc = 120; arc < 2880; arc += 240) {
	pen.setColor(c);
	paint.setPen(pen);
	paint.drawArc(indent, indent, width-2*indent, width-2*indent, shadowAngle + arc, 240);
	paint.drawArc(indent, indent, width-2*indent, width-2*indent, shadowAngle - arc, 240);
	c = c.light( 110 );
    }
    
    shadowAngle = 2160;
    c = colorGroup().dark();
    for (int arc = 120; arc < 2880; arc += 240) {
	pen.setColor(c);
	paint.setPen(pen);
	paint.drawArc(scale/2, scale/2, width-scale, width-scale, shadowAngle + arc, 240);
	paint.drawArc(scale/2, scale/2, width-scale, width-scale, shadowAngle - arc, 240);
	c = c.light( 109 );
    }
    
    // and un-draw the bottom part
    pen.setColor(paletteBackgroundColor());
    paint.setPen(pen);
    paint.drawArc(scale/2, scale/2, width-scale, width-scale,
		  -45 * 16, -90 * 16);

    double hyp = double(width) / 2.0;
    double len = hyp - indent;
    --len;

    double x0 = hyp;
    double y0 = hyp;

    double x = hyp - len * sin(angle);
    double y = hyp + len * cos(angle);

    pen.setWidth(scale * 2);
    pen.setColor(colorGroup().dark());
    paint.setPen(pen);

    paint.drawLine(int(x0), int(y0), int(x), int(y));

    paint.end();

    QImage i = map.convertToImage().smoothScale(m_size, m_size);
    m_pixmaps[index] = QPixmap(i);
    paint.begin(this);
    paint.drawPixmap(0, 0, m_pixmaps[index]);
    paint.end();
}

void
RosegardenRotary::drawTick(QPainter &paint, double angle, int size, bool internal)
{
    double hyp = double(size) / 2.0;
    double x0 = hyp - (hyp - 1) * sin(angle);
    double y0 = hyp + (hyp - 1) * cos(angle);
    
    if (internal) {

	double len = hyp / 4;
	double x1 = hyp - (hyp - len) * sin(angle);
	double y1 = hyp + (hyp - len) * cos(angle);
    
	paint.drawLine(int(x0), int(y0), int(x1), int(y1));

    } else {

	double len = hyp / 4;
	double x1 = hyp - (hyp + len) * sin(angle);
	double y1 = hyp + (hyp + len) * cos(angle);
    
	paint.drawLine(int(x0), int(y0), int(x1), int(y1));
    }
}
    


void
RosegardenRotary::snapPosition()
{
    m_snapPosition = m_position;

    if (m_snapToTicks) {

	switch (m_tickMode) {

	case NoTicks: break; // meaningless

	case LimitTicks:
	    if (m_position < (m_minValue + m_maxValue) / 2.0) {
		m_snapPosition = m_minValue;
	    } else {
		m_snapPosition = m_maxValue;
	    }
	    break;

	case IntervalTicks:
	    m_snapPosition = m_minValue +
		(m_maxValue - m_minValue) / 4.0 *
		int((m_snapPosition - m_minValue) /
		    ((m_maxValue - m_minValue) / 4.0));
	    break;

	case PageStepTicks:
	    m_snapPosition = m_minValue +
		m_pageStep *
		int((m_snapPosition - m_minValue) / m_pageStep);
	    break;

	case StepTicks:
	    m_snapPosition = m_minValue +
		m_step *
		int((m_snapPosition - m_minValue) / m_step);
	    break;
	}
    }
}
    

void
RosegardenRotary::mousePressEvent(QMouseEvent *e)
{
    if (e->button() == LeftButton)
    {
        m_buttonPressed = true;
        m_lastY = e->y();
        m_lastX = e->x();
        //_float->setText(QString("%1").arg(m_snapPosition));
    }
    else if (e->button() == MidButton) // reset to default
    {
        m_position = m_initialPosition;
	snapPosition();
	update();
        emit valueChanged(m_snapPosition);
    }
    else if (e->button() == RightButton) // reset to centre position
    {
        m_position = (m_maxValue + m_minValue) / 2.0;
	snapPosition();
	update();
        emit valueChanged(m_snapPosition);
    }

    QPoint totalPos = mapTo(topLevelWidget(), QPoint(0, 0));

    if (!_float) _float = new RosegardenTextFloat(this);
    _float->reparent(this);
    _float->move(totalPos + QPoint(width() + 2, -height()/2));
    _float->setText(QString("%1").arg(m_position));
    _float->show();

    if (e->button() == RightButton || e->button() == MidButton)
    {
        // one shot, 500ms
        _floatTimer->start(500, true);
    }
}


void
RosegardenRotary::mouseDoubleClickEvent(QMouseEvent * /*e*/)
{
    RosegardenFloatEdit dialog(this,
            i18n("Select a new value"),
            i18n("Enter a new value"),
            m_minValue,
            m_maxValue,
            m_position,
            m_step);

    if (dialog.exec() == QDialog::Accepted)
    {
        m_position = dialog.getValue();
        snapPosition();
        update();

        emit valueChanged(m_snapPosition);
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

        // Hide the float text
        //
        if (_float) _float->hide();
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

        m_lastY = e->y();
        m_lastX = e->x();

	snapPosition();

        // don't update if there's nothing to update
//        if (m_lastPosition == m_snapPosition) return;

	update();

        emit valueChanged(m_snapPosition);

        // draw on the float text
        _float->setText(QString("%1").arg(m_snapPosition));
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

    snapPosition();
    update();

    if (!_float) _float = new RosegardenTextFloat(this);

    // draw on the float text
    _float->setText(QString("%1").arg(m_snapPosition));

    // Reposition - we need to sum the relative positions up to the
    // topLevel or dialog to please move(). Move just top/right of the rotary
    //
    QPoint totalPos = mapTo(topLevelWidget(), QPoint(0, 0));
    _float->reparent(this);
    _float->move(totalPos + QPoint(width() + 2, -height()/2));
    _float->show();

    // one shot, 500ms
    _floatTimer->start(500, true);

    // set it to show for a timeout value
    emit valueChanged(m_snapPosition);
}


void
RosegardenRotary::setPosition(float position)
{
    m_position = position;
    
    snapPosition();
    update();
}


RosegardenQuantizeParameters::RosegardenQuantizeParameters(QWidget *parent,
							   QuantizerType defaultQuantizer,
							   bool showNotationOption,
							   bool showAdvancedButton,
							   QString configCategory,
							   QString preamble) :
    QFrame(parent),
    m_configCategory(configCategory),
    m_standardQuantizations
        (Rosegarden::BasicQuantizer::getStandardQuantizations())
{
    m_mainLayout = new QGridLayout(this,
				   preamble ? 3 : 4, 2,
				   preamble ? 10 : 0,
				   preamble ? 5 : 4);

    int zero = 0;
    if (preamble) {
	QLabel *label = new QLabel(preamble, this);
	label->setAlignment(Qt::WordBreak);
	m_mainLayout->addMultiCellWidget(label, 0, 0, 0, 1);
	zero = 1;
    }

    QGroupBox *quantizerBox = new QGroupBox
	(1, Horizontal, i18n("Quantizer"), this);

    m_mainLayout->addWidget(quantizerBox, zero, 0);
    QFrame *typeFrame = new QFrame(quantizerBox);

    QGridLayout *layout = new QGridLayout(typeFrame, 2, 2, 5, 3);
    layout->addWidget(new QLabel(i18n("Quantizer type:"), typeFrame), 0, 0);
    m_typeCombo = new KComboBox(typeFrame);
    m_typeCombo->insertItem(i18n("Grid quantizer"));
    m_typeCombo->insertItem(i18n("Legato quantizer"));
    m_typeCombo->insertItem(i18n("Heuristic notation quantizer"));
    layout->addWidget(m_typeCombo, 0, 1);

    m_notationTarget = new QCheckBox
	(i18n("Quantize for notation only (leave performance unchanged)"),
	 typeFrame);
    layout->addMultiCellWidget(m_notationTarget, 1, 1, 0, 1);
    if (!showNotationOption) m_notationTarget->hide();

    QHBox *parameterBox = new QHBox(this);
    m_mainLayout->addWidget(parameterBox, zero + 1, 0);

    m_notationBox = new QGroupBox
	(1, Horizontal, i18n("Notation parameters"), parameterBox);
    QFrame *notationFrame = new QFrame(m_notationBox);

    layout = new QGridLayout(notationFrame, 4, 2, 5, 3);

    layout->addWidget(new QLabel(i18n("Base grid unit:"), notationFrame),
		      1, 0);
    m_notationUnitCombo = new KComboBox(notationFrame);
    layout->addWidget(m_notationUnitCombo, 1, 1);

    layout->addWidget(new QLabel(i18n("Complexity:"),
				 notationFrame), 0, 0);

    m_simplicityCombo = new KComboBox(notationFrame);
    m_simplicityCombo->insertItem(i18n("Very high"));
    m_simplicityCombo->insertItem(i18n("High"));
    m_simplicityCombo->insertItem(i18n("Normal"));
    m_simplicityCombo->insertItem(i18n("Low"));
    m_simplicityCombo->insertItem(i18n("Very low"));
    layout->addWidget(m_simplicityCombo, 0, 1);

    layout->addWidget(new QLabel(i18n("Tuplet level:"),
				 notationFrame), 2, 0);
    m_maxTuplet = new KComboBox(notationFrame);
    m_maxTuplet->insertItem(i18n("None"));
    m_maxTuplet->insertItem(i18n("2-in-the-time-of-3"));
    m_maxTuplet->insertItem(i18n("Triplet"));
/*
    m_maxTuplet->insertItem(i18n("4-Tuplet"));
    m_maxTuplet->insertItem(i18n("5-Tuplet"));
    m_maxTuplet->insertItem(i18n("6-Tuplet"));
    m_maxTuplet->insertItem(i18n("7-Tuplet"));
    m_maxTuplet->insertItem(i18n("8-Tuplet"));
*/
    m_maxTuplet->insertItem(i18n("Any"));
    layout->addWidget(m_maxTuplet, 2, 1);

    m_counterpoint = new QCheckBox(i18n("Permit counterpoint"), notationFrame);
    layout->addMultiCellWidget(m_counterpoint, 3, 3, 0, 1);

    m_gridBox = new QGroupBox
	(1, Horizontal, i18n("Grid parameters"), parameterBox);
    QFrame *gridFrame = new QFrame(m_gridBox);

    layout = new QGridLayout(gridFrame, 4, 2, 5, 3);

    layout->addWidget(new QLabel(i18n("Base grid unit:"), gridFrame), 0, 0);
    m_gridUnitCombo = new KComboBox(gridFrame);
    layout->addWidget(m_gridUnitCombo, 0, 1);

    m_swingLabel = new QLabel(i18n("Swing:"), gridFrame);
    layout->addWidget(m_swingLabel, 1, 0);
    m_swingCombo = new KComboBox(gridFrame);
    layout->addWidget(m_swingCombo, 1, 1);

    m_iterativeLabel = new QLabel(i18n("Iterative amount:"), gridFrame);
    layout->addWidget(m_iterativeLabel, 2, 0);
    m_iterativeCombo = new KComboBox(gridFrame);
    layout->addWidget(m_iterativeCombo, 2, 1);

    m_durationCheckBox = new QCheckBox
	(i18n("Quantize durations as well as start times"), gridFrame);
    layout->addMultiCellWidget(m_durationCheckBox, 3, 3, 0, 1);

    m_postProcessingBox = new QGroupBox
	(1, Horizontal, i18n("After quantization"), this);

    if (preamble) {
	m_mainLayout->addMultiCellWidget(m_postProcessingBox,
					 zero, zero + 1,
					 1, 1);
    } else {
	m_mainLayout->addWidget(m_postProcessingBox, zero + 3, 0);
    }

    bool advanced = true;
    m_advancedButton = 0;
    if (showAdvancedButton) {
	m_advancedButton =
	    new QPushButton(i18n("Show advanced options"), this);
	m_mainLayout->addWidget(m_advancedButton, zero + 2, 0, Qt::AlignLeft);
	QObject::connect(m_advancedButton, SIGNAL(clicked()),
			 this, SLOT(slotAdvancedChanged()));
    }

    QFrame *postFrame = new QFrame(m_postProcessingBox);

    layout = new QGridLayout(postFrame, 4, 1, 5, 3);
    m_rebeam = new QCheckBox(i18n("Re-beam"), postFrame);
    m_articulate = new QCheckBox
	(i18n("Add articulations (staccato, tenuto, slurs)"), postFrame);
    m_makeViable = new QCheckBox(i18n("Tie notes at barlines etc"), postFrame);
    m_deCounterpoint = new QCheckBox(i18n("Split-and-tie overlapping chords"), postFrame);

    layout->addWidget(m_rebeam, 0, 0);
    layout->addWidget(m_articulate, 1, 0);
    layout->addWidget(m_makeViable, 2, 0);
    layout->addWidget(m_deCounterpoint, 3, 0);
    
    QPixmap noMap = NotePixmapFactory::toQPixmap
	(NotePixmapFactory::makeToolbarPixmap("menu-no-note"));

    int defaultType = 0;
    Rosegarden::timeT defaultUnit = 
	Rosegarden::Note(Rosegarden::Note::Demisemiquaver).getDuration();

    if (!m_configCategory) {
	if (defaultQuantizer == Notation) m_configCategory = "Quantize Dialog Notation";
	else m_configCategory = "Quantize Dialog Grid";
    }

    int defaultSwing = 0;
    int defaultIterate = 100;

    if (m_configCategory) {
	KConfig *config = kapp->config();
	config->setGroup(m_configCategory);
	defaultType =
	    config->readNumEntry("quantizetype",
				 (defaultQuantizer == Notation) ? 2 :
				 (defaultQuantizer == Legato) ? 1 :
				 0);
	defaultUnit =
	    config->readNumEntry("quantizeunit", defaultUnit);
	defaultSwing =
	    config->readNumEntry("quantizeswing", defaultSwing);
	defaultIterate =
	    config->readNumEntry("quantizeiterate", defaultIterate);
	m_notationTarget->setChecked
	    (config->readBoolEntry("quantizenotationonly",
				   defaultQuantizer == Notation));
	m_durationCheckBox->setChecked
	    (config->readBoolEntry("quantizedurations", false));
	m_simplicityCombo->setCurrentItem
	    (config->readNumEntry("quantizesimplicity", 13) - 11);
	m_maxTuplet->setCurrentItem
	    (config->readNumEntry("quantizemaxtuplet", 3) - 1);
	m_counterpoint->setChecked
	    (config->readBoolEntry("quantizecounterpoint", false));
	m_rebeam->setChecked
	    (config->readBoolEntry("quantizerebeam", true));
	m_makeViable->setChecked
	    (config->readBoolEntry("quantizemakeviable",
				   defaultQuantizer == Notation));
	m_deCounterpoint->setChecked
	    (config->readBoolEntry("quantizedecounterpoint",
				   defaultQuantizer == Notation));
	m_articulate->setChecked
	    (config->readBoolEntry("quantizearticulate", true));
	advanced = config->readBoolEntry("quantizeshowadvanced", false);
    } else {
	defaultType =
	    (defaultQuantizer == Notation) ? 2 :
	    (defaultQuantizer == Legato) ? 1 : 0;
	m_notationTarget->setChecked(defaultQuantizer == Notation);
	m_durationCheckBox->setChecked(false);
	m_simplicityCombo->setCurrentItem(2);
	m_maxTuplet->setCurrentItem(2);
	m_counterpoint->setChecked(false);
	m_rebeam->setChecked(true);
	m_makeViable->setChecked(defaultQuantizer == Notation);
	m_deCounterpoint->setChecked(defaultQuantizer == Notation);
	m_articulate->setChecked(true);
	advanced = false;
    }

    if (preamble || advanced) {
	m_postProcessingBox->show();
    } else {
	m_postProcessingBox->hide();
    }

    for (unsigned int i = 0; i < m_standardQuantizations.size(); ++i) {

	Rosegarden::timeT time = m_standardQuantizations[i];
	Rosegarden::timeT error = 0;

	QPixmap pmap = NotePixmapFactory::toQPixmap
	    (NotePixmapFactory::makeNoteMenuPixmap(time, error));
	QString label = NotationStrings::makeNoteMenuLabel(time, false, error);

	if (error == 0) {
	    m_gridUnitCombo->insertItem(pmap, label);
	    m_notationUnitCombo->insertItem(pmap, label);
	} else {
	    m_gridUnitCombo->insertItem(noMap, QString("%1").arg(time));
	    m_notationUnitCombo->insertItem(noMap, QString("%1").arg(time));
	}

	if (m_standardQuantizations[i] == defaultUnit) {
	    m_gridUnitCombo->setCurrentItem(m_gridUnitCombo->count() - 1);
	    m_notationUnitCombo->setCurrentItem
		(m_notationUnitCombo->count() - 1);
	}
    }

    for (int i = -100; i <= 200; i += 10) {
	m_swingCombo->insertItem(i == 0 ? i18n("None") : QString("%1%").arg(i));
	if (i == defaultSwing)
	    m_swingCombo->setCurrentItem(m_swingCombo->count()-1);
    }

    for (int i = 10; i <= 100; i += 10) {
	m_iterativeCombo->insertItem(i == 100 ? i18n("Full quantize") :
				     QString("%1%").arg(i));
	if (i == defaultIterate)
	    m_iterativeCombo->setCurrentItem(m_iterativeCombo->count()-1);
    }

    switch (defaultType) {
    case 0: // grid
	m_gridBox->show();
	m_swingLabel->show();
	m_swingCombo->show();
	m_iterativeLabel->show();
	m_iterativeCombo->show();
	m_notationBox->hide();
	m_durationCheckBox->show();
	m_typeCombo->setCurrentItem(0);
	break;
    case 1: // legato
	m_gridBox->show();
	m_swingLabel->hide();
	m_swingCombo->hide();
	m_iterativeLabel->hide();
	m_iterativeCombo->hide();
	m_notationBox->hide();
	m_durationCheckBox->hide();
	m_typeCombo->setCurrentItem(1);
    case 2: // notation
	m_gridBox->hide();
	m_notationBox->show();
	m_typeCombo->setCurrentItem(2);
	break;
    }	

    connect(m_typeCombo, SIGNAL(activated(int)), SLOT(slotTypeChanged(int)));
}

Rosegarden::Quantizer *
RosegardenQuantizeParameters::getQuantizer() const
{
    //!!! Excessive duplication with
    // EventQuantizeCommand::makeQuantizer in editcommands.cpp

    int type = m_typeCombo->currentItem();
    Rosegarden::timeT unit = 0;

    if (type == 0 || type == 1) {
	unit = m_standardQuantizations[m_gridUnitCombo->currentItem()];
    } else {
	unit = m_standardQuantizations[m_notationUnitCombo->currentItem()];
    }

    Rosegarden::Quantizer *quantizer = 0;

    int swing = m_swingCombo->currentItem();
    swing *= 10;
    swing -= 100;
    
    int iterate = m_iterativeCombo->currentItem();
    iterate *= 10;
    iterate += 10;

    if (type == 0) {
	
	if (m_notationTarget->isChecked()) {
	    quantizer = new Rosegarden::BasicQuantizer
		(Rosegarden::Quantizer::RawEventData,
		 Rosegarden::Quantizer::NotationPrefix,
		 unit, m_durationCheckBox->isChecked(),
		 swing, iterate);
	} else {
	    quantizer = new Rosegarden::BasicQuantizer
		(Rosegarden::Quantizer::RawEventData,
		 Rosegarden::Quantizer::RawEventData,
		 unit, m_durationCheckBox->isChecked(),
		 swing, iterate);
	}
    } else if (type == 1) {
	if (m_notationTarget->isChecked()) {
	    quantizer = new Rosegarden::LegatoQuantizer
		(Rosegarden::Quantizer::RawEventData,
		 Rosegarden::Quantizer::NotationPrefix, unit);
	} else {
	    quantizer = new Rosegarden::LegatoQuantizer
		(Rosegarden::Quantizer::RawEventData,
		 Rosegarden::Quantizer::RawEventData,
		 unit);
	}
    } else {
	
	Rosegarden::NotationQuantizer *nq;

	if (m_notationTarget->isChecked()) {
	    nq = new Rosegarden::NotationQuantizer();
	} else {
	    nq = new Rosegarden::NotationQuantizer
		(Rosegarden::Quantizer::RawEventData,
		 Rosegarden::Quantizer::RawEventData);
	}

	nq->setUnit(unit);
	nq->setSimplicityFactor(m_simplicityCombo->currentItem() + 11);
	nq->setMaxTuplet(m_maxTuplet->currentItem() + 1);
	nq->setContrapuntal(m_counterpoint->isChecked());
	nq->setArticulate(m_articulate->isChecked());

	quantizer = nq;
    }

    if (m_configCategory) {
	KConfig *config = kapp->config();
	config->setGroup(m_configCategory);
	config->writeEntry("quantizetype", type);
	config->writeEntry("quantizeunit", unit);
	config->writeEntry("quantizeswing", swing);
	config->writeEntry("quantizeiterate", iterate);
	config->writeEntry("quantizenotationonly",
			   m_notationTarget->isChecked());
	if (type == 0) {
	    config->writeEntry("quantizedurations",
			       m_durationCheckBox->isChecked());
	} else {
	    config->writeEntry("quantizesimplicity",
			       m_simplicityCombo->currentItem() + 11);
	    config->writeEntry("quantizemaxtuplet",
			       m_maxTuplet->currentItem() + 1);
	    config->writeEntry("quantizecounterpoint",
			       m_counterpoint->isChecked());
	    config->writeEntry("quantizearticulate",
			       m_articulate->isChecked());
	}
	config->writeEntry("quantizerebeam", m_rebeam->isChecked());
	config->writeEntry("quantizemakeviable", m_makeViable->isChecked());
	config->writeEntry("quantizedecounterpoint", m_deCounterpoint->isChecked());
    }

    return quantizer;
}

void
RosegardenQuantizeParameters::slotAdvancedChanged()
{
    if (m_postProcessingBox->isVisible()) {
	if (m_advancedButton)
	    m_advancedButton->setText(i18n("Show Advanced Options"));
	m_postProcessingBox->hide();
    } else {
	if (m_advancedButton)
	    m_advancedButton->setText(i18n("Hide Advanced Options"));
	m_postProcessingBox->show();
    }
    adjustSize();
}

void
RosegardenQuantizeParameters::showAdvanced(bool show)
{
    if (show) {
	m_postProcessingBox->show();
    } else {
	m_postProcessingBox->hide();
    }
    adjustSize();
}

void
RosegardenQuantizeParameters::slotTypeChanged(int index)
{
    if (index == 0) {
	m_gridBox->show();
	m_swingLabel->show();
	m_swingCombo->show();
	m_iterativeLabel->show();
	m_iterativeCombo->show();
	m_durationCheckBox->show();
	m_notationBox->hide();
    } else if (index == 1) {
	m_gridBox->show();
	m_swingLabel->hide();
	m_swingCombo->hide();
	m_iterativeLabel->hide();
	m_iterativeCombo->hide();
	m_durationCheckBox->hide();
	m_notationBox->hide();
    } else {
	m_gridBox->hide();
	m_notationBox->show();
    }
}

// ---------- RosegardenTextFloat -----------
//
//
RosegardenTextFloat::RosegardenTextFloat(QWidget *parent):
    QWidget(parent, "RosegardenTextFloat",
            WStyle_Customize  | WStyle_NoBorder | WStyle_StaysOnTop),
    m_text("")
{
    reparent(parentWidget());
    resize(20, 20);
}

void
RosegardenTextFloat::reparent(QWidget *newParent)
{
    QPoint position = newParent->pos();

    // Get position and reparent to either top level or dialog
    //
    while (newParent->parentWidget() && !newParent->isTopLevel() 
           && !newParent->isDialog())
    {
        newParent = newParent->parentWidget();
        position += newParent->pos();
    }

    // Position this widget to the right of the parent
    //
    //move(pos + QPoint(parent->width() + 5, 5));

    QWidget::reparent(newParent, 
             WStyle_Customize  | WStyle_NoBorder | WStyle_StaysOnTop,
             position + QPoint(20, 5));
}


void
RosegardenTextFloat::paintEvent(QPaintEvent *e)
{
    QPainter paint(this);

    paint.setClipRegion(e->region());
    paint.setClipRect(e->rect().normalize());

    paint.setPen(kapp->palette().color(QPalette::Active, QColorGroup::Dark));

    paint.setPen(Rosegarden::GUIPalette::getColour(Rosegarden::GUIPalette::RotaryFloatForeground));
    paint.setBrush(Rosegarden::GUIPalette::getColour(Rosegarden::GUIPalette::RotaryFloatBackground));

    QFontMetrics metrics(paint.fontMetrics());

    QRect r = metrics.boundingRect(0, 0, 400, 400, Qt::AlignAuto, m_text);
    resize(r.width() + 7, r.height() + 7);
    paint.drawRect(0, 0, r.width() + 6, r.height() + 6);
    paint.setPen(Qt::black);
    paint.drawText(QRect(3, 3, r.width(), r.height()), Qt::AlignAuto, m_text);

/*
    QRect textBound = metrics.boundingRect(m_text);

    resize(textBound.width() + 7, textBound.height() + 7);
    paint.drawRect(0, 0, textBound.width() + 6, textBound.height() + 6);

    paint.setPen(Qt::black);
    paint.drawText(3, textBound.height() + 3, m_text);
*/
}


void 
RosegardenTextFloat::setText(const QString &text)
{
    m_text = text;
    repaint();
}


RosegardenPitchDragLabel::RosegardenPitchDragLabel(QWidget *parent,
						   int defaultPitch) :
    QWidget(parent),
    m_pitch(defaultPitch),
    m_clickedY(0),
    m_clicked(false),
    m_npf(new NotePixmapFactory())
{
    calculatePixmap(true);
}

RosegardenPitchDragLabel::~RosegardenPitchDragLabel()
{
    delete m_npf;
}

void
RosegardenPitchDragLabel::slotSetPitch(int p)
{
    bool up = (p > m_pitch);
    if (m_pitch == p) return;
    m_pitch = p;
    calculatePixmap(up);
    emit pitchChanged(m_pitch);
    paintEvent(0);
}

void
RosegardenPitchDragLabel::mousePressEvent(QMouseEvent *e)
{
    if (e->button() == LeftButton) {
	m_clickedY = e->y();
	m_clickedPitch = m_pitch;
	m_clicked = true;
	emit preview(m_pitch);
    }
}

void
RosegardenPitchDragLabel::mouseMoveEvent(QMouseEvent *e)
{
    if (m_clicked) {

	int y = e->y();
	int diff = y - m_clickedY;
	int pitchDiff = diff * 4 / m_npf->getLineSpacing();

	int newPitch = m_clickedPitch - pitchDiff;
	if (newPitch < 0) newPitch = 0;
	if (newPitch > 127) newPitch = 127;

	if (m_pitch != newPitch) {
	    bool up = (newPitch > m_pitch);
	    m_pitch = newPitch;
	    calculatePixmap(up);
	    emit pitchDragged(m_pitch);
	    emit preview(m_pitch);
	    paintEvent(0);
	}
    }
}

void
RosegardenPitchDragLabel::mouseReleaseEvent(QMouseEvent *e)
{
    mouseMoveEvent(e);
    emit pitchChanged(m_pitch);
    m_clicked = false;
}

void
RosegardenPitchDragLabel::wheelEvent(QWheelEvent *e)
{
    if (e->delta() > 0) {
	if (m_pitch < 127) {
	    ++m_pitch;
	    calculatePixmap(true);
	    emit pitchChanged(m_pitch);
	    emit preview(m_pitch);
	    paintEvent(0);
	}
    } else {
	if (m_pitch > 0) {
	    --m_pitch;
	    calculatePixmap(false);
	    emit pitchChanged(m_pitch);
	    emit preview(m_pitch);
	    paintEvent(0);
	}
    }
}

void
RosegardenPitchDragLabel::paintEvent(QPaintEvent *)
{
    QPainter paint(this);
    paint.fillRect(0, 0, width(), height(), paint.backgroundColor());

    int x = width()/2 - m_pixmap.width()/2;
    if (x < 0) x = 0;

    int y = height()/2 - m_pixmap.height()/2;
    if (y < 0) y = 0;

    paint.drawPixmap(x, y, m_pixmap);

		     
}

QSize
RosegardenPitchDragLabel::sizeHint() const
{
    return QSize(150, 135);
}

void
RosegardenPitchDragLabel::calculatePixmap(bool useSharps) const
{
    std::string clefType = Rosegarden::Clef::Treble;
    int octaveOffset = 0;

    if (m_pitch > 94) {
	octaveOffset = 2;
    } else if (m_pitch > 82) {
	octaveOffset = 1;
    } else if (m_pitch < 60) {
	clefType = Rosegarden::Clef::Bass;
	if (m_pitch < 24) {
	    octaveOffset = -2;
	} else if (m_pitch < 36) {
	    octaveOffset = -1;
	}
    }

    QCanvasPixmap *pmap = m_npf->makePitchDisplayPixmap
	(m_pitch,
	 Rosegarden::Clef(clefType, octaveOffset),
	 useSharps);

    m_pixmap = *pmap;

    delete pmap;
}

RosegardenPitchChooser::RosegardenPitchChooser(QString title,
					       QWidget *parent,
					       int defaultPitch) :
    QGroupBox(1, Horizontal, title, parent),
    m_defaultPitch(defaultPitch)
{
    m_pitchDragLabel = new RosegardenPitchDragLabel(this, defaultPitch);

    QHBox *hbox = new QHBox(this);
    hbox->setSpacing(6);

    new QLabel(i18n("Pitch:"), hbox);

    m_pitch = new QSpinBox(hbox);
    m_pitch->setMinValue(0);
    m_pitch->setMaxValue(127);
    m_pitch->setValue(defaultPitch);

    Rosegarden::MidiPitchLabel pl(defaultPitch);
    m_pitchLabel = new QLabel(pl.getQString(), hbox);
    m_pitchLabel->setMinimumWidth(40);

    connect(m_pitch, SIGNAL(valueChanged(int)),
	    this, SLOT(slotSetPitch(int)));

    connect(m_pitch, SIGNAL(valueChanged(int)),
	    this, SIGNAL(pitchChanged(int)));

    connect(m_pitch, SIGNAL(valueChanged(int)),
            this, SIGNAL(preview(int)));

    connect(m_pitchDragLabel, SIGNAL(pitchDragged(int)),
	    this, SLOT(slotSetPitch(int)));

    connect(m_pitchDragLabel, SIGNAL(pitchChanged(int)),
	    this, SLOT(slotSetPitch(int)));

    connect(m_pitchDragLabel, SIGNAL(pitchChanged(int)),
	    this, SIGNAL(pitchChanged(int)));

    connect(m_pitchDragLabel, SIGNAL(preview(int)),
	    this, SIGNAL(preview(int)));

}

int
RosegardenPitchChooser::getPitch() const
{
    return m_pitch->value();
}

void
RosegardenPitchChooser::slotSetPitch(int p)
{
    if (m_pitch->value() != p) m_pitch->setValue(p);
    if (m_pitchDragLabel->getPitch() != p) m_pitchDragLabel->slotSetPitch(p);
    
    Rosegarden::MidiPitchLabel pl(p);
    m_pitchLabel->setText(pl.getQString());
    update();
}

void
RosegardenPitchChooser::slotResetToDefault()
{
    slotSetPitch(m_defaultPitch);
}
#include "widgets.moc"
