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
#include "audiopluginmanager.h"

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

// We define these such that the default of no-bits-set for the
// studio's mixer display options produces the most complete result
static const unsigned int MIXER_OMIT_FADERS            = 1 << 0;
static const unsigned int MIXER_OMIT_SUBMASTERS        = 1 << 1;
static const unsigned int MIXER_OMIT_PLUGINS           = 1 << 2;
static const unsigned int MIXER_OMIT_UNASSIGNED_FADERS = 1 << 3;


MixerWindow::MixerWindow(QWidget *parent,
			 RosegardenGUIDoc *document) :
    KMainWindow(parent, "mixerwindow"),
    m_document(document),
    m_studio(&document->getStudio()),
    m_currentId(0)
{
    m_mainBox = 0;
    populate();

    KStdAction::close(this,
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

    unsigned int mixerOptions = m_studio->getMixerDisplayOptions();

    (new KToggleAction(i18n("Show &Faders"), 0, this,
		       SLOT(slotShowFaders()), actionCollection(),
		       "show_audio_faders"))->setChecked
	(!(mixerOptions & MIXER_OMIT_FADERS));

    (new KToggleAction(i18n("Show &Submasters"), 0, this,
		       SLOT(slotShowSubmasters()), actionCollection(),
		       "show_audio_submasters"))->setChecked
	(!(mixerOptions & MIXER_OMIT_SUBMASTERS));

    (new KToggleAction(i18n("Show &Plugin Buttons"), 0, this,
		       SLOT(slotShowPluginButtons()), actionCollection(),
		       "show_plugin_buttons"))->setChecked
	(!(mixerOptions & MIXER_OMIT_PLUGINS));

    (new KToggleAction(i18n("Show &Unassigned Faders"), 0, this,
		       SLOT(slotShowUnassignedFaders()), actionCollection(),
		       "show_unassigned_faders"))->setChecked
	(!(mixerOptions & MIXER_OMIT_UNASSIGNED_FADERS));

    KRadioAction *action = 0;

    for (int i = 1; i <= 16; i *= 2) {
	action =
	    new KRadioAction
	    ((i == 1 ? i18n("%1 Input") : i18n("%1 Inputs")).arg(i),
	     0, this,
	     SLOT(slotSetInputCountFromAction()), actionCollection(),
	     QString("inputs_%1").arg(i));
	action->setExclusiveGroup("inputs");
	if (i == int(m_studio->getRecordIns().size())) action->setChecked(true);
    }

    action = new KRadioAction
	(i18n("No Submasters"),
	 0, this,
	 SLOT(slotSetSubmasterCountFromAction()), actionCollection(),
	 QString("submasters_0"));
    action->setExclusiveGroup("submasters");
    action->setChecked(true);
    
    for (int i = 2; i <= 8; i *= 2) {
	action = new KRadioAction
	    (i18n("%1 Submasters").arg(i),
	     0, this,
	     SLOT(slotSetSubmasterCountFromAction()), actionCollection(),
	     QString("submasters_%1").arg(i));
	action->setExclusiveGroup("submasters");
	if (i == int(m_studio->getBusses().size()) - 1) action->setChecked(true);
    }
    
    createGUI("mixer.rc");
}

MixerWindow::~MixerWindow()
{
    RG_DEBUG << "MixerWindow::~MixerWindow\n";
    depopulate();
}

void
MixerWindow::slotClose()
{
    RG_DEBUG << "MixerWindow::slotClose()\n";
    close();
}    

void
MixerWindow::depopulate()
{
    if (!m_mainBox) return;
    
    // All the widgets will be deleted when the main box goes,
    // except that we need to delete the AudioRouteMenus first
    // because they aren't actually widgets but do contain them
    
    for (FaderMap::iterator i = m_faders.begin();
	 i != m_faders.end(); ++i) {
	delete i->second.m_input;
	delete i->second.m_output;
    }
    
    m_faders.clear();
    m_submasters.clear();
    
    delete m_mainBox;
    m_mainBox = 0;
}    

void
MixerWindow::populate()
{
    if (m_mainBox) {

	depopulate();

    } else {

	m_surroundBox = new QHBox(this);
	setCentralWidget(m_surroundBox);
    }	

    QFont font;
    font.setPointSize(font.pointSize() * 8 / 10);
    font.setBold(false);
    setFont(font);

    QFont boldFont(font);
    boldFont.setBold(true);

    m_mainBox = new QFrame(m_surroundBox);

    Rosegarden::InstrumentList instruments = m_studio->getPresentationInstruments();
    Rosegarden::BussList busses = m_studio->getBusses();
    
    QString pixmapDir = KGlobal::dirs()->findResource("appdata", "pixmaps/");
    m_monoPixmap.load(QString("%1/misc/mono.xpm").arg(pixmapDir));
    m_stereoPixmap.load(QString("%1/misc/stereo.xpm").arg(pixmapDir));

    // Total cols: is 2 for each fader, submaster or master, plus 2
    // for the monitor strip, plus 1 for each spacer.
    QGridLayout *mainLayout = new QGridLayout
	(m_mainBox, (instruments.size() + busses.size() + 1) * 3, 7);

    setCaption(i18n("Mixer"));

    int count = 1;
    int col = 0;

    unsigned int mixerOptions = m_studio->getMixerDisplayOptions();

    bool showFaders     = (!(mixerOptions & MIXER_OMIT_FADERS));
    bool showSubmasters = (!(mixerOptions & MIXER_OMIT_SUBMASTERS));
    bool showPlugins    = (!(mixerOptions & MIXER_OMIT_PLUGINS));
    bool showUnassigned = (!(mixerOptions & MIXER_OMIT_UNASSIGNED_FADERS));

    for (Rosegarden::InstrumentList::iterator i = instruments.begin();
	 i != instruments.end(); ++i) {
	
	if (!showFaders) continue;

	if ((*i)->getType() != Rosegarden::Instrument::Audio) continue;

	FaderRec rec;

	if (!showUnassigned) {
	    // Do any tracks use this instrument?
	    if (!isInstrumentAssigned((*i)->getId())) {
		continue;
	    }
	}
	rec.m_populated = true;

	rec.m_input  = new AudioRouteMenu(m_mainBox,
					  AudioRouteMenu::In,
					  AudioRouteMenu::Compact,
					  m_studio, *i);
	rec.m_output = new AudioRouteMenu(m_mainBox,
					  AudioRouteMenu::Out,
					  AudioRouteMenu::Compact,
					  m_studio, *i);

	QToolTip::add(rec.m_input->getWidget(), i18n("Record input source"));
	QToolTip::add(rec.m_output->getWidget(), i18n("Output destination"));

	rec.m_input->getWidget()->setMaximumWidth(45);
	rec.m_output->getWidget()->setMaximumWidth(45);

	rec.m_pan = new RosegardenRotary
	    (m_mainBox, -100.0, 100.0, 1.0, 5.0, 0.0, 20);
	rec.m_pan->setKnobColour(RosegardenGUIColours::RotaryPastelGreen);

	QToolTip::add(rec.m_pan, i18n("Pan"));

	rec.m_fader = new RosegardenFader
	    (Rosegarden::AudioLevel::LongFader, 20, 240, m_mainBox);
	rec.m_meter = new AudioVUMeter
	    (m_mainBox, VUMeter::AudioPeakHoldLong, true, 20, 240);

	QToolTip::add(rec.m_fader, i18n("Audio level"));
	QToolTip::add(rec.m_meter, i18n("Audio level"));

	rec.m_stereoButton = new QPushButton(m_mainBox);
	rec.m_stereoButton->setPixmap(m_monoPixmap);
	rec.m_stereoButton->setFixedSize(20, 20);
	rec.m_stereoButton->setFlat(true);
	rec.m_stereoness = false;
	QToolTip::add(rec.m_stereoButton, i18n("Mono or stereo"));

	rec.m_muteButton = new QPushButton(m_mainBox);
	rec.m_muteButton->setText("M");
	rec.m_muteButton->setToggleButton(true);
	rec.m_muteButton->setFixedWidth(rec.m_stereoButton->width());
	rec.m_muteButton->setFixedHeight(rec.m_stereoButton->height());
	rec.m_muteButton->setFlat(true);
	QToolTip::add(rec.m_muteButton, i18n("Mute"));

	rec.m_soloButton = new QPushButton(m_mainBox);
	rec.m_soloButton->setText("S");
	rec.m_soloButton->setToggleButton(true);
	rec.m_soloButton->setFixedWidth(rec.m_stereoButton->width());
	rec.m_soloButton->setFixedHeight(rec.m_stereoButton->height());
	rec.m_soloButton->setFlat(true);
	QToolTip::add(rec.m_soloButton, i18n("Solo"));

	rec.m_recordButton = new QPushButton(m_mainBox);
	rec.m_recordButton->setText("R");
	rec.m_recordButton->setToggleButton(true);
	rec.m_recordButton->setFixedWidth(rec.m_stereoButton->width());
	rec.m_recordButton->setFixedHeight(rec.m_stereoButton->height());
	rec.m_recordButton->setFlat(true);
	QToolTip::add(rec.m_recordButton, i18n("Arm recording"));

	if (showPlugins) {
	    rec.m_pluginBox = new QVBox(m_mainBox);
	
	    for (int p = 0; p < 5; ++p) {
		QPushButton *plugin = new QPushButton(rec.m_pluginBox);
		plugin->setText(i18n("<none>"));
		plugin->setMaximumWidth(45);
		QToolTip::add(plugin, i18n("Audio plugin button"));
		rec.m_plugins.push_back(plugin);
		connect(plugin, SIGNAL(clicked()),
			this, SLOT(slotSelectPlugin()));
	    }
	}
	
	QLabel *idLabel = new QLabel(QString("%1").arg(count), m_mainBox);
	idLabel->setFont(boldFont);

	mainLayout->addMultiCellWidget(rec.m_input->getWidget(), 1, 1, col, col+1);
	mainLayout->addMultiCellWidget(rec.m_output->getWidget(), 2, 2, col, col+1);
//	mainLayout->addWidget(idLabel, 2, col, Qt::AlignCenter);
//	mainLayout->addWidget(rec.m_pan, 2, col+1, Qt::AlignLeft);

	mainLayout->addMultiCellWidget(idLabel, 0, 0, col, col+1, Qt::AlignCenter);
	mainLayout->addWidget(rec.m_pan, 5, col, Qt::AlignCenter);

	mainLayout->addWidget(rec.m_fader, 3, col, Qt::AlignCenter);
	mainLayout->addWidget(rec.m_meter, 3, col+1, Qt::AlignCenter);

// commented out until implemented
//	mainLayout->addWidget(rec.m_muteButton, 4, col);
//	mainLayout->addWidget(rec.m_soloButton, 4, col+1);
	rec.m_muteButton->hide();
	rec.m_soloButton->hide();

//	mainLayout->addWidget(rec.m_recordButton, 5, col);
//	mainLayout->addWidget(rec.m_stereoButton, 5, col+1);

	rec.m_recordButton->hide();
	mainLayout->addWidget(rec.m_stereoButton, 5, col+1);

	if (rec.m_pluginBox) {
	    mainLayout->addMultiCellWidget(rec.m_pluginBox, 6, 6, col, col+1);
	}

	m_faders[(*i)->getId()] = rec;
	updateFader((*i)->getId());
	updateRouteButtons((*i)->getId());
	updateStereoButton((*i)->getId());
	updatePluginButtons((*i)->getId());

	connect(rec.m_input, SIGNAL(changed()),
		this, SLOT(slotInputChanged()));

	connect(rec.m_output, SIGNAL(changed()),
		this, SLOT(slotOutputChanged()));

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

	if (!showSubmasters) continue;

	if (i == busses.begin()) continue; // that one's the master

	FaderRec rec;
	rec.m_populated = true;

	rec.m_pan = new RosegardenRotary
	    (m_mainBox, -100.0, 100.0, 1.0, 5.0, 0.0, 20);
	rec.m_pan->setKnobColour(RosegardenGUIColours::RotaryPastelBlue);

	QToolTip::add(rec.m_pan, i18n("Pan"));

	rec.m_fader = new RosegardenFader
	    (Rosegarden::AudioLevel::LongFader, 20, 240, m_mainBox);
	rec.m_meter = new AudioVUMeter
	    (m_mainBox, VUMeter::AudioPeakHoldLong, true, 20, 240);

	QToolTip::add(rec.m_fader, i18n("Audio level"));
	QToolTip::add(rec.m_meter, i18n("Audio level"));

	rec.m_muteButton = new QPushButton(m_mainBox);
	rec.m_muteButton->setText("M");
	rec.m_muteButton->setToggleButton(true);
	rec.m_muteButton->setFlat(true);

	QToolTip::add(rec.m_muteButton, i18n("Mute"));

	QLabel *idLabel = new QLabel(i18n("Sub %1").arg(count), m_mainBox);
	idLabel->setFont(boldFont);

//	mainLayout->addWidget(idLabel, 2, col, Qt::AlignCenter);
	mainLayout->addMultiCellWidget(idLabel, 0, 0, col, col+1, Qt::AlignCenter);

//	mainLayout->addWidget(rec.m_pan, 2, col+1, Qt::AlignLeft);
	mainLayout->addMultiCellWidget(rec.m_pan, 5, 5, col, col+1, Qt::AlignCenter);

	mainLayout->addWidget(rec.m_fader, 3, col, Qt::AlignCenter);
	mainLayout->addWidget(rec.m_meter, 3, col+1, Qt::AlignCenter);

//	mainLayout->addMultiCellWidget(rec.m_muteButton, 4, 4, col, col+1);
	rec.m_muteButton->hide();

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
	rec.m_populated = true;

	rec.m_fader = new RosegardenFader
	    (Rosegarden::AudioLevel::LongFader, 20, 240, m_mainBox);
	rec.m_meter = new AudioVUMeter
	    (m_mainBox, VUMeter::AudioPeakHoldLong, true, 20, 240);

	QToolTip::add(rec.m_fader, i18n("Audio record level"));
	QToolTip::add(rec.m_meter, i18n("Audio record level"));

	rec.m_muteButton = new QPushButton(m_mainBox);
	rec.m_muteButton->setText("M");
	rec.m_muteButton->setToggleButton(true);
	rec.m_muteButton->setFlat(true);

	QToolTip::add(rec.m_muteButton, i18n("Mute"));

	QLabel *idLabel = new QLabel(i18n("Rec"), m_mainBox);
	idLabel->setFont(boldFont);

	mainLayout->addMultiCellWidget(idLabel, 0, 0, col, col+1, Qt::AlignCenter);
	mainLayout->addWidget(rec.m_fader, 3, col, Qt::AlignCenter);
	mainLayout->addWidget(rec.m_meter, 3, col+1, Qt::AlignCenter);

//	mainLayout->addMultiCellWidget(rec.m_muteButton, 4, 4, col, col+1);
	rec.m_muteButton->hide();

	m_monitor = rec;
	updateFader(-1);

	connect(rec.m_fader, SIGNAL(faderChanged(float)),
		this, SLOT(slotFaderLevelChanged(float)));

	connect(rec.m_muteButton, SIGNAL(clicked()),
		this, SLOT(slotMuteChanged()));

	mainLayout->addMultiCell(new QSpacerItem(2, 0), 0, 6, col+2, col+2);

	col += 3;

	rec.m_fader = new RosegardenFader
	    (Rosegarden::AudioLevel::LongFader, 20, 240, m_mainBox);
	rec.m_meter = new AudioVUMeter
	    (m_mainBox, VUMeter::AudioPeakHoldLong, true, 20, 240);

	QToolTip::add(rec.m_fader, i18n("Audio master output level"));
	QToolTip::add(rec.m_meter, i18n("Audio master output level"));

	rec.m_muteButton = new QPushButton(m_mainBox);
	rec.m_muteButton->setText("M");
	rec.m_muteButton->setToggleButton(true);
	rec.m_muteButton->setFlat(true);

	idLabel = new QLabel(i18n("Master"), m_mainBox);
	idLabel->setFont(boldFont);

	mainLayout->addMultiCellWidget(idLabel, 0, 0, col, col+1, Qt::AlignCenter);
	mainLayout->addWidget(rec.m_fader, 3, col, Qt::AlignCenter);
	mainLayout->addWidget(rec.m_meter, 3, col+1, Qt::AlignCenter);

//	mainLayout->addMultiCellWidget(rec.m_muteButton, 4, 4, col, col+1);
	rec.m_muteButton->hide();
	
	mainLayout->addMultiCell(new QSpacerItem(2, 0), 0, 6, col+2, col+2);

	m_master = rec;
	updateFader(0);

	connect(rec.m_fader, SIGNAL(faderChanged(float)),
		this, SLOT(slotFaderLevelChanged(float)));

	connect(rec.m_muteButton, SIGNAL(clicked()),
		this, SLOT(slotMuteChanged()));
    }

    adjustSize();
    m_mainBox->show();
}

bool
MixerWindow::isInstrumentAssigned(Rosegarden::InstrumentId id)
{
    Rosegarden::Composition::trackcontainer &tracks =
	m_document->getComposition().getTracks();

    for (Rosegarden::Composition::trackcontainer::iterator ti =
	     tracks.begin(); ti != tracks.end(); ++ti) {
	if (ti->second->getInstrument() == id) {
	    return true;
	}
    }

    return false;
}    

void
MixerWindow::slotTrackAssignmentsChanged()
{
    for (FaderMap::iterator i = m_faders.begin(); i != m_faders.end(); ++i) {

	Rosegarden::InstrumentId id = i->first;
	bool assigned = isInstrumentAssigned(id);

	if (assigned != i->second.m_populated) {
	    // found an inconsistency
	    populate();
	    return;
	}
    }
}

void
MixerWindow::slotUpdateInstrument(Rosegarden::InstrumentId id)
{
    RG_DEBUG << "MixerWindow::slotUpdateInstrument(" << id << ")" << endl;

    blockSignals(true);

    updateFader(id);
    updateStereoButton(id);
    updateRouteButtons(id);
    updatePluginButtons(id);
    updateMiscButtons(id);

    Rosegarden::Composition &comp = m_document->getComposition();
    Rosegarden::TrackId recordTrackId = comp.getRecordTrack();
    Rosegarden::Track *recordTrack = comp.getTrackById(recordTrackId);
    if (recordTrack && recordTrack->getInstrument() == id) {
	updateFader(-1); // sekrit code for monitor fader
    }

    blockSignals(false);
}

void
MixerWindow::slotPluginSelected(Rosegarden::InstrumentId id,
				int index, int plugin)
{
    if (id >= (int)Rosegarden::AudioInstrumentBase) {

	FaderRec &rec = m_faders[id];
	if (!rec.m_populated || !rec.m_pluginBox) return;

	if (plugin == -1) {

	    rec.m_plugins[index]->setText(i18n("<none>"));

	    rec.m_plugins[index]->setPaletteBackgroundColor
		(kapp->palette().
		 color(QPalette::Active, QColorGroup::Button));

	} else {

	    Rosegarden::AudioPlugin *pluginClass 
		= m_document->getPluginManager()->getPlugin(plugin);

	    if (pluginClass)
		rec.m_plugins[index]->
		    setText(pluginClass->getLabel());

	    rec.m_plugins[index]->setPaletteBackgroundColor
		(kapp->palette().
		 color(QPalette::Active, QColorGroup::Light));
	}
    }
}

void
MixerWindow::slotPluginBypassed(Rosegarden::InstrumentId instrumentId,
				int , bool )
{
    updatePluginButtons(instrumentId);
}

void
MixerWindow::updateFader(int id)
{
    if (id == -1) {

	Rosegarden::Composition &comp = m_document->getComposition();
	Rosegarden::TrackId recordTrackId = comp.getRecordTrack();
	Rosegarden::Track *recordTrack = comp.getTrackById(recordTrackId);
	if (recordTrack) {
	    Rosegarden::InstrumentId instrumentId = recordTrack->getInstrument();
	    Rosegarden::Instrument *instrument = m_studio->getInstrumentById
		(instrumentId);
	    m_monitor.m_fader->setFader(instrument->getRecordLevel());
	}
	
    } else if (id >= (int)Rosegarden::AudioInstrumentBase) {

	FaderRec &rec = m_faders[id];
	if (!rec.m_populated) return;
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
	if (!rec.m_populated) return;
	rec.m_input->slotRepopulate();
	rec.m_output->slotRepopulate();
    }
}


void
MixerWindow::updateStereoButton(int id)
{
    if (id >= (int)Rosegarden::AudioInstrumentBase) {

	FaderRec &rec = m_faders[id];
	if (!rec.m_populated) return;
	Rosegarden::Instrument *i = m_studio->getInstrumentById(id);

	bool stereo = (i->getAudioChannels() > 1);
	if (stereo == rec.m_stereoness) return;

	rec.m_stereoness = stereo;

	if (stereo) rec.m_stereoButton->setPixmap(m_stereoPixmap);
	else rec.m_stereoButton->setPixmap(m_monoPixmap);
    }
}


void
MixerWindow::updateMiscButtons(int )
{
    //... complications here, because the mute/solo status is actually
    // per-track rather than per-instrument... doh.
}


void
MixerWindow::updatePluginButtons(int id)
{
    if (id >= (int)Rosegarden::AudioInstrumentBase) {

	FaderRec &rec = m_faders[id];
	if (!rec.m_populated || !rec.m_pluginBox) return;
	Rosegarden::Instrument *instrument = m_studio->getInstrumentById(id);

	for (unsigned int i = 0; i < rec.m_plugins.size(); i++) {

	    bool used = false;
	    bool bypass = false;

	    rec.m_plugins[i]->show();

	    Rosegarden::AudioPluginInstance *inst = instrument->getPlugin(i);

	    if (inst && inst->isAssigned()) {

		Rosegarden::AudioPlugin *pluginClass 
		    = m_document->getPluginManager()->getPlugin(
                        m_document->getPluginManager()->
			    getPositionByUniqueId(inst->getId()));

		if (pluginClass)
		    rec.m_plugins[i]->
			setText(pluginClass->getLabel());
		
		used = true;
		bypass = inst->isBypassed();

	    } else {

		rec.m_plugins[i]->setText(i18n("<none>"));

		if (inst) bypass = inst->isBypassed();
	    }

	    if (bypass) {
		
		rec.m_plugins[i]->setPaletteForegroundColor
		    (kapp->palette().
		     color(QPalette::Active, QColorGroup::Button));

		rec.m_plugins[i]->setPaletteBackgroundColor
		    (kapp->palette().
		     color(QPalette::Active, QColorGroup::ButtonText));

	    } else if (used) {

		rec.m_plugins[i]->setPaletteForegroundColor
		    (kapp->palette().
		     color(QPalette::Active, QColorGroup::ButtonText));

		rec.m_plugins[i]->setPaletteBackgroundColor
		    (kapp->palette().
		     color(QPalette::Active, QColorGroup::Light));

	    } else {

		rec.m_plugins[i]->setPaletteForegroundColor
		    (kapp->palette().
		     color(QPalette::Active, QColorGroup::ButtonText));

		rec.m_plugins[i]->setPaletteBackgroundColor
		    (kapp->palette().
		     color(QPalette::Active, QColorGroup::Button));
	    }
	}
    }	
}

void
MixerWindow::slotSelectPlugin()
{
    const QObject *s = sender();

    // no plugins anywhere except instruments yet

    for (FaderMap::iterator i = m_faders.begin();
	 i != m_faders.end(); ++i) {
	
	int index = 0;
	if (!i->second.m_populated || !i->second.m_pluginBox) continue;

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
MixerWindow::slotInputChanged()
{
    const QObject *s = sender();

    for (FaderMap::iterator i = m_faders.begin();
	 i != m_faders.end(); ++i) {

	if (i->second.m_input == s) emit instrumentParametersChanged(i->first);
    }
}

void
MixerWindow::slotOutputChanged()
{
    const QObject *s = sender();

    for (FaderMap::iterator i = m_faders.begin();
	 i != m_faders.end(); ++i) {

	if (i->second.m_output == s) emit instrumentParametersChanged(i->first);
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

	    if (instrument) {
		Rosegarden::StudioControl::setStudioObjectProperty
		    (Rosegarden::MappedObjectId
		     (instrument->getMappedId()),
		     Rosegarden::MappedAudioFader::FaderLevel,
		     Rosegarden::MappedObjectValue(dB));
		instrument->setLevel(dB);
	    }

	    emit instrumentParametersChanged(i->first);
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
	    
	    if (instrument) {
		Rosegarden::StudioControl::setStudioObjectProperty
		    (instrument->getMappedId(),
		     Rosegarden::MappedAudioFader::Pan,
		     Rosegarden::MappedObjectValue(pan));
		instrument->setPan(Rosegarden::MidiByte(pan + 100.0));
	    }

	    emit instrumentParametersChanged(i->first);
	}
    }
}

void
MixerWindow::slotChannelsChanged()
{
    const QObject *s = sender();

    // channels are only switchable on instruments

    //!!! need to reconnect input, or change input channel number anyway


    for (FaderMap::iterator i = m_faders.begin();
	 i != m_faders.end(); ++i) {

	if (s == i->second.m_stereoButton) {
	    
	    Rosegarden::Instrument *instrument =
		m_studio->getInstrumentById(i->first);
	    
	    if (instrument) {
		instrument->setAudioChannels
		    ((instrument->getAudioChannels() > 1) ? 1 : 2);
		updateStereoButton(instrument->getId());
		updateRouteButtons(instrument->getId());

		emit instrumentParametersChanged(i->first);

		return;
	    }
	}
    }
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
    RG_DEBUG << "MixerWindow::closeEvent()\n";
    emit closing();
    KMainWindow::closeEvent(e);
}

void
MixerWindow::updateMeters(SequencerMapper *mapper)
{
    for (FaderMap::iterator i = m_faders.begin(); i != m_faders.end(); ++i) {

	Rosegarden::InstrumentId id = i->first;
	FaderRec &rec = i->second;
	if (!rec.m_populated) continue;

	Rosegarden::LevelInfo info;
	if (!mapper->getInstrumentLevel(id, info)) continue;

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

void
MixerWindow::slotSetInputCountFromAction()
{
    const QObject *s = sender();
    QString name = s->name();

    if (name.left(7) == "inputs_") {

	int count = name.right(name.length() - 7).toInt();

	Rosegarden::RecordInList ins = m_studio->getRecordIns();
	int current = ins.size();

	if (count == current) return;

	m_studio->clearRecordIns(); // leaves the default 1

	for (int i = 1; i < count; ++i) {
	    m_studio->addRecordIn(new Rosegarden::RecordIn());
	}
    }
    
    m_document->initialiseStudio();
    
    for (FaderMap::iterator i = m_faders.begin();
	 i != m_faders.end(); ++i) {
	updateRouteButtons(i->first);
    }
}

void
MixerWindow::slotSetSubmasterCountFromAction()
{
    const QObject *s = sender();
    QString name = s->name();

    if (name.left(11) == "submasters_") {

	int count = name.right(name.length() - 11).toInt();

	Rosegarden::BussList busses = m_studio->getBusses();
	int current = busses.size();

	// offset by 1 generally to take into account the fact that
	// the first buss in the studio is the master, not a submaster

	if (count + 1 == current) return;

	Rosegarden::BussList dups;
	for (int i = 0; i < count; ++i) {
	    if (i + 1 < int(busses.size())) {
		dups.push_back(new Rosegarden::Buss(*busses[i+1]));
	    } else {
		dups.push_back(new Rosegarden::Buss(i + 1));
	    }
	}

	m_studio->clearBusses();

	for (Rosegarden::BussList::iterator i = dups.begin();
	     i != dups.end(); ++i) {
	    m_studio->addBuss(*i);
	}
    }

    m_document->initialiseStudio();

    populate();
}

void
MixerWindow::slotShowFaders()
{
    KToggleAction *action = dynamic_cast<KToggleAction *>
        (actionCollection()->action("show_audio_faders"));
    if (!action) return;

    m_studio->setMixerDisplayOptions(m_studio->getMixerDisplayOptions() ^
				     MIXER_OMIT_FADERS);
				     
    action->setChecked(!(m_studio->getMixerDisplayOptions() &
			 MIXER_OMIT_FADERS));

    populate();
}

void
MixerWindow::slotShowSubmasters()
{

    KToggleAction *action = dynamic_cast<KToggleAction *>
        (actionCollection()->action("show_audio_submasters"));
    if (!action) return;

    m_studio->setMixerDisplayOptions(m_studio->getMixerDisplayOptions() ^
				     MIXER_OMIT_SUBMASTERS);
				     
    action->setChecked(!(m_studio->getMixerDisplayOptions() &
			 MIXER_OMIT_SUBMASTERS));

    populate();
}

void
MixerWindow::slotShowPluginButtons()
{

    KToggleAction *action = dynamic_cast<KToggleAction *>
        (actionCollection()->action("show_plugin_buttons"));
    if (!action) return;

    m_studio->setMixerDisplayOptions(m_studio->getMixerDisplayOptions() ^
				     MIXER_OMIT_PLUGINS);
				     
    action->setChecked(!(m_studio->getMixerDisplayOptions() &
			 MIXER_OMIT_PLUGINS));

    populate();
}

void
MixerWindow::slotShowUnassignedFaders()
{
    KToggleAction *action = dynamic_cast<KToggleAction *>
        (actionCollection()->action("show_unassigned_faders"));
    if (!action) return;

    m_studio->setMixerDisplayOptions(m_studio->getMixerDisplayOptions() ^
				     MIXER_OMIT_UNASSIGNED_FADERS);
				     
    action->setChecked(!(m_studio->getMixerDisplayOptions() &
			 MIXER_OMIT_UNASSIGNED_FADERS));

    populate();
}
