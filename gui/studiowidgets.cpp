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

#include <kapp.h>
#include <klocale.h>
#include <kstddirs.h>
#include <kconfig.h>
#include <kcombobox.h>

#include <qlayout.h>
#include <qvbox.h>
#include <qtooltip.h>

#include "Instrument.h"
#include "AudioLevel.h"
#include "studiowidgets.h"
#include "colours.h"
#include "constants.h"
#include "rosedebug.h"

#include <cmath>

//----------------------------------------

RosegardenFader::RosegardenFader(Rosegarden::AudioLevel::FaderType type,
				 int w, int h, QWidget *parent) :
    QWidget(parent),
    m_integral(false),
    m_vertical(h > w),
    m_min(0),
    m_max(0),
    m_type(type),
    m_clickMousePos(-1),
    m_float(new RosegardenTextFloat(this)),
    m_floatTimer(new QTimer()),
    m_groovePixmap(0),
    m_buttonPixmap(0)
{
    setBackgroundMode(Qt::NoBackground);
    setFixedSize(w, h); // provisional
    calculateButtonPixmap();
//    if (m_vertical) {
//	setFixedSize(w, h + m_buttonPixmap->height() + 4);
//    } else {
//	setFixedSize(w + m_buttonPixmap->width() + 4, h);
//    }

    if (m_vertical) {
	m_sliderMin = m_buttonPixmap->height()/2 + 2;
	m_sliderMax = height() - m_sliderMin;
    } else {
	m_sliderMin = m_buttonPixmap->width()/2 + 2;
	m_sliderMax = width() - m_sliderMin;
    }	

    calculateGroovePixmap();
    setFader(0.0);

    connect(m_floatTimer, SIGNAL(timeout()), this, SLOT(slotFloatTimeout()));
    m_float->hide();
}

RosegardenFader::RosegardenFader(int min, int max, int deflt,
				 int w, int h, QWidget *parent) :
    QWidget(parent),
    m_integral(true),
    m_vertical(h > w),
    m_min(min),
    m_max(max),
    m_clickMousePos(-1),
    m_float(new RosegardenTextFloat(this)),
    m_floatTimer(new QTimer()),
    m_groovePixmap(0),
    m_buttonPixmap(0)
{
    setBackgroundMode(Qt::NoBackground);
    setFixedSize(w, h); // provisional
    calculateButtonPixmap();
//    if (m_vertical) {
//	setFixedSize(w, h + m_buttonPixmap->height() + 4);
//    } else {
//	setFixedSize(w + m_buttonPixmap->width() + 4, h);
//    }

    if (m_vertical) {
	m_sliderMin = m_buttonPixmap->height()/2 + 2;
	m_sliderMax = height() - m_sliderMin;
    } else {
	m_sliderMin = m_buttonPixmap->width()/2 + 2;
	m_sliderMax = width() - m_sliderMin;
    }	

    calculateGroovePixmap();
    setFader(deflt);

    connect(m_floatTimer, SIGNAL(timeout()), this, SLOT(slotFloatTimeout()));
    m_float->hide();
}

RosegardenFader::RosegardenFader(int min, int max, int deflt,
				 bool vertical, QWidget *parent) :
    QWidget(parent),
    m_integral(true),
    m_vertical(vertical),
    m_min(min),
    m_max(max),
    m_clickMousePos(-1),
    m_float(new RosegardenTextFloat(this)),
    m_floatTimer(new QTimer()),
    m_groovePixmap(0),
    m_buttonPixmap(0)
{
    setBackgroundMode(Qt::NoBackground);
    calculateButtonPixmap();
//    if (vertical) {
//	setFixedSize(m_buttonPixmap->width(),
//		     (max - min) + m_buttonPixmap->height() + 4);
//    } else {
//	setFixedSize((max - min) + m_buttonPixmap->height() + 4,
//		     m_buttonPixmap->height());
//    }

    if (m_vertical) {
	m_sliderMin = m_buttonPixmap->height()/2 + 2;
	m_sliderMax = height() - m_sliderMin;
    } else {
	m_sliderMin = m_buttonPixmap->width()/2 + 2;
	m_sliderMax = width() - m_sliderMin;
    }	

    calculateGroovePixmap();
    setFader(deflt);

    connect(m_floatTimer, SIGNAL(timeout()), this, SLOT(slotFloatTimeout()));
    m_float->hide();
}

RosegardenFader::~RosegardenFader()
{
    delete m_groovePixmap;
    delete m_buttonPixmap;
}

float
RosegardenFader::getFaderLevel() const
{
    return m_value;
}

void
RosegardenFader::setFader(float value)
{
    m_value = value;
    emit faderChanged(value);
    paintEvent(0);
}

float
RosegardenFader::position_to_value(int position)
{
    float value;

    if (m_integral) {
	float sliderLength = float(m_sliderMax) - float(m_sliderMin);
	value = float(position)
	    * (m_max - m_min) / sliderLength - m_min;
    } else {
	value = Rosegarden::AudioLevel::fader_to_dB
	    (position, m_sliderMax - m_sliderMin, m_type);
    }
    
    return value;
}

int
RosegardenFader::value_to_position(float value)
{
    int position;

    if (m_integral) {
	float sliderLength = float(m_sliderMax) - float(m_sliderMin);
	position = 
	    int(sliderLength * (value - m_min) / (m_max - m_min));
    } else {
	position = 
	    Rosegarden::AudioLevel::dB_to_fader
	    (value, m_sliderMax - m_sliderMin, m_type);
    }

    return position;
} 

void
RosegardenFader::paintEvent(QPaintEvent *)
{
    QPainter paint(this);
    int position = value_to_position(m_value);

    if (m_vertical) {

	int aboveButton = height() - position - m_sliderMin - m_buttonPixmap->height()/2;
	int belowButton = position + m_sliderMin - m_buttonPixmap->height()/2;

	if (aboveButton > 0) {
	    paint.drawPixmap(0, 0,
			     *m_groovePixmap,
			     0, 0,
			     m_groovePixmap->width(), aboveButton);
	}

	if (belowButton > 0) {
	    paint.drawPixmap(0, aboveButton + m_buttonPixmap->height(),
			     *m_groovePixmap,
			     0, aboveButton + m_buttonPixmap->height(),
			     m_groovePixmap->width(), belowButton);
	}

	int buttonMargin = (width() - m_buttonPixmap->width()) / 2;

	paint.drawPixmap(buttonMargin, aboveButton, *m_buttonPixmap);

	paint.drawPixmap(0, aboveButton,
			 *m_groovePixmap,
			 0, aboveButton,
			 buttonMargin, m_buttonPixmap->height());

	paint.drawPixmap(m_buttonPixmap->width(), aboveButton,
			 *m_groovePixmap,
			 m_buttonPixmap->width(), aboveButton,
			 width() - m_buttonPixmap->width(),
			 m_buttonPixmap->height());

    } else {
//!!!update
	int leftOfButton =
	    (m_sliderMax - m_sliderMin) - position - m_buttonPixmap->width()/2;

	int rightOfButton =
	    position - m_buttonPixmap->width()/2;

	if (leftOfButton > 0) {
	    paint.drawPixmap(0, 0,
			     *m_groovePixmap,
			     0, 0,
			     leftOfButton, m_groovePixmap->height());
	}

	if (rightOfButton > 0) {
	    paint.drawPixmap(rightOfButton + m_buttonPixmap->width(), 0,
			     *m_groovePixmap,
			     m_groovePixmap->width() - rightOfButton, 0,
			     rightOfButton, m_groovePixmap->height());
	}

	paint.drawPixmap(leftOfButton, 0, *m_buttonPixmap);
    }

    paint.end();
}

void
RosegardenFader::mousePressEvent(QMouseEvent *e)
{
    m_clickMousePos = -1;

    if (e->button() == LeftButton) {
	
	if (m_vertical) {
	    int buttonPosition = value_to_position(m_value);
	    int clickPosition = height() - e->y() - m_sliderMin;
	    
	    if (clickPosition < buttonPosition + m_buttonPixmap->height()/2 &&
		clickPosition > buttonPosition - m_buttonPixmap->height()/2) {
		m_clickMousePos = clickPosition;
		m_clickButtonPos = value_to_position(m_value);
		showFloatText();
	    }
	}
    }
}

void
RosegardenFader::mouseReleaseEvent(QMouseEvent *e)
{
    mouseMoveEvent(e);
    m_clickMousePos = -1;
}

void
RosegardenFader::mouseMoveEvent(QMouseEvent *e)
{
    if (m_clickMousePos >= 0) {
	if (m_vertical) {
	    int mousePosition = height() - e->y() - m_sliderMin;
	    int delta = mousePosition - m_clickMousePos;
	    int buttonPosition = m_clickButtonPos + delta;
	    if (buttonPosition < 0) buttonPosition = 0;
	    if (buttonPosition > m_sliderMax - m_sliderMin) {
		buttonPosition = m_sliderMax - m_sliderMin;
	    }
	    setFader(position_to_value(buttonPosition));
	    showFloatText();
	}
    }
}

void
RosegardenFader::wheelEvent(QWheelEvent *e)
{
    int buttonPosition = value_to_position(m_value);
    if (e->state() && ShiftButton) {
	if (e->delta() > 0) buttonPosition += 10;
	else buttonPosition -= 10;
    } else {
	if (e->delta() > 0) buttonPosition += 1;
	else buttonPosition -= 1;
    }
    setFader(position_to_value(buttonPosition));
}

void
RosegardenFader::showFloatText()
{
    // draw on the float text

    QString text;

    if (m_integral) {
	text = QString("%1").arg(int(m_value));
    } else if (m_value == Rosegarden::AudioLevel::DB_FLOOR) {
	text = "Off";
    } else {
	float v = fabs(m_value);
	text = QString("%1%2.%3%4%5 dB")
	    .arg(m_value < 0 ? '-' : '+')
	    .arg(int(v))
	    .arg(int(v * 10) % 10)
	    .arg(int(v * 100) % 10)
	    .arg(int(v * 1000) % 10);
    }

    m_float->setText(text);

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

void
RosegardenFader::calculateGroovePixmap()
{
    if (m_vertical) {
	m_groovePixmap = new QPixmap(width(), height());
	m_groovePixmap->fill(colorGroup().background());
	QPainter paint(m_groovePixmap);
	paint.setBrush(colorGroup().background());

	paint.setPen(colorGroup().mid());
	paint.drawRect(0, 0, width(), height());
 
	if (m_integral) {
	    //!!!
	} else {
	    for (int dB = -70; dB <= 10; ) {
		int position = value_to_position(float(dB));
		if (position >= 0 &&
		    position < m_sliderMax - m_sliderMin) {
		    if (dB == 0) paint.setPen(colorGroup().dark());
		    else paint.setPen(colorGroup().midlight());
		    paint.drawLine(1, (m_sliderMax - position),
				   width()-2, (m_sliderMax - position));
		}
		if (dB < -10) dB += 10;
		else dB += 2;
	    }
	}
	
	paint.setPen(colorGroup().dark());
	paint.setBrush(colorGroup().mid());
	paint.drawRect(width()/2 - 3, height() - m_sliderMax,
		       6, m_sliderMax - m_sliderMin);
	paint.end();
    } else {
	//!!!
    }
}

void
RosegardenFader::calculateButtonPixmap()
{
    if (m_vertical) {

	int buttonHeight = height()/7;
	buttonHeight /= 10;
	++buttonHeight;
	buttonHeight *= 10;
	++buttonHeight;
	int buttonWidth = width() * 2 / 3;
	buttonWidth /= 5;
	++buttonWidth;
	buttonWidth *= 5;
	if (buttonWidth > width()-2) buttonWidth = width()-2;

	m_buttonPixmap = new QPixmap(buttonWidth, buttonHeight);
	m_buttonPixmap->fill(colorGroup().background());

	int x = 0;
	int y = 0;

	QPainter paint(m_buttonPixmap);

	paint.setPen(colorGroup().light());
	paint.drawLine(x + 1, y, x + buttonWidth - 2, y);
	paint.drawLine(x, y + 1, x, y + buttonHeight - 2);

	paint.setPen(colorGroup().midlight());
	paint.drawLine(x + 1, y + 1, x + buttonWidth - 2, y + 1);
	paint.drawLine(x + 1, y + 1, x + 1, y + buttonHeight - 2);

	paint.setPen(colorGroup().mid());
	paint.drawLine(x + 2, y + buttonHeight - 2, x + buttonWidth - 2,
		       y + buttonHeight - 2);
	paint.drawLine(x + buttonWidth - 2, y + 2, x + buttonWidth - 2,
		       y + buttonHeight - 2);

	paint.setPen(colorGroup().dark());
	paint.drawLine(x + 1, y + buttonHeight - 1, x + buttonWidth - 2,
		       y + buttonHeight - 1);
	paint.drawLine(x + buttonWidth - 1, y + 1, x + buttonWidth - 1,
		       y + buttonHeight - 2);

	paint.setPen(colorGroup().shadow());
	paint.drawLine(x + 1, y + buttonHeight/2, x + buttonWidth - 2,
		       y + buttonHeight/2);

	paint.setPen(colorGroup().mid());
	paint.drawLine(x + 1, y + buttonHeight/2 - 1, x + buttonWidth - 2,
		       y + buttonHeight/2 - 1);
	paint.drawPoint(x, y + buttonHeight/2);

	paint.setPen(colorGroup().light());
	paint.drawLine(x + 1, y + buttonHeight/2 + 1, x + buttonWidth - 2,
		       y + buttonHeight/2 + 1);

	paint.setPen(colorGroup().button());
	paint.setBrush(colorGroup().button());
	paint.drawRect(x + 2, y + 2, buttonWidth - 4, buttonHeight/2 - 4);
	paint.drawRect(x + 2, y + buttonHeight/2 + 2,
		       buttonWidth - 4, buttonHeight/2 - 4);

	paint.end();
    } else {
	//!!!
    }
}


    


// ---------------- AudioFaderWidget ------------------
//

AudioFaderWidget::AudioFaderWidget(QWidget *parent,
				   LayoutType type,
				   QString id,
				   bool haveInOut,
                                   const char *name):
    QFrame(parent, name),
    m_signalMapper(new QSignalMapper(this)),
    m_id(id),
    m_isStereo(false)
{
    // Plugin box
    //
    QPushButton *plugin;
    QVBox *pluginVbox = new QVBox(this);
    pluginVbox->setSpacing(2);

    for (int i = 0; i < 5; i++)
    {
        plugin = new QPushButton(pluginVbox);
	if (type == FaderStrip) {
	    plugin->setText(i18n("<none>"));
	} else {
	    plugin->setText(i18n("<no plugin>"));
	}

        QToolTip::add(plugin, i18n("Audio plugin button"));

        m_plugins.push_back(plugin);
        m_signalMapper->setMapping(plugin, i);
        connect(plugin, SIGNAL(clicked()),
                m_signalMapper, SLOT(map()));
    }

    // VU meter and fader
    //
    if (type == FaderBox) {
	m_vuMeter = new AudioVUMeter(this);
    } else {
	m_vuMeter = new AudioVUMeter(this,
				     VUMeter::AudioPeakHold,
				     true,
				     14,
				     240);
    }

    m_fader = new RosegardenFader(type == FaderBox ?
				  Rosegarden::AudioLevel::ShortFader :
				  Rosegarden::AudioLevel::LongFader,
				  20, m_vuMeter->height(), this);
    
    if (type == FaderBox) {
	m_recordFader = new RosegardenFader(Rosegarden::AudioLevel::ShortFader,
					    20, m_vuMeter->height(), this);
    } else {
	m_recordFader = 0;
    }

    // Stereo, solo, mute and pan
    //
    QString pixmapDir = KGlobal::dirs()->findResource("appdata", "pixmaps/");
    m_monoPixmap.load(QString("%1/misc/mono.xpm").arg(pixmapDir));
    m_stereoPixmap.load(QString("%1/misc/stereo.xpm").arg(pixmapDir));

    m_pan = new RosegardenRotary(this, -100.0, 100.0, 1.0, 5.0, 0.0, 24);

    QLabel *panLabel = 0;

    if (type == FaderBox) {
//	panLabel = new QLabel(i18n("Pan"), this);
    }

    // same as the knob colour on the MIDI pan
    m_pan->setKnobColour(RosegardenGUIColours::RotaryPastelGreen);

    m_stereoButton = new QPushButton(this);
    m_stereoButton->setPixmap(m_monoPixmap); // default is mono
    m_stereoButton->setFixedSize(24, 24);

    connect(m_stereoButton, SIGNAL(clicked()),
            this, SLOT(slotChannelStateChanged()));

    m_muteButton = new QPushButton(this);
    m_muteButton->setText("M");
    m_muteButton->setToggleButton(true);

    m_soloButton = new QPushButton(this);
    m_soloButton->setText("S");
    m_soloButton->setToggleButton(true);

    m_recordButton = new QPushButton(this);
    m_recordButton->setText("R");
    m_recordButton->setToggleButton(true);

    m_muteButton->setMaximumWidth(m_stereoButton->width());
    m_soloButton->setMaximumWidth(m_stereoButton->width());
    m_recordButton->setMaximumWidth(m_stereoButton->width());

    if (haveInOut) {
	m_audioInput = new KComboBox(this);
	m_audioOutput = new KComboBox(this);
    } else {
	m_audioInput = 0;
	m_audioOutput = 0;
    }

    // Tooltips.  Use more verbose ones for the fader box than the
    // strip.
    if (type == FaderBox) {
	QToolTip::add(m_pan, i18n("Set the audio pan position in the stereo field"));
	QToolTip::add(m_recordButton, i18n("Arm recording for this audio Instrument"));
	QToolTip::add(m_soloButton, i18n("Solo the Track to which this Instrument is attached."));
	QToolTip::add(m_muteButton, i18n("Mute the Track to which this Instrument is attached."));
	QToolTip::add(m_stereoButton, i18n("Mono or Stereo Audio Instrument"));
	QToolTip::add(m_vuMeter, i18n("Audio Level Meter"));
    } else {
	QToolTip::add(m_pan, i18n("Pan"));
	QToolTip::add(m_recordButton, i18n("Arm Record"));
	QToolTip::add(m_soloButton, i18n("Solo"));
	QToolTip::add(m_muteButton, i18n("Mute"));
	QToolTip::add(m_stereoButton, i18n("Mono or Stereo"));
	QToolTip::add(m_vuMeter, i18n("Audio Level"));
    }
    
    // Sort out the layout accordingly
    //
    QGridLayout *grid;
   
    if (type == FaderStrip)
    {
	setFrameStyle(Box | Sunken);
	setLineWidth(1);

        grid = new QGridLayout(this, haveInOut ? 7 : 5, 2, 0, 1);

	int row = 0;
	if (haveInOut) {
	    grid->addMultiCellWidget(m_audioInput, 0, 0, 0, 1, AlignCenter);
	    grid->addMultiCellWidget(m_audioOutput, 1, 1, 0, 1, AlignCenter);
	    row = 2;
	}

	if (id != "") {
	    grid->addWidget(new QLabel(id, this), row, 0, AlignCenter);
	    grid->addWidget(m_pan, row, 1, AlignCenter);
	} else {
	    grid->addMultiCellWidget(m_pan, row, row, 0, 1, AlignCenter);
	}

	++row;

	grid->addMultiCellWidget(m_fader, row, row, 0, 0, AlignRight);
	grid->addMultiCellWidget(m_vuMeter, row, row, 1, 1, AlignLeft);

	++row;
	grid->addWidget(m_muteButton, row, 0, AlignCenter);
	grid->addWidget(m_soloButton, row, 1, AlignCenter);

	++row;
	grid->addWidget(m_recordButton, row, 0, AlignCenter);	
	grid->addWidget(m_stereoButton, row, 1, AlignCenter);
	
	++row;
        grid->addMultiCellWidget(pluginVbox, row, row, 0, 1, AlignCenter);
    }
    else
    {
        grid = new QGridLayout(this, 6, 6, 6, 6);

        grid->addMultiCellWidget(pluginVbox,    2, 6, 0, 1, AlignCenter);

        grid->addMultiCellWidget(m_vuMeter,     2, 6, 3, 3, AlignCenter);
        grid->addMultiCellWidget(m_fader,       2, 6, 2, 2, AlignCenter);
        grid->addMultiCellWidget(m_recordFader, 2, 6, 4, 4, AlignCenter);

        grid->addWidget(m_muteButton,           2, 5, AlignLeft);
        grid->addWidget(m_soloButton,           3, 5, AlignLeft);

        grid->addWidget(m_recordButton,         4, 5, AlignLeft);

//        grid->addMultiCellWidget(panLabel,      8, 8, 0, 1, AlignCenter);
        grid->addWidget(m_pan,                  5, 5, AlignLeft);
        grid->addWidget(m_stereoButton,         6, 5, AlignLeft);

	if (haveInOut) {

	    QLabel *inputLabel = new QLabel(i18n("In:"), this);
	    grid->addWidget(inputLabel,             0, 1, AlignRight);
	    grid->addMultiCellWidget(m_audioInput,  0, 0, 2, 5, AlignLeft);

	    QLabel *outputLabel = new QLabel(i18n("Out:"), this);
	    grid->addWidget(outputLabel,             1, 1, AlignRight);
	    grid->addMultiCellWidget(m_audioOutput,  1, 1, 2, 5, AlignLeft);
	}
    }

    for (int i = 0; i < 5; ++i) {
        // Force width
	if (type == FaderStrip) {
	    m_plugins[i]->setMaximumWidth(50);
	    if (haveInOut) {
		m_audioInput->setMaximumWidth(50);
		m_audioOutput->setMaximumWidth(50);
	    }
	} else {
	    m_plugins[i]->setFixedWidth(m_plugins[i]->width());
	}
    }

}


void
AudioFaderWidget::setAudioChannels(int channels)
{
    switch (channels)
    {
        case 1:
            m_stereoButton->setPixmap(m_monoPixmap);
            m_isStereo = false;
            break;

        case 2:
            m_stereoButton->setPixmap(m_stereoPixmap);
            m_isStereo = true;
            break;
        default:
            RG_DEBUG << "AudioFaderWidget::setAudioChannels - "
                     << "unsupported channel numbers (" << channels
                     << ")" << endl;
	    return;
    }

    if (!m_audioInput) return;

    // Populate audio inputs accordingly
    //
    KConfig* config = kapp->config();
    config->setGroup(Rosegarden::SequencerOptionsConfigGroup);

    int jackAudioInputs = config->readNumEntry("jackaudioinputs", 2);
    QString inputName;

    // clear existing entries
    m_audioInput->clear();

    for (int i = 0; i < jackAudioInputs; i+= channels)
    {
        if (channels == 1)
            inputName = QString("Input %1").arg(i + 1);
        else
        {
            if ((i + 1) > jackAudioInputs) break;
            inputName = QString("Input %1-%2").arg(i + 1).arg(i + 2);
        }

        m_audioInput->insertItem(inputName);
    }
}

void
AudioFaderWidget::slotChannelStateChanged()
{
    if (m_isStereo)
    {
        setAudioChannels(1);
        emit audioChannelsChanged(1);
    }
    else
    {
        setAudioChannels(2);
        emit audioChannelsChanged(2);
    }
}


