// -*- c-basic-offset: 4 -*-

/*
    Rosegarden-4
    A sequencer and musical notation editor.

    This program is Copyright 2000-2003
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
    init();
}

void RosegardenParameterBox::init()
{
    QFont plainFont;
    plainFont.setPointSize(plainFont.pointSize() * 9 / 10);
    plainFont.setBold(false);
    m_font = plainFont;

    QFont boldFont;
    boldFont.setPointSize(int(boldFont.pointSize() * 9.5 / 10.0 + 0.5));
    boldFont.setBold(true);

    setFont(boldFont);
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
    RG_DEBUG << "RosegardenProgressDialog::processEvents: modalVisible is "
	     << m_modalVisible << endl;
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
    m_knobColour(0, 0, 0),
    m_float(new RosegardenTextFloat(this)),
    m_floatTimer(new QTimer())
{
    QToolTip::add(this,
                 "Click and drag up and down or left and right to modify");
    setFixedSize(size, size);

    // connect timer
    connect(m_floatTimer, SIGNAL(timeout()), this, SLOT(slotFloatTimeout()));

    m_float->hide();

    // set the initial position
    drawPosition();

}

void
RosegardenRotary::slotFloatTimeout()
{
    m_float->hide();
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

        // draw on the float text
        m_float->setText(QString("%1").arg(m_position));

        // Reposition - remap to topLevel or dialog to please move().
        //
        QPoint totalPos = mapTo(topLevelWidget(), this->pos());

        // Move just top/right of the rotary
        //
        m_float->move(totalPos + QPoint(width() + 2, -height()/2));

        // Show
        m_float->show();
    }
    else if (e->button() == RightButton) // reset to centre position
    {
        m_position = (m_maxValue + m_minValue) / 2.0;
        drawPosition();
        emit valueChanged(m_position);
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
        if (m_float) m_float->hide();

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

        // draw on the float text
        m_float->setText(QString("%1").arg(m_position));
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

    // draw on the float text
    m_float->setText(QString("%1").arg(m_position));

    // Reposition - we need to sum the relative positions up to the
    // topLevel or dialog to please move().
    //
    QWidget *par = parentWidget();
    QPoint totalPos = this->pos();

    while (par->parentWidget() && !par->isTopLevel() && !par->isDialog())
    {
        totalPos += par->pos();
        par = par->parentWidget();
    }
    // Move just top/right of the rotary
    //
    m_float->move(totalPos + QPoint(width() + 2, -height()/2));
    m_float->show();

    // one shot, 500ms
    m_floatTimer->start(500, true);

    // set it to show for a timeout value
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
    double angle = (0.22 * M_PI) // offset 
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
    angle = (0.22 * M_PI) // offset 
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

    // modify tip
    m_float->setText(QString("%1").arg(m_position));

    drawPosition();
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

    layout = new QGridLayout(gridFrame, 2, 2, 5, 3);

    layout->addWidget(new QLabel(i18n("Base grid unit:"), gridFrame), 0, 0);
    m_gridUnitCombo = new KComboBox(gridFrame);
    layout->addWidget(m_gridUnitCombo, 0, 1);

    m_durationCheckBox = new QCheckBox
	(i18n("Quantize durations as well as start times"), gridFrame);
    layout->addMultiCellWidget(m_durationCheckBox, 1, 1, 0, 1);

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

    if (m_configCategory) {
	KConfig *config = kapp->config();
	config->setGroup(m_configCategory);
	defaultType =
	    config->readNumEntry("quantizetype",
				 (defaultQuantizer == Notation) ? 2 : 0);
	defaultUnit =
	    config->readNumEntry("quantizeunit", defaultUnit);
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
	defaultType = (defaultQuantizer == Notation) ? 2 : 0;
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

    switch (defaultType) {
    case 0:
	m_gridBox->show();
	m_notationBox->hide();
	m_typeCombo->setCurrentItem(0);
	break;
    case 1:
	// note quantizer, fall through for now
    case 2:
	m_gridBox->hide();
	m_notationBox->show();
	m_typeCombo->setCurrentItem(1);
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

    if (type == 0) {
	unit = m_standardQuantizations[m_gridUnitCombo->currentItem()];
    } else {
	unit = m_standardQuantizations[m_notationUnitCombo->currentItem()];
    }

    Rosegarden::Quantizer *quantizer = 0;

    if (type == 0) {
	if (m_notationTarget->isChecked()) {
	    quantizer = new Rosegarden::BasicQuantizer
		(Rosegarden::Quantizer::NotationPrefix,
		 unit, m_durationCheckBox->isChecked());
	} else {
	    quantizer = new Rosegarden::BasicQuantizer
		(unit, m_durationCheckBox->isChecked());
	}
    } else {
	
	Rosegarden::NotationQuantizer *nq;

	if (m_notationTarget->isChecked()) {
	    nq = new Rosegarden::NotationQuantizer();
	} else {
	    nq = new Rosegarden::NotationQuantizer
		(Rosegarden::Quantizer::RawEventData);
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
    QWidget *par = parentWidget();
    QPoint pos = parent->pos() + par->pos();

    // Get position and reparent to either top level or dialog
    //
    while (par->parentWidget() && !par->isTopLevel() && !par->isDialog())
    {
        par = par->parentWidget();
        pos += par->pos();
    }

    // Position this widget to the right of the parent
    //
    //move(pos + QPoint(parent->width() + 5, 5));

    reparent(par, WStyle_Customize  | WStyle_NoBorder | WStyle_StaysOnTop,
             pos + QPoint(20, 5));

    resize(20, 20);
}

void
RosegardenTextFloat::paintEvent(QPaintEvent *e)
{
    QPainter paint(this);

    paint.setClipRegion(e->region());
    paint.setClipRect(e->rect().normalize());

    paint.setPen(kapp->palette().color(QPalette::Active, QColorGroup::Dark));

    paint.setPen(RosegardenGUIColours::RotaryFloatForeground);
    paint.setBrush(RosegardenGUIColours::RotaryFloatBackground);

    QFontMetrics metrics(paint.fontMetrics());
    QRect textBound = metrics.boundingRect(m_text);

    resize(textBound.width() + 7, textBound.height() + 7);
    paint.drawRect(0, 0, textBound.width() + 6, textBound.height() + 6);

    paint.setPen(Qt::black);
    paint.drawText(3, textBound.height() + 3, m_text);
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
	    emit pitchChanged(m_pitch);
	    emit preview(m_pitch);
	    paintEvent(0);
	}
    }
}

void
RosegardenPitchDragLabel::mouseReleaseEvent(QMouseEvent *e)
{
    mouseMoveEvent(e);
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

RosegardenTimeWidget::RosegardenTimeWidget(QString title,
					   QWidget *parent,
					   Rosegarden::Composition *composition,
					   Rosegarden::timeT absTime,
					   bool editable) :
    QGroupBox(1, Horizontal, title, parent),
    m_composition(composition),
    m_isDuration(false),
    m_time(absTime),
    m_startTime(0),
    m_defaultTime(absTime)
{
    init(editable);
}

RosegardenTimeWidget::RosegardenTimeWidget(QString title,
					   QWidget *parent,
					   Rosegarden::Composition *composition,
					   Rosegarden::timeT startTime,
					   Rosegarden::timeT duration,
					   bool editable) :
    QGroupBox(1, Horizontal, title, parent),
    m_composition(composition),
    m_isDuration(true),
    m_time(duration),
    m_startTime(startTime),
    m_defaultTime(duration)
{
    init(editable);
}

void
RosegardenTimeWidget::init(bool editable)
{
    int denoms[] = {
	1, 2, 3, 4, 6, 8, 12, 16, 24, 32, 48, 64, 96, 128
    };

    bool savedEditable = editable;
    editable = true; //!!!

    QFrame *frame = new QFrame(this);
    QGridLayout *layout = new QGridLayout(frame, 7, 3, 5, 5);
    QLabel *label = 0;

    if (m_isDuration) {

	label = new QLabel(i18n("Note:"), frame);
	label->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
	layout->addWidget(label, 0, 0);

	if (editable) {
	    m_note = new QComboBox(frame);
	    m_noteDurations.push_back(0);
	    m_note->insertItem(i18n("<inexact>"));
	    for (size_t i = 0; i < sizeof(denoms)/sizeof(denoms[0]); ++i) {

		Rosegarden::timeT duration =
		    Rosegarden::Note(Rosegarden::Note::Breve).getDuration() / denoms[i];
		
		if (denoms[i] > 1 && denoms[i] < 128 && (denoms[i] % 3) != 0) {
		    // not breve or hemidemi, not a triplet
		    Rosegarden::timeT dottedDuration = duration * 3 / 2;
		    m_noteDurations.push_back(dottedDuration);
		    Rosegarden::timeT error = 0;
		    QString label = NotationStrings::makeNoteMenuLabel
			(dottedDuration, false, error);
		    QPixmap pmap = NotePixmapFactory::toQPixmap
			(NotePixmapFactory::makeNoteMenuPixmap(dottedDuration, error));
		    m_note->insertItem(pmap, label); // ignore error
		}		
		
		m_noteDurations.push_back(duration);
		Rosegarden::timeT error = 0;
		QString label = NotationStrings::makeNoteMenuLabel
		    (duration, false, error);
		QPixmap pmap = NotePixmapFactory::toQPixmap
		    (NotePixmapFactory::makeNoteMenuPixmap(duration, error));
		m_note->insertItem(pmap, label); // ignore error
	    }
	    connect(m_note, SIGNAL(activated(int)),
		    this, SLOT(slotNoteChanged(int)));
	    layout->addMultiCellWidget(m_note, 0, 0, 1, 3);

	} else {

	    m_note = 0;
	    Rosegarden::timeT error = 0;
	    QString label = NotationStrings::makeNoteMenuLabel
		(m_time, false, error);
	    if (error != 0) label = i18n("<inexact>");
	    QLineEdit *le = new QLineEdit(label, frame);
	    le->setReadOnly(true);
	    layout->addMultiCellWidget(le, 0, 0, 1, 3);
	}

	label = new QLabel(i18n("Units:"), frame);
	label->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
	layout->addWidget(label, 0, 4);

	if (editable) {
	    m_timeT = new QSpinBox(frame);
	    m_timeT->setLineStep
		(Rosegarden::Note(Rosegarden::Note::Shortest).getDuration());
	    connect(m_timeT, SIGNAL(valueChanged(int)),
		    this, SLOT(slotTimeTChanged(int)));
	    layout->addWidget(m_timeT, 0, 5);
	} else {
	    m_timeT = 0;
	    QLineEdit *le = new QLineEdit(QString("%1").arg(m_time), frame);
	    le->setReadOnly(true);
	    layout->addWidget(le, 0, 5);
	}

    } else {

	m_note = 0;

	label = new QLabel(i18n("Time:"), frame);
	label->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
	layout->addWidget(label, 0, 0);

	if (editable) {
	    m_timeT = new QSpinBox(frame);
	    m_timeT->setLineStep
		(Rosegarden::Note(Rosegarden::Note::Shortest).getDuration());
	    connect(m_timeT, SIGNAL(valueChanged(int)),
		    this, SLOT(slotTimeTChanged(int)));
	    layout->addWidget(m_timeT, 0,1);
	    layout->addWidget(new QLabel(i18n("units"), frame), 0, 2);
	} else {
	    m_timeT = 0;
	    QLineEdit *le = new QLineEdit(QString("%1").arg(m_time), frame);
	    le->setReadOnly(true);
	    layout->addWidget(le, 0, 2);
	}
    }

    label = new QLabel(m_isDuration ? i18n("Bars:") : i18n("Bar:"), frame);
    label->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    layout->addWidget(label, 1, 0);

    if (editable) {
	m_barLabel = 0;
	m_bar = new QSpinBox(frame);
	if (m_isDuration) m_bar->setMinValue(0);
	connect(m_bar, SIGNAL(valueChanged(int)),
		this, SLOT(slotBarBeatOrFractionChanged(int)));
	layout->addWidget(m_bar, 1, 1);
    } else {
	m_bar = 0;
	m_barLabel = new QLineEdit(frame);
	m_barLabel->setReadOnly(true);
	layout->addWidget(m_barLabel, 1, 1);
    }

    label = new QLabel(m_isDuration ? i18n("beats:") : i18n("beat:"), frame);
    label->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    layout->addWidget(label, 1, 2);

    if (editable) {
	m_beatLabel = 0;
	m_beat = new QSpinBox(frame);
	m_beat->setMinValue(1);
	connect(m_beat, SIGNAL(valueChanged(int)),
		this, SLOT(slotBarBeatOrFractionChanged(int)));
	layout->addWidget(m_beat, 1, 3);
    } else {
	m_beat = 0;
	m_beatLabel = new QLineEdit(frame);
	m_beatLabel->setReadOnly(true);
	layout->addWidget(m_beatLabel, 1, 3);
    }

    label = new QLabel(i18n("%1:").arg(NotationStrings::getShortNoteName
				       (Rosegarden::Note
					(Rosegarden::Note::Shortest), true)),
				       frame);
    label->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    layout->addWidget(label, 1, 4);

    if (editable) {
	m_fractionLabel = 0;
	m_fraction = new QSpinBox(frame);
	m_fraction->setMinValue(1);
	connect(m_fraction, SIGNAL(valueChanged(int)),
		this, SLOT(slotBarBeatOrFractionChanged(int)));
	layout->addWidget(m_fraction, 1, 5);
    } else {
	m_fraction = 0;
	m_fractionLabel = new QLineEdit(frame);
	m_fractionLabel->setReadOnly(true);
	layout->addWidget(m_fractionLabel, 1, 5);
    }

    m_timeSig = new QLabel(frame);
    layout->addWidget(m_timeSig, 1, 6);
    
    label = new QLabel(i18n("Seconds:"), frame);
    label->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    layout->addWidget(label, 2, 0);

    if (editable) {
	m_secLabel = 0;
	m_sec = new QSpinBox(frame);
	if (m_isDuration) m_sec->setMinValue(0);
	connect(m_sec, SIGNAL(valueChanged(int)),
		this, SLOT(slotSecOrMSecChanged(int)));
	layout->addWidget(m_sec, 2, 1);
    } else { 
	m_sec = 0;
	m_secLabel = new QLineEdit(frame);
	m_secLabel->setReadOnly(true);
	layout->addWidget(m_secLabel, 2, 1);
    }

    label = new QLabel(i18n("msec:"), frame);
    label->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    layout->addWidget(label, 2, 2);

    if (editable) {
	m_msecLabel = 0;
	m_msec = new QSpinBox(frame);
	m_msec->setMinValue(0);
	m_msec->setLineStep(10);
	connect(m_msec, SIGNAL(valueChanged(int)),
		this, SLOT(slotSecOrMSecChanged(int)));
	layout->addWidget(m_msec, 2, 3);
    } else { 
	m_msec = 0;
	m_msecLabel = new QLineEdit(frame);
	m_msecLabel->setReadOnly(true);
	layout->addWidget(m_msecLabel, 2, 3);
    }

    if (m_isDuration) {
	m_tempo = new QLabel(frame);
	layout->addWidget(m_tempo, 2, 6);
    } else {
	m_tempo = 0;
    }

    if (!savedEditable) {
	if (m_note)     m_note     ->setEnabled(false);
	if (m_timeT)    m_timeT    ->setEnabled(false);
	if (m_bar)      m_bar      ->setEnabled(false);
	if (m_beat)     m_beat     ->setEnabled(false);
	if (m_fraction) m_fraction ->setEnabled(false);
	if (m_sec)      m_sec      ->setEnabled(false);
	if (m_msec)     m_msec     ->setEnabled(false);
    }

    populate();
}

void
RosegardenTimeWidget::populate()
{
    // populate everything from m_time and m_startTime

    if (m_note)     m_note     ->blockSignals(true);
    if (m_timeT)    m_timeT    ->blockSignals(true);
    if (m_bar)      m_bar      ->blockSignals(true);
    if (m_beat)     m_beat     ->blockSignals(true);
    if (m_fraction) m_fraction ->blockSignals(true);
    if (m_sec)      m_sec      ->blockSignals(true);
    if (m_msec)     m_msec     ->blockSignals(true);

    if (m_isDuration) {

	if (m_time + m_startTime > m_composition->getEndMarker()) {
	    m_time = m_composition->getEndMarker() - m_startTime;
	}

	if (m_timeT) {
	    m_timeT->setMinValue(0);
	    m_timeT->setMaxValue(m_composition->getEndMarker() - m_startTime);
	    m_timeT->setValue(m_time);
	}
    
	if (m_note) {
	    m_note->setCurrentItem(0);
	    for (size_t i = 0; i < m_noteDurations.size(); ++i) {
		if (m_time == m_noteDurations[i]) {
		    m_note->setCurrentItem(i);
		    break;
		}
	    }
	}
	
	// the bar/beat etc timings are considered to be times of a note
	// starting at the start of a bar, in the time signature in effect
	// at m_startTime

	int bars = 0, beats = 0, hemidemis = 0, remainder = 0;
	m_composition->getMusicalTimeForDuration(m_startTime, m_time,
						 bars, beats, hemidemis, remainder);
	Rosegarden::TimeSignature timeSig =
	    m_composition->getTimeSignatureAt(m_startTime);
/*!!!
	Rosegarden::timeT barDuration = timeSig.getBarDuration();
	Rosegarden::timeT beatDuration = timeSig.getBeatDuration();
	int bars = m_time / barDuration;
	int beats = (m_time % barDuration) / beatDuration;
	Rosegarden::timeT remainder = (m_time % barDuration) % beatDuration;
	int hemidemis = remainder /
	    Rosegarden::Note(Rosegarden::Note::Shortest).getDuration();
*/
	if (m_bar) {
	    m_bar->setMinValue(0);
	    m_bar->setMaxValue
		(m_composition->getBarNumber(m_composition->getEndMarker()) -
		 m_composition->getBarNumber(m_startTime));
	    m_bar->setValue(bars);
	} else {
	    m_barLabel->setText(QString("%1").arg(bars));
	}

	if (m_beat) {
	    m_beat->setMinValue(0);
	    m_beat->setMaxValue(timeSig.getBeatsPerBar() - 1);
	    m_beat->setValue(beats);
	} else {
	    m_beatLabel->setText(QString("%1").arg(beats));
	}

	if (m_fraction) {
	    m_fraction->setMinValue(0);
	    m_fraction->setMaxValue(timeSig.getBeatDuration() /
				    Rosegarden::Note(Rosegarden::Note::Shortest).
				    getDuration() - 1);
	    m_fraction->setValue(hemidemis);
	} else {
	    m_fractionLabel->setText(QString("%1").arg(hemidemis));
	}

	m_timeSig->setText(i18n("(%1/%2 time)").arg(timeSig.getNumerator()).
			   arg(timeSig.getDenominator()));
	
	Rosegarden::timeT endTime = m_startTime + m_time;

	Rosegarden::RealTime rt = m_composition->getRealTimeDifference
	    (m_startTime, endTime);

	if (m_sec) {
	    m_sec->setMinValue(0);
	    m_sec->setMaxValue(m_composition->getRealTimeDifference
			       (m_startTime, m_composition->getEndMarker()).sec);
	    m_sec->setValue(rt.sec);
	} else {
	    m_secLabel->setText(QString("%1").arg(rt.sec));
	}

	if (m_msec) {
	    m_msec->setMinValue(0);
	    m_msec->setMaxValue(999);
	    m_msec->setValue(rt.msec());
	} else {
	    m_msecLabel->setText(QString("%1").arg(rt.msec()));
	}

	bool change = (m_composition->getTempoChangeNumberAt(endTime) !=
		       m_composition->getTempoChangeNumberAt(m_startTime));
	double tempo = m_composition->getTempoAt(m_startTime);
	int qpmc = int(tempo * 100.0);
	int bpmc = qpmc;
	if (timeSig.getBeatDuration()
	    != Rosegarden::Note(Rosegarden::Note::Crotchet).getDuration()) {
	    bpmc = int(tempo * 100.0 *
		       Rosegarden::Note(Rosegarden::Note::Crotchet).getDuration() /
		       timeSig.getBeatDuration());
	}
	if (change) {
	    if (bpmc != qpmc) {
		m_tempo->setText(i18n("(starting %1.%2 qpm, %2.%3 bpm)").
				 arg(qpmc / 100).
				 arg(qpmc % 100).
				 arg(bpmc / 100).
				 arg(bpmc % 100));
	    } else {
		m_tempo->setText(i18n("(starting %1.%2 bpm)").
				 arg(bpmc / 100).
				 arg(bpmc % 100));
	    }
	} else {
	    if (bpmc != qpmc) {
		m_tempo->setText(i18n("(%1.%2 qpm, %2.%3 bpm)").
				 arg(qpmc / 100).
				 arg(qpmc % 100).
				 arg(bpmc / 100).
				 arg(bpmc % 100));
	    } else {
		m_tempo->setText(i18n("(%1.%2 bpm)").
				 arg(bpmc / 100).
				 arg(bpmc % 100));
	    }
	}	    

    } else {

	if (m_time > m_composition->getEndMarker()) {
	    m_time = m_composition->getEndMarker();
	}

	if (m_timeT) {
	    m_timeT->setMinValue(INT_MIN);
	    m_timeT->setMaxValue(m_composition->getEndMarker());
	    m_timeT->setValue(m_time);
	}

	int bar = 1, beat = 1, hemidemis = 0, remainder = 0;
	m_composition->getMusicalTimeForAbsoluteTime
	    (m_time, bar, beat, hemidemis, remainder);
	
	Rosegarden::TimeSignature timeSig =
	    m_composition->getTimeSignatureAt(m_time);
/*!!!
	int bar = m_composition->getBarNumber(m_time);
	Rosegarden::timeT barStart = m_composition->getBarStart(bar);
	Rosegarden::timeT beatDuration = timeSig.getBeatDuration();
	int beat = (m_time - barStart) / beatDuration + 1;
	Rosegarden::timeT remainder = (m_time - barStart) % beatDuration;
	int hemidemis = remainder /
	    Rosegarden::Note(Rosegarden::Note::Shortest).getDuration();
*/

	if (m_bar) {
	    m_bar->setMinValue(INT_MIN);
	    m_bar->setMaxValue(m_composition->getBarNumber
			       (m_composition->getEndMarker()));
	    m_bar->setValue(bar + 1);
	} else {
	    m_barLabel->setText(QString("%1").arg(bar + 1));
	}

	if (m_beat) {
	    m_beat->setMinValue(1);
	    m_beat->setMaxValue(timeSig.getBeatsPerBar());
	    m_beat->setValue(beat);
	} else {
	    m_beatLabel->setText(QString("%1").arg(beat));
	}

	if (m_fraction) {
	    m_fraction->setMinValue(0);
	    m_fraction->setMaxValue(timeSig.getBeatDuration() /
				    Rosegarden::Note(Rosegarden::Note::Shortest).
				    getDuration() - 1);
	    m_fraction->setValue(hemidemis);
	} else {
	    m_fractionLabel->setText(QString("%1").arg(hemidemis));
	}

	m_timeSig->setText(i18n("(%1/%2 time)").arg(timeSig.getNumerator()).
			   arg(timeSig.getDenominator()));
	
	Rosegarden::RealTime rt = m_composition->getElapsedRealTime(m_time);

	if (m_sec) {
	    m_sec->setMinValue(INT_MIN);
	    m_sec->setMaxValue(m_composition->getElapsedRealTime
			       (m_composition->getEndMarker()).sec);
	    m_sec->setValue(rt.sec);
	} else {
	    m_secLabel->setText(QString("%1").arg(rt.sec));
	}

	if (m_msec) {
	    m_msec->setMinValue(0);
	    m_msec->setMaxValue(999);
	    m_msec->setValue(rt.msec());
	} else {
	    m_msecLabel->setText(QString("%1").arg(rt.msec()));
	}
    }

    if (m_note)     m_note     ->blockSignals(false);
    if (m_timeT)    m_timeT    ->blockSignals(false);
    if (m_bar)      m_bar      ->blockSignals(false);
    if (m_beat)     m_beat     ->blockSignals(false);
    if (m_fraction) m_fraction ->blockSignals(false);
    if (m_sec)      m_sec      ->blockSignals(false);
    if (m_msec)     m_msec     ->blockSignals(false);
}

Rosegarden::timeT
RosegardenTimeWidget::getTime()
{
    return m_time;
}

Rosegarden::RealTime
RosegardenTimeWidget::getRealTime()
{
    if (m_isDuration) {
	return m_composition->getRealTimeDifference(m_startTime,
						    m_startTime + m_time);
    } else {
	return m_composition->getElapsedRealTime(m_time);
    }
}

void
RosegardenTimeWidget::slotSetTime(Rosegarden::timeT t)
{
    bool change = (m_time != t);
    if (!change) return;
    m_time = t;
    populate();
    emit timeChanged(getTime());
    emit realTimeChanged(getRealTime());
}

void
RosegardenTimeWidget::slotSetRealTime(Rosegarden::RealTime rt)
{
    if (m_isDuration) {
	Rosegarden::RealTime startRT = m_composition->getElapsedRealTime(m_startTime);
	if (rt >= Rosegarden::RealTime::zeroTime) {
	    slotSetTime(m_composition->getElapsedTimeForRealTime(startRT + rt) -
			m_startTime);
	} else {
	    RG_DEBUG << "WARNING: RosegardenTimeWidget::slotSetRealTime: rt must be >0 for duration widget (was " << rt << ")" << endl;
	}
    } else {
	slotSetTime(m_composition->getElapsedTimeForRealTime(rt));
    }
}

void
RosegardenTimeWidget::slotResetToDefault()
{
    slotSetTime(m_defaultTime);
}
	
void
RosegardenTimeWidget::slotNoteChanged(int n)
{
    if (n > 0) {
	slotSetTime(m_noteDurations[n]);
    }
}

void
RosegardenTimeWidget::slotTimeTChanged(int t)
{
    RG_DEBUG << "slotTimeTChanged: t is " << t << ", value is " << m_timeT->value() << endl;

    slotSetTime(t);
}

void
RosegardenTimeWidget::slotBarBeatOrFractionChanged(int)
{
    int bar = m_bar->value();
    int beat = m_beat->value();
    int fraction = m_fraction->value();

    if (m_isDuration) {
	slotSetTime(m_composition->getDurationForMusicalTime
		    (m_startTime, bar, beat, fraction, 0));
/*!!!
	Rosegarden::TimeSignature timeSig =
	    m_composition->getTimeSignatureAt(m_startTime);
	Rosegarden::timeT barDuration = timeSig.getBarDuration();
	Rosegarden::timeT beatDuration = timeSig.getBeatDuration();
	Rosegarden::timeT t = bar * barDuration + beat * beatDuration + fraction *
	    Rosegarden::Note(Rosegarden::Note::Shortest).getDuration();
	slotSetTime(t);
*/
    } else {
	slotSetTime(m_composition->getAbsoluteTimeForMusicalTime
		    (bar, beat, fraction, 0));
	/*!!!
	Rosegarden::timeT t = m_composition->getBarStart(bar - 1);
	Rosegarden::TimeSignature timesig = m_composition->getTimeSignatureAt(t);
	t += (beat-1) * timesig.getBeatDuration();
	t += Rosegarden::Note(Rosegarden::Note::Shortest).getDuration() *
	    fraction;
	slotSetTime(t);
	*/
    }
}

void
RosegardenTimeWidget::slotSecOrMSecChanged(int)
{
    int sec = m_sec->value();
    int msec = m_msec->value();

    slotSetRealTime(Rosegarden::RealTime(sec, msec * 1000000));
}

