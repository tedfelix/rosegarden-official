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

#include <kapp.h>
#include <klocale.h>
#include <kconfig.h>
#include <kcombobox.h>

#include "widgets.h"
#include "rosedebug.h"
#include "rosestrings.h"
#include "notationstrings.h"
#include "notepixmapfactory.h"
#include "colours.h"
#include "midipitchlabel.h"

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
	    emit activated(currentText());
	    emit activated(currentItem());
        }
    }
    else
    {
        if (currentItem() > 0)
        {
            setCurrentItem(currentItem() - 1);
	    emit activated(currentText());
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
	e->type() == QEvent::MouseMove           ||
        e->type() == QEvent::MouseButtonRelease  ||
        e->type() == QEvent::MouseButtonDblClick ||
        e->type() == QEvent::KeyPress            ||
        e->type() == QEvent::KeyRelease          ||
        e->type() == QEvent::DragEnter           ||
        e->type() == QEvent::DragMove            ||
        e->type() == QEvent::DragLeave           ||
        e->type() == QEvent::Drop                ||
        e->type() == QEvent::DragResponse        ||
        e->type() == QEvent::Close)

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
					     bool /*useDelay*/,
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
	e->type() == QEvent::MouseMove           ||
        e->type() == QEvent::MouseButtonRelease  ||
        e->type() == QEvent::MouseButtonDblClick ||
        e->type() == QEvent::KeyPress            ||
        e->type() == QEvent::KeyRelease          ||
        e->type() == QEvent::DragEnter           ||
        e->type() == QEvent::DragMove            ||
        e->type() == QEvent::DragLeave           ||
        e->type() == QEvent::Drop                ||
        e->type() == QEvent::DragResponse        ||
        e->type() == QEvent::Close)

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
    QSlider(Qt::Vertical, parent),
    m_float(new RosegardenTextFloat(this)),
    m_floatTimer(new QTimer()),
    m_prependText("")
{
    connect(this, SIGNAL(valueChanged(int)),
            this, SLOT(slotValueChanged(int)));

    connect(this, SIGNAL(sliderPressed()),
            this, SLOT(slotShowFloatText()));

    // connect timer
    connect(m_floatTimer, SIGNAL(timeout()), this, SLOT(slotFloatTimeout()));

    m_float->hide(); // hide the floater
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

    slotShowFloatText();
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

void
RosegardenFader::slotShowFloatText()
{
    float dbValue = 10.0 * log10(float(maxValue() - value())/100.0);

    // draw on the float text
    //m_float->setText(QString("%1").arg(maxValue() - value()));
    m_float->setText(QString("%1%2 dB").arg(m_prependText).arg(dbValue));

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
    // Move just top/right
    //
    m_float->move(totalPos + QPoint(width() + 2, 0));

    // Show
    m_float->show();

    // one shot, 500ms
    m_floatTimer->start(500, true);
}


void
RosegardenFader::slotFloatTimeout()
{
    m_float->hide();
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

    layout = new QGridLayout(notationFrame, 3, 2, 5, 3);

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
    
    NotePixmapFactory npf;
    QPixmap noMap =
	NotePixmapFactory::toQPixmap(npf.makeToolbarPixmap("menu-no-note"));

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

	QPixmap pmap =
	    NotePixmapFactory::toQPixmap(npf.makeNoteMenuPixmap(time, error));
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
    QGroupBox(1, Horizontal, title, parent)
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


