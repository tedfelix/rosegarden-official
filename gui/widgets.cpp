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
#include <cmath>

#include <kapp.h>
#include <klocale.h>
#include <kconfig.h>

#include <qfontdatabase.h>
#include <qtimer.h>
#include <qapplication.h>
#include <qcursor.h>
#include <qpainter.h>
#include <qtooltip.h>
#include <qlayout.h>
#include <qtextedit.h>
#include <qlineedit.h>

#include "widgets.h"
#include "rosedebug.h"
#include "rosestrings.h"
#include "notepixmapfactory.h"

#include "Quantizer.h"

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
//!!!            emit propagate(currentItem());
	    emit activated(currentItem());
        }
    }
    else
    {
        if (currentItem() > 0)
        {
            setCurrentItem(currentItem() - 1);
//!!!            emit propagate(currentItem());
	    emit activated(currentItem());
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
    m_frozen(false)
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

void
RosegardenProgressDialog::polish()
{
    KProgressDialog::polish();

    if (allowCancel())
        setCursor(Qt::ArrowCursor);
    else
        QApplication::setOverrideCursor(QCursor(Qt::waitCursor));

    installFilter();
}

void RosegardenProgressDialog::hideEvent(QHideEvent* e)
{
    if (!allowCancel())
        QApplication::restoreOverrideCursor();
    
    KProgressDialog::hideEvent(e);
}



bool
RosegardenProgressDialog::eventFilter(QObject *watched, QEvent *e)
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

        return KProgressDialog::eventFilter(watched, e);
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
        kapp->processEvents();
    }
}

void RosegardenProgressDialog::slotFreeze()
{
    RG_DEBUG << "RosegardenProgressDialog::slotFreeze()\n";

    m_wasVisible = isVisible();
    if (isVisible()) hide();

    removeFilter();

    mShowTimer->stop();
    m_frozen = true;
}

void RosegardenProgressDialog::slotThaw()
{
    RG_DEBUG << "RosegardenProgressDialog::slotThaw()\n";

    if (m_wasVisible) show();
    installFilter();

    // Restart timer
    mShowTimer->start(minimumDuration());
    m_frozen = false;
    m_chrono.restart();
}

void RosegardenProgressDialog::installFilter()
{
    if (kapp->mainWidget())
        kapp->mainWidget()->installEventFilter(this);
}

void RosegardenProgressDialog::removeFilter()
{
    if (kapp->mainWidget())
        kapp->mainWidget()->removeEventFilter(this);
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


RosegardenQuantizeParameters::RosegardenQuantizeParameters(QWidget *parent,
							   bool showNotationOption,
							   QString configCategory,
							   QString preamble) :
    QFrame(parent),
    m_configCategory(configCategory),
    m_standardQuantizations
        (Rosegarden::BasicQuantizer::getStandardQuantizations())
{
    QGridLayout *mainLayout = new QGridLayout(this, preamble ? 3 : 2, 2, 10, 5);

    if (preamble) {
	QLabel *label = new QLabel(preamble, this);
	label->setAlignment(Qt::WordBreak);
	mainLayout->addMultiCellWidget(label, 0, 0, 0, 1);
    }

    QGroupBox *quantizerBox = new QGroupBox
	(1, Horizontal, i18n("Quantizer type"), this);
    mainLayout->addWidget(quantizerBox, preamble ? 1 : 0, 0);
    QFrame *typeFrame = new QFrame(quantizerBox);

    QGridLayout *layout = new QGridLayout(typeFrame, 1, 1, 5, 3);

//!!!    layout->addWidget(new QLabel(i18n("Quantizer type:"), typeFrame), 0, 0);
    m_typeCombo = new RosegardenComboBox(false, typeFrame);
    m_typeCombo->insertItem(i18n("Grid quantizer"));
    m_typeCombo->insertItem(i18n("Heuristic notation quantizer"));
    layout->addWidget(m_typeCombo, 0, 0);

    m_notationTarget = new QCheckBox
	(i18n("Quantize for notation only (leave performance unchanged)"),
	 quantizerBox);
    if (!showNotationOption) m_notationTarget->hide();

    QHBox *parameterBox = new QHBox(this);
    mainLayout->addWidget(parameterBox, preamble ? 1 : 0, 1);

    m_notationBox = new QGroupBox
	(1, Horizontal, i18n("Notation parameters"), parameterBox);
    QFrame *notationFrame = new QFrame(m_notationBox);

    layout = new QGridLayout(notationFrame, 3, 2, 5, 3);

    layout->addWidget(new QLabel(i18n("Base grid unit:"), notationFrame),
		      1, 0);
    m_notationUnitCombo = new RosegardenComboBox(false, notationFrame);
    layout->addWidget(m_notationUnitCombo, 1, 1);

    layout->addWidget(new QLabel(i18n("Complexity:"),
				 notationFrame), 0, 0);

    m_simplicityCombo = new RosegardenComboBox(false, notationFrame);
    m_simplicityCombo->insertItem(i18n("Very high"));
    m_simplicityCombo->insertItem(i18n("High"));
    m_simplicityCombo->insertItem(i18n("Normal"));
    m_simplicityCombo->insertItem(i18n("Low"));
    m_simplicityCombo->insertItem(i18n("Absurdly facile"));
    layout->addWidget(m_simplicityCombo, 0, 1);

    layout->addWidget(new QLabel(i18n("Tuplet level:"),
				 notationFrame), 2, 0);
    m_maxTuplet = new RosegardenComboBox(false, notationFrame);
    m_maxTuplet->insertItem(i18n("None"));
    m_maxTuplet->insertItem(i18n("2-in-the-time-of-3"));
    m_maxTuplet->insertItem(i18n("Triplet"));
    m_maxTuplet->insertItem(i18n("4-Tuplet"));
    m_maxTuplet->insertItem(i18n("5-Tuplet"));
    m_maxTuplet->insertItem(i18n("6-Tuplet"));
    m_maxTuplet->insertItem(i18n("7-Tuplet"));
    m_maxTuplet->insertItem(i18n("8-Tuplet"));
    m_maxTuplet->insertItem(i18n("Any"));
    layout->addWidget(m_maxTuplet, 2, 1);

    m_gridBox = new QGroupBox
	(1, Horizontal, i18n("Grid parameters"), parameterBox);
    QFrame *gridFrame = new QFrame(m_gridBox);

    layout = new QGridLayout(gridFrame, 2, 2, 5, 3);

    layout->addWidget(new QLabel(i18n("Base grid unit:"), gridFrame), 0, 0);
    m_gridUnitCombo = new RosegardenComboBox(false, gridFrame);
    layout->addWidget(m_gridUnitCombo, 0, 1);

    m_durationCheckBox = new QCheckBox
	(i18n("Quantize durations as well as start times"), gridFrame);
    layout->addMultiCellWidget(m_durationCheckBox, 1, 1, 0, 1);

    QGroupBox *postProcessingBox = new QGroupBox
	(1, Horizontal, i18n("After quantization"), this);
    mainLayout->addMultiCellWidget(postProcessingBox,
				   preamble ? 2 : 1,
				   preamble ? 2 : 1,
				   0, 1);
    QFrame *postFrame = new QFrame(postProcessingBox);

    layout = new QGridLayout(postFrame, 2, 2, 5, 3);
    m_makeViable = new QCheckBox(i18n("Tie notes at barlines etc"), postFrame);
    m_deCounterpoint = new QCheckBox(i18n("Split-and-tie overlapping chords"), postFrame);
    m_rebeam = new QCheckBox(i18n("Re-beam"), postFrame);
    m_articulate = new QCheckBox
	(i18n("Add articulations (staccato, tenuto, slurs)"), postFrame);

    layout->addWidget(m_makeViable, 0, 1);
    layout->addWidget(m_deCounterpoint, 1, 1);
    layout->addWidget(m_rebeam, 1, 0);
    layout->addWidget(m_articulate, 0, 0);
    

    NotePixmapFactory npf;
    QPixmap noMap =
	NotePixmapFactory::toQPixmap(npf.makeToolbarPixmap("menu-no-note"));

    int defaultType = 0;
    Rosegarden::timeT defaultUnit = 
	Rosegarden::Note(Rosegarden::Note::Semiquaver).getDuration();

    if (m_configCategory) {
	KConfig *config = kapp->config();
	config->setGroup(m_configCategory);
	defaultType =
	    config->readNumEntry("quantizetype", showNotationOption ? 2 : 0);
	defaultUnit =
	    config->readNumEntry("quantizeunit", defaultUnit);
	m_notationTarget->setChecked
	    (config->readBoolEntry("quantizenotationonly", showNotationOption));
	m_durationCheckBox->setChecked
	    (config->readBoolEntry("quantizedurations", false));
	m_simplicityCombo->setCurrentItem
	    (config->readNumEntry("quantizesimplicity", 13) - 11);
	m_maxTuplet->setCurrentItem
	    (config->readNumEntry("quantizemaxtuplet", 3) - 1);
	m_rebeam->setChecked
	    (config->readBoolEntry("quantizerebeam", true));
	m_makeViable->setChecked
	    (config->readBoolEntry("quantizemakeviable", false));
	m_deCounterpoint->setChecked
	    (config->readBoolEntry("quantizedecounterpoint", false));
	m_articulate->setChecked
	    (config->readBoolEntry("quantizearticulate", true));
    } else {
	defaultType = showNotationOption ? 2 : 0;
	m_notationTarget->setChecked(showNotationOption);
	m_durationCheckBox->setChecked(false);
	m_simplicityCombo->setCurrentItem(2);
	m_maxTuplet->setCurrentItem(2);
	m_rebeam->setChecked(true);
	m_makeViable->setChecked(false);
	m_deCounterpoint->setChecked(false);
	m_articulate->setChecked(true);
    }

    for (unsigned int i = 0; i < m_standardQuantizations.size(); ++i) {

	Rosegarden::timeT time = m_standardQuantizations[i];
	Rosegarden::timeT error = 0;

	QPixmap pmap =
	    NotePixmapFactory::toQPixmap(npf.makeNoteMenuPixmap(time, error));
	QString label = npf.makeNoteMenuLabel(time, false, error);

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
