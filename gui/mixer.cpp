// -*- c-basic-offset: 4 -*-

/*
    Rosegarden-4
    A sequencer and musical notation editor.

    This program is Copyright 2000-2005
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
#include "sequencemanager.h"
#include "rosestrings.h"

#include "Studio.h"
#include "AudioLevel.h"
#include "Midi.h"
#include "Instrument.h"

#include <klocale.h>
#include <kconfig.h>
#include <kstdaction.h>
#include <kaction.h>
#include <kglobal.h>
#include <kstddirs.h>

#include <iostream>

#include <qlayout.h>
#include <qpopupmenu.h>
#include <qaccel.h>

// We define these such that the default of no-bits-set for the
// studio's mixer display options produces the most sensible result
static const unsigned int MIXER_OMIT_FADERS            = 1 << 0;
static const unsigned int MIXER_OMIT_SUBMASTERS        = 1 << 1;
static const unsigned int MIXER_OMIT_PLUGINS           = 1 << 2;
static const unsigned int MIXER_SHOW_UNASSIGNED_FADERS = 1 << 3;
static const unsigned int MIXER_OMIT_SYNTH_FADERS      = 1 << 4;


MixerWindow::MixerWindow(QWidget *parent,
			      RosegardenGUIDoc *document) :
    KMainWindow(parent, "mixerwindow"),
    m_document(document),
    m_studio(&document->getStudio()),
    m_currentId(0)
{
    m_accelerators = new QAccel(this);
}

void
MixerWindow::closeEvent(QCloseEvent *e)
{
    RG_DEBUG << "MixerWindow::closeEvent()\n";
    emit closing();
    KMainWindow::closeEvent(e);
}

void
MixerWindow::slotClose()
{
    RG_DEBUG << "MixerWindow::slotClose()\n";
    close();
}    

AudioMixerWindow::AudioMixerWindow(QWidget *parent,
			 RosegardenGUIDoc *document):
        MixerWindow(parent, document)
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

    icon = QIconSet(NotePixmapFactory::toQPixmap(NotePixmapFactory::makeToolbarPixmap
                                                 ("transport-record")));
    new KAction(i18n("&Record"), icon, 0, this,
		SIGNAL(record()), actionCollection(),
		"record");

    unsigned int mixerOptions = m_studio->getMixerDisplayOptions();

    (new KToggleAction(i18n("Show Audio &Faders"), 0, this,
		       SLOT(slotToggleFaders()), actionCollection(),
		       "show_audio_faders"))->setChecked
	(!(mixerOptions & MIXER_OMIT_FADERS));

    (new KToggleAction(i18n("Show Synth &Faders"), 0, this,
		       SLOT(slotToggleSynthFaders()), actionCollection(),
		       "show_synth_faders"))->setChecked
	(!(mixerOptions & MIXER_OMIT_SYNTH_FADERS));

    (new KToggleAction(i18n("Show &Submasters"), 0, this,
		       SLOT(slotToggleSubmasters()), actionCollection(),
		       "show_audio_submasters"))->setChecked
	(!(mixerOptions & MIXER_OMIT_SUBMASTERS));

    (new KToggleAction(i18n("Show &Plugin Buttons"), 0, this,
		       SLOT(slotTogglePluginButtons()), actionCollection(),
		       "show_plugin_buttons"))->setChecked
	(!(mixerOptions & MIXER_OMIT_PLUGINS));

    (new KToggleAction(i18n("Show &Unassigned Faders"), 0, this,
		       SLOT(slotToggleUnassignedFaders()), actionCollection(),
		       "show_unassigned_faders"))->setChecked
	(mixerOptions & MIXER_SHOW_UNASSIGNED_FADERS);

    KRadioAction *action = 0;

    for (int i = 1; i <= 16; i *= 2) {
	action =
	    new KRadioAction(i18n("1 Input", "%n Inputs", i),
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

AudioMixerWindow::~AudioMixerWindow()
{
    RG_DEBUG << "AudioMixerWindow::~AudioMixerWindow\n";
    depopulate();
}

void
AudioMixerWindow::depopulate()
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
AudioMixerWindow::populate()
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

    // Total cols: is 2 for each fader, submaster or master, plus 1
    // for each spacer.
    QGridLayout *mainLayout = new QGridLayout
	(m_mainBox, (instruments.size() + busses.size()) * 3, 7);

    setCaption(i18n("Audio Mixer"));

    int count = 1;
    int col = 0;

    unsigned int mixerOptions = m_studio->getMixerDisplayOptions();

    bool showUnassigned = (mixerOptions & MIXER_SHOW_UNASSIGNED_FADERS);

    for (Rosegarden::InstrumentList::iterator i = instruments.begin();
	 i != instruments.end(); ++i) {
	
	if ((*i)->getType() != Rosegarden::Instrument::Audio &&
	    (*i)->getType() != Rosegarden::Instrument::SoftSynth) continue;

	FaderRec rec;

	if (!showUnassigned) {
	    // Do any tracks use this instrument?
	    if (!isInstrumentAssigned((*i)->getId())) {
		continue;
	    }
	}
	rec.m_populated = true;

	if ((*i)->getType() == Rosegarden::Instrument::Audio) {
	    rec.m_input  = new AudioRouteMenu(m_mainBox,
					      AudioRouteMenu::In,
					      AudioRouteMenu::Compact,
					      m_studio, *i);
	    QToolTip::add(rec.m_input->getWidget(), i18n("Record input source"));
	    rec.m_input->getWidget()->setMaximumWidth(45);
	} else {
	    rec.m_input = 0;
	}

	rec.m_output = new AudioRouteMenu(m_mainBox,
					  AudioRouteMenu::Out,
					  AudioRouteMenu::Compact,
					  m_studio, *i);
	QToolTip::add(rec.m_output->getWidget(), i18n("Output destination"));
	rec.m_output->getWidget()->setMaximumWidth(45);

	rec.m_pan = new RosegardenRotary
	    (m_mainBox, -100.0, 100.0, 1.0, 5.0, 0.0, 20,
	     RosegardenRotary::NoTicks, false, true);

	if ((*i)->getType() == Rosegarden::Instrument::Audio) {
	    rec.m_pan->setKnobColour(Rosegarden::GUIPalette::getColour(Rosegarden::GUIPalette::RotaryPastelGreen));
	} else {
	    rec.m_pan->setKnobColour(Rosegarden::GUIPalette::getColour(Rosegarden::GUIPalette::RotaryPastelYellow));
	}

	QToolTip::add(rec.m_pan, i18n("Pan"));

	rec.m_fader = new RosegardenFader
	    (Rosegarden::AudioLevel::LongFader, 20, 240, m_mainBox);
	rec.m_meter = new AudioVUMeter
	    (m_mainBox, VUMeter::AudioPeakHoldIECLong, true, rec.m_input != 0,
	     20, 240);

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

        rec.m_pluginBox = new QVBox(m_mainBox);
	
        for (int p = 0; p < 5; ++p) {
            QPushButton *plugin = new QPushButton(rec.m_pluginBox, "pluginButton");
            plugin->setText(i18n("<none>"));
            plugin->setMaximumWidth(45);
            QToolTip::add(plugin, i18n("Audio plugin button"));
            rec.m_plugins.push_back(plugin);
            connect(plugin, SIGNAL(clicked()),
                    this, SLOT(slotSelectPlugin()));
        }

	QLabel *idLabel;
	QString idString;
	if ((*i)->getType() == Rosegarden::Instrument::Audio) {
	    idString = i18n("Audio %1").arg((*i)->getId() -
					    Rosegarden::AudioInstrumentBase + 1);
	    idLabel = new QLabel(idString, m_mainBox, "audioIdLabel");
	} else {
	    idString = i18n("Synth %1").arg((*i)->getId() - 
					    Rosegarden::SoftSynthInstrumentBase + 1);
	    idLabel = new QLabel(idString, m_mainBox, "synthIdLabel");
	}
	idLabel->setFont(boldFont);

	if (rec.m_input) {
	    mainLayout->addMultiCellWidget(rec.m_input->getWidget(), 1, 1, col, col+1);
	}
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

	if (rec.m_input) {
	    connect(rec.m_input, SIGNAL(changed()),
		    this, SLOT(slotInputChanged()));
	}

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

	if (i == busses.begin()) continue; // that one's the master

	FaderRec rec;
	rec.m_populated = true;

	rec.m_pan = new RosegardenRotary
	    (m_mainBox, -100.0, 100.0, 1.0, 5.0, 0.0, 20,
	     RosegardenRotary::NoTicks, false, true);
	rec.m_pan->setKnobColour(Rosegarden::GUIPalette::getColour(Rosegarden::GUIPalette::RotaryPastelBlue));

	QToolTip::add(rec.m_pan, i18n("Pan"));

	rec.m_fader = new RosegardenFader
	    (Rosegarden::AudioLevel::LongFader, 20, 240, m_mainBox);
	rec.m_meter = new AudioVUMeter
	    (m_mainBox, VUMeter::AudioPeakHoldIECLong, true, false, 20, 240);

	QToolTip::add(rec.m_fader, i18n("Audio level"));
	QToolTip::add(rec.m_meter, i18n("Audio level"));

	rec.m_muteButton = new QPushButton(m_mainBox);
	rec.m_muteButton->setText("M");
	rec.m_muteButton->setToggleButton(true);
	rec.m_muteButton->setFlat(true);

	QToolTip::add(rec.m_muteButton, i18n("Mute"));

        rec.m_pluginBox = new QVBox(m_mainBox);
	
        for (int p = 0; p < 5; ++p) {
            QPushButton *plugin = new QPushButton(rec.m_pluginBox, "pluginButton");
            plugin->setText(i18n("<none>"));
            plugin->setMaximumWidth(45);
            QToolTip::add(plugin, i18n("Audio plugin button"));
            rec.m_plugins.push_back(plugin);
            connect(plugin, SIGNAL(clicked()),
                    this, SLOT(slotSelectPlugin()));
        }

	QLabel *idLabel = new QLabel(i18n("Sub %1").arg(count), m_mainBox, "subMaster");
	idLabel->setFont(boldFont);

//	mainLayout->addWidget(idLabel, 2, col, Qt::AlignCenter);
	mainLayout->addMultiCellWidget(idLabel, 0, 0, col, col+1, Qt::AlignCenter);

//	mainLayout->addWidget(rec.m_pan, 2, col+1, Qt::AlignLeft);
	mainLayout->addMultiCellWidget(rec.m_pan, 5, 5, col, col+1, Qt::AlignCenter);

	mainLayout->addWidget(rec.m_fader, 3, col, Qt::AlignCenter);
	mainLayout->addWidget(rec.m_meter, 3, col+1, Qt::AlignCenter);

//	mainLayout->addMultiCellWidget(rec.m_muteButton, 4, 4, col, col+1);
	rec.m_muteButton->hide();

	if (rec.m_pluginBox) {
	    mainLayout->addMultiCellWidget(rec.m_pluginBox, 6, 6, col, col+1);
	}

	m_submasters.push_back(rec);
	updateFader(count);
	updatePluginButtons(count);

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
	    (m_mainBox, VUMeter::AudioPeakHoldIEC, true, false, 20, 240);

	QToolTip::add(rec.m_fader, i18n("Audio master output level"));
	QToolTip::add(rec.m_meter, i18n("Audio master output level"));

	rec.m_muteButton = new QPushButton(m_mainBox);
	rec.m_muteButton->setText("M");
	rec.m_muteButton->setToggleButton(true);
	rec.m_muteButton->setFlat(true);
	
	QLabel *idLabel = new QLabel(i18n("Master"), m_mainBox);
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

    m_mainBox->show();

    slotUpdateFaderVisibility();
    slotUpdateSynthFaderVisibility();
    slotUpdateSubmasterVisibility();
    slotUpdatePluginButtonVisibility();

    adjustSize();
}

bool
AudioMixerWindow::isInstrumentAssigned(Rosegarden::InstrumentId id)
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
AudioMixerWindow::slotTrackAssignmentsChanged()
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
AudioMixerWindow::slotUpdateInstrument(Rosegarden::InstrumentId id)
{
    RG_DEBUG << "AudioMixerWindow::slotUpdateInstrument(" << id << ")" << endl;

    blockSignals(true);

    updateFader(id);
    updateStereoButton(id);
    updateRouteButtons(id);
    updatePluginButtons(id);
    updateMiscButtons(id);

    blockSignals(false);
}

void
AudioMixerWindow::slotPluginSelected(Rosegarden::InstrumentId id,
				     int index, int plugin)
{
    if (id >= (int)Rosegarden::AudioInstrumentBase) {

	FaderRec &rec = m_faders[id];
	if (!rec.m_populated || !rec.m_pluginBox) return;

	// nowhere to display synth plugin info yet
	if (index >= rec.m_plugins.size()) return;

	if (plugin == -1) {

	    rec.m_plugins[index]->setText(i18n("<none>"));
            QToolTip::add(rec.m_plugins[index], i18n("<no plugin>"));

	    rec.m_plugins[index]->setPaletteBackgroundColor
		(kapp->palette().
		 color(QPalette::Active, QColorGroup::Button));

	} else {

	    Rosegarden::AudioPlugin *pluginClass 
		= m_document->getPluginManager()->getPlugin(plugin);

            QColor pluginBgColour =
                kapp->palette().color(QPalette::Active, QColorGroup::Light);

	    if (pluginClass)
            {
		rec.m_plugins[index]->
		    setText(pluginClass->getLabel());
                QToolTip::add(rec.m_plugins[index], pluginClass->getLabel());

                pluginBgColour = pluginClass->getColour();
            }


            rec.m_plugins[index]->setPaletteForegroundColor(Qt::white);
	    rec.m_plugins[index]->setPaletteBackgroundColor(pluginBgColour);
	}
    } else if (id > 0 && id <= m_submasters.size()) {

	FaderRec &rec = m_submasters[id-1];
	if (!rec.m_populated || !rec.m_pluginBox) return;
	if (index >= rec.m_plugins.size()) return;

	if (plugin == -1) {

	    rec.m_plugins[index]->setText(i18n("<none>"));
            QToolTip::add(rec.m_plugins[index], i18n("<no plugin>"));

	    rec.m_plugins[index]->setPaletteBackgroundColor
		(kapp->palette().
		 color(QPalette::Active, QColorGroup::Button));

	} else {

	    Rosegarden::AudioPlugin *pluginClass 
		= m_document->getPluginManager()->getPlugin(plugin);

            QColor pluginBgColour =
                kapp->palette().color(QPalette::Active, QColorGroup::Light);

	    if (pluginClass)
            {
		rec.m_plugins[index]->
		    setText(pluginClass->getLabel());
                QToolTip::add(rec.m_plugins[index], pluginClass->getLabel());

                pluginBgColour = pluginClass->getColour();
            }


            rec.m_plugins[index]->setPaletteForegroundColor(Qt::white);
	    rec.m_plugins[index]->setPaletteBackgroundColor(pluginBgColour);
	}
    }
}

void
AudioMixerWindow::slotPluginBypassed(Rosegarden::InstrumentId instrumentId,
				     int , bool )
{
    RG_DEBUG << "AudioMixerWindow::slotPluginBypassed(" << instrumentId << ")" << endl;

    updatePluginButtons(instrumentId);
}

void
AudioMixerWindow::updateFader(int id)
{
    if (id == -1) {

	// This used to be the special code for updating the monitor fader.
	return;

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
AudioMixerWindow::updateRouteButtons(int id)
{
    if (id >= (int)Rosegarden::AudioInstrumentBase) {
	FaderRec &rec = m_faders[id];
	if (!rec.m_populated) return;
	if (rec.m_input) rec.m_input->slotRepopulate();
	rec.m_output->slotRepopulate();
    }
}


void
AudioMixerWindow::updateStereoButton(int id)
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
AudioMixerWindow::updateMiscButtons(int )
{
    //... complications here, because the mute/solo status is actually
    // per-track rather than per-instrument... doh.
}


void
AudioMixerWindow::updatePluginButtons(int id)
{
    FaderRec *rec = 0;
    Rosegarden::PluginContainer *container = 0;

    if (id >= (int)Rosegarden::AudioInstrumentBase) {

	container = m_studio->getInstrumentById(id);
	rec = &m_faders[id];
	if (!rec->m_populated || !rec->m_pluginBox) return;

    } else {

	Rosegarden::BussList busses = m_studio->getBusses();
	if (busses.size() > id) {
	    container = busses[id];
	}
	rec = &m_submasters[id-1];
	if (!rec->m_populated || !rec->m_pluginBox) return;
    }

    if (rec && container) {

	for (unsigned int i = 0; i < rec->m_plugins.size(); i++) {

	    bool used = false;
	    bool bypass = false;
            QColor pluginBgColour = 
                kapp->palette().color(QPalette::Active, QColorGroup::Light);

	    rec->m_plugins[i]->show();

	    Rosegarden::AudioPluginInstance *inst = container->getPlugin(i);

	    if (inst && inst->isAssigned()) {

		Rosegarden::AudioPlugin *pluginClass 
		    = m_document->getPluginManager()->getPlugin(
                        m_document->getPluginManager()->
			    getPositionByIdentifier(inst->getIdentifier().c_str()));

		if (pluginClass)
                {
		    rec->m_plugins[i]->setText(pluginClass->getLabel());
                    QToolTip::add(rec->m_plugins[i], pluginClass->getLabel());

                    pluginBgColour = pluginClass->getColour();
                }
		
		used = true;
		bypass = inst->isBypassed();

	    } else {

		rec->m_plugins[i]->setText(i18n("<none>"));
                QToolTip::add(rec->m_plugins[i], i18n("<no plugin>"));

		if (inst) bypass = inst->isBypassed();
	    }

	    if (bypass) {
		
		rec->m_plugins[i]->setPaletteForegroundColor
		    (kapp->palette().
		     color(QPalette::Active, QColorGroup::Button));

		rec->m_plugins[i]->setPaletteBackgroundColor
		    (kapp->palette().
		     color(QPalette::Active, QColorGroup::ButtonText));

	    } else if (used) {

		rec->m_plugins[i]->setPaletteForegroundColor(Qt::white);
		rec->m_plugins[i]->setPaletteBackgroundColor(pluginBgColour);


	    } else {

		rec->m_plugins[i]->setPaletteForegroundColor
		    (kapp->palette().
		     color(QPalette::Active, QColorGroup::ButtonText));

		rec->m_plugins[i]->setPaletteBackgroundColor
		    (kapp->palette().
		     color(QPalette::Active, QColorGroup::Button));
	    }
	}
    }	
}

void
AudioMixerWindow::slotSelectPlugin()
{
    const QObject *s = sender();

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


    int b = 1;

    for (FaderVector::iterator i = m_submasters.begin();
	 i != m_submasters.end(); ++i) {
	
	int index = 0;
	if (!i->m_populated || !i->m_pluginBox) continue;

	for (std::vector<QPushButton *>::iterator pli = i->m_plugins.begin();
	     pli != i->m_plugins.end(); ++pli) {

	    if (*pli == s) {

		emit selectPlugin(this, b, index);
		return;
	    }

	    ++index;
	}
	
	++b;
    }
}

void
AudioMixerWindow::slotInputChanged()
{
    const QObject *s = sender();

    for (FaderMap::iterator i = m_faders.begin();
	 i != m_faders.end(); ++i) {

	if (i->second.m_input == s) emit instrumentParametersChanged(i->first);
    }
}

void
AudioMixerWindow::slotOutputChanged()
{
    const QObject *s = sender();

    for (FaderMap::iterator i = m_faders.begin();
	 i != m_faders.end(); ++i) {

	if (i->second.m_output == s) emit instrumentParametersChanged(i->first);
    }
}

void
AudioMixerWindow::slotFaderLevelChanged(float dB)
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
AudioMixerWindow::slotPanChanged(float pan)
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
AudioMixerWindow::slotChannelsChanged()
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
AudioMixerWindow::slotSoloChanged()
{
    //...
}

void
AudioMixerWindow::slotMuteChanged()
{
    //...
}

void
AudioMixerWindow::slotRecordChanged()
{
    //...
}

void
AudioMixerWindow::updateMeters(SequencerMapper *mapper)
{
    for (FaderMap::iterator i = m_faders.begin(); i != m_faders.end(); ++i) {

	Rosegarden::InstrumentId id = i->first;
	FaderRec &rec = i->second;
	if (!rec.m_populated) continue;

	Rosegarden::LevelInfo info;

	if (mapper->getInstrumentLevelForMixer(id, info)) {

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

    updateMonitorMeters(mapper);

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
AudioMixerWindow::updateMonitorMeters(SequencerMapper *mapper)
{
    // only show monitor levels when quiescent or when recording (as
    // record levels)
    if (m_document->getSequenceManager() &&
	m_document->getSequenceManager()->getTransportStatus() == PLAYING) {
	return;
    }

    Rosegarden::Composition &comp = m_document->getComposition();
    Rosegarden::Composition::trackcontainer &tracks = comp.getTracks();

    for (FaderMap::iterator i = m_faders.begin(); i != m_faders.end(); ++i) {

	Rosegarden::InstrumentId id = i->first;
	FaderRec &rec = i->second;
	if (!rec.m_populated) continue;

	Rosegarden::LevelInfo info;

	if (mapper->getInstrumentRecordLevel(id, info)) {

	    bool armed = false;

	    for (Rosegarden::Composition::trackcontainer::iterator ti =
		     tracks.begin(); ti != tracks.end(); ++ti) {
		if (ti->second->getInstrument() == id) {
		    if (comp.isTrackRecording(ti->second->getId())) {
			armed = true;
			break;
		    }
		}
	    }

	    if (!armed) continue;

	    // The values passed through are long-fader values
	    float dBleft = Rosegarden::AudioLevel::fader_to_dB
		(info.level, 127, Rosegarden::AudioLevel::LongFader);

	    if (rec.m_stereoness) {
		float dBright = Rosegarden::AudioLevel::fader_to_dB
		    (info.levelRight, 127, Rosegarden::AudioLevel::LongFader);
		
		rec.m_meter->setRecordLevel(dBleft, dBright);
		
	    } else {
		rec.m_meter->setRecordLevel(dBleft);
	    }
	}
    }
}

void
AudioMixerWindow::slotSetInputCountFromAction()
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
AudioMixerWindow::slotSetSubmasterCountFromAction()
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


void AudioMixerWindow::FaderRec::setVisible(bool visible)
{
    if (visible) {
        if (m_input)  m_input->getWidget()->show();
        if (m_output) m_output->getWidget()->show();
        if (m_pan)    m_pan->show();
        if (m_fader)  m_fader->show();
        if (m_meter)  m_meter->show();
// commented out until implemented
//         if (m_muteButton)   m_muteButton->show();
//         if (m_soloButton)   m_soloButton->show();
//         if (m_recordButton) m_recordButton->show();
        if (m_stereoButton) m_stereoButton->show();

    } else {

        if (m_input)        m_input->getWidget()->hide();
        if (m_output)       m_output->getWidget()->hide();
        if (m_pan)          m_pan->hide();
        if (m_fader)        m_fader->hide();
        if (m_meter)        m_meter->hide();
// commented out until implemented
//         if (m_muteButton)   m_muteButton->hide();
//         if (m_soloButton)   m_soloButton->hide();
//         if (m_recordButton) m_recordButton->hide();
        if (m_stereoButton) m_stereoButton->hide();
    }

    setPluginButtonsVisible(visible);

}


void
AudioMixerWindow::FaderRec::setPluginButtonsVisible(bool visible)
{
    if (!m_pluginBox) return;

    if (visible) {
        m_pluginBox->show();
    } else {
        m_pluginBox->hide();
    }
}



void
AudioMixerWindow::slotToggleFaders()
{
    m_studio->setMixerDisplayOptions(m_studio->getMixerDisplayOptions() ^
				     MIXER_OMIT_FADERS);

    slotUpdateFaderVisibility();
}

void
AudioMixerWindow::slotUpdateFaderVisibility()
{
    bool d = !(m_studio->getMixerDisplayOptions() & MIXER_OMIT_FADERS);

    KToggleAction *action = dynamic_cast<KToggleAction *>
        (actionCollection()->action("show_audio_faders"));
    if (action) {
	action->setChecked(d);
    }

    RG_DEBUG << "AudioMixerWindow::slotUpdateFaderVisibility: visiblility is " << d << " (options " << m_studio->getMixerDisplayOptions() << ")" << endl;

    for (FaderMap::iterator i = m_faders.begin(); i != m_faders.end(); ++i) {
	if (i->first < Rosegarden::SoftSynthInstrumentBase) {
	    FaderRec rec = i->second;
	    rec.setVisible(d);
	}
    }

    toggleNamedWidgets(d, "audioIdLabel");

    adjustSize();
}

void
AudioMixerWindow::slotToggleSynthFaders()
{
    m_studio->setMixerDisplayOptions(m_studio->getMixerDisplayOptions() ^
				     MIXER_OMIT_SYNTH_FADERS);

    slotUpdateSynthFaderVisibility();
}

void
AudioMixerWindow::slotUpdateSynthFaderVisibility()
{
    KToggleAction *action = dynamic_cast<KToggleAction *>
        (actionCollection()->action("show_synth_faders"));
    if (!action) return;

    action->setChecked(!(m_studio->getMixerDisplayOptions() &
			 MIXER_OMIT_SYNTH_FADERS));

    for (FaderMap::iterator i = m_faders.begin(); i != m_faders.end(); ++i) {
	if (i->first >= Rosegarden::SoftSynthInstrumentBase) {
	    FaderRec rec = i->second;
	    rec.setVisible(action->isChecked());
	}
    }

    toggleNamedWidgets(action->isChecked(), "synthIdLabel");

    adjustSize();
}

void
AudioMixerWindow::slotToggleSubmasters()
{
    m_studio->setMixerDisplayOptions(m_studio->getMixerDisplayOptions() ^
				     MIXER_OMIT_SUBMASTERS);

    slotUpdateSubmasterVisibility();
}

void
AudioMixerWindow::slotUpdateSubmasterVisibility()
{
    KToggleAction *action = dynamic_cast<KToggleAction *>
        (actionCollection()->action("show_audio_submasters"));
    if (!action) return;
				     
    action->setChecked(!(m_studio->getMixerDisplayOptions() &
			 MIXER_OMIT_SUBMASTERS));

    for(FaderVector::iterator i = m_submasters.begin(); i != m_submasters.end(); ++i) {
        FaderRec rec = *i;
        rec.setVisible(action->isChecked());
    }

    toggleNamedWidgets(action->isChecked(), "subMaster");
    
    adjustSize();
}

void
AudioMixerWindow::slotTogglePluginButtons()
{
    m_studio->setMixerDisplayOptions(m_studio->getMixerDisplayOptions() ^
				     MIXER_OMIT_PLUGINS);

    slotUpdatePluginButtonVisibility();
}

void
AudioMixerWindow::slotUpdatePluginButtonVisibility()
{
    KToggleAction *action = dynamic_cast<KToggleAction *>
        (actionCollection()->action("show_plugin_buttons"));
    if (!action) return;
				     
    action->setChecked(!(m_studio->getMixerDisplayOptions() &
			 MIXER_OMIT_PLUGINS));

    for(FaderMap::iterator i = m_faders.begin(); i != m_faders.end(); ++i) {
        FaderRec rec = i->second;
        rec.setPluginButtonsVisible(action->isChecked());
    }

    adjustSize();
}

void
AudioMixerWindow::slotToggleUnassignedFaders()
{
    KToggleAction *action = dynamic_cast<KToggleAction *>
        (actionCollection()->action("show_unassigned_faders"));
    if (!action) return;

    m_studio->setMixerDisplayOptions(m_studio->getMixerDisplayOptions() ^
				     MIXER_SHOW_UNASSIGNED_FADERS);
				     
    action->setChecked(m_studio->getMixerDisplayOptions() &
		       MIXER_SHOW_UNASSIGNED_FADERS);

    populate();
}

void
AudioMixerWindow::toggleNamedWidgets(bool show, const char* const name)
{
    QLayoutIterator it = m_mainBox->layout()->iterator();
    QLayoutItem *child;
    while ( (child = it.current()) != 0 ) {
        QWidget* widget = child->widget();
        if (widget && widget->name() && !strcmp(widget->name(), name)) {
            if (show)
                widget->show();
            else
                widget->hide();
        }
        
        ++it;
    }

}

// ---- MidiMixerWindow ----
//
//
MidiMixerWindow::MidiMixerWindow(QWidget *parent,
			 RosegardenGUIDoc *document):
        MixerWindow(parent, document),
        m_tabFrame(0)
{
    // Initial setup
    //
    setupTabs();

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

    icon = QIconSet(NotePixmapFactory::toQPixmap(NotePixmapFactory::makeToolbarPixmap
                                                 ("transport-record")));
    new KAction(i18n("&Record"), icon, 0, this,
		SIGNAL(record()), actionCollection(),
		"record");

    createGUI("midimixer.rc");

}

void 
MidiMixerWindow::setupTabs()
{
    Rosegarden::DeviceListConstIterator it;
    Rosegarden::MidiDevice *dev = 0;
    Rosegarden::InstrumentList instruments;
    Rosegarden::InstrumentList::const_iterator iIt;
    int faderCount = 0, deviceCount = 1;

    if (m_tabFrame) delete m_tabFrame;

    // Setup m_tabFrame
    //
    m_tabWidget = new QTabWidget(this);
    setCentralWidget(m_tabWidget);
    m_tabWidget->setTabPosition(QTabWidget::Bottom);
    setCaption(i18n("MIDI Mixer"));

    for (it = m_studio->begin(); it != m_studio->end(); ++it)
    {
        dev = dynamic_cast<Rosegarden::MidiDevice*>(*it);

        if (dev)
        {
            // Get the control parameters that are on the IPB (and hence can
            // be shown here too).
            //
            Rosegarden::ControlList controls = dev->getIPBControlParameters();

            instruments = dev->getPresentationInstruments();

            // Don't add a frame for empty devices
            //
            if (!instruments.size()) continue;

            m_tabFrame = new QFrame(m_tabWidget);
            m_tabFrame->setFrameStyle(QFrame::TabWidgetPanel);
            m_tabFrame->setMargin(10);

            QGridLayout *mainLayout = new QGridLayout
                (m_tabFrame, instruments.size() + 4, controls.size() + 4, 5);

            // MIDI Mixer label
            //
            //QLabel *label = new QLabel(QString("%1 %2").
                //arg(strtoqstr(dev->getName()))
                //.arg(i18n("MIDI Mixer")), m_tabFrame);

            QLabel *label = new QLabel("", m_tabFrame);
            mainLayout->addMultiCellWidget(label, 0, 0, 0, 16, Qt::AlignCenter);

            // control labels
            for (unsigned int i = 0; i < controls.size(); ++i)
            {
                label = new QLabel(strtoqstr(controls[i].getName()), m_tabFrame);
                mainLayout->addWidget(label, i + 1, 0, Qt::AlignCenter);
            }

            // meter label
            //
            //label = new QLabel(i18n("Meter"), m_tabFrame);
            //mainLayout->addWidget(label, 
                //controls.size() + 1, 0, Qt::AlignCenter);

            // volume label
            label = new QLabel(i18n("Volume"), m_tabFrame);
            mainLayout->addWidget(label, controls.size() + 2, 0, 
                    Qt::AlignCenter);

            // instrument label
            label = new QLabel(i18n("Instrument"), m_tabFrame);
            mainLayout->addWidget(label, controls.size() + 3, 0, 
                    Qt::AlignCenter);

            int posCount = 1;
            int firstInstrument = -1;

            for (iIt = instruments.begin(); iIt != instruments.end(); ++iIt)
            {

                // Add new fader struct
                //
                m_faders.push_back(new FaderStruct());

                // Store the first ID
                //
                if (firstInstrument == -1) firstInstrument = (*iIt)->getId();


                // Add the controls
                //
                for (unsigned int i = 0; i < controls.size(); ++i)
                {
                    QColor knobColour = Qt::white;

                    if (controls[i].getColourIndex() > 0) 
                    {
                        Rosegarden::Colour c =
                            m_document->getComposition().getGeneralColourMap().
                            getColourByIndex(controls[i].getColourIndex());

                        knobColour = QColor(c.getRed(), 
                                c.getGreen(), c.getBlue());
                    }

                    RosegardenRotary *controller = 
                        new RosegardenRotary(m_tabFrame,
                                controls[i].getMin(),
                                controls[i].getMax(),
                                1.0,
                                5.0,
                                controls[i].getDefault(),
                                20,
				RosegardenRotary::NoTicks, 
			        false,
				controls[i].getDefault() == 64); //!!! hacky

                    controller->setKnobColour(knobColour);

	            connect(controller, SIGNAL(valueChanged(float)),
		            this, SLOT(slotControllerChanged(float)));

                    mainLayout->addWidget(controller, i + 1, posCount, 
                            Qt::AlignCenter);

                    // Store the rotary
                    //
                    m_faders[faderCount]->m_controllerRotaries.push_back(
                            std::pair<Rosegarden::MidiByte, RosegardenRotary*>
                                (controls[i].getControllerValue(), controller));
                }

                // Pan rotary
                //
                MidiMixerVUMeter *meter = 
                    new MidiMixerVUMeter(m_tabFrame,
                            VUMeter::FixedHeightVisiblePeakHold, 6, 30);
                mainLayout->addWidget(meter, controls.size() + 1, 
                        posCount, Qt::AlignCenter);
                m_faders[faderCount]->m_vuMeter = meter;

                // Volume fader
                //
                RosegardenFader *fader = 
                    new RosegardenFader(0, 127, 100, 20, 80, m_tabFrame);
                mainLayout->addWidget(fader, controls.size() + 2, 
                        posCount, Qt::AlignCenter);
                m_faders[faderCount]->m_volumeFader = fader;
                //fader->setFader(float((*iIt)->getVolume()));

                // Label
                //
	        QLabel *idLabel = new QLabel(QString("%1").
                        arg((*iIt)->getId() - firstInstrument + 1),
                        m_tabFrame, "idLabel");

                mainLayout->addWidget(idLabel, controls.size() + 3, 
                        posCount, Qt::AlignCenter);

                // store id in struct
                m_faders[faderCount]->m_id = (*iIt)->getId();

                // Connect them up
                //
	        connect(fader, SIGNAL(faderChanged(float)),
		        this, SLOT(slotFaderLevelChanged(float)));

                // Update all the faders and controllers
                //
                slotUpdateInstrument((*iIt)->getId());

                // Increment counters
                //
                posCount++;
                faderCount++;
            }

            QString name = QString("%1 (%2)").arg(strtoqstr(dev->getName()))
                                      .arg(deviceCount++);

            addTab(m_tabFrame, name);
        }
    }
}

void 
MidiMixerWindow::addTab(QWidget *tab, const QString &title)
{
    m_tabWidget->addTab(tab, title);
}

void
MidiMixerWindow::slotFaderLevelChanged(float value)
{
    const QObject *s = sender();

    for (FaderVector::const_iterator it = m_faders.begin();
         it != m_faders.end(); ++it)
    {
        if ((*it)->m_volumeFader == s)
        {
            Rosegarden::Instrument *instr = m_studio->
                getInstrumentById((*it)->m_id);

            if (instr)
            {
                instr->setVolume(Rosegarden::MidiByte(value));
                Rosegarden::MappedEvent mE((*it)->m_id,
                                       Rosegarden::MappedEvent::MidiController,
                                       Rosegarden::MIDI_CONTROLLER_VOLUME,
                                       Rosegarden::MidiByte(value));
                Rosegarden::StudioControl::sendMappedEvent(mE);
            }

            emit instrumentParametersChanged((*it)->m_id);
            return;
        }
    }
}

void
MidiMixerWindow::slotControllerChanged(float value)
{
    const QObject *s = sender();
    unsigned int i = 0, j = 0;

    for (i = 0; i < m_faders.size(); ++i)
    {
        for (j = 0; j < m_faders[i]->m_controllerRotaries.size(); ++j)
        {
            if (m_faders[i]->m_controllerRotaries[j].second == s)
                break;
        }

        // break out on match
        if (j != m_faders[i]->m_controllerRotaries.size()) break;
    }

    // Don't do anything if we've not matched and got solid values
    // for i and j
    //
    if (i == m_faders.size() || j == m_faders[i]->m_controllerRotaries.size())
        return;

    //RG_DEBUG << "MidiMixerWindow::slotControllerChanged - found a controller"
               //<< endl;
    
    Rosegarden::Instrument *instr = m_studio->getInstrumentById(
            m_faders[i]->m_id);

    if (instr) {

        //RG_DEBUG << "MidiMixerWindow::slotControllerChanged - "
                   //<< "got instrument to change" << endl;

        if (m_faders[i]->m_controllerRotaries[j].first == 
                Rosegarden::MIDI_CONTROLLER_PAN)
            instr->setPan(Rosegarden::MidiByte(value));
        else
        {
            instr->setControllerValue(m_faders[i]->
                    m_controllerRotaries[j].first, 
                    Rosegarden::MidiByte(value));
        }

        Rosegarden::MappedEvent mE(m_faders[i]->m_id,
                                   Rosegarden::MappedEvent::MidiController,
                                   m_faders[i]->m_controllerRotaries[j].first,
                                   Rosegarden::MidiByte(value));
        Rosegarden::StudioControl::sendMappedEvent(mE);

        emit instrumentParametersChanged(m_faders[i]->m_id);
    }
}


void 
MidiMixerWindow::slotUpdateInstrument(Rosegarden::InstrumentId id)
{
    //RG_DEBUG << "MidiMixerWindow::slotUpdateInstrument - id = " << id << endl;

    Rosegarden::DeviceListConstIterator it;
    Rosegarden::MidiDevice *dev = 0;
    Rosegarden::InstrumentList instruments;
    Rosegarden::InstrumentList::const_iterator iIt;
    int count = 0;

    blockSignals(true);

    for (it = m_studio->begin(); it != m_studio->end(); ++it)
    {
        dev = dynamic_cast<Rosegarden::MidiDevice*>(*it);

        if (dev)
        {
            instruments = dev->getPresentationInstruments();
            Rosegarden::ControlList controls = dev->getIPBControlParameters();

            for (iIt = instruments.begin(); iIt != instruments.end(); ++iIt)
            {
                // Match and set
                //
                if ((*iIt)->getId() == id)
                {
                    // Set Volume fader
                    //
                    m_faders[count]->m_volumeFader->
                        setFader(float((*iIt)->getVolume()));

                    /*
                    Rosegarden::StaticControllers &staticControls = 
                        (*iIt)->getStaticControllers();
                    RG_DEBUG << "STATIC CONTROLS SIZE = " 
                             << staticControls.size() << endl;
                    */

                    // Set all controllers for this Instrument
                    //
                    for (unsigned int i = 0; i < controls.size(); ++i)
                    {
                        float value = 0.0;

                        if (controls[i].getControllerValue() == 
                                Rosegarden::MIDI_CONTROLLER_PAN)
                        {
                            m_faders[count]->m_controllerRotaries[i].
                                second->setPosition((*iIt)->getPan());
                        }
                        else
                        {
                            // The ControllerValues might not yet be set on 
                            // the actual Instrument so don't always expect
                            // to find one.  There might be a hole here for
                            // deleted Controllers to hang around on 
                            // Instruments..
                            //
                            try
                            {
                                value = float((*iIt)->getControllerValue
                                        (controls[i].getControllerValue()));
                            }
                            catch(std::string s)
                            {
                                /*
                                RG_DEBUG << 
                                "MidiMixerWindow::slotUpdateInstrument - "
                                         << "can't match controller " 
                                         << int(controls[i].
                                             getControllerValue()) << " - \""
                                         << s << "\"" << endl;
                                         */
                                continue;
                            }

                            /*
                            RG_DEBUG << "MidiMixerWindow::slotUpdateInstrument"
                                     << " - MATCHED "
                                     << int(controls[i].getControllerValue())
                                     << endl;
                                     */

                            m_faders[count]->m_controllerRotaries[i].
                                second->setPosition(value);
                        }
                    }
                }
                count++;
            }
        }
    }

    blockSignals(false);
}


void
MidiMixerWindow::updateMeters(SequencerMapper *mapper)
{
    for (unsigned int i = 0; i != m_faders.size(); ++i) 
    {
	Rosegarden::LevelInfo info;
	if (!mapper->
                getInstrumentLevelForMixer(m_faders[i]->m_id, info)) continue;
        m_faders[i]->m_vuMeter->setLevel(double(info.level/127.0));
        RG_DEBUG << "MidiMixerWindow::updateMeters - level  " << info.level << endl;
    }
}

void
MidiMixerWindow::updateMonitorMeter(SequencerMapper *)
{
    // none here
}

void 
MidiMixerWindow::slotSynchronise()
{
    RG_DEBUG << "MidiMixer::slotSynchronise" << endl;
    //setupTabs();
}


// ---- MidiMixerVUMeter ----
//
//

MidiMixerVUMeter::MidiMixerVUMeter(QWidget *parent,
                           VUMeterType type,
                           int width,
                           int height,
                           const char *name):
    VUMeter(parent, type, false, false, width, height, VUMeter::Vertical, name)
{
    setAlignment(AlignCenter);
}


void
MidiMixerVUMeter::meterStart()
{
}


void
MidiMixerVUMeter::meterStop()
{
}



#include "mixer.moc"
