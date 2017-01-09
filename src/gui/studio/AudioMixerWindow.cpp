/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2017 the Rosegarden development team.
 
    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.
 
    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#define RG_MODULE_STRING "[AudioMixerWindow]"

#include "AudioMixerWindow.h"

#include "AudioPlugin.h"
#include "AudioPluginManager.h"
#include "MixerWindow.h"
#include "StudioControl.h"
#include "sound/Midi.h"
#include "sound/SequencerDataBlock.h"
#include "misc/Debug.h"
#include "gui/application/TransportStatus.h"
#include "base/AudioLevel.h"
#include "base/AudioPluginInstance.h"
#include "base/Composition.h"
#include "base/Device.h"
#include "base/Instrument.h"
#include "base/InstrumentStaticSignals.h"
#include "base/MidiProgram.h"
#include "base/Studio.h"
#include "document/RosegardenDocument.h"
#include "gui/editors/notation/NotePixmapFactory.h"
#include "gui/general/GUIPalette.h"
#include "gui/general/IconLoader.h"
#include "gui/general/ActionFileClient.h"
#include "misc/Strings.h"
#include "gui/seqmanager/SequenceManager.h"
#include "gui/widgets/AudioRouteMenu.h"
#include "gui/widgets/AudioVUMeter.h"
#include "gui/widgets/Fader.h"
#include "gui/widgets/Rotary.h"
#include "gui/widgets/VUMeter.h"
#include "sound/MappedCommon.h"
#include "sound/MappedEvent.h"
#include "sound/MappedStudio.h"
#include "gui/widgets/PluginPushButton.h"
#include "gui/widgets/InstrumentAliasButton.h"
#include "gui/dialogs/AboutDialog.h"

#include <QLayout>
#include <QApplication>
#include <QMainWindow>
#include <QShortcut>
#include <QAction>
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
#include <QGroupBox>
#include <QDesktopServices>
#include <QToolBar>
#include <QToolButton>
#include <QtGlobal>

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
                                   RosegardenDocument *document):
        MixerWindow(parent, document),
        m_surroundBoxLayout(0),
        m_mainBox (0)
{
    setObjectName("MixerWindow");

    createAction("file_close", SLOT(slotClose()));
    
    createAction("play", SIGNAL(play()));
    createAction("stop", SIGNAL(stop()));
    createAction("playback_pointer_back_bar", SIGNAL(rewindPlayback()));
    createAction("playback_pointer_forward_bar", SIGNAL(fastForwardPlayback()));
    createAction("playback_pointer_start", SIGNAL(rewindPlaybackToBeginning()));
    createAction("playback_pointer_end", SIGNAL(fastForwardPlaybackToEnd()));
    createAction("record", SIGNAL(record()));
    createAction("panic", SIGNAL(panic()));
    createAction("mixer_help", SLOT(slotHelpRequested()));
    createAction("help_about_app", SLOT(slotHelpAbout()));

    unsigned int mixerOptions = m_studio->getMixerDisplayOptions();

    createAction("show_audio_faders", SLOT(slotToggleFaders()))
        ->setChecked(!(mixerOptions & MIXER_OMIT_FADERS));

    createAction("show_synth_faders", SLOT(slotToggleSynthFaders()))
        ->setChecked(!(mixerOptions & MIXER_OMIT_SYNTH_FADERS));

    createAction("show_audio_submasters", SLOT(slotToggleSubmasters()))
        ->setChecked(!(mixerOptions & MIXER_OMIT_SUBMASTERS));

    createAction("show_plugin_buttons", SLOT(slotTogglePluginButtons()))
        ->setChecked(!(mixerOptions & MIXER_OMIT_PLUGINS));

    RG_DEBUG << "AudioMixerWindow::CTOR: action \"show_plugin_buttons\" has been created.  State should be: "
             << ( !(mixerOptions & MIXER_OMIT_PLUGINS) ? "true" : "false");

    createAction("show_unassigned_faders", SLOT(slotToggleUnassignedFaders()))
        ->setChecked(mixerOptions & MIXER_SHOW_UNASSIGNED_FADERS);

    QAction *ri_action[17];
    for (int i = 0; i < 17; i++){
      ri_action[i] = 0;
    }

    for (int i = 1; i <= 16; i *= 2) {
        ri_action[i] = createAction
            (QString("inputs_%1").arg(i), SLOT(slotSetInputCountFromAction()));
    }

    QAction *sm_action[9];
    for (int i = 0; i < 9; i++){
      sm_action[i] = 0;
    }

    createAction("submasters_0", SLOT(slotSetSubmasterCountFromAction()));
    
    for (int i = 2; i <= 8; i *= 2) {
        sm_action[i] = createAction
            (QString("submasters_%1").arg(i), SLOT(slotSetSubmasterCountFromAction()));
    }

    QAction *pl_action[4];
    for (int i = 0; i < 4; i++){
        pl_action[i] = 0;
    }

    for (int i = 0; i < 4; i++){
        pl_action[i] = createAction
                (QString("panlaw_%1").arg(i), SLOT(slotSetPanLaw()));
    }

    createGUI("mixer.rc");

    // The action->setChecked() stuff must be done after createGUI("mixer.rc").
    for (int i = 1; i <= 16; i *= 2) {
        if (i == int(m_studio->getRecordIns().size())) {
            ri_action[i]->setChecked(true);
        }
    }
    // "submasters_0" is checked by default in data/rc/mixer.rc
    for (int i = 2; i <= 8; i *= 2) {
        if (i == int(m_studio->getBusses().size()) - 1) {
            sm_action[i]->setChecked(true);
        }
    }

    for (int i = 0; i < 4; i++) {
        if (i == AudioLevel::getPanLaw()) {
            pl_action[i]->setChecked(true);
        }
    }

    setRewFFwdToAutoRepeat();

    // We must populate AFTER the actions are created, or else all the
    // action->isChecked() based tests will use a default false action on the
    // first pass here in the ctor.  This is apparently a change from KDE/Qt3.
    //
    // This has the interesting side effect that the menu bar comes out with a
    // much smaller font than normal, as do all the buttons.  (The button fonts
    // returned to normal after I implemented Rosegarden::PluginPushButton.  Oh
    // well.)
    populate();

    connect(Instrument::getStaticSignals().data(),
            SIGNAL(changed(Instrument *)),
            this,
            SLOT(slotInstrumentChanged(Instrument *)));
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
        m_surroundBox = new QGroupBox(this);
        m_surroundBoxLayout = new QHBoxLayout;
        m_surroundBox->setLayout(m_surroundBoxLayout);
        setCentralWidget(m_surroundBox);
    }

    QFont font;
//    font.setPointSize(font.pointSize() * 8 / 10);
    font.setPointSize(6);
    font.setBold(false);
    setFont(font);

    QFont boldFont(font);
    boldFont.setBold(true);

    m_mainBox = new QFrame(m_surroundBox);
    m_surroundBoxLayout->addWidget(m_mainBox);

    InstrumentList instruments = m_studio->getPresentationInstruments();
    BussList busses = m_studio->getBusses();

    IconLoader il;
    
    // perhaps due to the compression enacted through the stylesheet, the icons
    // on these buttons were bug eyed monstrosities, so I created an alternate
    // set for use here
    m_monoPixmap = il.loadPixmap("mono-tiny");
    m_stereoPixmap = il.loadPixmap("stereo-tiny");

    // Total cols: is 2 for each fader, submaster or master, plus 1
    // for each spacer.
    QGridLayout *mainLayout = new QGridLayout(m_mainBox);

    setWindowTitle(tr("Audio Mixer"));
    setWindowIcon(IconLoader().loadPixmap("window-audiomixer"));

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
            rec.m_input->getWidget()->setToolTip(tr("Record input source"));
            rec.m_input->getWidget()->setMaximumWidth(45);
        } else {
            rec.m_input = 0;
        }

        rec.m_output = new AudioRouteMenu(m_mainBox,
                                          AudioRouteMenu::Out,
                                          AudioRouteMenu::Compact,
                                          m_studio, *i);
        rec.m_output->getWidget()->setToolTip(tr("Output destination"));
        rec.m_output->getWidget()->setMaximumWidth(45);

        rec.m_pan = new Rotary
                    (m_mainBox, -100.0, 100.0, 1.0, 5.0, 0.0, 20,
                     Rotary::NoTicks, false, true);

        if ((*i)->getType() == Instrument::Audio) {
            rec.m_pan->setKnobColour(GUIPalette::getColour(GUIPalette::RotaryPastelGreen));
        } else {
            rec.m_pan->setKnobColour(GUIPalette::getColour(GUIPalette::RotaryPastelYellow));
        }

        rec.m_pan->setToolTip(tr("Pan"));

        rec.m_fader = new Fader
                      (AudioLevel::LongFader, 20, 240, m_mainBox);
        rec.m_meter = new AudioVUMeter
                      (m_mainBox, VUMeter::AudioPeakHoldIECLong, true, rec.m_input != 0,
                       20, 240);

        rec.m_fader->setToolTip(tr("Audio level"));
        rec.m_meter->setToolTip(tr("Audio level"));

        rec.m_stereoButton = new QPushButton(m_mainBox);
        rec.m_stereoButton->setIcon(m_monoPixmap);
        rec.m_stereoButton->setFixedSize(20, 20);
        rec.m_stereoButton->setFlat(true);
        rec.m_stereoness = false;
        rec.m_stereoButton->setToolTip(tr("Mono or stereo"));

        rec.m_recordButton = new QPushButton(m_mainBox);
        rec.m_recordButton->setText("R");
        rec.m_recordButton->setCheckable(true);
        rec.m_recordButton->setFixedWidth(rec.m_stereoButton->width());
        rec.m_recordButton->setFixedHeight(rec.m_stereoButton->height());
        rec.m_recordButton->setFlat(true);
        rec.m_recordButton->setToolTip(tr("Arm recording"));

        rec.m_pluginBox = new QWidget(m_mainBox);
        QVBoxLayout *pluginBoxLayout = new QVBoxLayout;

        for (int p = 0; p < 5; ++p) {
            PluginPushButton *plugin = new PluginPushButton(rec.m_pluginBox);
            pluginBoxLayout->addWidget(plugin);
            QFont font;
            font.setPointSize(6);
            plugin->setFont(font);
            plugin->setText(tr("<none>"));
            plugin->setMaximumWidth(45);
            plugin->setToolTip(tr("Click to load an audio plugin"));
            rec.m_plugins.push_back(plugin);
            connect(plugin, SIGNAL(clicked()),
                    this, SLOT(slotSelectPlugin()));
        }

        rec.m_pluginBox->setLayout(pluginBoxLayout);
        rec.m_pluginBox->show();

        InstrumentAliasButton *aliasButton = new InstrumentAliasButton(m_mainBox, (*i));
        aliasButton->setFixedSize(10, 6); // golden rectangle
        aliasButton->setToolTip(tr("Click to rename this instrument"));
        connect (aliasButton, SIGNAL(changed()), this, SLOT(slotRepopulate()));
        mainLayout->addWidget(aliasButton, 0, col, 1, 2, Qt::AlignLeft);

        // use the instrument alias if one is set, else a standard label
        std::string alias = (*i)->getAlias();
        QString idString;
        QLabel *idLabel = NULL;

        //NB. The objectName property is used to address widgets in a nice piece
        // of old school Qt2 style faffery, so we DO need to set these.
        if ((*i)->getType() == Instrument::Audio) {
            // use the instrument alias if one is set, else a standard label
            if (alias.size()) {
                idString = strtoqstr(alias);
            } else {
                idString = tr("Audio %1").arg((*i)->getId() - AudioInstrumentBase + 1);
            }
            idLabel = new QLabel(idString, m_mainBox);
            idLabel->setObjectName("audioIdLabel");
        } else {
            // use the instrument alias if one is set, else a standard label
            if (alias.size()) {
                idString = strtoqstr(alias);
            } else {
                idString = tr("Synth %1").arg((*i)->getId() - SoftSynthInstrumentBase + 1);
            }
            idLabel = new QLabel(idString, m_mainBox);
            idLabel->setObjectName("synthIdLabel");
        }

        Q_ASSERT(idLabel);

        idLabel->setFont(boldFont);
        idLabel->setToolTip(tr("Click the button above to rename this instrument"));

        if (rec.m_input) {
            mainLayout->addWidget(rec.m_input->getWidget(), 2, col, 1, 2);
        }
        mainLayout->addWidget(rec.m_output->getWidget(), 3, col, 1, 2);

        mainLayout->addWidget(idLabel, 1, col, 1, 2, Qt::AlignLeft);
        mainLayout->addWidget(rec.m_pan, 6, col, Qt::AlignCenter);

        mainLayout->addWidget(rec.m_fader, 4, col, Qt::AlignCenter);
        mainLayout->addWidget(rec.m_meter, 4, col + 1, Qt::AlignCenter);

        rec.m_recordButton->hide();
        mainLayout->addWidget(rec.m_stereoButton, 6, col + 1);

        if (rec.m_pluginBox) {
            mainLayout->addWidget(rec.m_pluginBox, 7, col, 1, 2);
        }

        m_faders[(*i)->getId()] = rec;
        updateFader((*i)->getId());
        updateRouteButtons((*i)->getId());
        updateStereoButton((*i)->getId());
        updatePluginButtons((*i)->getId());

        connect(rec.m_fader, SIGNAL(faderChanged(float)),
                this, SLOT(slotFaderLevelChanged(float)));

        connect(rec.m_pan, SIGNAL(valueChanged(float)),
                this, SLOT(slotPanChanged(float)));

        connect(rec.m_stereoButton, SIGNAL(clicked()),
                this, SLOT(slotChannelsChanged()));

        connect(rec.m_recordButton, SIGNAL(clicked()),
                this, SLOT(slotRecordChanged()));

        ++count;

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

        rec.m_pan->setToolTip(tr("Pan"));

        rec.m_fader = new Fader
                      (AudioLevel::LongFader, 20, 240, m_mainBox);
        rec.m_meter = new AudioVUMeter
                      (m_mainBox, VUMeter::AudioPeakHoldIECLong, true, false, 20, 240);

        rec.m_fader->setToolTip(tr("Audio level"));
        rec.m_meter->setToolTip(tr("Audio level"));

        rec.m_pluginBox = new QWidget(m_mainBox);
        QVBoxLayout *pluginBoxLayout = new QVBoxLayout;

        for (int p = 0; p < 5; ++p) {
            PluginPushButton *plugin = new PluginPushButton(rec.m_pluginBox);
            pluginBoxLayout->addWidget(plugin);
            QFont font;
            font.setPointSize(6);
            plugin->setFont(font);
            plugin->setText(tr("<none>"));
            plugin->setMaximumWidth(45);
            plugin->setToolTip(tr("Click to load an audio plugin"));
            rec.m_plugins.push_back(plugin);
            connect(plugin, SIGNAL(clicked()),
                    this, SLOT(slotSelectPlugin()));
        }

        rec.m_pluginBox->setLayout(pluginBoxLayout);

        QLabel *idLabel = new QLabel(tr("Sub %1").arg(count), m_mainBox);
        idLabel->setFont(boldFont);
        //NB. objectName matters here:
        idLabel->setObjectName("subMaster");

        mainLayout->addWidget(idLabel, 1, col, 1, 2, Qt::AlignLeft);

        mainLayout->addWidget(rec.m_pan, 6, col, 1, 2, Qt::AlignCenter);

        mainLayout->addWidget(rec.m_fader, 4, col, Qt::AlignCenter);
        mainLayout->addWidget(rec.m_meter, 4, col + 1, Qt::AlignCenter);

        if (rec.m_pluginBox) {
            mainLayout->addWidget(rec.m_pluginBox, 7, col, 1, 2);
        }

        m_submasters.push_back(rec);
        updateFader(count);
        updatePluginButtons(count);

        connect(rec.m_fader, SIGNAL(faderChanged(float)),
                this, SLOT(slotFaderLevelChanged(float)));

        connect(rec.m_pan, SIGNAL(valueChanged(float)),
                this, SLOT(slotPanChanged(float)));

        ++count;

        col += 3;
    }

    if (busses.size() > 0) {

        FaderRec rec;
        rec.m_populated = true;

        rec.m_fader = new Fader
                      (AudioLevel::LongFader, 20, 240, m_mainBox);
        rec.m_meter = new AudioVUMeter
                      (m_mainBox, VUMeter::AudioPeakHoldIEC, true, false, 20, 240);

        rec.m_fader->setToolTip(tr("Audio master output level"));
        rec.m_meter->setToolTip(tr("Audio master output level"));

        QLabel *idLabel = new QLabel(tr("Master"), m_mainBox);
        idLabel->setFont(boldFont);

        mainLayout->addWidget(idLabel, 1, col, 1,  2, Qt::AlignLeft);
        mainLayout->addWidget(rec.m_fader, 4, col, Qt::AlignCenter);
        mainLayout->addWidget(rec.m_meter, 4, col + 1, Qt::AlignCenter);

        m_master = rec;
        updateFader(0);

        connect(rec.m_fader, SIGNAL(faderChanged(float)),
                this, SLOT(slotFaderLevelChanged(float)));
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
        // If we are pointing to the correct Instrument.
        bool assigned = isInstrumentAssigned(id);

        if (assigned != i->second.m_populated) {
            // found an inconsistency
            populate();
            return ;
        }
    }
}

void
AudioMixerWindow::slotInstrumentChanged(Instrument *instrument)
{
    if (!instrument)
        return;

    InstrumentId id = instrument->getId();

    RG_DEBUG << "AudioMixerWindow::slotInstrumentChanged(): id = " << id;

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
        if (index >= int(rec.m_plugins.size()))
            return ;

        if (plugin == -1) {

            rec.m_plugins[index]->setText(tr("<none>"));
            rec.m_plugins[index]->setToolTip(tr("<no plugin>"));

            //QT3: color hackery simply ignored here.  
            //
//            rec.m_plugins[index]->setPaletteBackgroundColor
//            (qApp->palette().
//             color(QPalette::Active, QColorGroup::Button));

        } else {

            AudioPlugin *pluginClass = m_document->getPluginManager()->getPlugin(plugin);

            //!!! Hacky.  We still rely on the old "colour" property to figure
            // out the state, instead of doing something far more pleasant and
            // intelligible three years from now.  (Remember this when you wince
            // in 2015.  Kind of like that photo of you wearing nothing but a
            // sock and an electric guitar, drunk off your ass, innit?)
            QColor pluginBgColour = Qt::blue;  // anything random will do

            if (pluginClass) {
                rec.m_plugins[index]->
                setText(pluginClass->getLabel());
                rec.m_plugins[index]->setToolTip(pluginClass->getLabel());

                pluginBgColour = pluginClass->getColour();
            }

            //!!! NB - Before I added more code later on in slotUpdateButtons, I
            // never saw this code here do anything.  I'm not at all sure I've
            // hit on the correct assumptions about what the colo(u)r property
            // was supposed to tell this code here, and this might well be a
            // future source of mysterious bugs.  So far, I can't find any
            // problems though, and I wonder exactly what this code here was
            // really for at all.  If other people would write some comments
            // once in awhile, I might already have a clear idea.  Oh wait,
            // code gazelles don't need to write no stinkin' comments.

            if (pluginBgColour == Qt::darkRed) {
                rec.m_plugins[index]->setState(PluginPushButton::Active);
            } else if (pluginBgColour == Qt::black) {
                rec.m_plugins[index]->setState(PluginPushButton::Bypassed);
            } else {
                rec.m_plugins[index]->setState(PluginPushButton::Normal);
            }

        }
    } else if (id > 0 && id <= (unsigned int)m_submasters.size()) {

        FaderRec &rec = m_submasters[id - 1];
        if (!rec.m_populated || !rec.m_pluginBox)
            return ;
        if (index >= int(rec.m_plugins.size()))
            return ;

        if (plugin == -1) {

            rec.m_plugins[index]->setText(tr("<none>"));
            rec.m_plugins[index]->setToolTip(tr("<no plugin>"));

            //QT3: color hackery just plowed through and glossed over all
            // through here...  It's all too complicated to sort out without
            // being able to run and look at things, so this will just have to
            // be something where we take a look back and figure it out later.

//            rec.m_plugins[index]->setPaletteBackgroundColor
//            (qApp->palette().
//             color(QPalette::Active, QColorGroup::Button));

        } else {

            AudioPlugin *pluginClass
            = m_document->getPluginManager()->getPlugin(plugin);

            QColor pluginBgColour = Qt::yellow; // QT3: junk color replaces following:
//                qApp->palette().color(QPalette::Active, QColorGroup::Light);

            if (pluginClass) {
                rec.m_plugins[index]->
                setText(pluginClass->getLabel());
                rec.m_plugins[index]->setToolTip(pluginClass->getLabel());

                pluginBgColour = pluginClass->getColour();
            }


//            rec.m_plugins[index]->setPaletteForegroundColor(QColor(Qt::white));
//            rec.m_plugins[index]->setPaletteBackgroundColor(pluginBgColour);
        }
    }
    // Force an immediate update of button colors.
    populate();
}

void
AudioMixerWindow::slotPluginBypassed(InstrumentId instrumentId,
                                     int , bool)
{
    RG_DEBUG << "AudioMixerWindow::slotPluginBypassed(" << instrumentId << ")";

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
            rec.m_stereoButton->setIcon(m_stereoPixmap);
        else
            rec.m_stereoButton->setIcon(m_monoPixmap);
    }
}

void
AudioMixerWindow::updateMiscButtons(int)
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
        if (int(busses.size()) > id) {
            container = busses[id];
        }
        rec = &m_submasters[id - 1];
        if (!rec->m_populated || !rec->m_pluginBox)
            return ;
    }

    if (rec && container) {

        for (size_t i = 0; i < rec->m_plugins.size(); i++) {

            bool used = false;
            bool bypass = false;
            QColor pluginBgColour = Qt::green;
//                qApp->palette().color(QPalette::Active, QColorGroup::Light);

            rec->m_plugins[i]->show();

            AudioPluginInstance *inst = container->getPlugin(i);

            if (inst && inst->isAssigned()) {

                AudioPlugin *pluginClass = m_document->getPluginManager()->getPlugin(
                      m_document->getPluginManager()->
                      getPositionByIdentifier(inst->getIdentifier().c_str()));

                if (pluginClass) {
                    rec->m_plugins[i]->setText(pluginClass->getLabel());
                    rec->m_plugins[i]->setToolTip(pluginClass->getLabel());

                    pluginBgColour = pluginClass->getColour();
                }

                used = true;
                bypass = inst->isBypassed();

            } else {

                rec->m_plugins[i]->setText(tr("<none>"));
                rec->m_plugins[i]->setToolTip(tr("<no plugin>"));

                if (inst)
                    bypass = inst->isBypassed();
            }

            if (bypass) {
                rec->m_plugins[i]->setState(PluginPushButton::Bypassed);
            } else if (used) {
                rec->m_plugins[i]->setState(PluginPushButton::Active);
            } else {
                rec->m_plugins[i]->setState(PluginPushButton::Normal);
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

        for (std::vector<PluginPushButton *>::iterator pli = i->second.m_plugins.begin();
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

        for (std::vector<PluginPushButton *>::iterator pli = i->m_plugins.begin();
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
                StudioControl::setStudioObjectProperty(
                        MappedObjectId(instrument->getMappedId()),
                        MappedAudioFader::FaderLevel,
                        MappedObjectValue(dB));
                instrument->setLevel(dB);
                instrument->changed();
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
                StudioControl::setStudioObjectProperty(
                        instrument->getMappedId(),
                        MappedAudioFader::Pan,
                        MappedObjectValue(pan));
                instrument->setPan(MidiByte(pan + 100.0));
                instrument->changed();
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
                instrument->setAudioChannels(
                        (instrument->getAudioChannels() > 1) ? 1 : 2);
                updateStereoButton(instrument->getId());
                updateRouteButtons(instrument->getId());

                instrument->changed();

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
AudioMixerWindow::updateMeters()
{
    for (FaderMap::iterator i = m_faders.begin(); i != m_faders.end(); ++i) {

        InstrumentId id = i->first;
        FaderRec &rec = i->second;
        if (!rec.m_populated)
            continue;

        LevelInfo info;

        if (SequencerDataBlock::getInstance()->
            getInstrumentLevelForMixer(id, info)) {

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
        if (!SequencerDataBlock::getInstance()->getSubmasterLevel(i, info)) {
            continue;
        }

        // The values passed through are long-fader values
        float dBleft = AudioLevel::fader_to_dB
                       (info.level, 127, AudioLevel::LongFader);
        float dBright = AudioLevel::fader_to_dB
                        (info.levelRight, 127, AudioLevel::LongFader);

        rec.m_meter->setLevel(dBleft, dBright);
    }

    updateMonitorMeters();

    LevelInfo masterInfo;
    if (SequencerDataBlock::getInstance()->getMasterLevel(masterInfo)) {

        float dBleft = AudioLevel::fader_to_dB
                       (masterInfo.level, 127, AudioLevel::LongFader);
        float dBright = AudioLevel::fader_to_dB
                        (masterInfo.levelRight, 127, AudioLevel::LongFader);

        m_master.m_meter->setLevel(dBleft, dBright);
    }
}

void
AudioMixerWindow::updateMonitorMeters()
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

        if (SequencerDataBlock::getInstance()->
            getInstrumentRecordLevelForMixer(id, info)) {

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
        return;

    RG_DEBUG << "slotControllerDeviceEventReceived(): this one's for me";

    raise();

    // get channel number n from event
    // update instrument for nth fader in m_faders

    if (e->getType() != MappedEvent::MidiController)
        return ;
    unsigned int channel = e->getRecordedChannel();
    MidiByte controller = e->getData1();
    MidiByte value = e->getData2();

    unsigned int count = 0;
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

        // Inform everyone of the change.  Note that this will also call
        // slotInstrumentChanged() to update the fader.
        instrument->changed();

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

        if (count + 1 < current) {

            BussList::iterator it = busses.end();
            --it;  // Now this actually points to something

            while (count + 1 < current--) {
                m_studio->removeBuss((*it--)->getId());
            }

        } else {

            BussList::iterator it = busses.end();
            --it;
            unsigned int lastId = (*it)->getId();

            while (count + 1 > current++) {
                m_studio->addBuss(new Buss(++lastId));
            }

        }
//      busses = m_studio->getBusses();
//      for (BussList::iterator it = busses.begin(); it != busses.end(); it++)
//          RG_DEBUG << "******* BussId:" << (*it)->getId();
    }

    m_document->initialiseStudio();

    populate();
}

void AudioMixerWindow::slotSetPanLaw()
{
    const QObject *s = sender();
    QString name = s->objectName();

    if (name.left(7) == "panlaw_") {
        int panLaw = name.right(name.length() - 7).toInt();
        AudioLevel::setPanLaw(panLaw);
    }
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

    QAction *action = findAction("show_audio_faders");
    if (action) {
        action->setChecked(d);
    }

    RG_DEBUG << "AudioMixerWindow::slotUpdateFaderVisibility: visiblility is " << d << " (options " << m_studio->getMixerDisplayOptions() << ")";

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
    QAction *action = findAction("show_synth_faders");
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
    QAction *action = findAction("show_audio_submasters");

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
    QAction *action = findAction("show_plugin_buttons");
    if (!action)
        return ;

    action->setChecked(!(m_studio->getMixerDisplayOptions() &
                         MIXER_OMIT_PLUGINS));

    RG_DEBUG << "AudioMixerWindow::slotUpdatePluginButtonVisibility() action->isChecked("
             << (action->isChecked() ? "true" : "false") << ")";

    for (FaderMap::iterator i = m_faders.begin(); i != m_faders.end(); ++i) {
        FaderRec rec = i->second;
        rec.setPluginButtonsVisible(action->isChecked());
    }

    adjustSize();
}

void
AudioMixerWindow::slotToggleUnassignedFaders()
{
    QAction *action = findAction("show_unassigned_faders");
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
    //NB. Completely rewritten to get around the disappearance of
    // QLayoutIterator.
    int i = 0;
    QLayoutItem *child;
    while ((child = layout()->itemAt(i)) != 0) {
        QWidget *widget = child->widget();
        if (widget &&
            widget->objectName() == QString::fromUtf8(name)) {
            if (show)
                widget->show();
            else
                widget->hide();
        }

        ++i;
    }
}

void
AudioMixerWindow::slotRepopulate()
{
    // this destroys everything if it already exists and rebuilds it, so while
    // it's overkill for changing one label, it should get the job done without
    // the need for even more new plumbing
    RG_DEBUG << "POPULATE";
    populate();
}



void
AudioMixerWindow::slotHelpRequested()
{
    // TRANSLATORS: if the manual is translated into your language, you can
    // change the two-letter language code in this URL to point to your language
    // version, eg. "http://rosegardenmusic.com/wiki/doc:audioMixerWindow-es" for the
    // Spanish version. If your language doesn't yet have a translation, feel
    // free to create one.
    QString helpURL = tr("http://rosegardenmusic.com/wiki/doc:audioMixerWindow-en");
    QDesktopServices::openUrl(QUrl(helpURL));
}

void
AudioMixerWindow::slotHelpAbout()
{
    new AboutDialog(this);
}


void
AudioMixerWindow::setRewFFwdToAutoRepeat()
{
    // This one didn't work in Classic either.  Looking at it as a fresh
    // problem, it was tricky.  The QAction has an objectName() of "rewind"
    // but the QToolButton associated with that action has no object name at
    // all.  We kind of have to go around our ass to get to our elbow on
    // this one.
    
    // get pointers to the actual actions    
    QAction *rewAction = findAction("playback_pointer_back_bar");    // rewind
    QAction *ffwAction = findAction("playback_pointer_forward_bar"); // fast forward

    QWidget* transportToolbar = this->findToolbar("Transport Toolbar");

    if (transportToolbar) {

        // get a list of all the toolbar's children (presumably they're
        // QToolButtons, but use this kind of thing with caution on customized
        // QToolBars!)
        QList<QToolButton *> widgets = transportToolbar->findChildren<QToolButton *>();

        // iterate through the entire list of children
        for (QList<QToolButton *>::iterator i = widgets.begin(); i != widgets.end(); ++i) {

            // get a pointer to the button's default action
            QAction *act = (*i)->defaultAction();

            // compare pointers, if they match, we've found the button
            // associated with that action
            //
            // we then have to not only setAutoRepeat() on it, but also connect
            // it up differently from what it got in createAction(), as
            // determined empirically (bleargh!!)
            if (act == rewAction) {
                connect((*i), SIGNAL(clicked()), this, SIGNAL(rewindPlayback()));

            } else if (act == ffwAction) {
                connect((*i), SIGNAL(clicked()), this, SIGNAL(fastForwardPlayback()));

            } else  {
                continue;
            }

            //  Must have found an button to update
            (*i)->removeAction(act);
            (*i)->setAutoRepeat(true);
        }

    }

}


}
