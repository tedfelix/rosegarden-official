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

#include "mixer.h"
#include "rosegardenguidoc.h"
#include "sequencermapper.h"
#include "rosedebug.h"
#include "studiocontrol.h"
#include "constants.h"

#include "Studio.h"
#include "AudioLevel.h"

#include <klocale.h>
#include <kconfig.h>
#include <kstdaction.h>
#include <kaction.h>
#include <kglobal.h>
#include <kstddirs.h>

#include <qlayout.h>
#include <qpopupmenu.h>


WheelyButton::~WheelyButton() { }

MixerWindow::MixerWindow(QWidget *parent,
			 RosegardenGUIDoc *document) :
    KMainWindow(parent, "mixerwindow"),
    m_document(document),
    m_studio(&document->getStudio()),
    m_currentId(0)
{
    QFont font;
    font.setPointSize(font.pointSize() * 8 / 10);
    font.setBold(false);
    setFont(font);

    QFont boldFont(font);
    boldFont.setBold(true);

    QFrame *mainBox = new QFrame(this);
//    mainBox->setBackgroundMode(Qt::PaletteMidlight);
//    QHBoxLayout *mainLayout = new QHBoxLayout(mainBox, 4, 4);

    Rosegarden::InstrumentList instruments = m_studio->getPresentationInstruments();
    Rosegarden::BussList busses = m_studio->getBusses();
    
    QString pixmapDir = KGlobal::dirs()->findResource("appdata", "pixmaps/");
    m_monoPixmap.load(QString("%1/misc/mono.xpm").arg(pixmapDir));
    m_stereoPixmap.load(QString("%1/misc/stereo.xpm").arg(pixmapDir));

    // Total cols: is 2 for each fader, submaster or master, plus 2
    // for the monitor strip, plus 1 for each spacer.
    QGridLayout *mainLayout = new QGridLayout
	(mainBox, (instruments.size() + busses.size() + 1) * 3, 7);

    setCaption(i18n("Mixer"));

    int count = 1;
    int col = 0;

    for (Rosegarden::InstrumentList::iterator i = instruments.begin();
	 i != instruments.end(); ++i) {
	
	if ((*i)->getType() != Rosegarden::Instrument::Audio) continue;

	FaderRec rec;

//!!! bring across the tooltips

	rec.m_input = new WheelyButton(mainBox);
	rec.m_output = new WheelyButton(mainBox);

	rec.m_pan = new RosegardenRotary
	    (mainBox, -100.0, 100.0, 1.0, 5.0, 0.0, 20);
	rec.m_fader = new RosegardenFader
	    (Rosegarden::AudioLevel::LongFader, 20, 240, mainBox);
	rec.m_meter = new AudioVUMeter
	    (mainBox, VUMeter::AudioPeakHoldLong, true, 20, 240);

	rec.m_stereoButton = new QPushButton(mainBox);
	rec.m_stereoButton->setPixmap(m_monoPixmap); // default is mono
	rec.m_stereoButton->setFixedSize(20, 20);
	rec.m_stereoButton->setFlat(true);
	rec.m_stereoness = false;

	rec.m_muteButton = new QPushButton(mainBox);
	rec.m_muteButton->setText("M");
	rec.m_muteButton->setToggleButton(true);
	rec.m_muteButton->setFixedWidth(rec.m_stereoButton->width());
	rec.m_muteButton->setFixedHeight(rec.m_stereoButton->height());
	rec.m_muteButton->setFlat(true);

	rec.m_soloButton = new QPushButton(mainBox);
	rec.m_soloButton->setText("S");
	rec.m_soloButton->setToggleButton(true);
	rec.m_soloButton->setFixedWidth(rec.m_stereoButton->width());
	rec.m_soloButton->setFixedHeight(rec.m_stereoButton->height());
	rec.m_soloButton->setFlat(true);

	rec.m_recordButton = new QPushButton(mainBox);
	rec.m_recordButton->setText("R");
	rec.m_recordButton->setToggleButton(true);
	rec.m_recordButton->setFixedWidth(rec.m_stereoButton->width());
	rec.m_recordButton->setFixedHeight(rec.m_stereoButton->height());
	rec.m_recordButton->setFlat(true);

	rec.m_pluginBox = new QVBox(mainBox);
	
	for (int p = 0; p < 5; ++p) {
	    QPushButton *plugin = new QPushButton(rec.m_pluginBox);
	    plugin->setText(i18n("<none>"));
	    QToolTip::add(plugin, i18n("Audio plugin button"));
	    rec.m_plugins.push_back(plugin);
	    connect(plugin, SIGNAL(clicked()),
		    this, SLOT(slotSelectPlugin()));
	}
	
	QLabel *idLabel = new QLabel(QString("%1").arg(count), mainBox);
	idLabel->setFont(boldFont);

	mainLayout->addMultiCellWidget(rec.m_input, 0, 0, col, col+1);
	mainLayout->addMultiCellWidget(rec.m_output, 1, 1, col, col+1);
	mainLayout->addWidget(idLabel, 2, col, Qt::AlignCenter);
	mainLayout->addWidget(rec.m_pan, 2, col+1, Qt::AlignLeft);
	mainLayout->addWidget(rec.m_fader, 3, col, Qt::AlignCenter);
	mainLayout->addWidget(rec.m_meter, 3, col+1, Qt::AlignCenter);
	mainLayout->addWidget(rec.m_muteButton, 4, col);
	mainLayout->addWidget(rec.m_soloButton, 4, col+1);
	mainLayout->addWidget(rec.m_recordButton, 5, col);
	mainLayout->addWidget(rec.m_stereoButton, 5, col+1);
	mainLayout->addMultiCellWidget(rec.m_pluginBox, 6, 6, col, col+1);

	m_faders[(*i)->getId()] = rec;
	updateFader((*i)->getId());
	updateRouteButtons((*i)->getId());
	updateStereoButton((*i)->getId());

	connect(rec.m_input, SIGNAL(wheel(bool)),
		this, SLOT(slotRoutingButtonWheeled(bool)));
		
	connect(rec.m_input, SIGNAL(clicked()),
		this, SLOT(slotRoutingButtonPressed()));

	connect(rec.m_output, SIGNAL(wheel(bool)),
		this, SLOT(slotRoutingButtonWheeled(bool)));
		
	connect(rec.m_output, SIGNAL(clicked()),
		this, SLOT(slotRoutingButtonPressed()));

	connect(rec.m_fader, SIGNAL(faderChanged(float)),
		this, SLOT(slotFaderLevelChanged(float)));

	connect(rec.m_pan, SIGNAL(valueChanged(float)),
		this, SLOT(slotPanChanged(float)));

	connect(rec.m_soloButton, SIGNAL(clicked()),
		this, SLOT(slotSoloChanged()));

	connect(rec.m_muteButton, SIGNAL(clicked()),
		this, SLOT(slotMuteChanged()));

	connect(rec.m_stereoButton, SIGNAL(clicked()),
		this, SLOT(slotChannelsChanged()));

	connect(rec.m_recordButton, SIGNAL(clicked()),
		this, SLOT(slotRecordChanged()));

	++count;

	mainLayout->addMultiCell(new QSpacerItem(2, 0), 0, 6, col+2, col+2);

	col += 3;
    }
    
    count = 1;

    for (Rosegarden::BussList::iterator i = busses.begin();
	 i != busses.end(); ++i) {

	if (i == busses.begin()) continue; // that one's the master

	FaderRec rec;

//!!! tooltips

	rec.m_pan = new RosegardenRotary
	    (mainBox, -100.0, 100.0, 1.0, 5.0, 0.0, 20);
	rec.m_fader = new RosegardenFader
	    (Rosegarden::AudioLevel::LongFader, 20, 240, mainBox);
	rec.m_meter = new AudioVUMeter
	    (mainBox, VUMeter::AudioPeakHoldLong, true, 20, 240);

	rec.m_muteButton = new QPushButton(mainBox);
	rec.m_muteButton->setText("M");
	rec.m_muteButton->setToggleButton(true);
	rec.m_muteButton->setFlat(true);

	QLabel *idLabel = new QLabel(i18n("S%1").arg(count), mainBox);
	idLabel->setFont(boldFont);

	mainLayout->addWidget(idLabel, 2, col, Qt::AlignCenter);
	mainLayout->addWidget(rec.m_pan, 2, col+1, Qt::AlignLeft);
	mainLayout->addWidget(rec.m_fader, 3, col, Qt::AlignCenter);
	mainLayout->addWidget(rec.m_meter, 3, col+1, Qt::AlignCenter);
	mainLayout->addMultiCellWidget(rec.m_muteButton, 4, 4, col, col+1);

	m_submasters.push_back(rec);
	updateFader(count);

	connect(rec.m_fader, SIGNAL(faderChanged(float)),
		this, SLOT(slotFaderLevelChanged(float)));

	connect(rec.m_pan, SIGNAL(valueChanged(float)),
		this, SLOT(slotPanChanged(float)));

	connect(rec.m_muteButton, SIGNAL(clicked()),
		this, SLOT(slotMuteChanged()));

	++count;

	mainLayout->addMultiCell(new QSpacerItem(2, 0), 0, 6, col+2, col+2);

	col += 3;
    }

    if (busses.size() > 0) {

	FaderRec rec;

//!!! tooltips

	rec.m_fader = new RosegardenFader
	    (Rosegarden::AudioLevel::LongFader, 20, 240, mainBox);
	rec.m_meter = new AudioVUMeter
	    (mainBox, VUMeter::AudioPeakHoldLong, true, 20, 240);

	rec.m_muteButton = new QPushButton(mainBox);
	rec.m_muteButton->setText("M");
	rec.m_muteButton->setToggleButton(true);
	rec.m_muteButton->setFlat(true);

	QLabel *idLabel = new QLabel(i18n("Rec"), mainBox);
	idLabel->setFont(boldFont);

	mainLayout->addMultiCellWidget(idLabel, 2, 2, col, col+1, Qt::AlignCenter);
	mainLayout->addWidget(rec.m_fader, 3, col, Qt::AlignCenter);
	mainLayout->addWidget(rec.m_meter, 3, col+1, Qt::AlignCenter);
	mainLayout->addMultiCellWidget(rec.m_muteButton, 4, 4, col, col+1);

	m_monitor = rec;

	connect(rec.m_fader, SIGNAL(faderChanged(float)),
		this, SLOT(slotFaderLevelChanged(float)));

	connect(rec.m_muteButton, SIGNAL(clicked()),
		this, SLOT(slotMuteChanged()));

	mainLayout->addMultiCell(new QSpacerItem(2, 0), 0, 6, col+2, col+2);

	col += 3;

	rec.m_fader = new RosegardenFader
	    (Rosegarden::AudioLevel::LongFader, 20, 240, mainBox);
	rec.m_meter = new AudioVUMeter
	    (mainBox, VUMeter::AudioPeakHoldLong, true, 20, 240);

	rec.m_muteButton = new QPushButton(mainBox);
	rec.m_muteButton->setText("M");
	rec.m_muteButton->setToggleButton(true);
	rec.m_muteButton->setFlat(true);

	idLabel = new QLabel(i18n("M"), mainBox);
	idLabel->setFont(boldFont);

	mainLayout->addMultiCellWidget(idLabel, 2, 2, col, col+1, Qt::AlignCenter);
	mainLayout->addWidget(rec.m_fader, 3, col, Qt::AlignCenter);
	mainLayout->addWidget(rec.m_meter, 3, col+1, Qt::AlignCenter);
	mainLayout->addMultiCellWidget(rec.m_muteButton, 4, 4, col, col+1);
	mainLayout->addMultiCell(new QSpacerItem(2, 0), 0, 6, col+2, col+2);

	m_master = rec;
	updateFader(0);

	connect(rec.m_fader, SIGNAL(faderChanged(float)),
		this, SLOT(slotFaderLevelChanged(float)));

	connect(rec.m_muteButton, SIGNAL(clicked()),
		this, SLOT(slotMuteChanged()));
    }

    setCentralWidget(mainBox);

    KAction* close = KStdAction::close(this,
                                       SLOT(slotClose()),
                                       actionCollection());

    QIconSet icon = QIconSet(NotePixmapFactory::toQPixmap(NotePixmapFactory::makeToolbarPixmap
                                                 ("transport-play")));
    new KAction(i18n("&Play"), icon, Key_Enter, this,
		SIGNAL(play()), actionCollection(), "play");

    icon = QIconSet(NotePixmapFactory::toQPixmap(NotePixmapFactory::makeToolbarPixmap
                                                 ("transport-stop")));
    new KAction(i18n("&Stop"), icon, Key_Insert, this,
		SIGNAL(stop()), actionCollection(), "stop");

    icon = QIconSet(NotePixmapFactory::toQPixmap(NotePixmapFactory::makeToolbarPixmap
                                                 ("transport-rewind")));
    new KAction(i18n("Re&wind"), icon, Key_End, this,
		SIGNAL(rewindPlayback()), actionCollection(),
		"playback_pointer_back_bar");

    icon = QIconSet(NotePixmapFactory::toQPixmap(NotePixmapFactory::makeToolbarPixmap
                                                 ("transport-ffwd")));
    new KAction(i18n("&Fast Forward"), icon, Key_PageDown, this,
		SIGNAL(fastForwardPlayback()), actionCollection(),
		"playback_pointer_forward_bar");

    icon = QIconSet(NotePixmapFactory::toQPixmap(NotePixmapFactory::makeToolbarPixmap
                                                 ("transport-rewind-end")));
    new KAction(i18n("Rewind to &Beginning"), icon, 0, this,
		SIGNAL(rewindPlaybackToBeginning()), actionCollection(),
		"playback_pointer_start");

    icon = QIconSet(NotePixmapFactory::toQPixmap(NotePixmapFactory::makeToolbarPixmap
                                                 ("transport-ffwd-end")));
    new KAction(i18n("Fast Forward to &End"), icon, 0, this,
		SIGNAL(fastForwardPlaybackToEnd()), actionCollection(),
		"playback_pointer_end");

    createGUI("mixer.rc");
}

MixerWindow::~MixerWindow()
{

}

void
MixerWindow::slotClose()
{
    close();
}    


void
MixerWindow::updateFader(int id)
{
    if (id >= (int)Rosegarden::AudioInstrumentBase) {
	FaderRec &rec = m_faders[id];
	Rosegarden::Instrument *instrument = m_studio->getInstrumentById(id);
	rec.m_fader->setFader(instrument->getLevel());
	rec.m_pan->setPosition(instrument->getPan() - 100);
    } else {
	FaderRec &rec = (id == 0 ? m_master : m_submasters[id-1]);
	Rosegarden::BussList busses = m_studio->getBusses();
	Rosegarden::Buss *buss = busses[id];
	rec.m_fader->setFader(buss->getLevel());
	if (rec.m_pan) rec.m_pan->setPosition(buss->getPan() - 100);
    }
}


void
MixerWindow::updateRouteButtons(int id)
{
    if (id >= (int)Rosegarden::AudioInstrumentBase) {

	FaderRec &rec = m_faders[id];
	Rosegarden::Instrument *i = m_studio->getInstrumentById(id);

	bool isBuss = false;
	int index = i->getAudioInput(isBuss);
	int channel = 0;

	bool stereo = (i->getAudioChannels() > 1);

	//!!! erch, not right -- elsewhere we treat the value in the
	//same way as a stereo one
	if (!stereo) {
	    channel = index % 2;
	    index /= 2;
	}

	if (isBuss) {
	    if (index > 0) {
		if (stereo) {
		    rec.m_input->setText(i18n("Sub %1").arg(index));
		} else {
		    rec.m_input->setText((channel ?
					  i18n("Sub %1 R") :
					  i18n("Sub %1 L")).arg(index));
		}
	    } else {
		if (stereo) {
		    rec.m_input->setText(i18n("Master"));
		} else {
		    rec.m_input->setText(channel ?
					 i18n("Master R") :
					 i18n("Master L"));
		}
	    }
	} else {
	    if (stereo) {
		rec.m_input->setText(i18n("In %1").arg(index+1));
	    } else {
		rec.m_input->setText((channel ?
				      i18n("In %1 R") :
				      i18n("In %1 L")).arg(index+1));
	    }
	}

	Rosegarden::BussId bussId = i->getAudioOutput();
	if (bussId > 0) {
	    rec.m_output->setText(i18n("Sub %1").arg(bussId));
	} else {
	    rec.m_output->setText(i18n("Master"));
	}
    }
}


void
MixerWindow::updateStereoButton(int id)
{
    if (id >= (int)Rosegarden::AudioInstrumentBase) {

	FaderRec &rec = m_faders[id];
	Rosegarden::Instrument *i = m_studio->getInstrumentById(id);

	bool stereo = (i->getAudioChannels() > 1);
	if (stereo == rec.m_stereoness) return;

	rec.m_stereoness = stereo;

	if (stereo) rec.m_stereoButton->setPixmap(m_stereoPixmap);
	else rec.m_stereoButton->setPixmap(m_monoPixmap);
    }
}
	

void
MixerWindow::slotRoutingButtonWheeled(bool up)
{
    const QObject *s = sender();

    Rosegarden::InstrumentId id = 0;
    bool output = false;
    bool stereo = false;

    for (FaderMap::iterator i = m_faders.begin();
	 i != m_faders.end(); ++i) {

	if (i->second.m_input == s) {
	    id = i->first;
	    stereo = i->second.m_stereoness;
	    break;
	} else if (i->second.m_output == s) {
	    id = i->first;
	    stereo = i->second.m_stereoness;
	    output = true;
	    break;
	}
    }

    if (id == 0) return;
    Rosegarden::Instrument *instrument = m_studio->getInstrumentById(id);
    if (!instrument) return;

    if (output) {

	int max, min = 0;
	if (stereo) {
	    max = m_studio->getBusses().size() - 1;
	} else {
	    max = m_studio->getBusses().size() * 2 - 1;
	}

	Rosegarden::BussId out = instrument->getAudioOutput();
	if (up) {
	    if (out < max) slotOutputChanged(out+1);
	} else {
	    if (out > min) slotOutputChanged(out-1);
	}
    } else {

	//!!!
    }
}


void
MixerWindow::slotRoutingButtonPressed()
{
    const QObject *s = sender();

    Rosegarden::InstrumentId id = 0;
    bool output = false;
    bool stereo = false;

    for (FaderMap::iterator i = m_faders.begin();
	 i != m_faders.end(); ++i) {

	if (i->second.m_input == s) {
	    id = i->first;
	    stereo = i->second.m_stereoness;
	    break;
	} else if (i->second.m_output == s) {
	    id = i->first;
	    stereo = i->second.m_stereoness;
	    output = true;
	    break;
	}
    }

    if (id == 0) return;

    QPopupMenu *menu = new QPopupMenu(this);

    int submasterCount = m_studio->getBusses().size() - 1;
    int mid = 1;

    if (output) {

	menu->insertItem(i18n("Master"),
			 this, SLOT(slotOutputChanged(int)), 0, mid);
	menu->setItemParameter(mid, mid-1);
	++mid;

	for (int i = 0; i < submasterCount; ++i) {
	    menu->insertItem(i18n("Sub %1").arg(i+1),
			     this, SLOT(slotOutputChanged(int)), 0, mid);
	    menu->setItemParameter(mid, mid-1);
	    ++mid;
	}

    } else {

	// id/param stuff as above

	int jackAudioInputs = m_studio->getRecordIns().size();

	for (int i = 0; i < jackAudioInputs; ++i) {
	    if (!stereo) {
		menu->insertItem(i18n("In %1 L").arg(i+1),
				 this, SLOT(slotInputChanged(int)), 0, mid);
		menu->setItemParameter(mid, mid-1);
		++mid;
		menu->insertItem(i18n("In %1 R").arg(i+1),
				 this, SLOT(slotInputChanged(int)), 0, mid);
		menu->setItemParameter(mid, mid-1);
		++mid;
	    } else {
		menu->insertItem(i18n("In %1").arg(i+1),
				 this, SLOT(slotInputChanged(int)), 0, mid);
		menu->setItemParameter(mid, mid-1);
		++mid;
	    }
	}
	
	for (int i = 0; i < submasterCount; ++i) {
	    if (!stereo) {
		menu->insertItem(i18n("Sub %1 L").arg(i+1),
				 this, SLOT(slotInputChanged(int)), 0, mid);
		menu->setItemParameter(mid, mid-1);
		++mid;
		menu->insertItem(i18n("Sub %1 R").arg(i+1),
				 this, SLOT(slotInputChanged(int)), 0, mid);
		menu->setItemParameter(mid, mid-1);
		++mid;
	    } else {
		menu->insertItem(i18n("Sub %1").arg(i+1),
				 this, SLOT(slotInputChanged(int)), 0, mid);
		menu->setItemParameter(mid, mid-1);
		++mid;
	    }
	}
	
	if (!stereo) {
	    menu->insertItem(i18n("Master L"),
			     this, SLOT(slotInputChanged(int)), 0, mid);
	    menu->setItemParameter(mid, mid-1);
	    ++mid;
	    menu->insertItem(i18n("Master R"),
			     this, SLOT(slotInputChanged(int)), 0, mid);
	    menu->setItemParameter(mid, mid-1);
	    ++mid;
	} else {
	    menu->insertItem(i18n("Master"),
			     this, SLOT(slotInputChanged(int)), 0, mid);
	    menu->setItemParameter(mid, mid-1);
	    ++mid;
	}
    }

    m_currentId = id;

    //!!! need to popup with current item under pointer
    menu->popup(QCursor::pos());
}

void
MixerWindow::slotInputChanged(int i)
{
    //!!!

}

void
MixerWindow::slotOutputChanged(int i)
{
    RG_DEBUG << "MixerWindow::slotOutputChanged(" << i << ") for instrument "
	     << m_currentId << endl;

    Rosegarden::Instrument *instrument = m_studio->getInstrumentById(m_currentId);
    if (!instrument) return;

    Rosegarden::BussId bussId = instrument->getAudioOutput();
    RG_DEBUG << "MixerWindow::slotOutputChanged(" << i << ") for instrument "
	     << m_currentId << endl;

    Rosegarden::Buss *oldBuss = m_studio->getBussById(bussId);
    Rosegarden::Buss *newBuss = m_studio->getBussById(i);
    if (!newBuss) return;

    if (oldBuss) {
	Rosegarden::StudioControl::disconnectStudioObjects(instrument->getMappedId(),
							   oldBuss->getMappedId());
    } else {
	Rosegarden::StudioControl::disconnectStudioObject(instrument->getMappedId());
    }

    Rosegarden::StudioControl::connectStudioObjects(instrument->getMappedId(),
						    newBuss->getMappedId());

    instrument->setAudioOutput(i);

    m_faders[m_currentId].m_output->
	setText(i > 0 ? i18n("Sub %1").arg(i) : i18n("Master"));
}

void
MixerWindow::slotSelectPlugin()
{
    const QObject *s = sender();

    // no plugins anywhere except instruments yet

    for (FaderMap::iterator i = m_faders.begin();
	 i != m_faders.end(); ++i) {
	
	int index = 0;

	for (std::vector<QPushButton *>::iterator pli = i->second.m_plugins.begin();
	     pli != i->second.m_plugins.end(); ++pli) {

	    if (*pli == s) {

		emit selectPlugin(this, i->first, index);
		return;
	    }

	    ++index;
	}
    }	    
}

void
MixerWindow::slotFaderLevelChanged(float dB)
{
    const QObject *s = sender();

    Rosegarden::BussList busses = m_studio->getBusses();
    
    if (m_master.m_fader == s) {

	if (busses.size() > 0) {
	    Rosegarden::StudioControl::setStudioObjectProperty
		(Rosegarden::MappedObjectId(busses[0]->getMappedId()),
		 Rosegarden::MappedAudioBuss::Level,
		 Rosegarden::MappedObjectValue(dB));
	    busses[0]->setLevel(dB);
	}

	return;
    } 

    int index = 1;

    for (FaderVector::iterator i = m_submasters.begin();
	 i != m_submasters.end(); ++i) {

	if (i->m_fader == s) {
	    if ((int)busses.size() > index) {
		Rosegarden::StudioControl::setStudioObjectProperty
		    (Rosegarden::MappedObjectId(busses[index]->getMappedId()),
		     Rosegarden::MappedAudioBuss::Level,
		     Rosegarden::MappedObjectValue(dB));
		busses[index]->setLevel(dB);
	    }

	    return;
	}

	++index;
    }

    for (FaderMap::iterator i = m_faders.begin();
	 i != m_faders.end(); ++i) {
	
	if (i->second.m_fader == s) {
	    Rosegarden::Instrument *instrument =
		m_studio->getInstrumentById(i->first);
	    Rosegarden::StudioControl::setStudioObjectProperty
		(Rosegarden::MappedObjectId
		 (instrument->getMappedId()),
		 Rosegarden::MappedAudioFader::FaderLevel,
		 Rosegarden::MappedObjectValue(dB));
	    instrument->setLevel(dB);
	}
    }	    
}

void
MixerWindow::slotPanChanged(float pan)
{
    const QObject *s = sender();

    Rosegarden::BussList busses = m_studio->getBusses();

    int index = 1;

    for (FaderVector::iterator i = m_submasters.begin();
	 i != m_submasters.end(); ++i) {

	if (i->m_pan == s) {
	    if ((int)busses.size() > index) {
		Rosegarden::StudioControl::setStudioObjectProperty
		    (Rosegarden::MappedObjectId(busses[index]->getMappedId()),
		     Rosegarden::MappedAudioBuss::Pan,
		     Rosegarden::MappedObjectValue(pan));
		busses[index]->setPan(Rosegarden::MidiByte(pan + 100.0));
	    }
	    return;
	}

	++index;
    }

    for (FaderMap::iterator i = m_faders.begin();
	 i != m_faders.end(); ++i) {
	
	if (i->second.m_pan == s) {
	    Rosegarden::Instrument *instrument =
		m_studio->getInstrumentById(i->first);
	    Rosegarden::StudioControl::setStudioObjectProperty
		(instrument->getMappedId(),
		 Rosegarden::MappedAudioFader::Pan,
		 Rosegarden::MappedObjectValue(pan));
	    instrument->setPan(Rosegarden::MidiByte(pan + 100.0));
	}
    }
}

void
MixerWindow::slotChannelsChanged()
{
    //...
}

void
MixerWindow::slotSoloChanged()
{
    //...
}

void
MixerWindow::slotMuteChanged()
{
    //...
}

void
MixerWindow::slotRecordChanged()
{
    //...
}

void
MixerWindow::closeEvent(QCloseEvent *e)
{
    emit closing();
    KMainWindow::closeEvent(e);
}

void
MixerWindow::updateMeters(SequencerMapper *mapper)
{
    for (FaderMap::iterator i = m_faders.begin(); i != m_faders.end(); ++i) {

	Rosegarden::InstrumentId id = i->first;
	FaderRec &rec = i->second;

	Rosegarden::LevelInfo info;
	if (!mapper->getInstrumentLevel(id, info)) continue;

	RG_DEBUG << "MixerWindow::updateMeters: id " << id << ", level left "
		 << info.level << endl;
	
	// The values passed through are long-fader values
	float dBleft = Rosegarden::AudioLevel::fader_to_dB
	    (info.level, 127, Rosegarden::AudioLevel::LongFader);

	if (rec.m_stereoness) {
	    float dBright = Rosegarden::AudioLevel::fader_to_dB
		(info.levelRight, 127, Rosegarden::AudioLevel::LongFader);

	    rec.m_meter->setLevel(dBleft, dBright);

	} else {
	    rec.m_meter->setLevel(dBleft);
	}
    }

    for (unsigned int i = 0; i < m_submasters.size(); ++i) {

	FaderRec &rec = m_submasters[i];

	Rosegarden::LevelInfo info;
	if (!mapper->getSubmasterLevel(i, info)) continue;

	RG_DEBUG << "MixerWindow::updateMeters: sub " << i << ", level left "
		 << info.level << endl;
	
	// The values passed through are long-fader values
	float dBleft = Rosegarden::AudioLevel::fader_to_dB
	    (info.level, 127, Rosegarden::AudioLevel::LongFader);
	float dBright = Rosegarden::AudioLevel::fader_to_dB
	    (info.levelRight, 127, Rosegarden::AudioLevel::LongFader);

	rec.m_meter->setLevel(dBleft, dBright);
    }

    Rosegarden::LevelInfo monitorInfo;
    if (mapper->getRecordLevel(monitorInfo)) {

	float dBleft = Rosegarden::AudioLevel::fader_to_dB
	    (monitorInfo.level, 127, Rosegarden::AudioLevel::LongFader);
	float dBright = Rosegarden::AudioLevel::fader_to_dB
	    (monitorInfo.levelRight, 127, Rosegarden::AudioLevel::LongFader);

	m_monitor.m_meter->setLevel(dBleft, dBright);
    }

    Rosegarden::LevelInfo masterInfo;
    if (mapper->getMasterLevel(masterInfo)) {

	float dBleft = Rosegarden::AudioLevel::fader_to_dB
	    (masterInfo.level, 127, Rosegarden::AudioLevel::LongFader);
	float dBright = Rosegarden::AudioLevel::fader_to_dB
	    (masterInfo.levelRight, 127, Rosegarden::AudioLevel::LongFader);

	m_master.m_meter->setLevel(dBleft, dBright);
    }
}

