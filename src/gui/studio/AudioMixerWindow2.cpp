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

#define RG_MODULE_STRING "[AudioMixerWindow2]"

#include "AudioMixerWindow2.h"

#include "AudioStrip.h"

#include "gui/dialogs/AboutDialog.h"
#include "base/AudioLevel.h"
#include "misc/Debug.h"
#include "gui/general/IconLoader.h"
#include "base/Instrument.h"
#include "base/InstrumentStaticSignals.h"
#include "sound/MappedEvent.h"
#include "document/RosegardenDocument.h"
#include "gui/application/RosegardenMainViewWidget.h"
#include "gui/application/RosegardenMainWindow.h"
#include "base/Studio.h"

#include <QDesktopServices>
#include <QHBoxLayout>

namespace Rosegarden
{


AudioMixerWindow2::AudioMixerWindow2(QWidget *parent) :
        QMainWindow(parent),
        m_centralWidget(new QWidget),
        m_layout(new QHBoxLayout(m_centralWidget)),
        m_masterStrip(new AudioStrip(this, 0))
{
    setObjectName("AudioMixerWindow2");

    setWindowTitle(tr("Audio Mixer"));
    setWindowIcon(IconLoader().loadPixmap("window-audiomixer"));

    // This avoids using CPU when we're "closed".  Normally with Qt,
    // "closed" really just means "hidden".
    setAttribute(Qt::WA_DeleteOnClose);

    // Connect for RosegardenDocument changes.
    // ??? If the doc changes, this will get disconnected.  Who do we have
    //     to connect to to get wind of document changes?
    //     RMW::documentAboutToChange()?
    connect(RosegardenMainWindow::self()->getDocument(),
                SIGNAL(documentModified(bool)),
            SLOT(slotDocumentModified(bool)));
    // Connect for External Controller events.
    connect(RosegardenMainWindow::self()->getView(),
                SIGNAL(controllerDeviceEventReceived(MappedEvent *, const void *)),
            SLOT(slotExternalControllerEvent(MappedEvent *, const void *)));
    // Connect to make sure "external controller" events get here when
    // we are the active window.
    connect(this, SIGNAL(windowActivated()),
            RosegardenMainWindow::self()->getView(),
                SLOT(slotActiveMainWindowChanged()));
    // Connect for high-frequency control change notifications.
    connect(Instrument::getStaticSignals().data(),
                SIGNAL(controlChange(Instrument *, int)),
            SLOT(slotControlChange(Instrument *, int)));

    // File > Close
    createAction("file_close", SLOT(slotClose()));

    // Transport Menu
    createAction("play", RosegardenMainWindow::self(), SLOT(slotPlay()));
    createAction("stop", RosegardenMainWindow::self(), SLOT(slotStop()));
    createAction("playback_pointer_back_bar", RosegardenMainWindow::self(), SLOT(slotRewind()));
    createAction("playback_pointer_forward_bar", RosegardenMainWindow::self(), SLOT(slotFastforward()));
    createAction("playback_pointer_start", RosegardenMainWindow::self(), SLOT(slotRewindToBeginning()));
    createAction("playback_pointer_end", RosegardenMainWindow::self(), SLOT(slotFastForwardToEnd()));
    createAction("record", RosegardenMainWindow::self(), SLOT(slotRecord()));
    createAction("panic", RosegardenMainWindow::self(), SLOT(slotPanic()));

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

    // Help > Help
    createAction("mixer_help", SLOT(slotHelp()));

    // Help > About Rosegarden
    createAction("help_about_app", SLOT(slotAboutRosegarden()));

    createMenusAndToolbars("mixer.rc");

    // Set the rewind and fast-forward buttons for auto-repeat.
    enableAutoRepeat("Transport Toolbar", "playback_pointer_back_bar");
    enableAutoRepeat("Transport Toolbar", "playback_pointer_forward_bar");

    // Since we can't set the layout for QMainWindow, we have to use
    // a central widget and set the layout there.
    setCentralWidget(m_centralWidget);

    // Space between strips.
    m_layout->setSpacing(7);

    // Force an initial update to make sure we're in sync.
    updateWidgets();

    show();
}

AudioMixerWindow2::~AudioMixerWindow2()
{
}

void
AudioMixerWindow2::updateStripCounts()
{
    RosegardenDocument *doc = RosegardenMainWindow::self()->getDocument();
    Studio &studio = doc->getStudio();

    // Verify Input Strips

    InstrumentList instruments = studio.getPresentationInstruments();
    std::vector<InstrumentId> instrumentIds;

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
        if (!studio.amwShowUnassignedFaders  &&
            !doc->getComposition().hasTrack(instrument->getId()))
            continue;

        // Add to list
        instrumentIds.push_back(instrument->getId());
    }

    // First, make sure the sizes match.
    bool inputStripsMatch = (instrumentIds.size() == m_inputStrips.size());

    // If the sizes match, make sure the ids match.
    if (inputStripsMatch) {
        for (unsigned i = 0; i < instrumentIds.size(); ++i) {
            if (instrumentIds[i] != static_cast<InstrumentId>(m_inputStrips[i]->getId())) {
                inputStripsMatch = false;
                break;
            }
        }
    }

    bool submasterStripsMatch =
            (m_submasterStrips.size() == studio.getBusses().size()-1);

    // If everything is in order, bail.
    if (inputStripsMatch  &&  submasterStripsMatch)
        return;

    // Changes to the strip counts and positions are very rare, so there is
    // no need to be very efficient from here on out.  Go for readable
    // instead.

    bool recreateLayout = false;

    // If the input Strip count is wrong, fix it.
    if (m_inputStrips.size() != instrumentIds.size()) {
        // If we don't have enough strips
        if (m_inputStrips.size() < instrumentIds.size()) {
            unsigned count = instrumentIds.size() - m_inputStrips.size();
            // Add the appropriate number.
            for (unsigned i = 0; i < count; ++i) {
                // Add a new one.  We'll fix the ID later.
                m_inputStrips.push_back(new AudioStrip(this));
            }
        } else {  // We have too many input strips
            unsigned count = m_inputStrips.size() - instrumentIds.size();
            // Remove the appropriate number.
            for (unsigned i = 0; i < count; ++i) {
                // Delete and remove.
                delete m_inputStrips.back();
                m_inputStrips.pop_back();
            }
        }

        recreateLayout = true;
    }

    // If the submaster Strip count is wrong, fix it.
    if (!submasterStripsMatch) {
        // If we don't have enough strips
        if (m_submasterStrips.size() < studio.getBusses().size()-1) {
            unsigned count =
                    (studio.getBusses().size()-1) - m_submasterStrips.size();
            // Add the appropriate number
            for (unsigned i = 0; i < count; ++i) {
                int id = static_cast<int>(m_submasterStrips.size()) + 1;
                m_submasterStrips.push_back(new AudioStrip(this, id));
            }
        } else {  // We have too many submaster strips
            unsigned count =
                    m_submasterStrips.size() - (studio.getBusses().size()-1);
            // Remove the appropriate number
            for (unsigned i = 0; i < count; ++i) {
                // Delete and remove.
                delete m_submasterStrips.back();
                m_submasterStrips.pop_back();
            }
        }

        recreateLayout = true;
    }

    // If either Strip count was wrong, recreate the layout.
    if (recreateLayout) {
        // Remove the widgets from the layout.
        while (m_layout->takeAt(0) != 0) {
            // Do nothing.  Normally, we might delete the child widgets.
            // But we're about to re-add them below.
        }

        // Add the input strips
        for (unsigned i = 0; i < m_inputStrips.size(); ++i) {
            m_layout->addWidget(m_inputStrips[i]);
        }

        // Add the submaster strips
        for (unsigned i = 0; i < m_submasterStrips.size(); ++i) {
            m_layout->addWidget(m_submasterStrips[i]);
        }

        // Add the master strip
        m_layout->addWidget(m_masterStrip);
    }

    // Make sure the InstrumentIds are correct in each input Strip.
    for (unsigned i = 0; i < m_inputStrips.size(); ++i) {
        m_inputStrips[i]->setId(instrumentIds[i]);
    }
}

void AudioMixerWindow2::updateWidgets()
{
    RosegardenDocument *doc = RosegardenMainWindow::self()->getDocument();
    Studio &studio = doc->getStudio();

    // Menu items.

    // Update "Settings > Number of Stereo Inputs"
    findAction(QString("inputs_%1").arg(studio.getRecordIns().size()))->
            setChecked(true);

    // Update "Settings > Number of Submasters"
    findAction(QString("submasters_%1").arg(studio.getBusses().size()-1))->
            setChecked(true);

    // Update "Settings > Panning Law"
    findAction(QString("panlaw_%1").arg(AudioLevel::getPanLaw()))->
            setChecked(true);

    bool visible = studio.amwShowAudioFaders;
    QAction *action = findAction("show_audio_faders");
    if (action)
        action->setChecked(visible);

    // ??? The strips will query studio.amwShowAudioFaders and show/hide
    //     the faders as appropriate.

    visible = studio.amwShowSynthFaders;
    action = findAction("show_synth_faders");
    if (action)
        action->setChecked(visible);

    visible = studio.amwShowAudioSubmasters;
    action = findAction("show_audio_submasters");
    if (action)
        action->setChecked(visible);

    visible = studio.amwShowPluginButtons;
    action = findAction("show_plugin_buttons");
    if (action)
        action->setChecked(visible);

    visible = studio.amwShowUnassignedFaders;
    action = findAction("show_unassigned_faders");
    if (action)
        action->setChecked(visible);

    updateStripCounts();

    // At this point, the strips match the document.  We can just update them.

    // Update the input strips.
    for (unsigned i = 0; i < m_inputStrips.size(); ++i) {
        m_inputStrips[i]->updateWidgets();

        if (i < 16)
            m_inputStrips[i]->setExternalControllerChannel(i);
    }

    // Update the submaster strips.
    for (unsigned i = 0; i < m_submasterStrips.size(); ++i) {
        m_submasterStrips[i]->updateWidgets();
    }

    // Update the master strip.
    m_masterStrip->updateWidgets();
}

void AudioMixerWindow2::slotDocumentModified(bool /*modified*/)
{
    // ??? Does this get hit hard when recording???  Need to determine.
    //     If so, then we need to figure out how to handle it.

    // It's really this simple.
    updateWidgets();
}

void
AudioMixerWindow2::slotClose()
{
    close();
}

void
AudioMixerWindow2::slotNumberOfStereoInputs()
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

    RosegardenDocument *doc = RosegardenMainWindow::self()->getDocument();
    Studio &studio = doc->getStudio();

    studio.setRecordInCount(count);

    // Set the mapped IDs for the RecordIns.
    // ??? Overkill?  Can we do better?
    doc->initialiseStudio();

    doc->slotDocumentModified();
}

void
AudioMixerWindow2::slotNumberOfSubmasters()
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

    RosegardenDocument *doc = RosegardenMainWindow::self()->getDocument();
    Studio &studio = doc->getStudio();

    // Add one for the master buss.
    studio.setBussCount(count + 1);

    doc->initialiseStudio();

    doc->slotDocumentModified();
}

void
AudioMixerWindow2::slotPanningLaw()
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

    RosegardenDocument *doc = RosegardenMainWindow::self()->getDocument();

    doc->slotDocumentModified();
}

void
AudioMixerWindow2::slotShowAudioFaders()
{
    RosegardenDocument *doc = RosegardenMainWindow::self()->getDocument();
    Studio &studio = doc->getStudio();

    studio.amwShowAudioFaders = !studio.amwShowAudioFaders;

    doc->slotDocumentModified();
}

void
AudioMixerWindow2::slotShowSynthFaders()
{
    RosegardenDocument *doc = RosegardenMainWindow::self()->getDocument();
    Studio &studio = doc->getStudio();

    studio.amwShowSynthFaders = !studio.amwShowSynthFaders;

    doc->slotDocumentModified();
}

void
AudioMixerWindow2::slotShowAudioSubmasters()
{
    RosegardenDocument *doc = RosegardenMainWindow::self()->getDocument();
    Studio &studio = doc->getStudio();

    studio.amwShowAudioSubmasters = !studio.amwShowAudioSubmasters;

    doc->slotDocumentModified();
}

void
AudioMixerWindow2::slotShowPluginButtons()
{
    RosegardenDocument *doc = RosegardenMainWindow::self()->getDocument();
    Studio &studio = doc->getStudio();

    studio.amwShowPluginButtons = !studio.amwShowPluginButtons;

    doc->slotDocumentModified();
}

void
AudioMixerWindow2::slotShowUnassignedFaders()
{
    RosegardenDocument *doc = RosegardenMainWindow::self()->getDocument();
    Studio &studio = doc->getStudio();

    studio.amwShowUnassignedFaders = !studio.amwShowUnassignedFaders;

    doc->slotDocumentModified();
}

void
AudioMixerWindow2::slotHelp()
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
AudioMixerWindow2::slotAboutRosegarden()
{
    new AboutDialog(this);
}

void
AudioMixerWindow2::slotExternalControllerEvent(
        MappedEvent *event,
        const void *preferredCustomer)
{
    if (preferredCustomer != this)
        return;

    //RG_DEBUG << "slotExternalControllerEvent(): this one's for me";

    raise();

    // If this isn't a MIDI controller, bail.
    if (event->getType() != MappedEvent::MidiController)
        return;

    unsigned channel = event->getRecordedChannel();
    if (channel >= m_inputStrips.size())
        return;

    MidiByte controller = event->getData1();
    MidiByte value = event->getData2();

    switch (controller) {

    case MIDI_CONTROLLER_VOLUME:
        {
            float level = AudioLevel::fader_to_dB(
                    value, 127, AudioLevel::LongFader);

            m_inputStrips[channel]->faderLevelChanged(level);

            break;
        }

    case MIDI_CONTROLLER_PAN:
        {
            // Convert to approximately (-100, 100).
            // ??? Why the epsilon (.01)?  To avoid -100?
            double pan = static_cast<double>(value) / 64 * 100 + .01 - 100;

            // ??? This gives -100 to 100:
            //
            //      double pan = 0;
            //      if (value <= 64)
            //          pan = static_cast<double>(value) / 64 * 100 - 100;
            //      else
            //          pan = (static_cast<double>(value) - 64) / 63 * 100;
            //
            //     Might be a good idea to factor out some standard
            //     pan math functions to be reused everywhere.  The AudioLevel
            //     class might be a good place to put them.
            //
            //     // Normal is [-1,1].  -1 = left, 0 = center, 1 = right.
            //     MidiByte panNormalToMidi(double pan);
            //     double panMidiToNormal(MidiByte pan);

            m_inputStrips[channel]->panChanged(static_cast<float>(pan));

            break;
        }

    default:
        break;
    }
}

void
AudioMixerWindow2::slotControlChange(Instrument *instrument, int cc)
{
    InstrumentId instrumentId = instrument->getId();

    // for each input strip
    // ??? Performance: LINEAR SEARCH.  std::map<id, strip> might be better.
    for (unsigned i = 0; i < m_inputStrips.size(); ++i) {
        // If this is the one
        if (m_inputStrips[i]->getId() == instrumentId) {
            m_inputStrips[i]->controlChange(cc);
            break;
        }
    }
}

void
AudioMixerWindow2::changeEvent(QEvent *event)
{
    // Let baseclass handle first.
    // ??? Is this really the right thing to do?
    QWidget::changeEvent(event);

    if (event->type() == QEvent::ActivationChange) {
        //RG_DEBUG << "changeEvent(): Received activation change.";
        if (isActiveWindow()) {
            emit windowActivated();

            size_t count = m_inputStrips.size();
            if (count > 16)
                count = 16;

            // For each strip, send vol/pan to the external controller port.
            for (unsigned i = 0; i < count; i++)
                m_inputStrips[i]->updateExternalController();
        }
    }
}


}
