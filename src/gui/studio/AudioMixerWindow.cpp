/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2008 the Rosegarden development team.
 
    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.
 
    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/


#include "AudioMixerWindow.h"
#include <QLayout>
#include <QApplication>

#include "AudioPlugin.h"
#include "AudioPluginManager.h"
#include "MixerWindow.h"
#include "StudioControl.h"
#include "sound/Midi.h"
#include "misc/Debug.h"
#include "gui/application/TransportStatus.h"
#include "base/AudioLevel.h"
#include "base/AudioPluginInstance.h"
#include "base/Composition.h"
#include "base/Device.h"
#include "base/Instrument.h"
#include "base/MidiProgram.h"
#include "base/Studio.h"
#include "document/RosegardenGUIDoc.h"
#include "gui/editors/notation/NotePixmapFactory.h"
#include "gui/general/GUIPalette.h"
#include "gui/seqmanager/SequencerMapper.h"
#include "gui/seqmanager/SequenceManager.h"
#include "gui/widgets/AudioRouteMenu.h"
#include "gui/widgets/AudioVUMeter.h"
#include "gui/widgets/Fader.h"
#include "gui/widgets/Rotary.h"
#include "gui/widgets/VUMeter.h"
#include "sound/MappedCommon.h"
#include "sound/MappedEvent.h"
#include "sound/MappedStudio.h"
#include <klocale.h>
#include <kstandarddirs.h>
#include <kaction.h>
#include <kglobal.h>
#include <kmainwindow.h>
#include <kstandardaction.h>
#include <qshortcut.h>
#include <QColor>
#include <QFont>
#include <QFrame>
#include <QIcon>
#include <QLabel>
#include <QObject>
#include <QPalette>
#include <QPixmap>
#include <QPushButton>
#include <QString>
#include <QToolTip>
#include <QWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>


namespace Rosegarden
{

// We define these such that the default of no-bits-set for the
// studio's mixer display options produces the most sensible result
static const unsigned int MIXER_OMIT_FADERS            = 1 << 0;
static const unsigned int MIXER_OMIT_SUBMASTERS        = 1 << 1;
static const unsigned int MIXER_OMIT_PLUGINS           = 1 << 2;
static const unsigned int MIXER_SHOW_UNASSIGNED_FADERS = 1 << 3;
static const unsigned int MIXER_OMIT_SYNTH_FADERS      = 1 << 4;


AudioMixerWindow::AudioMixerWindow(QWidget *parent,
                                   RosegardenGUIDoc *document):
        MixerWindow(parent, document),
        m_surroundBoxLayout(0),
        m_mainBox (0)
{
    populate();

    KStandardAction::close(this,
                      SLOT(slotClose()),
                      actionCollection());

    QIcon icon = QIcon(NotePixmapFactory::toQPixmap(NotePixmapFactory::makeToolbarPixmap
                             ("transport-play")));
    KAction *play = new KAction(i18n("&Play"), icon, Key_Enter, this,
                SIGNAL(play()), actionCollection(), "play");
    // Alternative shortcut for Play
    KShortcut playShortcut = play->shortcut();
    playShortcut.append( KKey(Key_Return + CTRL) );
    play->setShortcut(playShortcut);

    icon = QIcon(NotePixmapFactory::toQPixmap(NotePixmapFactory::makeToolbarPixmap
                    ("transport-stop")));
    new KAction(i18n("&Stop"), icon, Key_Insert, this,
                SIGNAL(stop()), actionCollection(), "stop");

    icon = QIcon(NotePixmapFactory::toQPixmap(NotePixmapFactory::makeToolbarPixmap
                    ("transport-rewind")));
    new KAction(i18n("Re&wind"), icon, Key_End, this,
                SIGNAL(rewindPlayback()), actionCollection(),
                "playback_pointer_back_bar");

    icon = QIcon(NotePixmapFactory::toQPixmap(NotePixmapFactory::makeToolbarPixmap
                    ("transport-ffwd")));
    new KAction(i18n("&Fast Forward"), icon, Key_PageDown, this,
                SIGNAL(fastForwardPlayback()), actionCollection(),
                "playback_pointer_forward_bar");

    icon = QIcon(NotePixmapFactory::toQPixmap(NotePixmapFactory::makeToolbarPixmap
                    ("transport-rewind-end")));
    new KAction(i18n("Rewind to &Beginning"), icon, 0, this,
                SIGNAL(rewindPlaybackToBeginning()), actionCollection(),
                "playback_pointer_start");

    icon = QIcon(NotePixmapFactory::toQPixmap(NotePixmapFactory::makeToolbarPixmap
                    ("transport-ffwd-end")));
    new KAction(i18n("Fast Forward to &End"), icon, 0, this,
                SIGNAL(fastForwardPlaybackToEnd()), actionCollection(),
                "playback_pointer_end");

    icon = QIcon(NotePixmapFactory::toQPixmap(NotePixmapFactory::makeToolbarPixmap
                    ("transport-record")));
    new KAction(i18n("&Record"), icon, 0, this,
                SIGNAL(record()), actionCollection(),
                "record");

    icon = QIcon(NotePixmapFactory::toQPixmap(NotePixmapFactory::makeToolbarPixmap
                    ("transport-panic")));
    new KAction(i18n("Panic"), icon, Key_P + CTRL + ALT, this,
                SIGNAL(panic()), actionCollection(),
                "panic");

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
            new KRadioAction(i18np("1 Input", "%1 Inputs", i),
                             0, this,
                             SLOT(slotSetInputCountFromAction()), actionCollection(),
                             QString("inputs_%1").arg(i));
        action->setExclusiveGroup("inputs");
        if (i == int(m_studio->getRecordIns().size()))
            action->setChecked(true);
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
                 (i18np("1 Submaster", "%1 Submasters", i),
                  0, this,
                  SLOT(slotSetSubmasterCountFromAction()), actionCollection(),
                  QString("submasters_%1").arg(i));
        action->setExclusiveGroup("submasters");
        if (i == int(m_studio->getBusses().size()) - 1)
            action->setChecked(true);
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
    if (!m_mainBox)
        return ;

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

    if (m_surroundBoxLayout)
        m_surroundBoxLayout->removeWidget(m_mainBox);   // Needed ???
    delete m_mainBox;
    m_mainBox = 0;
}

void
AudioMixerWindow::populate()
{

    if (m_mainBox) {

        depopulate();

    } else {

        m_surroundBox = new QWidget(this);
        m_surroundBoxLayout = new QHBoxLayout;
        m_surroundBox->setLayout(m_surroundBoxLayout);
        setCentralWidget(m_surroundBox);
    }

    QFont font;
    font.setPointSize(font.pointSize() * 8 / 10);
    font.setBold(false);
    setFont(font);

    QFont boldFont(font);
    boldFont.setBold(true);

    m_mainBox = new QFrame(m_surroundBox);
    m_surroundBoxLayout->addWidget(m_mainBox);

    InstrumentList instruments = m_studio->getPresentationInstruments();
    BussList busses = m_studio->getBusses();

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

    for (InstrumentList::iterator i = instruments.begin();
            i != instruments.end(); ++i) {

        if ((*i)->getType() != Instrument::Audio &&
                (*i)->getType() != Instrument::SoftSynth)
            continue;

        FaderRec rec;

        if (!showUnassigned) {
            // Do any tracks use this instrument?
            if (!isInstrumentAssigned((*i)->getId())) {
                continue;
            }
        }
        rec.m_populated = true;

        if ((*i)->getType() == Instrument::Audio) {
            rec.m_input = new AudioRouteMenu(m_mainBox,
                                             AudioRouteMenu::In,
                                             AudioRouteMenu::Compact,
                                             m_studio, *i);
            QToolTip::add
                (rec.m_input->getWidget(), i18n("Record input source"));
            rec.m_input->getWidget()->setMaximumWidth(45);
        } else {
            rec.m_input = 0;
        }

        rec.m_output = new AudioRouteMenu(m_mainBox,
                                          AudioRouteMenu::Out,
                                          AudioRouteMenu::Compact,
                                          m_studio, *i);
        QToolTip::add
            (rec.m_output->getWidget(), i18n("Output destination"));
        rec.m_output->getWidget()->setMaximumWidth(45);

        rec.m_pan = new Rotary
                    (m_mainBox, -100.0, 100.0, 1.0, 5.0, 0.0, 20,
                     Rotary::NoTicks, false, true);

        if ((*i)->getType() == Instrument::Audio) {
            rec.m_pan->setKnobColour(GUIPalette::getColour(GUIPalette::RotaryPastelGreen));
        } else {
            rec.m_pan->setKnobColour(GUIPalette::getColour(GUIPalette::RotaryPastelYellow));
        }

        QToolTip::add
            (rec.m_pan, i18n("Pan"));

        rec.m_fader = new Fader
                      (AudioLevel::LongFader, 20, 240, m_mainBox);
        rec.m_meter = new AudioVUMeter
                      (m_mainBox, VUMeter::AudioPeakHoldIECLong, true, rec.m_input != 0,
                       20, 240);

        QToolTip::add
            (rec.m_fader, i18n("Audio level"));
        QToolTip::add
            (rec.m_meter, i18n("Audio level"));

        rec.m_stereoButton = new QPushButton(m_mainBox);
        rec.m_stereoButton->setPixmap(m_monoPixmap);
        rec.m_stereoButton->setFixedSize(20, 20);
        rec.m_stereoButton->setFlat(true);
        rec.m_stereoness = false;
        QToolTip::add
            (rec.m_stereoButton, i18n("Mono or stereo"));

        rec.m_muteButton = new QPushButton(m_mainBox);
        rec.m_muteButton->setText("M");
        rec.m_muteButton->setToggleButton(true);
        rec.m_muteButton->setFixedWidth(rec.m_stereoButton->width());
        rec.m_muteButton->setFixedHeight(rec.m_stereoButton->height());
        rec.m_muteButton->setFlat(true);
        QToolTip::add
            (rec.m_muteButton, i18n("Mute"));

        rec.m_soloButton = new QPushButton(m_mainBox);
        rec.m_soloButton->setText("S");
        rec.m_soloButton->setToggleButton(true);
        rec.m_soloButton->setFixedWidth(rec.m_stereoButton->width());
        rec.m_soloButton->setFixedHeight(rec.m_stereoButton->height());
        rec.m_soloButton->setFlat(true);
        QToolTip::add
            (rec.m_soloButton, i18n("Solo"));

        rec.m_recordButton = new QPushButton(m_mainBox);
        rec.m_recordButton->setText("R");
        rec.m_recordButton->setToggleButton(true);
        rec.m_recordButton->setFixedWidth(rec.m_stereoButton->width());
        rec.m_recordButton->setFixedHeight(rec.m_stereoButton->height());
        rec.m_recordButton->setFlat(true);
        QToolTip::add
            (rec.m_recordButton, i18n("Arm recording"));

        rec.m_pluginBox = new QWidget(m_mainBox);
        QVBoxLayout *pluginBoxLayout = new QVBoxLayout;

        for (int p = 0; p < 5; ++p) {
            QPushButton *plugin = new QPushButton(rec.m_pluginBox, "pluginButton");
            pluginBoxLayout->addWidget(plugin);
            plugin->setText(i18n("<none>"));
            plugin->setMaximumWidth(45);
            QToolTip::add
                (plugin, i18n("Audio plugin button"));
            rec.m_plugins.push_back(plugin);
            connect(plugin, SIGNAL(clicked()),
                    this, SLOT(slotSelectPlugin()));
        }

        rec.m_pluginBox->setLayout(pluginBoxLayout);

        QLabel *idLabel;
        QString idString;
        if ((*i)->getType() == Instrument::Audio) {
            idString = i18n("Audio %1", (*i)->getId() -
                                            AudioInstrumentBase + 1);
            idLabel = new QLabel(idString, m_mainBox, "audioIdLabel");
        } else {
            idString = i18n("Synth %1", (*i)->getId() -
                                            SoftSynthInstrumentBase + 1);
            idLabel = new QLabel(idString, m_mainBox, "synthIdLabel");
        }
        idLabel->setFont(boldFont);

        if (rec.m_input) {
            mainLayout->addMultiCellWidget(rec.m_input->getWidget(), 1, 1, col, col + 1);
        }
        mainLayout->addMultiCellWidget(rec.m_output->getWidget(), 2, 2, col, col + 1);
        //	mainLayout->addWidget(idLabel, 2, col, Qt::AlignCenter);
        //	mainLayout->addWidget(rec.m_pan, 2, col+1, Qt::AlignLeft);

        mainLayout->addWidget(idLabel, 0, col, 0- 1, col + 1- col+1, Qt::AlignCenter);
        mainLayout->addWidget(rec.m_pan, 5, col, Qt::AlignCenter);

        mainLayout->addWidget(rec.m_fader, 3, col, Qt::AlignCenter);
        mainLayout->addWidget(rec.m_meter, 3, col + 1, Qt::AlignCenter);

        // commented out until implemented
        //	mainLayout->addWidget(rec.m_muteButton, 4, col);
        //	mainLayout->addWidget(rec.m_soloButton, 4, col+1);
        rec.m_muteButton->hide();
        rec.m_soloButton->hide();

        //	mainLayout->addWidget(rec.m_recordButton, 5, col);
        //	mainLayout->addWidget(rec.m_stereoButton, 5, col+1);

        rec.m_recordButton->hide();
        mainLayout->addWidget(rec.m_stereoButton, 5, col + 1);

        if (rec.m_pluginBox) {
            mainLayout->addWidget(rec.m_pluginBox, 6, col, 1, col + 1- col+1);
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

        mainLayout->addMultiCell(new QSpacerItem(2, 0), 0, 6, col + 2, col + 2);

        col += 3;
    }

    count = 1;

    for (BussList::iterator i = busses.begin();
            i != busses.end(); ++i) {

        if (i == busses.begin())
            continue; // that one's the master

        FaderRec rec;
        rec.m_populated = true;

        rec.m_pan = new Rotary
                    (m_mainBox, -100.0, 100.0, 1.0, 5.0, 0.0, 20,
                     Rotary::NoTicks, false, true);
        rec.m_pan->setKnobColour(GUIPalette::getColour(GUIPalette::RotaryPastelBlue));

        QToolTip::add
            (rec.m_pan, i18n("Pan"));

        rec.m_fader = new Fader
                      (AudioLevel::LongFader, 20, 240, m_mainBox);
        rec.m_meter = new AudioVUMeter
                      (m_mainBox, VUMeter::AudioPeakHoldIECLong, true, false, 20, 240);

        QToolTip::add
            (rec.m_fader, i18n("Audio level"));
        QToolTip::add
            (rec.m_meter, i18n("Audio level"));

        rec.m_muteButton = new QPushButton(m_mainBox);
        rec.m_muteButton->setText("M");
        rec.m_muteButton->setToggleButton(true);
        rec.m_muteButton->setFlat(true);

        QToolTip::add
            (rec.m_muteButton, i18n("Mute"));

        rec.m_pluginBox = new QWidget(m_mainBox);
        QVBoxLayout *pluginBoxLayout = new QVBoxLayout;

        for (int p = 0; p < 5; ++p) {
            QPushButton *plugin = new QPushButton(rec.m_pluginBox, "pluginButton");
            pluginBoxLayout->addWidget(plugin);
            plugin->setText(i18n("<none>"));
            plugin->setMaximumWidth(45);
            QToolTip::add
                (plugin, i18n("Audio plugin button"));
            rec.m_plugins.push_back(plugin);
            connect(plugin, SIGNAL(clicked()),
                    this, SLOT(slotSelectPlugin()));
        }

        rec.m_pluginBox->setLayout(pluginBoxLayout);

        QLabel *idLabel = new QLabel(i18n("Sub %1", count), m_mainBox, "subMaster");
        idLabel->setFont(boldFont);

        //	mainLayout->addWidget(idLabel, 2, col, Qt::AlignCenter);
        mainLayout->addWidget(idLabel, 0, col, 0- 1, col + 1- col+1, Qt::AlignCenter);

        //	mainLayout->addWidget(rec.m_pan, 2, col+1, Qt::AlignLeft);
        mainLayout->addWidget(rec.m_pan, 5, col, 1, col + 1- col+1, Qt::AlignCenter);

        mainLayout->addWidget(rec.m_fader, 3, col, Qt::AlignCenter);
        mainLayout->addWidget(rec.m_meter, 3, col + 1, Qt::AlignCenter);

        //	mainLayout->addWidget(rec.m_muteButton, 4, col, 1, col+1- col+1);
        rec.m_muteButton->hide();

        if (rec.m_pluginBox) {
            mainLayout->addWidget(rec.m_pluginBox, 6, col, 1, col + 1- col+1);
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

        mainLayout->addMultiCell(new QSpacerItem(2, 0), 0, 6, col + 2, col + 2);

        col += 3;
    }

    if (busses.size() > 0) {

        FaderRec rec;
        rec.m_populated = true;

        rec.m_fader = new Fader
                      (AudioLevel::LongFader, 20, 240, m_mainBox);
        rec.m_meter = new AudioVUMeter
                      (m_mainBox, VUMeter::AudioPeakHoldIEC, true, false, 20, 240);

        QToolTip::add
            (rec.m_fader, i18n("Audio master output level"));
        QToolTip::add
            (rec.m_meter, i18n("Audio master output level"));

        rec.m_muteButton = new QPushButton(m_mainBox);
        rec.m_muteButton->setText("M");
        rec.m_muteButton->setToggleButton(true);
        rec.m_muteButton->setFlat(true);

        QLabel *idLabel = new QLabel(i18n("Master"), m_mainBox);
        idLabel->setFont(boldFont);

        mainLayout->addWidget(idLabel, 0, col, 0- 1, col + 1- col+1, Qt::AlignCenter);
        mainLayout->addWidget(rec.m_fader, 3, col, Qt::AlignCenter);
        mainLayout->addWidget(rec.m_meter, 3, col + 1, Qt::AlignCenter);

        //	mainLayout->addWidget(rec.m_muteButton, 4, col, 1, col+1- col+1);
        rec.m_muteButton->hide();

        mainLayout->addMultiCell(new QSpacerItem(2, 0), 0, 6, col + 2, col + 2);

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
AudioMixerWindow::isInstrumentAssigned(InstrumentId id)
{
    Composition::trackcontainer &tracks =
        m_document->getComposition().getTracks();

    for (Composition::trackcontainer::iterator ti =
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

        InstrumentId id = i->first;
        bool assigned = isInstrumentAssigned(id);

        if (assigned != i->second.m_populated) {
            // found an inconsistency
            populate();
            return ;
        }
    }
}

void
AudioMixerWindow::slotUpdateInstrument(InstrumentId id)
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
AudioMixerWindow::slotPluginSelected(InstrumentId id,
                                     int index, int plugin)
{
    if (id >= (int)AudioInstrumentBase) {

        FaderRec &rec = m_faders[id];
        if (!rec.m_populated || !rec.m_pluginBox)
            return ;

        // nowhere to display synth plugin info yet
        if (index >= rec.m_plugins.size())
            return ;

        if (plugin == -1) {

            rec.m_plugins[index]->setText(i18n("<none>"));
            QToolTip::add
                (rec.m_plugins[index], i18n("<no plugin>"));

            rec.m_plugins[index]->setPaletteBackgroundColor
            (qApp->palette().
             color(QPalette::Active, QColorGroup::Button));

        } else {

            AudioPlugin *pluginClass
            = m_document->getPluginManager()->getPlugin(plugin);

            QColor pluginBgColour =
                qApp->palette().color(QPalette::Active, QColorGroup::Light);

            if (pluginClass) {
                rec.m_plugins[index]->
                setText(pluginClass->getLabel());
                QToolTip::add
                    (rec.m_plugins[index], pluginClass->getLabel());

                pluginBgColour = pluginClass->getColour();
            }


            rec.m_plugins[index]->setPaletteForegroundColor(QColor(Qt::white));
            rec.m_plugins[index]->setPaletteBackgroundColor(pluginBgColour);
        }
    } else if (id > 0 && id <= m_submasters.size()) {

        FaderRec &rec = m_submasters[id - 1];
        if (!rec.m_populated || !rec.m_pluginBox)
            return ;
        if (index >= rec.m_plugins.size())
            return ;

        if (plugin == -1) {

            rec.m_plugins[index]->setText(i18n("<none>"));
            QToolTip::add
                (rec.m_plugins[index], i18n("<no plugin>"));

            rec.m_plugins[index]->setPaletteBackgroundColor
            (qApp->palette().
             color(QPalette::Active, QColorGroup::Button));

        } else {

            AudioPlugin *pluginClass
            = m_document->getPluginManager()->getPlugin(plugin);

            QColor pluginBgColour =
                qApp->palette().color(QPalette::Active, QColorGroup::Light);

            if (pluginClass) {
                rec.m_plugins[index]->
                setText(pluginClass->getLabel());
                QToolTip::add
                    (rec.m_plugins[index], pluginClass->getLabel());

                pluginBgColour = pluginClass->getColour();
            }


            rec.m_plugins[index]->setPaletteForegroundColor(QColor(Qt::white));
            rec.m_plugins[index]->setPaletteBackgroundColor(pluginBgColour);
        }
    }
}

void
AudioMixerWindow::slotPluginBypassed(InstrumentId instrumentId,
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
        return ;

    } else if (id >= (int)AudioInstrumentBase) {

        FaderRec &rec = m_faders[id];
        if (!rec.m_populated)
            return ;
        Instrument *instrument = m_studio->getInstrumentById(id);

        rec.m_fader->blockSignals(true);
        rec.m_fader->setFader(instrument->getLevel());
        rec.m_fader->blockSignals(false);

        rec.m_pan->blockSignals(true);
        rec.m_pan->setPosition(instrument->getPan() - 100);
        rec.m_pan->blockSignals(false);

    } else {

        FaderRec &rec = (id == 0 ? m_master : m_submasters[id - 1]);
        BussList busses = m_studio->getBusses();
        Buss *buss = busses[id];

        rec.m_fader->blockSignals(true);
        rec.m_fader->setFader(buss->getLevel());
        rec.m_fader->blockSignals(false);

        if (rec.m_pan) {
            rec.m_pan->blockSignals(true);
            rec.m_pan->setPosition(buss->getPan() - 100);
            rec.m_pan->blockSignals(false);
        }
    }
}

void
AudioMixerWindow::updateRouteButtons(int id)
{
    if (id >= (int)AudioInstrumentBase) {
        FaderRec &rec = m_faders[id];
        if (!rec.m_populated)
            return ;
        if (rec.m_input)
            rec.m_input->slotRepopulate();
        rec.m_output->slotRepopulate();
    }
}

void
AudioMixerWindow::updateStereoButton(int id)
{
    if (id >= (int)AudioInstrumentBase) {

        FaderRec &rec = m_faders[id];
        if (!rec.m_populated)
            return ;
        Instrument *i = m_studio->getInstrumentById(id);

        bool stereo = (i->getAudioChannels() > 1);
        if (stereo == rec.m_stereoness)
            return ;

        rec.m_stereoness = stereo;

        if (stereo)
            rec.m_stereoButton->setPixmap(m_stereoPixmap);
        else
            rec.m_stereoButton->setPixmap(m_monoPixmap);
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
    PluginContainer *container = 0;

    if (id >= (int)AudioInstrumentBase) {

        container = m_studio->getInstrumentById(id);
        rec = &m_faders[id];
        if (!rec->m_populated || !rec->m_pluginBox)
            return ;

    } else {

        BussList busses = m_studio->getBusses();
        if (busses.size() > id) {
            container = busses[id];
        }
        rec = &m_submasters[id - 1];
        if (!rec->m_populated || !rec->m_pluginBox)
            return ;
    }

    if (rec && container) {

        for (unsigned int i = 0; i < rec->m_plugins.size(); i++) {

            bool used = false;
            bool bypass = false;
            QColor pluginBgColour =
                qApp->palette().color(QPalette::Active, QColorGroup::Light);

            rec->m_plugins[i]->show();

            AudioPluginInstance *inst = container->getPlugin(i);

            if (inst && inst->isAssigned()) {

                AudioPlugin *pluginClass
                = m_document->getPluginManager()->getPlugin(
                      m_document->getPluginManager()->
                      getPositionByIdentifier(inst->getIdentifier().c_str()));

                if (pluginClass) {
                    rec->m_plugins[i]->setText(pluginClass->getLabel());
                    QToolTip::add
                        (rec->m_plugins[i], pluginClass->getLabel());

                    pluginBgColour = pluginClass->getColour();
                }

                used = true;
                bypass = inst->isBypassed();

            } else {

                rec->m_plugins[i]->setText(i18n("<none>"));
                QToolTip::add
                    (rec->m_plugins[i], i18n("<no plugin>"));

                if (inst)
                    bypass = inst->isBypassed();
            }

            if (bypass) {

                rec->m_plugins[i]->setPaletteForegroundColor
                (qApp->palette().
                 color(QPalette::Active, QColorGroup::Button));

                rec->m_plugins[i]->setPaletteBackgroundColor
                (qApp->palette().
                 color(QPalette::Active, QColorGroup::ButtonText));

            } else if (used) {

                rec->m_plugins[i]->setPaletteForegroundColor(QColor(Qt::white));
                rec->m_plugins[i]->setPaletteBackgroundColor(pluginBgColour);


            } else {

                rec->m_plugins[i]->setPaletteForegroundColor
                (qApp->palette().
                 color(QPalette::Active, QColorGroup::ButtonText));

                rec->m_plugins[i]->setPaletteBackgroundColor
                (qApp->palette().
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
        if (!i->second.m_populated || !i->second.m_pluginBox)
            continue;

        for (std::vector<QPushButton *>::iterator pli = i->second.m_plugins.begin();
                pli != i->second.m_plugins.end(); ++pli) {

            if (*pli == s) {

                emit selectPlugin(this, i->first, index);
                return ;
            }

            ++index;
        }
    }


    int b = 1;

    for (FaderVector::iterator i = m_submasters.begin();
            i != m_submasters.end(); ++i) {

        int index = 0;
        if (!i->m_populated || !i->m_pluginBox)
            continue;

        for (std::vector<QPushButton *>::iterator pli = i->m_plugins.begin();
                pli != i->m_plugins.end(); ++pli) {

            if (*pli == s) {

                emit selectPlugin(this, b, index);
                return ;
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

        if (i->second.m_input == s)
            emit instrumentParametersChanged(i->first);
    }
}

void
AudioMixerWindow::slotOutputChanged()
{
    const QObject *s = sender();

    for (FaderMap::iterator i = m_faders.begin();
            i != m_faders.end(); ++i) {

        if (i->second.m_output == s)
            emit instrumentParametersChanged(i->first);
    }
}

void
AudioMixerWindow::sendControllerRefresh()
{
    //!!! really want some notification of whether we have an external controller!
    int controllerChannel = 0;

    for (FaderMap::iterator i = m_faders.begin(); i != m_faders.end(); ++i) {

        if (controllerChannel >= 16)
            break;

        Instrument *instrument =
            m_studio->getInstrumentById(i->first);

        int value = AudioLevel::dB_to_fader
                    (instrument->getLevel(), 127, AudioLevel::LongFader);
        MappedEvent mE(instrument->getId(),
                       MappedEvent::MidiController,
                       MIDI_CONTROLLER_VOLUME,
                       MidiByte(value));
        mE.setRecordedChannel(controllerChannel);
        mE.setRecordedDevice(Device::CONTROL_DEVICE);
        StudioControl::sendMappedEvent(mE);

        int ipan = (int(instrument->getPan()) * 64) / 100;
        if (ipan < 0)
            ipan = 0;
        if (ipan > 127)
            ipan = 127;
        MappedEvent mEp(instrument->getId(),
                        MappedEvent::MidiController,
                        MIDI_CONTROLLER_PAN,
                        MidiByte(ipan));
        mEp.setRecordedChannel(controllerChannel);
        mEp.setRecordedDevice(Device::CONTROL_DEVICE);
        StudioControl::sendMappedEvent(mEp);

        ++controllerChannel;
    }
}

void
AudioMixerWindow::slotFaderLevelChanged(float dB)
{
    const QObject *s = sender();

    BussList busses = m_studio->getBusses();

    if (m_master.m_fader == s) {

        if (busses.size() > 0) {
            StudioControl::setStudioObjectProperty
            (MappedObjectId(busses[0]->getMappedId()),
             MappedAudioBuss::Level,
             MappedObjectValue(dB));
            busses[0]->setLevel(dB);
        }

        return ;
    }

    int index = 1;

    for (FaderVector::iterator i = m_submasters.begin();
            i != m_submasters.end(); ++i) {

        if (i->m_fader == s) {
            if ((int)busses.size() > index) {
                StudioControl::setStudioObjectProperty
                (MappedObjectId(busses[index]->getMappedId()),
                 MappedAudioBuss::Level,
                 MappedObjectValue(dB));
                busses[index]->setLevel(dB);
            }

            return ;
        }

        ++index;
    }

    int controllerChannel = 0;

    for (FaderMap::iterator i = m_faders.begin();
            i != m_faders.end(); ++i) {

        if (i->second.m_fader == s) {

            Instrument *instrument =
                m_studio->getInstrumentById(i->first);

            if (instrument) {
                StudioControl::setStudioObjectProperty
                (MappedObjectId
                 (instrument->getMappedId()),
                 MappedAudioFader::FaderLevel,
                 MappedObjectValue(dB));
                instrument->setLevel(dB);
            }

            // send out to external controllers as well.
            //!!! really want some notification of whether we have any!
            if (controllerChannel < 16) {
                int value = AudioLevel::dB_to_fader
                            (dB, 127, AudioLevel::LongFader);
                MappedEvent mE(instrument->getId(),
                               MappedEvent::MidiController,
                               MIDI_CONTROLLER_VOLUME,
                               MidiByte(value));
                mE.setRecordedChannel(controllerChannel);
                mE.setRecordedDevice(Device::CONTROL_DEVICE);
                StudioControl::sendMappedEvent(mE);
            }

            emit instrumentParametersChanged(i->first);
        }

        ++controllerChannel;
    }
}

void
AudioMixerWindow::slotPanChanged(float pan)
{
    const QObject *s = sender();

    BussList busses = m_studio->getBusses();

    int index = 1;

    for (FaderVector::iterator i = m_submasters.begin();
            i != m_submasters.end(); ++i) {

        if (i->m_pan == s) {
            if ((int)busses.size() > index) {
                StudioControl::setStudioObjectProperty
                (MappedObjectId(busses[index]->getMappedId()),
                 MappedAudioBuss::Pan,
                 MappedObjectValue(pan));
                busses[index]->setPan(MidiByte(pan + 100.0));
            }
            return ;
        }

        ++index;
    }

    int controllerChannel = 0;

    for (FaderMap::iterator i = m_faders.begin();
            i != m_faders.end(); ++i) {

        if (i->second.m_pan == s) {

            Instrument *instrument =
                m_studio->getInstrumentById(i->first);

            if (instrument) {
                StudioControl::setStudioObjectProperty
                (instrument->getMappedId(),
                 MappedAudioFader::Pan,
                 MappedObjectValue(pan));
                instrument->setPan(MidiByte(pan + 100.0));
            }

            // send out to external controllers as well.
            //!!! really want some notification of whether we have any!
            if (controllerChannel < 16) {
                int ipan = (int(instrument->getPan()) * 64) / 100;
                if (ipan < 0)
                    ipan = 0;
                if (ipan > 127)
                    ipan = 127;
                MappedEvent mE(instrument->getId(),
                               MappedEvent::MidiController,
                               MIDI_CONTROLLER_PAN,
                               MidiByte(ipan));
                mE.setRecordedChannel(controllerChannel);
                mE.setRecordedDevice(Device::CONTROL_DEVICE);
                StudioControl::sendMappedEvent(mE);
            }

            emit instrumentParametersChanged(i->first);
        }

        ++controllerChannel;
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

            Instrument *instrument =
                m_studio->getInstrumentById(i->first);

            if (instrument) {
                instrument->setAudioChannels
                ((instrument->getAudioChannels() > 1) ? 1 : 2);
                updateStereoButton(instrument->getId());
                updateRouteButtons(instrument->getId());

                emit instrumentParametersChanged(i->first);

                return ;
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

        InstrumentId id = i->first;
        FaderRec &rec = i->second;
        if (!rec.m_populated)
            continue;

        LevelInfo info;

        if (mapper->getInstrumentLevelForMixer(id, info)) {

            // The values passed through are long-fader values
            float dBleft = AudioLevel::fader_to_dB
                           (info.level, 127, AudioLevel::LongFader);

            if (rec.m_stereoness) {
                float dBright = AudioLevel::fader_to_dB
                                (info.levelRight, 127, AudioLevel::LongFader);

                rec.m_meter->setLevel(dBleft, dBright);

            } else {
                rec.m_meter->setLevel(dBleft);
            }
        }
    }

    for (unsigned int i = 0; i < m_submasters.size(); ++i) {

        FaderRec &rec = m_submasters[i];

        LevelInfo info;
        if (!mapper->getSubmasterLevel(i, info))
            continue;

        // The values passed through are long-fader values
        float dBleft = AudioLevel::fader_to_dB
                       (info.level, 127, AudioLevel::LongFader);
        float dBright = AudioLevel::fader_to_dB
                        (info.levelRight, 127, AudioLevel::LongFader);

        rec.m_meter->setLevel(dBleft, dBright);
    }

    updateMonitorMeters(mapper);

    LevelInfo masterInfo;
    if (mapper->getMasterLevel(masterInfo)) {

        float dBleft = AudioLevel::fader_to_dB
                       (masterInfo.level, 127, AudioLevel::LongFader);
        float dBright = AudioLevel::fader_to_dB
                        (masterInfo.levelRight, 127, AudioLevel::LongFader);

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
        return ;
    }

    Composition &comp = m_document->getComposition();
    Composition::trackcontainer &tracks = comp.getTracks();

    for (FaderMap::iterator i = m_faders.begin(); i != m_faders.end(); ++i) {

        InstrumentId id = i->first;
        FaderRec &rec = i->second;
        if (!rec.m_populated)
            continue;

        LevelInfo info;

        if (mapper->getInstrumentRecordLevelForMixer(id, info)) {

            bool armed = false;

            for (Composition::trackcontainer::iterator ti =
                        tracks.begin(); ti != tracks.end(); ++ti) {
                if (ti->second->getInstrument() == id) {
                    if (comp.isTrackRecording(ti->second->getId())) {
                        armed = true;
                        break;
                    }
                }
            }

            if (!armed)
                continue;

            // The values passed through are long-fader values
            float dBleft = AudioLevel::fader_to_dB
                           (info.level, 127, AudioLevel::LongFader);

            if (rec.m_stereoness) {
                float dBright = AudioLevel::fader_to_dB
                                (info.levelRight, 127, AudioLevel::LongFader);

                rec.m_meter->setRecordLevel(dBleft, dBright);

            } else {
                rec.m_meter->setRecordLevel(dBleft);
            }
        }
    }
}

void
AudioMixerWindow::slotControllerDeviceEventReceived(MappedEvent *e,
        const void *preferredCustomer)
{
    if (preferredCustomer != this)
        return ;
    RG_DEBUG << "AudioMixerWindow::slotControllerDeviceEventReceived: this one's for me" << endl;
    raise();

    // get channel number n from event
    // update instrument for nth fader in m_faders

    if (e->getType() != MappedEvent::MidiController)
        return ;
    unsigned int channel = e->getRecordedChannel();
    MidiByte controller = e->getData1();
    MidiByte value = e->getData2();

    int count = 0;
    for (FaderMap::iterator i = m_faders.begin(); i != m_faders.end(); ++i) {

        if (count < channel) {
            ++count;
            continue;
        }

        Instrument *instrument =
            m_studio->getInstrumentById(i->first);
        if (!instrument)
            continue;

        switch (controller) {

        case MIDI_CONTROLLER_VOLUME: {
                float level = AudioLevel::fader_to_dB
                              (value, 127, AudioLevel::LongFader);

                StudioControl::setStudioObjectProperty
                (instrument->getMappedId(),
                 MappedAudioFader::FaderLevel,
                 MappedObjectValue(level));

                instrument->setLevel(level);
                break;
            }

        case MIDI_CONTROLLER_PAN: {
                MidiByte ipan = MidiByte((value / 64.0) * 100.0 + 0.01);

                StudioControl::setStudioObjectProperty
                (instrument->getMappedId(),
                 MappedAudioFader::Pan,
                 MappedObjectValue(float(ipan) - 100.0));

                instrument->setPan(ipan);
                break;
            }

        default:
            break;
        }

        slotUpdateInstrument(i->first);
        emit instrumentParametersChanged(i->first);

        break;
    }
}

void
AudioMixerWindow::slotSetInputCountFromAction()
{
    const QObject *s = sender();
    QString name = s->objectName();

    if (name.left(7) == "inputs_") {

        int count = name.right(name.length() - 7).toInt();

        RecordInList ins = m_studio->getRecordIns();
        int current = ins.size();

        if (count == current)
            return ;

        m_studio->clearRecordIns(); // leaves the default 1

        for (int i = 1; i < count; ++i) {
            m_studio->addRecordIn(new RecordIn());
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
    QString name = s->objectName();

    if (name.left(11) == "submasters_") {

        int count = name.right(name.length() - 11).toInt();

        BussList busses = m_studio->getBusses();
        int current = busses.size();

        // offset by 1 generally to take into account the fact that
        // the first buss in the studio is the master, not a submaster

        if (count + 1 == current)
            return ;

        BussList dups;
        for (int i = 0; i < count; ++i) {
            if (i + 1 < int(busses.size())) {
                dups.push_back(new Buss(*busses[i + 1]));
            } else {
                dups.push_back(new Buss(i + 1));
            }
        }

        m_studio->clearBusses();

        for (BussList::iterator i = dups.begin();
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
        if (m_input)
            m_input->getWidget()->show();
        if (m_output)
            m_output->getWidget()->show();
        if (m_pan)
            m_pan->show();
        if (m_fader)
            m_fader->show();
        if (m_meter)
            m_meter->show();
        // commented out until implemented
        //         if (m_muteButton)   m_muteButton->show();
        //         if (m_soloButton)   m_soloButton->show();
        //         if (m_recordButton) m_recordButton->show();
        if (m_stereoButton)
            m_stereoButton->show();

    } else {

        if (m_input)
            m_input->getWidget()->hide();
        if (m_output)
            m_output->getWidget()->hide();
        if (m_pan)
            m_pan->hide();
        if (m_fader)
            m_fader->hide();
        if (m_meter)
            m_meter->hide();
        // commented out until implemented
        //         if (m_muteButton)   m_muteButton->hide();
        //         if (m_soloButton)   m_soloButton->hide();
        //         if (m_recordButton) m_recordButton->hide();
        if (m_stereoButton)
            m_stereoButton->hide();
    }

    setPluginButtonsVisible(visible);

}

void
AudioMixerWindow::FaderRec::setPluginButtonsVisible(bool visible)
{
    if (!m_pluginBox)
        return ;

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
        if (i->first < SoftSynthInstrumentBase) {
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
    if (!action)
        return ;

    action->setChecked(!(m_studio->getMixerDisplayOptions() &
                         MIXER_OMIT_SYNTH_FADERS));

    for (FaderMap::iterator i = m_faders.begin(); i != m_faders.end(); ++i) {
        if (i->first >= SoftSynthInstrumentBase) {
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
    if (!action)
        return ;

    action->setChecked(!(m_studio->getMixerDisplayOptions() &
                         MIXER_OMIT_SUBMASTERS));

    for (FaderVector::iterator i = m_submasters.begin(); i != m_submasters.end(); ++i) {
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
    if (!action)
        return ;

    action->setChecked(!(m_studio->getMixerDisplayOptions() &
                         MIXER_OMIT_PLUGINS));

    for (FaderMap::iterator i = m_faders.begin(); i != m_faders.end(); ++i) {
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
    if (!action)
        return ;

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
        QWidget * widget = child->widget();
        if (widget && widget->objectName() && !strcmp(widget->objectName(), name)) {
            if (show)
                widget->show();
            else
                widget->hide();
        }

        ++it;
    }

}

}
#include "AudioMixerWindow.moc"
