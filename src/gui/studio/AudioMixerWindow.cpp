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
#include "sound/Midi.h"  // MIDI_CONTROLLER_VOLUME, MIDI_CONTROLLER_PAN...
#include "sound/SequencerDataBlock.h"
#include "misc/Debug.h"
#include "base/AudioLevel.h"
#include "base/AudioPluginInstance.h"
#include "base/Composition.h"
#include "base/Device.h"
#include "base/Instrument.h"
#include "base/InstrumentStaticSignals.h"
#include "document/RosegardenDocument.h"
#include "gui/general/GUIPalette.h"
#include "gui/general/IconLoader.h"
#include "gui/general/ActionFileClient.h"
#include "gui/seqmanager/SequenceManager.h"
#include "gui/widgets/AudioRouteMenu.h"
#include "gui/widgets/AudioVUMeter.h"
#include "gui/widgets/Fader.h"
#include "gui/widgets/Rotary.h"
#include "gui/widgets/VUMeter.h"
#include "sound/MappedEvent.h"
#include "gui/widgets/PluginPushButton.h"
#include "gui/widgets/InstrumentAliasButton.h"
#include "gui/dialogs/AboutDialog.h"

#include <QAction>
#include <QColor>
#include <QFont>
#include <QFrame>
#include <QLabel>
#include <QObject>
#include <QPixmap>
#include <QPushButton>
#include <QString>
#include <QWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGroupBox>
#include <QDesktopServices>
#include <QToolBar>
#include <QToolButton>

namespace Rosegarden
{


AudioMixerWindow::AudioMixerWindow(QWidget *parent,
                                   RosegardenDocument *document) :
        MixerWindow(parent, document),
        m_surroundBox(new QGroupBox(this)),
        m_surroundBoxLayout(new QHBoxLayout),
        m_mainBox(0)
{
    // ??? UNUSED.  A search on this string only yields this line.  A better
    //     object name would be "AudioMixerWindow".
    setObjectName("MixerWindow");

    setWindowTitle(tr("Audio Mixer"));
    setWindowIcon(IconLoader().loadPixmap("window-audiomixer"));

    m_surroundBox->setLayout(m_surroundBoxLayout);
    setCentralWidget(m_surroundBox);

    createAction("file_close", SLOT(slotClose()));

    // Transport ToolBar
    createAction("play", SIGNAL(play()));
    createAction("stop", SIGNAL(stop()));
    createAction("playback_pointer_back_bar", SIGNAL(rewindPlayback()));
    createAction("playback_pointer_forward_bar", SIGNAL(fastForwardPlayback()));
    createAction("playback_pointer_start", SIGNAL(rewindPlaybackToBeginning()));
    createAction("playback_pointer_end", SIGNAL(fastForwardPlaybackToEnd()));
    createAction("record", SIGNAL(record()));
    createAction("panic", SIGNAL(panic()));

    // Help > Help
    createAction("mixer_help", SLOT(slotHelpRequested()));
    // Help > About Rosegarden
    createAction("help_about_app", SLOT(slotHelpAbout()));

    // Settings > Show Audio Faders
    createAction("show_audio_faders", SLOT(slotToggleFaders()));

    // Settings > Show Synth Faders
    createAction("show_synth_faders", SLOT(slotToggleSynthFaders()));

    // Settings > Show Audio Submasters
    createAction("show_audio_submasters", SLOT(slotToggleSubmasters()));

    // Settings > Show Plugin Buttons
    createAction("show_plugin_buttons", SLOT(slotTogglePluginButtons()));

    // Settings > Show Unassigned Faders
    createAction("show_unassigned_faders", SLOT(slotToggleUnassignedFaders()));

    // "Settings > Number of Stereo Inputs" Actions
    // For i in {1,2,4,8,16}
    for (int i = 1; i <= 16; i *= 2) {
        createAction(QString("inputs_%1").arg(i),
                     SLOT(slotSetInputCountFromAction()));
    }

    // "Settings > Number of Submasters" Actions
    createAction("submasters_0", SLOT(slotSetSubmasterCountFromAction()));
    createAction("submasters_2", SLOT(slotSetSubmasterCountFromAction()));
    createAction("submasters_4", SLOT(slotSetSubmasterCountFromAction()));
    createAction("submasters_8", SLOT(slotSetSubmasterCountFromAction()));

    // "Settings > Panning Law" Actions
    // ??? Can we do something easier to understand than panlaw_0,
    //     panlaw_1, etc...?  E.g. panlaw_0dB, panlaw_-3dB, panlaw_-6dB,
    //     panlaw_alt-3dB.
    createAction("panlaw_0", SLOT(slotSetPanLaw()));
    createAction("panlaw_1", SLOT(slotSetPanLaw()));
    createAction("panlaw_2", SLOT(slotSetPanLaw()));
    createAction("panlaw_3", SLOT(slotSetPanLaw()));

    createMenusAndToolbars("mixer.rc");

    // The action->setChecked() stuff must be done after createMenusAndToolbars().

    // Update "Settings > Number of Stereo Inputs"
    findAction(QString("inputs_%1").arg(m_studio->getRecordIns().size()))->
            setChecked(true);

    // Update "Settings > Number of Submasters"
    findAction(QString("submasters_%1").arg(m_studio->getBusses().size()-1))->
            setChecked(true);

    // Update "Settings > Panning Law"
    findAction(QString("panlaw_%1").arg(AudioLevel::getPanLaw()))->
            setChecked(true);

    // Set the rewind and fast-forward buttons for auto-repeat.
    enableAutoRepeat("Transport Toolbar", "playback_pointer_back_bar");
    enableAutoRepeat("Transport Toolbar", "playback_pointer_forward_bar");

    IconLoader il;

    // perhaps due to the compression enacted through the stylesheet, the icons
    // on these buttons were bug eyed monstrosities, so I created an alternate
    // set for use here
    m_monoPixmap = il.loadPixmap("mono-tiny");
    m_stereoPixmap = il.loadPixmap("stereo-tiny");

    // Create all the widgets.
    populate();

    connect(Instrument::getStaticSignals().data(),
            SIGNAL(changed(Instrument *)),
            this,
            SLOT(slotInstrumentChanged(Instrument *)));
}

AudioMixerWindow::~AudioMixerWindow()
{
    //RG_DEBUG << "dtor";

    // Destroy all the widgets.
    depopulate();
}

void
AudioMixerWindow::depopulate()
{
    if (!m_mainBox)
        return;

    // All the widgets will be deleted when the main box goes,
    // except that we need to delete the AudioRouteMenus first
    // because they aren't actually widgets but do contain them.

    // For each input strip
    for (StripMap::iterator i = m_inputs.begin();
         i != m_inputs.end();
         ++i) {
        delete i->second.m_input;
        delete i->second.m_output;
    }

    m_inputs.clear();
    m_submasters.clear();

    if (m_surroundBoxLayout)
        m_surroundBoxLayout->removeWidget(m_mainBox);   // Needed ???

    // Delete the main box and all the widgets.
    delete m_mainBox;
    m_mainBox = 0;
}

void
AudioMixerWindow::populate()
{
    //RG_DEBUG << "populate()";

    // If this isn't the first time we've been asked to create/update
    // the widgets,
    if (m_mainBox) {

        // Destroy all the widgets.  Start from scratch.
        depopulate();

    }

    QFont font;
    font.setPointSize(6);
    font.setBold(false);
    setFont(font);

    QFont boldFont(font);
    boldFont.setBold(true);

    m_mainBox = new QFrame(m_surroundBox);
    // ??? Enable these to see the frame.
    //m_mainBox->setFrameStyle(QFrame::Panel | QFrame::Raised);
    //m_mainBox->setLineWidth(2);
    m_surroundBoxLayout->addWidget(m_mainBox);

    // Strips take up 3 columns, 2 for the controls and 1 for the spacer.
    QGridLayout *mainLayout = new QGridLayout(m_mainBox);

    const int numberOfPlugins = 5;

    // Inputs

    int column = 0;

    InstrumentList instruments = m_studio->getPresentationInstruments();

    // For each instrument
    for (InstrumentList::iterator i = instruments.begin();
         i != instruments.end();
         ++i) {
        Instrument *instrument = *i;

        // Not audio or softsynth, try the next.
        if (instrument->getType() != Instrument::Audio  &&
            instrument->getType() != Instrument::SoftSynth)
            continue;

        // If we shouldn't show unassigned faders and this Instrument
        // is unassigned, try the next.
        if (!m_studio->amwShowUnassignedFaders  &&
            !m_document->getComposition().hasTrack(instrument->getId()))
            continue;

        // Create a new Strip.
        Strip &strip = m_inputs[instrument->getId()];

        // Instrument Alias Button (Click to rename)

        // ??? Get rid of this.  This is a usability issue.  Let the user
        //     click directly on the name to change it.  Note that AIPP
        //     uses InstrumentAliasButton as well.  Remove it from there too.
        //     Then remove InstrumentAliasButton from the sourcebase.

        InstrumentAliasButton *aliasButton =
                new InstrumentAliasButton(m_mainBox, instrument);
        aliasButton->setFixedSize(10, 6);  // golden rectangle
        aliasButton->setToolTip(tr("Click to rename this instrument"));

        connect(aliasButton, SIGNAL(changed()), this, SLOT(slotRepopulate()));

        // Label

        QLabel *idLabel = new QLabel(m_mainBox);
        idLabel->setFont(boldFont);
        //idLabel->setToolTip(tr("Click the button above to rename this instrument"));
        // Switching to this since the strip width is frequently too narrow to
        // see e.g. "Synth plugin #2".
        idLabel->setToolTip(strtoqstr(instrument->getAlias()));
        idLabel->setMaximumWidth(45);
        idLabel->setText(strtoqstr(instrument->getAlias()));

        // Audio Instrument
        if (instrument->getType() == Instrument::Audio) {
            // Used by updateFaderVisibility() to show/hide this label.
            idLabel->setObjectName("audioIdLabel");
        } else {  // Softsynth Instrument
            // Used by updateSynthFaderVisibility() to show/hide this label.
            idLabel->setObjectName("synthIdLabel");
        }

        // Input

        if (instrument->getType() == Instrument::Audio) {
            strip.m_input = new AudioRouteMenu(m_mainBox,
                                               AudioRouteMenu::In,
                                               AudioRouteMenu::Compact,
                                               m_studio,
                                               instrument);
            strip.m_input->getWidget()->setToolTip(tr("Record input source"));
            strip.m_input->getWidget()->setMaximumWidth(45);
        } else {
            strip.m_input = 0;
        }

        // Output

        strip.m_output = new AudioRouteMenu(m_mainBox,
                                            AudioRouteMenu::Out,
                                            AudioRouteMenu::Compact,
                                            m_studio,
                                            instrument);
        strip.m_output->getWidget()->setToolTip(tr("Output destination"));
        strip.m_output->getWidget()->setMaximumWidth(45);

        // Fader

        strip.m_fader = new Fader(AudioLevel::LongFader, 20, 240, m_mainBox);
        strip.m_fader->setToolTip(tr("Audio level"));

        connect(strip.m_fader, SIGNAL(faderChanged(float)),
                this, SLOT(slotFaderLevelChanged(float)));

        // Meter

        strip.m_meter = new AudioVUMeter(
                m_mainBox,  // parent
                VUMeter::AudioPeakHoldIECLong,  // type
                true,  // stereo
                strip.m_input != 0,  // hasRecord
                20,  // width
                240);  // height
        strip.m_meter->setToolTip(tr("Audio level"));

        // Pan

        strip.m_pan = new Rotary(
                m_mainBox,  // parent
                -100.0, 100.0,  // minimum, maximum
                1.0,  // step
                5.0,  // pageStep
                0.0,  // initialPosition
                20,  // size
                Rotary::NoTicks,  // ticks
                false,  // centred
                true);  // logarithmic

        if (instrument->getType() == Instrument::Audio) {
            strip.m_pan->setKnobColour(
                    GUIPalette::getColour(GUIPalette::RotaryPastelGreen));
        } else {  // Softsynth
            strip.m_pan->setKnobColour(
                    GUIPalette::getColour(GUIPalette::RotaryPastelYellow));
        }

        strip.m_pan->setToolTip(tr("Pan"));

        connect(strip.m_pan, SIGNAL(valueChanged(float)),
                this, SLOT(slotPanChanged(float)));

        // Stereo

        strip.m_stereoButton = new QPushButton(m_mainBox);
        strip.m_stereoButton->setIcon(m_monoPixmap);
        strip.m_stereoButton->setFixedSize(20, 20);
        strip.m_stereoButton->setFlat(true);
        strip.m_stereoButton->setToolTip(tr("Mono or stereo"));

        connect(strip.m_stereoButton, SIGNAL(clicked()),
                this, SLOT(slotChannelsChanged()));

        strip.m_stereo = false;

        // Record (UNUSED)

#if 0
        strip.m_recordButton = new QPushButton(m_mainBox);
        strip.m_recordButton->setText("R");
        strip.m_recordButton->setCheckable(true);
        strip.m_recordButton->setFixedWidth(strip.m_stereoButton->width());
        strip.m_recordButton->setFixedHeight(strip.m_stereoButton->height());
        strip.m_recordButton->setFlat(true);
        strip.m_recordButton->setToolTip(tr("Arm recording"));
        strip.m_recordButton->hide();

        connect(strip.m_recordButton, SIGNAL(clicked()),
                this, SLOT(slotRecordChanged()));
#endif

        // Plugins

        strip.m_pluginBox = new QWidget(m_mainBox);
        QVBoxLayout *pluginBoxLayout = new QVBoxLayout;
        pluginBoxLayout->setContentsMargins(0,0,0,0);
        strip.m_pluginBox->setLayout(pluginBoxLayout);

        // For each PluginPushButton
        for (int p = 0; p < numberOfPlugins; ++p) {
            PluginPushButton *plugin = new PluginPushButton(strip.m_pluginBox);
            plugin->setFont(font);
            plugin->setText(tr("<none>"));
            plugin->setMaximumWidth(45);
            plugin->setToolTip(tr("Click to load an audio plugin"));
            pluginBoxLayout->addWidget(plugin);

            connect(plugin, SIGNAL(clicked()),
                    this, SLOT(slotSelectPlugin()));

            strip.m_plugins.push_back(plugin);
        }

        // Layout

        mainLayout->addWidget(aliasButton, 0, column, 1, 2, Qt::AlignLeft);

        mainLayout->addWidget(idLabel, 1, column, 1, 2, Qt::AlignLeft);

        if (strip.m_input)
            mainLayout->addWidget(strip.m_input->getWidget(), 2, column, 1, 2);

        mainLayout->addWidget(strip.m_output->getWidget(), 3, column, 1, 2);

        mainLayout->addWidget(strip.m_fader, 4, column, Qt::AlignCenter);
        mainLayout->addWidget(strip.m_meter, 4, column + 1, Qt::AlignCenter);

        mainLayout->addWidget(strip.m_pan, 5, column, Qt::AlignCenter);
        mainLayout->addWidget(strip.m_stereoButton, 5, column + 1);

        mainLayout->addWidget(strip.m_pluginBox, 6, column, 1, 2);

        // Spacer
        mainLayout->setColumnMinimumWidth(column + 2, 4);

        // Need to indicate Strip is populated for updates to work.
        strip.m_populated = true;

        // Update Widgets

        updateInputFader(instrument->getId());
        updateRouteButtons(instrument->getId());
        updateStereoButton(instrument->getId());
        updatePluginButtons(instrument->getId());

        // Two for the controls, one for the spacer.
        column += 3;
    }

    // Submasters

    int submasterNumber = 1;

    BussList busses = m_studio->getBusses();

    // For each buss.
    for (BussList::iterator i = busses.begin();
         i != busses.end();
         ++i) {

        // The first one is the master, skip it.
        if (i == busses.begin())
            continue;

        Strip strip;

        // Label

        QLabel *idLabel = new QLabel(
                tr("Sub %1").arg(submasterNumber), m_mainBox);
        idLabel->setFont(boldFont);
        // Used by updateSubmasterVisibility() to show/hide the label.
        idLabel->setObjectName("subMaster");

        // Fader

        strip.m_fader = new Fader(
                AudioLevel::LongFader,  // type
                20, 240,  // width, height
                m_mainBox);  // parent
        strip.m_fader->setToolTip(tr("Audio level"));
        connect(strip.m_fader, SIGNAL(faderChanged(float)),
                this, SLOT(slotFaderLevelChanged(float)));

        // Meter

        strip.m_meter = new AudioVUMeter(
                m_mainBox,  // parent
                VUMeter::AudioPeakHoldIECLong,  // type
                true,  // stereo
                false,  // hasRecord
                20, 240);  // width, height

        strip.m_meter->setToolTip(tr("Audio level"));

        // Pan

        strip.m_pan = new Rotary(
                m_mainBox,  // parent
                -100.0, 100.0,  // minimum, maximum
                1.0,  // step
                5.0,  // pageStep
                0.0,  // initialPosition
                20,  // size
                Rotary::NoTicks,  // ticks
                false,  // centred
                true);  // logarithmic

        strip.m_pan->setKnobColour(
                GUIPalette::getColour(GUIPalette::RotaryPastelBlue));

        strip.m_pan->setToolTip(tr("Pan"));

        connect(strip.m_pan, SIGNAL(valueChanged(float)),
                this, SLOT(slotPanChanged(float)));

        // Plugins

        strip.m_pluginBox = new QWidget(m_mainBox);
        QVBoxLayout *pluginBoxLayout = new QVBoxLayout;
        pluginBoxLayout->setContentsMargins(0,0,0,0);
        strip.m_pluginBox->setLayout(pluginBoxLayout);

        // For each PluginPushButton
        for (int p = 0; p < numberOfPlugins; ++p) {
            PluginPushButton *plugin = new PluginPushButton(strip.m_pluginBox);
            plugin->setFont(font);
            plugin->setText(tr("<none>"));
            plugin->setMaximumWidth(45);
            plugin->setToolTip(tr("Click to load an audio plugin"));
            pluginBoxLayout->addWidget(plugin);

            connect(plugin, SIGNAL(clicked()),
                    this, SLOT(slotSelectPlugin()));

            strip.m_plugins.push_back(plugin);
        }

        strip.m_populated = true;

        // Layout

        // Row 0, InstrumentAliasButton, don't need one.

        mainLayout->addWidget(idLabel, 1, column, 1, 2, Qt::AlignLeft);

        // Row 2, In Button, don't need one.
        // Row 3, Out Button, don't need one.

        mainLayout->addWidget(strip.m_fader, 4, column, Qt::AlignCenter);
        mainLayout->addWidget(strip.m_meter, 4, column + 1, Qt::AlignCenter);

        mainLayout->addWidget(strip.m_pan, 5, column, 1, 2, Qt::AlignCenter);

        mainLayout->addWidget(strip.m_pluginBox, 6, column, 1, 2);

        // Spacer
        mainLayout->setColumnMinimumWidth(column + 2, 4);

        // ??? COPY?!
        m_submasters.push_back(strip);

        // Update Widgets

        updateBussFader(submasterNumber);
        updatePluginButtons(submasterNumber);

        ++submasterNumber;

        // Two columns for controls, one for the spacer.
        column += 3;
    }

    // Master

    if (busses.size() > 0) {

        // Widgets

        // Label
        QLabel *idLabel = new QLabel(tr("Master"), m_mainBox);
        idLabel->setFont(boldFont);

        // Fader
        m_master.m_fader = new Fader(
                AudioLevel::LongFader,  // type
                20, 240,  // width, height
                m_mainBox);  // parent
        m_master.m_fader->setToolTip(tr("Audio master output level"));
        connect(m_master.m_fader, SIGNAL(faderChanged(float)),
                this, SLOT(slotFaderLevelChanged(float)));

        // Meter
        m_master.m_meter = new AudioVUMeter(
                m_mainBox,  // parent
                VUMeter::AudioPeakHoldIEC,  // type
                true,  // stereo
                false,  // hasRecord
                20, 240);  // width, height
        m_master.m_meter->setToolTip(tr("Audio master output level"));

        m_master.m_populated = true;

        // Layout

        mainLayout->addWidget(idLabel, 1, column, 1,  2, Qt::AlignLeft);
        mainLayout->addWidget(m_master.m_fader, 4, column, Qt::AlignCenter);
        mainLayout->addWidget(m_master.m_meter, 4, column + 1, Qt::AlignCenter);

        // Update Widgets

        // Update the master fader
        updateBussFader(0);
    }

    updateFaderVisibility();
    updateSynthFaderVisibility();
    updateSubmasterVisibility();
    updatePluginButtonVisibility();

    // The update visibility routines already called this.
    //adjustSize();
}

void
AudioMixerWindow::slotTrackAssignmentsChanged()
{
    //RG_DEBUG << "slotTrackAssignmentsChanged()";

    // For each input strip
    for (StripMap::const_iterator i = m_inputs.begin();
         i != m_inputs.end();
         ++i) {
        InstrumentId id = i->first;

        // Do any Tracks in the Composition use this Instrument?
        const bool assigned = m_document->getComposition().hasTrack(id);
        const bool populated = i->second.m_populated;

        // If we find an input that is assigned but not populated,
        // or we find an input that is populated but not assigned...
        // ??? But what if unassigned instruments are visible?  Then
        //     this check is incorrect.
        if (assigned != populated) {
            // found an inconsistency
            populate();
            return;
        }
    }

    // ??? BUG.  What if a Track is switched from MIDI to Audio?
    //     Since the above loop only goes through the inputs here,
    //     it doesn't see the new Audio track in the Composition.
    //     The inputs do not get updated so the new Audio input
    //     doesn't appear.  Need something like the following.  However,
    //     I think it's better to spend time on the redesign than to fix
    //     bugs in the old design/implementation.
    // for each Track in the Composition
        // if the Track's Instrument isn't audio or softsynth, continue.
        // if the Track's Instrument doesn't have a Strip
            //populate();
            //return;
        // fi
    // rof
}

void
AudioMixerWindow::slotInstrumentChanged(Instrument *instrument)
{
    if (!instrument)
        return;

    const InstrumentId instrumentId = instrument->getId();

    RG_DEBUG << "slotInstrumentChanged(): instrument ID = " << instrumentId;

    // Prevent update loops.
    blockSignals(true);

    updateInputFader(instrumentId);
    updateStereoButton(instrumentId);
    updateRouteButtons(instrumentId);
    updatePluginButtons(instrumentId);
    //updateMiscButtons(instrumentId);

    blockSignals(false);
}

void
AudioMixerWindow::slotPluginSelected(InstrumentId instrumentId,
                                     int /*index*/, int /*plugin*/)
{
    updatePluginButtons(instrumentId);
}

void
AudioMixerWindow::slotPluginBypassed(InstrumentId instrumentId,
                                     int /*index*/, bool /*bypass*/)
{
    //RG_DEBUG << "slotPluginBypassed(" << instrumentId << ")";

    updatePluginButtons(instrumentId);
}

void
AudioMixerWindow::updateInputFader(InstrumentId instrumentId)
{
    if (instrumentId < AudioInstrumentBase)
        return;

    Strip &strip = m_inputs[instrumentId];

    if (!strip.m_populated)
        return;

    Instrument *instrument = m_studio->getInstrumentById(instrumentId);

    strip.m_fader->blockSignals(true);
    strip.m_fader->setFader(instrument->getLevel());
    strip.m_fader->blockSignals(false);

    strip.m_pan->blockSignals(true);
    strip.m_pan->setPosition(instrument->getPan() - 100);
    strip.m_pan->blockSignals(false);
}

void
AudioMixerWindow::updateBussFader(unsigned bussId)
{
    Strip &strip = (bussId == 0 ? m_master : m_submasters[bussId - 1]);

    if (!strip.m_populated)
        return;

    BussList busses = m_studio->getBusses();
    Buss *buss = busses[bussId];

    strip.m_fader->blockSignals(true);
    strip.m_fader->setFader(buss->getLevel());
    strip.m_fader->blockSignals(false);

    if (strip.m_pan) {
        strip.m_pan->blockSignals(true);
        strip.m_pan->setPosition(buss->getPan() - 100);
        strip.m_pan->blockSignals(false);
    }
}

void
AudioMixerWindow::updateRouteButtons(InstrumentId instrumentId)
{
    Strip &strip = m_inputs[instrumentId];

    if (!strip.m_populated)
        return;

    if (strip.m_input)
        strip.m_input->slotRepopulate();

    strip.m_output->slotRepopulate();
}

void
AudioMixerWindow::updateStereoButton(InstrumentId instrumentId)
{
    Strip &strip = m_inputs[instrumentId];
    if (!strip.m_populated)
        return;

    Instrument *instrument = m_studio->getInstrumentById(instrumentId);

    const bool stereo = (instrument->getAudioChannels() > 1);
    // No change, bail.
    if (stereo == strip.m_stereo)
        return;

    strip.m_stereo = stereo;

    if (stereo)
        strip.m_stereoButton->setIcon(m_stereoPixmap);
    else
        strip.m_stereoButton->setIcon(m_monoPixmap);
}

#if 0
void
AudioMixerWindow::updateMiscButtons(int)
{
    //... complications here, because the mute/solo status is actually
    // per-track rather than per-instrument... doh.
}
#endif

void
AudioMixerWindow::updatePluginButtons(int id)
{
    // ??? split the first part of this into two routines.
    //     updateInputPluginButtons() and updateBussPluginButtons().

    const PluginContainer *pluginContainer = 0;
    Strip *strip = 0;

    // Input Strip
    if (id >= (int)AudioInstrumentBase) {

        pluginContainer = m_studio->getInstrumentById(id);

        strip = &m_inputs[id];

    } else {  // Buss Strip

        BussList busses = m_studio->getBusses();
        if (id >= int(busses.size()))
            return;

        pluginContainer = busses[id];

        strip = &m_submasters[id - 1];

    }

    // ??? Pull the rest of this out into an
    //     updatePluginButtons(const PluginContainer *, Strip *).

    if (!pluginContainer)
        return;
    if (!strip)
        return;

    if (!strip->m_populated  ||  !strip->m_pluginBox)
        return;

    for (size_t i = 0; i < strip->m_plugins.size(); i++) {

        bool used = false;
        bool bypass = false;
        QColor pluginBgColour = Qt::green;
//                qApp->palette().color(QPalette::Active, QColorGroup::Light);

        strip->m_plugins[i]->show();

        AudioPluginInstance *inst = pluginContainer->getPlugin(i);

        if (inst && inst->isAssigned()) {

            AudioPlugin *pluginClass = m_document->getPluginManager()->getPlugin(
                  m_document->getPluginManager()->
                  getPositionByIdentifier(inst->getIdentifier().c_str()));

            if (pluginClass) {
                strip->m_plugins[i]->setText(pluginClass->getLabel());
                strip->m_plugins[i]->setToolTip(pluginClass->getLabel());

                pluginBgColour = pluginClass->getColour();
            }

            used = true;
            bypass = inst->isBypassed();

        } else {

            strip->m_plugins[i]->setText(tr("<none>"));
            strip->m_plugins[i]->setToolTip(tr("<no plugin>"));

            if (inst)
                bypass = inst->isBypassed();
        }

        if (bypass) {
            strip->m_plugins[i]->setState(PluginPushButton::Bypassed);
        } else if (used) {
            strip->m_plugins[i]->setState(PluginPushButton::Active);
        } else {
            strip->m_plugins[i]->setState(PluginPushButton::Normal);
        }
    }
}

void
AudioMixerWindow::slotSelectPlugin()
{
    const QObject *s = sender();

    for (StripMap::iterator i = m_inputs.begin();
            i != m_inputs.end(); ++i) {

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

    for (StripVector::iterator i = m_submasters.begin();
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

    for (StripMap::iterator i = m_inputs.begin(); i != m_inputs.end(); ++i) {

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

    for (StripVector::iterator i = m_submasters.begin();
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

    for (StripMap::iterator i = m_inputs.begin();
            i != m_inputs.end(); ++i) {

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

    for (StripVector::iterator i = m_submasters.begin();
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

    for (StripMap::iterator i = m_inputs.begin();
            i != m_inputs.end(); ++i) {

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


    for (StripMap::iterator i = m_inputs.begin();
            i != m_inputs.end(); ++i) {

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

#if 0
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
#endif

void
AudioMixerWindow::updateMeters()
{
    for (StripMap::iterator i = m_inputs.begin(); i != m_inputs.end(); ++i) {

        InstrumentId id = i->first;
        Strip &strip = i->second;
        if (!strip.m_populated)
            continue;

        LevelInfo info;

        if (SequencerDataBlock::getInstance()->
            getInstrumentLevelForMixer(id, info)) {

            // The values passed through are long-fader values
            float dBleft = AudioLevel::fader_to_dB
                           (info.level, 127, AudioLevel::LongFader);

            if (strip.m_stereo) {
                float dBright = AudioLevel::fader_to_dB
                                (info.levelRight, 127, AudioLevel::LongFader);

                strip.m_meter->setLevel(dBleft, dBright);

            } else {
                strip.m_meter->setLevel(dBleft);
            }
        }
    }

    for (unsigned int i = 0; i < m_submasters.size(); ++i) {

        Strip &strip = m_submasters[i];

        LevelInfo info;
        if (!SequencerDataBlock::getInstance()->getSubmasterLevel(i, info)) {
            continue;
        }

        // The values passed through are long-fader values
        float dBleft = AudioLevel::fader_to_dB
                       (info.level, 127, AudioLevel::LongFader);
        float dBright = AudioLevel::fader_to_dB
                        (info.levelRight, 127, AudioLevel::LongFader);

        strip.m_meter->setLevel(dBleft, dBright);
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

    for (StripMap::iterator i = m_inputs.begin(); i != m_inputs.end(); ++i) {

        InstrumentId id = i->first;
        Strip &strip = i->second;
        if (!strip.m_populated)
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

            if (strip.m_stereo) {
                float dBright = AudioLevel::fader_to_dB
                                (info.levelRight, 127, AudioLevel::LongFader);

                strip.m_meter->setRecordLevel(dBleft, dBright);

            } else {
                strip.m_meter->setRecordLevel(dBleft);
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
    // update instrument for nth input in m_inputs

    if (e->getType() != MappedEvent::MidiController)
        return ;
    unsigned int channel = e->getRecordedChannel();
    MidiByte controller = e->getData1();
    MidiByte value = e->getData2();

    unsigned int count = 0;
    for (StripMap::iterator i = m_inputs.begin(); i != m_inputs.end(); ++i) {

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
        // slotInstrumentChanged() to update the input strip.
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

    for (StripMap::iterator i = m_inputs.begin();
            i != m_inputs.end(); ++i) {
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

void AudioMixerWindow::Strip::setVisible(bool visible)
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
AudioMixerWindow::Strip::setPluginButtonsVisible(bool visible)
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
    m_studio->amwShowAudioFaders = !m_studio->amwShowAudioFaders;

    updateFaderVisibility();
}

void
AudioMixerWindow::updateFaderVisibility()
{
    bool d = m_studio->amwShowAudioFaders;

    QAction *action = findAction("show_audio_faders");
    if (action) {
        action->setChecked(d);
    }

    RG_DEBUG << "updateFaderVisibility(): visiblility is " << d;

    for (StripMap::iterator i = m_inputs.begin(); i != m_inputs.end(); ++i) {
        if (i->first < SoftSynthInstrumentBase) {
            i->second.setVisible(d);
        }
    }

    toggleNamedWidgets(d, "audioIdLabel");

    adjustSize();
}

void
AudioMixerWindow::slotToggleSynthFaders()
{
    m_studio->amwShowSynthFaders = !m_studio->amwShowSynthFaders;

    updateSynthFaderVisibility();
}

void
AudioMixerWindow::updateSynthFaderVisibility()
{
    QAction *action = findAction("show_synth_faders");
    if (!action)
        return ;

    action->setChecked(m_studio->amwShowSynthFaders);

    for (StripMap::iterator i = m_inputs.begin(); i != m_inputs.end(); ++i) {
        if (i->first >= SoftSynthInstrumentBase) {
            i->second.setVisible(action->isChecked());
        }
    }

    toggleNamedWidgets(action->isChecked(), "synthIdLabel");

    adjustSize();
}

void
AudioMixerWindow::slotToggleSubmasters()
{
    m_studio->amwShowAudioSubmasters = !m_studio->amwShowAudioSubmasters;

    updateSubmasterVisibility();
}

void
AudioMixerWindow::updateSubmasterVisibility()
{
    QAction *action = findAction("show_audio_submasters");

    if (!action)
        return ;

    action->setChecked(m_studio->amwShowAudioSubmasters);

    for (StripVector::iterator i = m_submasters.begin(); i != m_submasters.end(); ++i) {
        i->setVisible(action->isChecked());
    }

    toggleNamedWidgets(action->isChecked(), "subMaster");

    adjustSize();
}

void
AudioMixerWindow::slotTogglePluginButtons()
{
    m_studio->amwShowPluginButtons = !m_studio->amwShowPluginButtons;

    updatePluginButtonVisibility();
}

void
AudioMixerWindow::updatePluginButtonVisibility()
{
    QAction *action = findAction("show_plugin_buttons");
    if (!action)
        return ;

    action->setChecked(m_studio->amwShowPluginButtons);

    RG_DEBUG << "AudioMixerWindow::updatePluginButtonVisibility() action->isChecked("
             << (action->isChecked() ? "true" : "false") << ")";

    for (StripMap::iterator i = m_inputs.begin(); i != m_inputs.end(); ++i) {
        i->second.setPluginButtonsVisible(action->isChecked());
    }

    adjustSize();
}

void
AudioMixerWindow::slotToggleUnassignedFaders()
{
    // ??? When hiding these, the layout doesn't compress horizontally
    //     as it is supposed to.  Usually the calls to adjustSize() fix
    //     this, but not in this case.

    QAction *action = findAction("show_unassigned_faders");
    if (!action)
        return ;

    m_studio->amwShowUnassignedFaders = !m_studio->amwShowUnassignedFaders;

    action->setChecked(m_studio->amwShowUnassignedFaders);

    populate();
}

void
AudioMixerWindow::toggleNamedWidgets(bool show, const char *name)
{
    //RG_DEBUG << "toggleNamedWidgets(" << show << ", " << name << ")";

    //NB. Completely rewritten to get around the disappearance of
    // QLayoutIterator.

    // ??? This routine doesn't work at all.  All it finds is an
    //     object named "Transport Toolbar" and an object named "".

    int i = 0;
    QLayoutItem *child;
    while ((child = layout()->itemAt(i)) != 0) {
        QWidget *widget = child->widget();

        //if (widget)
        //    RG_DEBUG << "  name:" << widget->objectName();

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
    //RG_DEBUG << "slotRepopulate()";

    // this destroys everything if it already exists and rebuilds it, so while
    // it's overkill for changing one label, it should get the job done without
    // the need for even more new plumbing
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

#if 0
void
AudioMixer::updateWidgets()
{
    // Updates all widgets based on the Composition.

    // Note: Make sure that this routine is efficient enough to avoid doing
    //       work when nothing has actually changed.  This will make it
    //       safe to call in all situations.  Most of the Qt widgets are
    //       very efficient.  E.g. calling QLabel::setText() with the same
    //       text that is already being displayed is a no-op.  We need to
    //       make sure that RG's custom widgets (e.g. Fader) are just as
    //       efficient.

    // Determine whether the number of inputs/submasters/masters is correct.
    // If it isn't, adjust as needed.  Note that QGridLayout does not provide
    // an easy way to insert/delete columns.  It might be easiest to make each
    // strip its own QWidget-based class.  This way we are simply adding/
    // removing single controls from the layout.  In fact, it could then
    // become a QHBoxLayout.  QHBoxLayout just happens to offer an
    // insertWidget() and a takeAt() which might be exactly what we need here
    // to insert/delete columns.  Vertical alignment might be an issue,
    // though.  QGridLayout has an advantage WRT that.

    // At this point, the widgets in the layout will match the number of
    // inputs/submasters/masters, so all that is needed is to update the
    // widgets in each strip.

    // At this point, we should also have Strips for the unassigned
    // Instruments if that is enabled.  So, we can just iterate over the
    // input Strips without worrying about assigned/unassigned.

    // for each input Strip
        // update the widgets
    // rof

    // for each submaster Strip
        // update the widgets
    // rof

    // for the master Strip, update the widgets.
}
#endif


}
