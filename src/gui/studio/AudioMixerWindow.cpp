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
#include "gui/widgets/InputDialog.h"
#include "gui/widgets/Label.h"
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
    setObjectName("AudioMixerWindow");

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
    createAction("mixer_help", SLOT(slotHelp()));
    // Help > About Rosegarden
    createAction("help_about_app", SLOT(slotAboutRosegarden()));

    // Settings > Show Audio Faders
    createAction("show_audio_faders", SLOT(slotShowAudioFaders()));

    // Settings > Show Synth Faders
    createAction("show_synth_faders", SLOT(slotShowSynthFaders()));

    // Settings > Show Audio Submasters
    createAction("show_audio_submasters", SLOT(slotShowAudioSubmasters()));

    // Settings > Show Plugin Buttons
    createAction("show_plugin_buttons", SLOT(slotShowPluginButtons()));

    // Settings > Show Unassigned Faders
    createAction("show_unassigned_faders", SLOT(slotShowUnassignedFaders()));

    // "Settings > Number of Stereo Inputs" Actions
    // For i in {1,2,4,8,16}
    for (int i = 1; i <= 16; i *= 2) {
        createAction(QString("inputs_%1").arg(i),
                     SLOT(slotNumberOfStereoInputs()));
    }

    // "Settings > Number of Submasters" Actions
    createAction("submasters_0", SLOT(slotNumberOfSubmasters()));
    createAction("submasters_2", SLOT(slotNumberOfSubmasters()));
    createAction("submasters_4", SLOT(slotNumberOfSubmasters()));
    createAction("submasters_8", SLOT(slotNumberOfSubmasters()));

    // "Settings > Panning Law" Actions
    // ??? Can we do something easier to understand than panlaw_0,
    //     panlaw_1, etc...?  E.g. panlaw_0dB, panlaw_-3dB, panlaw_-6dB,
    //     panlaw_alt-3dB.
    createAction("panlaw_0", SLOT(slotPanningLaw()));
    createAction("panlaw_1", SLOT(slotPanningLaw()));
    createAction("panlaw_2", SLOT(slotPanningLaw()));
    createAction("panlaw_3", SLOT(slotPanningLaw()));

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
    // Uncomment these to see the frame.
    //m_mainBox->setFrameStyle(QFrame::Panel | QFrame::Raised);
    //m_mainBox->setLineWidth(2);
    m_surroundBoxLayout->addWidget(m_mainBox);

    // Strips take up 3 columns, 2 for the controls and 1 for the spacer.
    QGridLayout *mainLayout = new QGridLayout(m_mainBox);

    const int numberOfPlugins = 5;

    // Inputs

    int column = 0;

    int externalControllerChannel = 0;

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

        strip.m_label = new Label(strtoqstr(instrument->getAlias()), m_mainBox);
        strip.m_label->setFont(boldFont);
        //strip.m_label->setToolTip(tr("Click the button above to rename this instrument"));
        // Switching to this since the strip width is frequently too narrow to
        // see e.g. "Synth plugin #2".
        strip.m_label->setToolTip(strtoqstr(instrument->getAlias()));
        strip.m_label->setMaximumWidth(45);
        strip.m_label->setProperty("instrumentId", instrument->getId());
        connect(strip.m_label, SIGNAL(clicked()),
                this, SLOT(slotLabelClicked()));

        // Audio Instrument
        if (instrument->getType() == Instrument::Audio) {
            // Used by updateFaderVisibility() to show/hide this label.
            strip.m_label->setObjectName("audioIdLabel");
        } else {  // Softsynth Instrument
            // Used by updateSynthFaderVisibility() to show/hide this label.
            strip.m_label->setObjectName("synthIdLabel");
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
        strip.m_fader->setProperty("instrumentId", instrument->getId());
        strip.m_fader->setProperty(
                "externalControllerChannel", externalControllerChannel);

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
        strip.m_pan->setProperty("instrumentId", instrument->getId());
        strip.m_pan->setProperty(
                "externalControllerChannel", externalControllerChannel);

        connect(strip.m_pan, SIGNAL(valueChanged(float)),
                this, SLOT(slotPanChanged(float)));

        // Stereo

        strip.m_stereoButton = new QPushButton(m_mainBox);
        strip.m_stereoButton->setIcon(m_monoPixmap);
        strip.m_stereoButton->setFixedSize(20, 20);
        strip.m_stereoButton->setFlat(true);
        strip.m_stereoButton->setToolTip(tr("Mono or stereo"));
        strip.m_stereoButton->setProperty("instrumentId", instrument->getId());

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
        for (unsigned p = 0; p < numberOfPlugins; ++p) {
            PluginPushButton *plugin = new PluginPushButton(strip.m_pluginBox);
            plugin->setFont(font);
            plugin->setText(tr("<none>"));
            plugin->setMaximumWidth(45);
            plugin->setToolTip(tr("Click to load an audio plugin"));
            plugin->setProperty("instrumentId", instrument->getId());
            plugin->setProperty("pluginIndex", p);
            pluginBoxLayout->addWidget(plugin);

            connect(plugin, SIGNAL(clicked()),
                    this, SLOT(slotSelectPlugin()));

            strip.m_plugins.push_back(plugin);
        }

        // Layout

        mainLayout->addWidget(aliasButton, 0, column, 1, 2, Qt::AlignLeft);

        mainLayout->addWidget(strip.m_label, 1, column, 1, 2, Qt::AlignLeft);

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
        updateInputPluginButtons(instrument->getId());

        // Next "external controller" channel.
        ++externalControllerChannel;

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

        strip.m_label = new Label(
                tr("Sub %1").arg(submasterNumber), m_mainBox);
        strip.m_label->setFont(boldFont);
        // Used by updateSubmasterVisibility() to show/hide the label.
        strip.m_label->setObjectName("subMaster");
        strip.m_label->setProperty("instrumentId", submasterNumber);

        // Fader

        strip.m_fader = new Fader(
                AudioLevel::LongFader,  // type
                20, 240,  // width, height
                m_mainBox);  // parent
        strip.m_fader->setToolTip(tr("Audio level"));
        strip.m_fader->setProperty("instrumentId", submasterNumber);
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
        strip.m_pan->setProperty("instrumentId", submasterNumber);

        connect(strip.m_pan, SIGNAL(valueChanged(float)),
                this, SLOT(slotPanChanged(float)));

        // Plugins

        strip.m_pluginBox = new QWidget(m_mainBox);
        QVBoxLayout *pluginBoxLayout = new QVBoxLayout;
        pluginBoxLayout->setContentsMargins(0,0,0,0);
        strip.m_pluginBox->setLayout(pluginBoxLayout);

        // For each PluginPushButton
        for (unsigned p = 0; p < numberOfPlugins; ++p) {
            PluginPushButton *plugin = new PluginPushButton(strip.m_pluginBox);
            plugin->setFont(font);
            plugin->setText(tr("<none>"));
            plugin->setMaximumWidth(45);
            plugin->setToolTip(tr("Click to load an audio plugin"));
            plugin->setProperty("instrumentId", submasterNumber);
            plugin->setProperty("pluginIndex", p);
            pluginBoxLayout->addWidget(plugin);

            connect(plugin, SIGNAL(clicked()),
                    this, SLOT(slotSelectPlugin()));

            strip.m_plugins.push_back(plugin);
        }

        strip.m_populated = true;

        // Layout

        // Row 0, InstrumentAliasButton, don't need one.

        mainLayout->addWidget(strip.m_label, 1, column, 1, 2, Qt::AlignLeft);

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
        updateBussPluginButtons(submasterNumber);

        ++submasterNumber;

        // Two columns for controls, one for the spacer.
        column += 3;
    }

    // Master

    if (busses.size() > 0) {

        // Widgets

        // Label
        m_master.m_label = new Label(tr("Master"), m_mainBox);
        m_master.m_label->setFont(boldFont);
        m_master.m_label->setProperty("instrumentId", 0);

        // Fader
        m_master.m_fader = new Fader(
                AudioLevel::LongFader,  // type
                20, 240,  // width, height
                m_mainBox);  // parent
        m_master.m_fader->setToolTip(tr("Audio master output level"));
        m_master.m_fader->setProperty("instrumentId", 0);
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

        mainLayout->addWidget(m_master.m_label, 1, column, 1,  2, Qt::AlignLeft);
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
    updateInputPluginButtons(instrumentId);
    //updateMiscButtons(instrumentId);

    blockSignals(false);
}

void
AudioMixerWindow::slotPluginSelected(InstrumentId instrumentId,
                                     int /*index*/, int /*plugin*/)
{
    if (instrumentId >= AudioInstrumentBase)
        updateInputPluginButtons(instrumentId);
    else
        updateBussPluginButtons(instrumentId);
}

void
AudioMixerWindow::slotPluginBypassed(InstrumentId instrumentId,
                                     int /*index*/, bool /*bypass*/)
{
    //RG_DEBUG << "slotPluginBypassed(" << instrumentId << ")";

    if (instrumentId >= AudioInstrumentBase)
        updateInputPluginButtons(instrumentId);
    else
        updateBussPluginButtons(instrumentId);
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
AudioMixerWindow::updateInputPluginButtons(InstrumentId instrumentId)
{
    if (instrumentId < AudioInstrumentBase)
        return;

    const PluginContainer *pluginContainer =
            dynamic_cast<const PluginContainer *>(
                    m_studio->getInstrumentById(instrumentId));

    Strip *strip = &m_inputs[instrumentId];

    updatePluginButtons(pluginContainer, strip);
}

void
AudioMixerWindow::updateBussPluginButtons(unsigned bussId)
{
    // Master has no plugins.
    if (bussId == 0)
        return;

    BussList busses = m_studio->getBusses();
    if (bussId >= busses.size())
        return;

    const PluginContainer *pluginContainer =
            dynamic_cast<const PluginContainer *>(busses[bussId]);

    Strip *strip = &m_submasters[bussId - 1];

    updatePluginButtons(pluginContainer, strip);
}

void
AudioMixerWindow::updatePluginButtons(
        const PluginContainer *pluginContainer, Strip *strip)
{
    if (!pluginContainer)
        return;
    if (!strip)
        return;

    if (!strip->m_populated  ||  !strip->m_pluginBox)
        return;

    // For each PluginPushButton on the Strip
    for (size_t i = 0; i < strip->m_plugins.size(); i++) {
        PluginPushButton *pluginButton = strip->m_plugins[i];

        AudioPluginInstance *plugin = pluginContainer->getPlugin(i);

        bool used = false;
        bool bypass = false;

        if (plugin  &&  plugin->isAssigned()) {

            AudioPluginManager *pluginMgr = m_document->getPluginManager();
            AudioPlugin *pluginClass = pluginMgr->getPluginByIdentifier(
                    plugin->getIdentifier().c_str());

            if (pluginClass) {
                pluginButton->setText(pluginClass->getLabel());
                pluginButton->setToolTip(pluginClass->getLabel());
            }

            used = true;
            bypass = plugin->isBypassed();

        } else {

            pluginButton->setText(tr("<none>"));
            pluginButton->setToolTip(tr("<no plugin>"));

            if (plugin)
                bypass = plugin->isBypassed();
        }

        if (bypass) {
            pluginButton->setState(PluginPushButton::Bypassed);
        } else if (used) {
            pluginButton->setState(PluginPushButton::Active);
        } else {
            pluginButton->setState(PluginPushButton::Normal);
        }
    }
}

void
AudioMixerWindow::slotSelectPlugin()
{
    const PluginPushButton *pluginButton =
            dynamic_cast<const PluginPushButton *>(sender());

    if (!pluginButton)
        return;

    // Launch the AudioPluginDialog.
    emit selectPlugin(this,
                      pluginButton->property("instrumentId").toUInt(),
                      pluginButton->property("pluginIndex").toUInt());
}

void
AudioMixerWindow::sendControllerRefresh()
{
    // To keep the device connected to the "external controller" port in
    // sync with the "Audio Mixer" window, send out MIDI volume and pan
    // messages to it.

    // !!! Really want some notification of whether we have a device
    //     connected to the "external controller" port!  Otherwise this
    //     is a waste of time.  Is there a way in ALSA to ask if the port
    //     is connected?

    int controllerChannel = 0;

    // For each input Strip.
    for (StripMap::iterator i = m_inputs.begin(); i != m_inputs.end(); ++i) {

        // Only do the first 16 since a single MIDI device can only handle
        // 16 channels.
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
AudioMixerWindow::slotLabelClicked()
{
    const Label *label = dynamic_cast<const Label *>(sender());

    if (!label)
        return;

    InstrumentId instrumentId = label->property("instrumentId").toUInt();

    Instrument *instrument = m_studio->getInstrumentById(instrumentId);

    if (!instrument)
        return;

    bool ok = false;

    QString newAlias = InputDialog::getText(
            this,  // parent
            tr("Rosegarden"),  // title
            tr("Enter instrument alias:"),  // label
            LineEdit::Normal,  // mode (echo)
            strtoqstr(instrument->getAlias()),  // text
            &ok);  // ok

    if (ok) {
        instrument->setAlias(newAlias.toStdString());
        instrument->changed();
        populate();
    }
}

void
AudioMixerWindow::slotFaderLevelChanged(float dB)
{
    const Fader *fader = dynamic_cast<const Fader *>(sender());

    if (!fader)
        return;

    InstrumentId instrumentId = fader->property("instrumentId").toUInt();

    // If this is an input Fader
    if (instrumentId >= AudioInstrumentBase) {
        Instrument *instrument =
            m_studio->getInstrumentById(instrumentId);

        if (!instrument)
            return;

        StudioControl::setStudioObjectProperty(
                MappedObjectId(instrumentId),
                MappedAudioFader::FaderLevel,
                MappedObjectValue(dB));

        instrument->setLevel(dB);
        instrument->changed();

        int externalControllerChannel =
                fader->property("externalControllerChannel").toInt();

        // Send out to "external controller" port as well.
        // ??? Would be nice to know whether anything is connected
        //     to the "external controller" port.  Otherwise this is
        //     a waste.  Especially with a potentially very frequent
        //     update such as this.
        if (externalControllerChannel < 16) {
            int value = AudioLevel::dB_to_fader(
                    dB, 127, AudioLevel::LongFader);

            MappedEvent mE(instrumentId,
                           MappedEvent::MidiController,
                           MIDI_CONTROLLER_VOLUME,
                           MidiByte(value));
            mE.setRecordedChannel(externalControllerChannel);
            mE.setRecordedDevice(Device::CONTROL_DEVICE);

            StudioControl::sendMappedEvent(mE);
        }

        return;
    }

    // If this is the master or a submaster Fader
    if (instrumentId < AudioInstrumentBase) {

        BussList busses = m_studio->getBusses();

        // If the buss ID is out of range, bail.
        if (instrumentId >= busses.size())
            return;

        StudioControl::setStudioObjectProperty(
                MappedObjectId(busses[instrumentId]->getMappedId()),
                MappedAudioBuss::Level,
                MappedObjectValue(dB));

        busses[instrumentId]->setLevel(dB);

        return;
    }
}

void
AudioMixerWindow::slotPanChanged(float panValue)
{
    const Rotary *panRotary = dynamic_cast<const Rotary *>(sender());

    if (!panRotary)
        return;

    InstrumentId instrumentId = panRotary->property("instrumentId").toUInt();

    // If this is an input pan widget
    if (instrumentId >= AudioInstrumentBase) {
        int externalControllerChannel =
                panRotary->property("externalControllerChannel").toInt();

        Instrument *instrument =
            m_studio->getInstrumentById(instrumentId);

        if (!instrument)
            return;

        StudioControl::setStudioObjectProperty(
                instrument->getMappedId(),
                MappedAudioFader::Pan,
                MappedObjectValue(panValue));

        instrument->setPan(MidiByte(panValue + 100.0));
        instrument->changed();

        // Send out to "external controller" port as well.
        // ??? Would be nice to know whether anything is connected
        //     to the "external controller" port.  Otherwise this is
        //     a waste.  Especially with a potentially very frequent
        //     update such as this.
        if (externalControllerChannel < 16) {
            int ipan = (int(instrument->getPan()) * 64) / 100;
            if (ipan < 0)
                ipan = 0;
            if (ipan > 127)
                ipan = 127;

            MappedEvent mE(instrument->getId(),
                           MappedEvent::MidiController,
                           MIDI_CONTROLLER_PAN,
                           MidiByte(ipan));
            mE.setRecordedChannel(externalControllerChannel);
            mE.setRecordedDevice(Device::CONTROL_DEVICE);

            StudioControl::sendMappedEvent(mE);
        }

        return;
    }

    // If this is a buss pan widget
    if (instrumentId < AudioInstrumentBase) {
        BussList busses = m_studio->getBusses();

        if (instrumentId >= busses.size())
            return;

        StudioControl::setStudioObjectProperty(
                MappedObjectId(busses[instrumentId]->getMappedId()),
                MappedAudioBuss::Pan,
                MappedObjectValue(panValue));

        busses[instrumentId]->setPan(MidiByte(panValue + 100.0));

        return;
    }
}

void
AudioMixerWindow::slotChannelsChanged()
{
    const QPushButton *stereoButton =
            dynamic_cast<const QPushButton *>(sender());

    if (!stereoButton)
        return;

    InstrumentId instrumentId =
            stereoButton->property("instrumentId").toUInt();

    // Channels are only switchable on instruments.

    Instrument *instrument = m_studio->getInstrumentById(instrumentId);

    if (!instrument)
        return;

    // Toggle number of channels
    instrument->setAudioChannels(
            (instrument->getAudioChannels() > 1) ? 1 : 2);

    updateStereoButton(instrumentId);
    //!!! need to reconnect input, or change input channel number anyway
    updateRouteButtons(instrumentId);

    instrument->changed();
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
    // Inputs

    // For each input Strip
    for (StripMap::iterator i = m_inputs.begin(); i != m_inputs.end(); ++i) {

        InstrumentId id = i->first;
        Strip &strip = i->second;

        if (!strip.m_populated)
            continue;

        LevelInfo info;

        // Get the level.  If there was no change, try the next Strip.
        if (!SequencerDataBlock::getInstance()->
                getInstrumentLevelForMixer(id, info))
            continue;

        // Convert to dB for display.
        // The values passed through are long-fader values
        float dBleft = AudioLevel::fader_to_dB(
                info.level, 127, AudioLevel::LongFader);

        if (strip.m_stereo) {
            // Convert to dB for display.
            float dBright = AudioLevel::fader_to_dB(
                    info.levelRight, 127, AudioLevel::LongFader);

            strip.m_meter->setLevel(dBleft, dBright);
        } else {  // mono
            strip.m_meter->setLevel(dBleft);
        }
    }

    // Update the input Strip meters for record monitoring.
    updateMonitorMeters();

    // Submasters

    // For each submaster Strip
    for (unsigned int i = 0; i < m_submasters.size(); ++i) {

        Strip &strip = m_submasters[i];

        LevelInfo info;

        // Get the level.  If there was no change, try the next Strip.
        if (!SequencerDataBlock::getInstance()->getSubmasterLevel(i, info))
            continue;

        // Convert to dB for display.
        // The values passed through are long-fader values
        float dBleft = AudioLevel::fader_to_dB(
                info.level, 127, AudioLevel::LongFader);
        float dBright = AudioLevel::fader_to_dB(
                info.levelRight, 127, AudioLevel::LongFader);

        // Update the meter.
        strip.m_meter->setLevel(dBleft, dBright);
    }

    // Master

    LevelInfo masterInfo;

    // Get the master level.  If there was a change, update the meter.
    if (SequencerDataBlock::getInstance()->getMasterLevel(masterInfo)) {

        // Convert to dB for display.
        float dBleft = AudioLevel::fader_to_dB(
                masterInfo.level, 127, AudioLevel::LongFader);
        float dBright = AudioLevel::fader_to_dB(
                masterInfo.levelRight, 127, AudioLevel::LongFader);

        // Update the meter.
        m_master.m_meter->setLevel(dBleft, dBright);
    }
}

void
AudioMixerWindow::updateMonitorMeters()
{
    // If we're playing, bail.
    // We only show monitor levels when stopped or when recording (as
    // record levels).
    if (m_document->getSequenceManager()  &&
        m_document->getSequenceManager()->getTransportStatus() == PLAYING)
        return;

    Composition &comp = m_document->getComposition();
    Composition::trackcontainer &tracks = comp.getTracks();

    // For each input Strip
    for (StripMap::iterator i = m_inputs.begin(); i != m_inputs.end(); ++i) {

        InstrumentId id = i->first;
        Strip &strip = i->second;

        if (!strip.m_populated)
            continue;

        LevelInfo info;

        // Get the record level.  If there was no change, try the next Strip.
        if (!SequencerDataBlock::getInstance()->
                getInstrumentRecordLevelForMixer(id, info))
            continue;

        bool armed = false;

        // For each Track in the Composition
        // ??? Performance: LINEAR SEARCH
        //     I see no easy fix.  Each Instrument would need to keep a list
        //     of the Tracks it is on.  Or something equally complicated.
        for (Composition::trackcontainer::iterator ti =
                 tracks.begin();
             ti != tracks.end();
             ++ti) {
            Track *track = ti->second;

            // If this Track has this Instrument
            if (track->getInstrument() == id) {
                if (comp.isTrackRecording(track->getId())) {
                    armed = true;
                    // Only one Track can be armed per Instrument.
                    break;
                }
            }
        }

        if (!armed)
            continue;

        // Convert to dB for display.
        // The values passed through are long-fader values
        float dBleft = AudioLevel::fader_to_dB(
                info.level, 127, AudioLevel::LongFader);

        if (strip.m_stereo) {
            // Convert to dB for display.
            float dBright = AudioLevel::fader_to_dB(
                    info.levelRight, 127, AudioLevel::LongFader);

            strip.m_meter->setRecordLevel(dBleft, dBright);
        } else {
            strip.m_meter->setRecordLevel(dBleft);
        }
    }
}

void
AudioMixerWindow::slotControllerDeviceEventReceived(MappedEvent *e,
        const void *preferredCustomer)
{
    if (preferredCustomer != this)
        return;

    //RG_DEBUG << "slotControllerDeviceEventReceived(): this one's for me";

    raise();

    // If this isn't a MIDI controller, bail.
    if (e->getType() != MappedEvent::MidiController)
        return;

    // Get channel number n from MappedEvent.
    // Update Instrument for nth input Strip in m_inputs.

    unsigned int channel = e->getRecordedChannel();
    if (channel >= m_inputs.size())
        return;

    MidiByte controller = e->getData1();
    MidiByte value = e->getData2();

    // ??? Yet another search.  Would anyone else benefit from a
    //     std::vector<Strip *> for finding the nth Strip?

    StripMap::iterator stripIter = m_inputs.begin();

    // Skip to the requested input Strip.
    for (unsigned i = 0; i < channel; ++i)
        ++stripIter;

    Instrument *instrument =
        m_studio->getInstrumentById(stripIter->first);

    if (!instrument)
        return;

    switch (controller) {

    case MIDI_CONTROLLER_VOLUME:
        {
            float level = AudioLevel::fader_to_dB(
                    value, 127, AudioLevel::LongFader);

            StudioControl::setStudioObjectProperty(
                    instrument->getMappedId(),
                    MappedAudioFader::FaderLevel,
                    MappedObjectValue(level));

            instrument->setLevel(level);
            // Inform everyone of the change.  Note that this will also call
            // slotInstrumentChanged() to update the input strip.
            instrument->changed();

            break;
        }

    case MIDI_CONTROLLER_PAN:
        {
            // Convert to audio Instrument pan format, 0-200.
            MidiByte ipan = MidiByte((value / 64.0) * 100.0 + 0.01);

            StudioControl::setStudioObjectProperty(
                    instrument->getMappedId(),
                    MappedAudioFader::Pan,
                    MappedObjectValue(float(ipan) - 100));  // -100 - 100

            instrument->setPan(ipan);
            // Inform everyone of the change.  Note that this will also call
            // slotInstrumentChanged() to update the input strip.
            instrument->changed();

            break;
        }

    default:
        break;
    }
}

void
AudioMixerWindow::slotNumberOfStereoInputs()
{
    const QAction *action = dynamic_cast<const QAction *>(sender());

    if (!action)
        return;

    QString name = action->objectName();

    // Not the expected action name?  Bail.
    if (name.left(7) != "inputs_")
        return;

    // Extract the number of inputs from the action name.
    unsigned count = name.mid(7).toUInt();

    // No change?  Bail.
    if (count == m_studio->getRecordIns().size())
        return;

    // ??? Pull the next few lines into a Studio::setRecordInCount(int n).

    // Remove all except 1.
    m_studio->clearRecordIns();

    // Add the rest.
    for (unsigned i = 1; i < count; ++i) {
        m_studio->addRecordIn(new RecordIn());
    }

    m_document->initialiseStudio();

    // For each input Strip
    for (StripMap::iterator i = m_inputs.begin();
         i != m_inputs.end();
         ++i) {
        updateRouteButtons(i->first);
    }
}

void
AudioMixerWindow::slotNumberOfSubmasters()
{
    const QAction *action = dynamic_cast<const QAction *>(sender());

    if (!action)
        return;

    QString name = action->objectName();

    // Not the expected action name?  Bail.
    if (name.left(11) != "submasters_")
        return;

    // Extract the count from the name.
    int count = name.mid(11).toInt();

    // Add one for the master buss.
    m_studio->setBussCount(count + 1);

    m_document->initialiseStudio();

    populate();
}

void AudioMixerWindow::slotPanningLaw()
{
    const QAction *action = dynamic_cast<const QAction *>(sender());

    if (!action)
        return;

    QString name = action->objectName();

    // Not the expected action name?  Bail.
    if (name.left(7) != "panlaw_")
        return;

    int panLaw = name.mid(7).toInt();

    AudioLevel::setPanLaw(panLaw);
}

void AudioMixerWindow::Strip::setVisible(bool visible)
{
    if (m_input)
        m_input->getWidget()->setVisible(visible);
    if (m_output)
        m_output->getWidget()->setVisible(visible);
    if (m_pan)
        m_pan->setVisible(visible);
    if (m_fader)
        m_fader->setVisible(visible);
    if (m_meter)
        m_meter->setVisible(visible);
    if (m_stereoButton)
        m_stereoButton->setVisible(visible);

    setPluginButtonsVisible(visible);
}

void
AudioMixerWindow::Strip::setPluginButtonsVisible(bool visible)
{
    if (m_pluginBox)
        m_pluginBox->setVisible(visible);
}

void
AudioMixerWindow::slotShowAudioFaders()
{
    m_studio->amwShowAudioFaders = !m_studio->amwShowAudioFaders;

    updateFaderVisibility();
}

void
AudioMixerWindow::updateFaderVisibility()
{
    bool visible = m_studio->amwShowAudioFaders;

    QAction *action = findAction("show_audio_faders");
    if (action)
        action->setChecked(visible);

    //RG_DEBUG << "updateFaderVisibility(): visibility is " << d;

    // For each input Strip.
    for (StripMap::iterator i = m_inputs.begin(); i != m_inputs.end(); ++i) {
        // If it's an audio input, set its visibility.
        if (i->first < SoftSynthInstrumentBase)
            i->second.setVisible(visible);
    }

    toggleNamedWidgets(visible, "audioIdLabel");

    adjustSize();
}

void
AudioMixerWindow::slotShowSynthFaders()
{
    m_studio->amwShowSynthFaders = !m_studio->amwShowSynthFaders;

    updateSynthFaderVisibility();
}

void
AudioMixerWindow::updateSynthFaderVisibility()
{
    bool visible = m_studio->amwShowSynthFaders;

    QAction *action = findAction("show_synth_faders");
    if (action)
        action->setChecked(visible);

    // For each input Strip.
    for (StripMap::iterator i = m_inputs.begin(); i != m_inputs.end(); ++i) {
        // If it's a softsynth input, set its visibility.
        if (i->first >= SoftSynthInstrumentBase)
            i->second.setVisible(visible);
    }

    toggleNamedWidgets(visible, "synthIdLabel");

    adjustSize();
}

void
AudioMixerWindow::slotShowAudioSubmasters()
{
    m_studio->amwShowAudioSubmasters = !m_studio->amwShowAudioSubmasters;

    updateSubmasterVisibility();
}

void
AudioMixerWindow::updateSubmasterVisibility()
{
    bool visible = m_studio->amwShowAudioSubmasters;

    QAction *action = findAction("show_audio_submasters");
    if (action)
        action->setChecked(visible);

    // For each submaster
    for (StripVector::iterator i = m_submasters.begin();
         i != m_submasters.end();
         ++i) {
        i->setVisible(visible);
    }

    toggleNamedWidgets(visible, "subMaster");

    adjustSize();
}

void
AudioMixerWindow::slotShowPluginButtons()
{
    m_studio->amwShowPluginButtons = !m_studio->amwShowPluginButtons;

    updatePluginButtonVisibility();
}

void
AudioMixerWindow::updatePluginButtonVisibility()
{
    bool visible = m_studio->amwShowPluginButtons;

    QAction *action = findAction("show_plugin_buttons");
    if (action)
        action->setChecked(visible);

    // For each input Strip
    for (StripMap::iterator i = m_inputs.begin(); i != m_inputs.end(); ++i) {
        i->second.setPluginButtonsVisible(visible);
    }

    // ??? What about the submaster Strips?

    adjustSize();
}

void
AudioMixerWindow::slotShowUnassignedFaders()
{
    // ??? When hiding these, the layout doesn't compress horizontally
    //     as it is supposed to.  Usually the calls to adjustSize() fix
    //     this, but not in this case.

    m_studio->amwShowUnassignedFaders = !m_studio->amwShowUnassignedFaders;

    QAction *action = findAction("show_unassigned_faders");
    if (action)
        action->setChecked(m_studio->amwShowUnassignedFaders);

    populate();
}

void
AudioMixerWindow::toggleNamedWidgets(bool show, const QString &name)
{
    //RG_DEBUG << "toggleNamedWidgets(" << show << ", " << name << ")";

    // ??? An alternative approach that would be more direct would
    //     be to add the label to the Strip so they come and go
    //     like all the others.  m_label has been added to Strip.
    //     Move away from this.  Should be able to get rid of this
    //     routine.

    if (!m_mainBox)
        return;

    const QObjectList &children = m_mainBox->children();

    // For each child
    for (int i = 0; i < children.size(); ++i)
    {
        QWidget *widget = dynamic_cast<QWidget *>(children.at(i));
        if (!widget)
            continue;

        if (widget->objectName() == name)
            widget->setVisible(show);
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
AudioMixerWindow::slotHelp()
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
AudioMixerWindow::slotAboutRosegarden()
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
