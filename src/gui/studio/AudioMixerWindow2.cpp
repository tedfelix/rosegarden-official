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
#include "document/RosegardenDocument.h"
#include "gui/application/RosegardenMainWindow.h"
#include "base/Studio.h"

#include <QDesktopServices>

namespace Rosegarden
{


AudioMixerWindow2::AudioMixerWindow2(QWidget *parent) :
        QMainWindow(parent),
        m_masterStrip(new AudioStrip(this, 0))
{
    setObjectName("AudioMixerWindow2");

    setWindowTitle(tr("Audio Mixer"));
    setWindowIcon(IconLoader().loadPixmap("window-audiomixer"));

    // This avoids using CPU when we're "closed".  Normally with Qt,
    // "closed" really just means "hidden".
    setAttribute(Qt::WA_DeleteOnClose);

    RosegardenDocument *doc = RosegardenMainWindow::self()->getDocument();
    // Connect for RosegardenDocument changes.
    // ??? If the doc changes, this will get disconnected.  Who do we have
    //     to connect to to get wind of document changes?
    connect(doc, SIGNAL(documentModified(bool)),
            SLOT(slotDocumentModified(bool)));

    // File > Close
    createAction("file_close", SLOT(slotClose()));

    // Transport Menu
    createAction("play", SLOT(slotPlay()));
    createAction("stop", SLOT(slotStop()));
    createAction("playback_pointer_back_bar", SLOT(slotRewind()));
    createAction("playback_pointer_forward_bar", SLOT(slotFastforward()));
    createAction("playback_pointer_start", SLOT(slotRewindToBeginning()));
    createAction("playback_pointer_end", SLOT(slotFastForwardToEnd()));
    createAction("record", SLOT(slotRecord()));
    createAction("panic", SLOT(slotPanic()));

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

    // ??? TESTING
    setCentralWidget(m_masterStrip);

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
            if (instrumentIds[i] != m_inputStrips[i]->id) {
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
        // ??? Adjust the size of m_inputStrips.

        // Be sure to delete the AudioStrip objects.

        recreateLayout = true;
    }

    // If the submaster Strip count is wrong, fix it.
    if (!submasterStripsMatch) {
        // ??? Adjust the size of m_submasterStrips.

        // Be sure to delete the AudioStrip objects.

        recreateLayout = true;
    }

    // If either Strip count was wrong, recreate the layout.
    if (recreateLayout) {
        // Can we clear the layout somehow, or do we need to delete it
        // and then recreate it?  I would think that would be a problem
        // since it is parent to all the widgets.  Deleting it will destroy
        // all the child widgets.

        // Docs for QLayout::takeAt() show how to safely remove all
        // widgets from a layout:

        //QLayoutItem *child;
        //while ((child = m_layout->takeAt(0)) != 0) {
        //    delete child;
        //}

    }

    // Make sure the InstrumentIds are correct in each Strip since
    // Strips may have been shuffled.
    for (unsigned i = 0; i < m_inputStrips.size(); ++i) {
        m_inputStrips[i]->id = instrumentIds[i];
    }

    // ??? Submasters too?  Since they are fixed ids, we can probably
    //     handle it up where we adjust the count.
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

    //updateStripCounts();

    // At this point, the strips match the document.  We can just update them.

    // For each input strip, call updateWidgets().

    // For each submaster strip, call updateWidgets().

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
AudioMixerWindow2::slotPlay()
{
    // Delegate to RMW.
    RosegardenMainWindow::self()->slotPlay();
}

void
AudioMixerWindow2::slotStop()
{
    // Delegate to RMW.
    RosegardenMainWindow::self()->slotStop();
}

void
AudioMixerWindow2::slotRewind()
{
    // Delegate to RMW.
    RosegardenMainWindow::self()->slotRewind();
}

void
AudioMixerWindow2::slotFastforward()
{
    // Delegate to RMW.
    RosegardenMainWindow::self()->slotFastforward();
}

void
AudioMixerWindow2::slotRewindToBeginning()
{
    // Delegate to RMW.
    RosegardenMainWindow::self()->slotRewindToBeginning();
}

void
AudioMixerWindow2::slotFastForwardToEnd()
{
    // Delegate to RMW.
    RosegardenMainWindow::self()->slotFastForwardToEnd();
}

void
AudioMixerWindow2::slotRecord()
{
    // Delegate to RMW.
    RosegardenMainWindow::self()->slotRecord();
}

void
AudioMixerWindow2::slotPanic()
{
    // Delegate to RMW.
    RosegardenMainWindow::self()->slotPanic();
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


}
