// -*- c-basic-offset: 4 -*-

/*
    Rosegarden-4
    A sequencer and musical notation editor.

    This program is Copyright 2000-2004
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

#include "config.h"

#include <kapp.h>
#include <klocale.h>
#include <kstddirs.h>
#include <kconfig.h>
#include <kcombobox.h>

#include <qlayout.h>
#include <qvbox.h>
#include <qtooltip.h>
#include <qpopupmenu.h>
#include <qcursor.h>

#include "Instrument.h"
#include "AudioLevel.h"
#include "studiowidgets.h"
#include "colours.h"
#include "constants.h"
#include "rosedebug.h"
#include "Studio.h"
#include "MappedStudio.h"
#include "studiocontrol.h"
#include "rosegardengui.h"
#include "audiopluginoscgui.h"

#include <cmath>

//----------------------------------------

RosegardenFader::PixmapCache RosegardenFader::m_pixmapCache;

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
    m_floatTimer(new QTimer())
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
	m_sliderMin = buttonPixmap()->height()/2 + 2;
	m_sliderMax = height() - m_sliderMin;
    } else {
	m_sliderMin = buttonPixmap()->width()/2 + 2;
	m_sliderMax = width() - m_sliderMin;
    }	

    m_outlineColour = colorGroup().mid();

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
    m_floatTimer(new QTimer())
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
	m_sliderMin = buttonPixmap()->height()/2 + 2;
	m_sliderMax = height() - m_sliderMin;
    } else {
	m_sliderMin = buttonPixmap()->width()/2 + 2;
	m_sliderMax = width() - m_sliderMin;
    }	

    m_outlineColour = colorGroup().mid();

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
    m_floatTimer(new QTimer())
{
    setBackgroundMode(Qt::NoBackground);
    calculateButtonPixmap();

    if (m_vertical) {
	m_sliderMin = buttonPixmap()->height()/2 + 2;
	m_sliderMax = height() - m_sliderMin;
    } else {
	m_sliderMin = buttonPixmap()->width()/2 + 2;
	m_sliderMax = width() - m_sliderMin;
    }	

    m_outlineColour = colorGroup().mid();

    calculateGroovePixmap();
    setFader(deflt);

    connect(m_floatTimer, SIGNAL(timeout()), this, SLOT(slotFloatTimeout()));
    m_float->hide();
}

RosegardenFader::~RosegardenFader()
{
}

void
RosegardenFader::setOutlineColour(QColor c)
{
    m_outlineColour = c;
    calculateGroovePixmap();
}

QPixmap *
RosegardenFader::groovePixmap()
{
    PixmapCache::iterator i = m_pixmapCache.find(SizeRec(width(), height()));
    if (i != m_pixmapCache.end()) {
	ColourPixmapRec::iterator j = i->second.first.find(m_outlineColour.pixel());
	if (j != i->second.first.end()) {
	    return j->second;
	}
    }
    return 0;
}

QPixmap *
RosegardenFader::buttonPixmap()
{
    PixmapCache::iterator i = m_pixmapCache.find(SizeRec(width(), height()));
    if (i != m_pixmapCache.end()) {
	return i->second.second;
    }
    else return 0;
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
	    * float(m_max - m_min) / sliderLength - float(m_min);
        if (value > m_max) value = float(m_max);
        if (value < m_min) value = float(m_min);
    } else {
	value = Rosegarden::AudioLevel::fader_to_dB
	    (position, m_sliderMax - m_sliderMin, m_type);
    }
/*
    RG_DEBUG << "RosegardenFader::position_to_value - position = " << position
             << ", new value = " << value << endl;

    if (m_min != m_max) // works for integral case
    {
        if (value > m_max) value = float(m_max);
        if (value < m_min) value = float(m_min);
    }

    RG_DEBUG << "RosegardenFader::position_to_value - limited value = " << value << endl;
*/
    return value;
}

int
RosegardenFader::value_to_position(float value)
{
    int position;

    if (m_integral) {
	float sliderLength = float(m_sliderMax) - float(m_sliderMin);
	position = 
	    int(sliderLength * (value - float(m_min)) / float(m_max - m_min) + 0.1);
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

	int aboveButton = height() - position - m_sliderMin - buttonPixmap()->height()/2;
	int belowButton = position + m_sliderMin - buttonPixmap()->height()/2;

	if (aboveButton > 0) {
	    paint.drawPixmap(0, 0,
			     *groovePixmap(),
			     0, 0,
			     groovePixmap()->width(), aboveButton);
	}

	if (belowButton > 0) {
	    paint.drawPixmap(0, aboveButton + buttonPixmap()->height(),
			     *groovePixmap(),
			     0, aboveButton + buttonPixmap()->height(),
			     groovePixmap()->width(), belowButton);
	}

	int buttonMargin = (width() - buttonPixmap()->width()) / 2;

	paint.drawPixmap(buttonMargin, aboveButton, *buttonPixmap());

	paint.drawPixmap(0, aboveButton,
			 *groovePixmap(),
			 0, aboveButton,
			 buttonMargin, buttonPixmap()->height());

	paint.drawPixmap(buttonMargin + buttonPixmap()->width(), aboveButton,
			 *groovePixmap(),
			 buttonMargin + buttonPixmap()->width(), aboveButton,
			 width() - buttonMargin - buttonPixmap()->width(),
			 buttonPixmap()->height());

    } else {
//... update
	int leftOfButton =
	    (m_sliderMax - m_sliderMin) - position - buttonPixmap()->width()/2;

	int rightOfButton =
	    position - buttonPixmap()->width()/2;

	if (leftOfButton > 0) {
	    paint.drawPixmap(0, 0,
			     *groovePixmap(),
			     0, 0,
			     leftOfButton, groovePixmap()->height());
	}

	if (rightOfButton > 0) {
	    paint.drawPixmap(rightOfButton + buttonPixmap()->width(), 0,
			     *groovePixmap(),
			     groovePixmap()->width() - rightOfButton, 0,
			     rightOfButton, groovePixmap()->height());
	}

	paint.drawPixmap(leftOfButton, 0, *buttonPixmap());
    }

    paint.end();
}

void
RosegardenFader::mousePressEvent(QMouseEvent *e)
{
    m_clickMousePos = -1;

    if (e->button() == LeftButton) {
	
	if (e->type() == QEvent::MouseButtonDblClick) {
	    setFader(0);
	    return;
	}

	if (m_vertical) {
	    int buttonPosition = value_to_position(m_value);
	    int clickPosition = height() - e->y() - m_sliderMin;
	    
	    if (clickPosition < buttonPosition + buttonPixmap()->height()/2 &&
		clickPosition > buttonPosition - buttonPixmap()->height()/2) {
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
    if (e->state() & ShiftButton) {
	if (e->delta() > 0) buttonPosition += 10;
	else buttonPosition -= 10;
    } else {
	if (e->delta() > 0) buttonPosition += 1;
	else buttonPosition -= 1;
    }
    RG_DEBUG << "RosegardenFader::wheelEvent - button position = " << buttonPosition << endl;
    setFader(position_to_value(buttonPosition));
    RG_DEBUG << "RosegardenFader::wheelEvent - value = " << m_value << endl;

    showFloatText();
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
    QPixmap *& map = m_pixmapCache[SizeRec(width(), height())].first[m_outlineColour.pixel()];

    delete map;
    map = new QPixmap(width(), height());
    map->fill(colorGroup().background());
    QPainter paint(map);
    paint.setBrush(colorGroup().background());

    if (m_vertical) {

	paint.setPen(m_outlineColour);
	paint.drawRect(0, 0, width(), height());
 
	if (m_integral) {
	    //...
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
	//...
    }
}

void
RosegardenFader::calculateButtonPixmap()
{
    PixmapCache::iterator i = m_pixmapCache.find(SizeRec(width(), height()));
    if (i != m_pixmapCache.end() && i->second.second) return;
    
    QPixmap *& map = m_pixmapCache[SizeRec(width(), height())].second;

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

	map = new QPixmap(buttonWidth, buttonHeight);
	map->fill(colorGroup().background());

	int x = 0;
	int y = 0;

	QPainter paint(map);

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
	//...
    }
}


    
//  ------------------  AudioVUMeter ---------------------
//
AudioVUMeter::AudioVUMeter(QWidget *parent,
                           VUMeter::VUMeterType type,
                           bool stereo,
                           int width,
                           int height,
                           const char *name) :
    QWidget(parent, name),
    m_stereo(stereo)
{
    setBackgroundMode(Qt::NoBackground);
    setFixedSize(width, height);

    // This offset is intended to match that for the height of the
    // button pixmap in RosegardenFader (in studiowidgets.cpp, which
    // is probably where this class should be too)

    m_yoff = height / 7;
    m_yoff /= 10;
    ++m_yoff;
    m_yoff *= 10;
    ++m_yoff;

    // This one is _not_ intended to match that for the button width

    m_xoff = width / 4;
    if (m_xoff % 2 == 1) ++m_xoff;

    m_meter = new AudioVUMeterImpl(this, type, stereo,
				   width - m_xoff, height - m_yoff, name);

    m_meter->move(m_xoff/2, m_yoff/2);
}

void
AudioVUMeter::paintEvent(QPaintEvent *e)
{
    QPainter paint(this);
    paint.setPen(colorGroup().mid());
    paint.drawRect(0, 0, width(), height());

    paint.setPen(colorGroup().background());
    paint.setBrush(colorGroup().background());
    paint.drawRect(1, 1, width() - 2, m_yoff/2 - 1);
    paint.drawRect(1, 1, m_xoff/2 - 1, height() - 2);
    paint.drawRect(width() - m_xoff/2 - 1, 1, m_xoff/2, height() - 2);
    paint.drawRect(1, height() - m_yoff/2 - 1, width() - 2, m_yoff/2);
    paint.end();

    m_meter->paintEvent(e);
}

AudioVUMeter::AudioVUMeterImpl::AudioVUMeterImpl(QWidget *parent,
						 VUMeterType type,
						 bool stereo,
						 int width,
						 int height,
						 const char *name) :
    VUMeter(parent, type, stereo, width, height, VUMeter::Vertical, name)
{
}


// ---------------- AudioRouteMenu ------------------
//

AudioRouteMenu::AudioRouteMenu(QWidget *par,
			       Direction direction,
			       Format format,
			       Rosegarden::Studio *studio,
			       Rosegarden::Instrument *instrument) :
    QObject(par),
    m_studio(studio),
    m_instrument(instrument),
    m_direction(direction),
    m_format(format)
{
    switch (format) {

    case Compact:
    {
	m_combo = 0;
	m_button = new WheelyButton(par);
	connect(m_button, SIGNAL(wheel(bool)), this, SLOT(slotWheel(bool)));
	connect(m_button, SIGNAL(clicked()), this, SLOT(slotShowMenu()));
	break;
    }

    case Regular:
    {
	m_button = 0;
	m_combo = new KComboBox(par);
	connect(m_combo, SIGNAL(activated(int)), this, SLOT(slotEntrySelected(int)));
	break;
    }

    }

    slotRepopulate();
}

QWidget *
AudioRouteMenu::getWidget()
{
    if (m_button) return m_button;
    else return m_combo;
}

void
AudioRouteMenu::slotRepopulate()
{
    switch (m_format) {

    case Compact:
	m_button->setText(getEntryText(getCurrentEntry()));
	break;

    case Regular:
	m_combo->clear();
	for (int i = 0; i < getNumEntries(); ++i) {
	    m_combo->insertItem(getEntryText(i));
	}
	m_combo->setCurrentItem(getCurrentEntry());
	break;
    }
}

void
AudioRouteMenu::slotSetInstrument(Rosegarden::Studio *studio,
				  Rosegarden::Instrument *instrument)
{
    m_studio = studio;
    m_instrument = instrument;
    slotRepopulate();
}

void
AudioRouteMenu::slotWheel(bool up)
{
    int current = getCurrentEntry();
    if (up) { // actually moves down the list
	if (current > 0) slotEntrySelected(current - 1);
    } else {
	if (current < getNumEntries() - 1) slotEntrySelected(current + 1);
    }
}

class BlahPopupMenu : public QPopupMenu
{
    // just to make itemHeight public
public:
    BlahPopupMenu(QWidget *parent) : QPopupMenu(parent) { }
    using QPopupMenu::itemHeight;
};

void
AudioRouteMenu::slotShowMenu()
{
    if (getNumEntries() == 0) return;

    BlahPopupMenu *menu = new BlahPopupMenu((QWidget *)parent());

    for (int i = 0; i < getNumEntries(); ++i) {

	menu->insertItem(getEntryText(i), this, SLOT(slotEntrySelected(int)),
			 0, i);
	menu->setItemParameter(i, i);
    }

    int itemHeight = menu->itemHeight(0) + 2;
    QPoint pos = QCursor::pos();
    
    pos.rx() -= 10;
    pos.ry() -= (itemHeight / 2 + getCurrentEntry() * itemHeight);

    menu->popup(pos);
}

int
AudioRouteMenu::getNumEntries()
{
    if (!m_instrument) return 0;

    switch (m_direction) {

    case In:
    {
	int stereoIns =
	    m_studio->getRecordIns().size() +
	    m_studio->getBusses().size();

	if (m_instrument->getAudioChannels() > 1) {
	    return stereoIns;
	} else {
	    return stereoIns * 2;
	}

	break;
    }

    case Out:
	return m_studio->getBusses().size();
    }

    return 0;
}

int
AudioRouteMenu::getCurrentEntry()
{
    if (!m_instrument) return 0;

    switch (m_direction) {

    case In:
    {
	bool stereo = (m_instrument->getAudioChannels() > 1);

	bool isBuss;
	int channel;
	int input = m_instrument->getAudioInput(isBuss, channel);

	if (isBuss) {
	    int recordIns = m_studio->getRecordIns().size();
	    if (stereo) {
		return recordIns + input;
	    } else {
		return recordIns * 2 + input * 2 + channel;
	    }
	} else {
	    if (stereo) {
		return input;
	    } else {
		return input * 2 + channel;
	    }
	}

	break;
    }

    case Out:
	return m_instrument->getAudioOutput();
    }

    return 0;
}    

QString
AudioRouteMenu::getEntryText(int entry)
{
    switch (m_direction) {

    case In:
    {
	bool stereo = (m_instrument->getAudioChannels() > 1);
	int recordIns = m_studio->getRecordIns().size();

	if (stereo) {
	    if (entry < recordIns) {
		return i18n("In %1").arg(entry + 1);
	    } else if (entry == recordIns) {
		return i18n("Master");
	    } else {
		return i18n("Sub %1").arg(entry - recordIns);
	    }
	} else {
	    int channel = entry % 2;
	    entry /= 2;
	    if (entry < recordIns) {
		return (channel ? i18n("In %1 R") :
			          i18n("In %1 L")).arg(entry + 1);
	    } else if (entry == recordIns) {
		return (channel ? i18n("Master R") :
			          i18n("Master L"));
	    } else {
		return (channel ? i18n("Sub %1 R") :
			          i18n("Sub %1 L")).arg(entry - recordIns);
	    }
	}
	break;
    }

    case Out:
	if (entry == 0) return i18n("Master");
	else return i18n("Sub %1").arg(entry);
    }

    return QString();
}

void
AudioRouteMenu::slotEntrySelected(int i)
{
    switch (m_direction) {

    case In:
    {
	bool stereo = (m_instrument->getAudioChannels() > 1);

	bool oldIsBuss;
	int oldChannel;
	int oldInput = m_instrument->getAudioInput(oldIsBuss, oldChannel);

	bool newIsBuss;
	int newChannel = 0;
	int newInput;

	int recordIns = m_studio->getRecordIns().size();

	if (stereo) {
	    newIsBuss = (i >= recordIns);
	    if (newIsBuss) {
		newInput = i - recordIns;
	    } else {
		newInput = i;
	    }
	} else {
	    newIsBuss = (i >= recordIns * 2);
	    newChannel = i % 2;
	    if (newIsBuss) {
		newInput = i/2 - recordIns;
	    } else {
		newInput = i/2;
	    }
	}

	Rosegarden::MappedObjectId oldMappedId = 0, newMappedId = 0;

	if (oldIsBuss) {
	    Rosegarden::Buss *buss = m_studio->getBussById(oldInput);
	    if (buss) oldMappedId = buss->getMappedId();
	} else {
	    Rosegarden::RecordIn *in = m_studio->getRecordIn(oldInput);
	    if (in) oldMappedId = in->getMappedId();
	}

	if (newIsBuss) {
	    Rosegarden::Buss *buss = m_studio->getBussById(newInput);
	    if (!buss) return;
	    newMappedId = buss->getMappedId();
	} else {
	    Rosegarden::RecordIn *in = m_studio->getRecordIn(newInput);
	    if (!in) return;
	    newMappedId = in->getMappedId();
	}

	if (oldMappedId != 0) {
	    Rosegarden::StudioControl::disconnectStudioObjects
		(oldMappedId, m_instrument->getMappedId());
	} else {
	    Rosegarden::StudioControl::disconnectStudioObject
		(m_instrument->getMappedId());
	}

	Rosegarden::StudioControl::setStudioObjectProperty
	    (m_instrument->getMappedId(),
	     Rosegarden::MappedAudioFader::InputChannel, 
	     Rosegarden::MappedObjectValue(newChannel));

	if (newMappedId != 0) {
	    Rosegarden::StudioControl::connectStudioObjects
		(newMappedId, m_instrument->getMappedId());
	}
	
	if (newIsBuss) {
	    m_instrument->setAudioInputToBuss(newInput, newChannel);
	} else {
	    m_instrument->setAudioInputToRecord(newInput, newChannel);
	}
	
	break;
    }

    case Out:
    {
	Rosegarden::BussId bussId = m_instrument->getAudioOutput();
	Rosegarden::Buss *oldBuss = m_studio->getBussById(bussId);
	Rosegarden::Buss *newBuss = m_studio->getBussById(i);
	if (!newBuss) return;

	if (oldBuss) {
	    Rosegarden::StudioControl::disconnectStudioObjects
		(m_instrument->getMappedId(), oldBuss->getMappedId());
	} else {
	    Rosegarden::StudioControl::disconnectStudioObject
		(m_instrument->getMappedId());
	}

	Rosegarden::StudioControl::connectStudioObjects
	    (m_instrument->getMappedId(), newBuss->getMappedId());

	m_instrument->setAudioOutput(i);
	break;
    }
    }
    
    slotRepopulate();
    emit changed();
}
    



// ---------------- AudioFaderBox ------------------
//


AudioFaderBox::AudioFaderBox(QWidget *parent,
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
    QVBox *pluginVbox = 0;
    
    pluginVbox = new QVBox(this);
    pluginVbox->setSpacing(2);
    
    for (int i = 0; i < 5; i++)
    {
	plugin = new QPushButton(pluginVbox);
	plugin->setText(i18n("<no plugin>"));
	
	QToolTip::add(plugin, i18n("Audio plugin button"));
	
	m_plugins.push_back(plugin);
	m_signalMapper->setMapping(plugin, i);
	connect(plugin, SIGNAL(clicked()),
		m_signalMapper, SLOT(map()));
    }

    m_synthButton = new QPushButton(this);
    m_synthButton->setText(i18n("<no synth>"));
    QToolTip::add(m_synthButton, i18n("Synth plugin button"));

    // VU meter and fader
    //
    QHBox *faderHbox = new QHBox(this);
    
    m_vuMeter = new AudioVUMeter(faderHbox);
    
    m_recordFader = new RosegardenFader(Rosegarden::AudioLevel::ShortFader,
					20, m_vuMeter->height(), faderHbox);

    m_recordFader->setOutlineColour(Rosegarden::GUIPalette::getColour(Rosegarden::GUIPalette::RecordFaderOutline));

    delete m_vuMeter; // only used the first one to establish height,
		      // actually want it after the record fader in
		      // hbox
    m_vuMeter = new AudioVUMeter(faderHbox);

    m_fader = new RosegardenFader(Rosegarden::AudioLevel::ShortFader,
				  20, m_vuMeter->height(), faderHbox);

    m_fader->setOutlineColour(Rosegarden::GUIPalette::getColour(Rosegarden::GUIPalette::PlaybackFaderOutline));

    QString pixmapDir = KGlobal::dirs()->findResource("appdata", "pixmaps/");
    m_monoPixmap.load(QString("%1/misc/mono.xpm").arg(pixmapDir));
    m_stereoPixmap.load(QString("%1/misc/stereo.xpm").arg(pixmapDir));

    m_pan = new RosegardenRotary(this, -100.0, 100.0, 1.0, 5.0, 0.0, 22,
				 RosegardenRotary::NoTicks, false, true);

    // same as the knob colour on the MIDI pan
    m_pan->setKnobColour(Rosegarden::GUIPalette::getColour(Rosegarden::GUIPalette::RotaryPastelGreen));

    m_stereoButton = new QPushButton(this);
    m_stereoButton->setPixmap(m_monoPixmap); // default is mono
    m_stereoButton->setFixedSize(24, 24);
    
    connect(m_stereoButton, SIGNAL(clicked()),
	    this, SLOT(slotChannelStateChanged()));

    m_synthGUIButton = new QPushButton(this);
    m_synthGUIButton->setText(i18n("Editor"));

    if (haveInOut) {
	m_audioInput  = new AudioRouteMenu(this,
					   AudioRouteMenu::In,
					   AudioRouteMenu::Regular);
	m_audioOutput = new AudioRouteMenu(this,
					   AudioRouteMenu::Out,
					   AudioRouteMenu::Regular);
    } else {
	m_pan->setKnobColour(Rosegarden::GUIPalette::getColour(Rosegarden::GUIPalette::RotaryPastelOrange));

	m_audioInput = 0;
	m_audioOutput = 0;
    }

    QToolTip::add(m_pan, i18n("Set the audio pan position in the stereo field"));
    QToolTip::add(m_synthGUIButton, i18n("Open synth plugin's native editor"));
    QToolTip::add(m_stereoButton, i18n("Mono or Stereo Instrument"));
    QToolTip::add(m_recordFader, i18n("Record level"));
    QToolTip::add(m_fader, i18n("Playback level"));
    QToolTip::add(m_vuMeter, i18n("Audio level"));

    QGridLayout *grid = new QGridLayout(this, 3, 6, 4, 4);
    
    grid->addMultiCellWidget(m_synthButton, 0, 0, 0, 2);

    if (haveInOut) {
	m_inputLabel = new QLabel(i18n("In:"), this);
	grid->addWidget(m_inputLabel, 0, 0, AlignRight);
	grid->addMultiCellWidget(m_audioInput->getWidget(), 0, 0, 1, 2);
	m_outputLabel = new QLabel(i18n("Out:"), this);
	grid->addWidget(m_outputLabel, 0, 3, AlignRight);
	grid->addMultiCellWidget(m_audioOutput->getWidget(), 0, 0, 4, 5);
    }

    grid->addMultiCellWidget(pluginVbox,   2, 2, 0, 2);
    grid->addMultiCellWidget(faderHbox,    1, 2, 3, 5);

    grid->addWidget(m_synthGUIButton,       1, 0);
    grid->addWidget(m_pan,                  1, 2);
    grid->addWidget(m_stereoButton,         1, 1);

    for (int i = 0; i < 5; ++i) {
	// Force width
	m_plugins[i]->setFixedWidth(m_plugins[i]->width());
    }
    m_synthButton->setFixedWidth(m_plugins[0]->width());
    
    m_synthButton->hide();
    m_synthGUIButton->hide();
}

void
AudioFaderBox::setIsSynth(bool isSynth)
{
    if (isSynth) {
	m_inputLabel->hide();
	m_synthButton->show();
	m_synthGUIButton->show();
	m_audioInput->getWidget()->hide();
	m_recordFader->hide();
    } else {
	m_synthButton->hide();
	m_synthGUIButton->hide();
	m_inputLabel->show();
	m_audioInput->getWidget()->show();
	m_recordFader->show();
    }
}

void
AudioFaderBox::slotSetInstrument(Rosegarden::Studio *studio,
				 Rosegarden::Instrument *instrument)
{
    if (m_audioInput) m_audioInput->slotSetInstrument(studio, instrument);
    if (m_audioOutput) m_audioOutput->slotSetInstrument(studio, instrument);
    if (instrument) setAudioChannels(instrument->getAudioChannels());
    if (instrument) {

	RG_DEBUG << "AudioFaderBox::slotSetInstrument(" << instrument->getId() << ")" << endl;

	setIsSynth(instrument->getType() == Rosegarden::Instrument::SoftSynth);
	if (instrument->getType() == Rosegarden::Instrument::SoftSynth) {
	    bool gui = false;
	    RG_DEBUG << "AudioFaderBox::slotSetInstrument(" << instrument->getId() << "): is soft synth" << endl;
#ifdef HAVE_LIBLO
	    gui = RosegardenGUIApp::self()->getPluginGUIManager()->hasGUI
		(instrument->getId(), Rosegarden::Instrument::SYNTH_PLUGIN_POSITION);
	    RG_DEBUG << "AudioFaderBox::slotSetInstrument(" << instrument->getId() << "): has gui = " << gui << endl;
#endif
	    m_synthGUIButton->setEnabled(gui);
	}	    
    }
}

bool
AudioFaderBox::owns(const QObject *object)
{
    return (object &&
	    ((object->parent() == this) ||
	     (object->parent() && (object->parent()->parent() == this))));
}

void
AudioFaderBox::setAudioChannels(int channels)
{
    m_isStereo = (channels > 1);

    switch (channels)
    {
        case 1:
	    if (m_stereoButton) m_stereoButton->setPixmap(m_monoPixmap);
            m_isStereo = false;
            break;

        case 2:
            if (m_stereoButton) m_stereoButton->setPixmap(m_stereoPixmap);
            m_isStereo = true;
            break;
        default:
            RG_DEBUG << "AudioFaderBox::setAudioChannels - "
                     << "unsupported channel numbers (" << channels
                     << ")" << endl;
	    return;
    }

    if (m_audioInput) m_audioInput->slotRepopulate();
    if (m_audioOutput) m_audioOutput->slotRepopulate();
}

void
AudioFaderBox::slotChannelStateChanged()
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


