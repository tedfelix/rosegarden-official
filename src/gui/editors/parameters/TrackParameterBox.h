/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2021 the Rosegarden development team.

    This file is Copyright 2006
        Pedro Lopez-Cabanillas <plcl@users.sourceforge.net>
        D. Michael McIntyre <dmmcintyr@users.sourceforge.net>

    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#ifndef RG_TRACKPARAMETERBOX_H
#define RG_TRACKPARAMETERBOX_H

#include "RosegardenParameterBox.h"

#include "base/Track.h"
#include "base/Composition.h"
#include "gui/widgets/ColourTable.h"
#include "base/Device.h"  // For DeviceId
#include "base/Instrument.h"
#include "gui/widgets/SqueezedLabel.h"


#include <QString>
#include <QStringList>

class QWidget;
class QPushButton;
class QLabel;
class QComboBox;
class QCheckBox;

#include <map>
#include <vector>

namespace Rosegarden
{


class CollapsingFrame;
class ColorCombo;
class RosegardenDocument;


/// The "Track Parameters" box in the left pane of the main window.
class TrackParameterBox : public RosegardenParameterBox,
                          public CompositionObserver
{
    Q_OBJECT
        
public:
    TrackParameterBox(QWidget *parent = nullptr);
    
    void setDocument(RosegardenDocument *doc);

    // CompositionObserver overrides.
    void trackChanged(const Composition *comp, Track *track) override;
    void trackSelectionChanged(const Composition *, TrackId) override;

public slots:
    /// Connected to RosegardenDocument::docColoursChanged().
    void slotDocColoursChanged();

    /// Refresh the Playback and Recording Device lists.
    /**
     * Connected to DeviceManagerDialog::deviceNamesChanged() and
     * BankEditorDialog::deviceNamesChanged().
     */
    void devicesChanged();

signals:
    /// Connected to TrackButtons::slotTPBInstrumentSelected().
    /**
     *  ??? This is never emitted.
     */
    void instrumentSelected(TrackId, int);

private slots:
    /// Called when a new document is loaded.
    void slotDocumentLoaded(RosegardenDocument *);
    /// Called when the document is modified in some way.
    void slotDocumentModified(bool);

    // Signals from widgets.

    /// Playback parameters: Device
    void slotPlaybackDeviceChanged(int index);
    /// Playback parameters: Instrument
    void slotInstrumentChanged(int index);
    /// Playback parameters: Archive
    void slotArchiveChanged(bool checked);

    /// Recording filters: Device
    void slotRecordingDeviceChanged(int index);
    /// Recording filters: Channel
    void slotRecordingChannelChanged(int index);
    /// Recording filters: Thru Routing
    void slotThruRoutingChanged(int index);

    /// Staff export options: Notation size
    void slotNotationSizeChanged(int index);
    /// Staff export options: Bracket type
    void slotBracketTypeChanged(int index);

    /// Create segments with: Preset Load
    void slotLoadPressed();
    /// Create segments with: Clef
    void slotClefChanged(int clef);
    /// Create segments with: Transpose
    void slotTransposeChanged(int index);
    /// Create segments with: Pitch Lowest
    void slotLowestPressed();
    /// Create segments with: Pitch Highest
    void slotHighestPressed();
    /// Create segments with: Color
    void slotColorChanged(int index);
    /// Create segments with: Reset to Defaults
    void slotResetToDefaultsPressed();

private:
    RosegardenDocument *m_doc;

    TrackId m_selectedTrackId;
    Track *getTrack();

    // Track number and name
    SqueezedLabel *m_trackLabel;

    // --- Playback parameters --------------------------------------

    /// Playback parameters: Device
    QComboBox *m_playbackDevice;
    /// Cache for detecting changes.
    std::vector<DeviceId> m_playbackDeviceIds2;
    /// Cache for detecting changes.
    QStringList m_playbackDeviceNames;

    /// Playback parameters: Instrument
    QComboBox *m_instrument;
    std::vector<InstrumentId> m_instrumentIds2;
    QStringList m_instrumentNames2;

    /// Playback parameters: Archive
    QCheckBox *m_archive;

    // --- Recording filters -------------------------------------------

    CollapsingFrame *m_recordingFiltersFrame;

    /// Recording filters: Device
    QComboBox *m_recordingDevice;
    std::vector<DeviceId> m_recordingDeviceIds2;
    QStringList m_recordingDeviceNames;
    /// Cache to detect change.
    Instrument::InstrumentType m_lastInstrumentType;

    /// Recording filters: Channel
    QComboBox *m_recordingChannel;

    /// Recording filters: Thru Routing
    QComboBox *m_thruRouting;

    // --- Staff export options -------------------------------------

    CollapsingFrame *m_staffExportOptionsFrame;

    /// Staff export options: Notation size
    QComboBox *m_notationSize;

    /// Staff export options: Bracket type
    QComboBox *m_bracketType;

    // --- Create segments with -------------------------------------

    CollapsingFrame *m_createSegmentsWithFrame;

    /// Create segments with: Preset Load
    QLabel *m_presetLabel;
    QLabel *m_preset;
    QPushButton *m_load;

    /// Create segments with: Clef
    QLabel *m_clefLabel;
    QComboBox *m_clef;

    /// Create segments with: Transpose
    QLabel *m_transposeLabel;
    QComboBox *m_transpose;

    /// Create segments with: Pitch (Lowest/Highest)
    QLabel *m_pitchLabel;
    QLabel *m_lowestLabel;
    QPushButton *m_lowest;
    QLabel *m_highestLabel;
    QPushButton *m_highest;

    /// Create segments with: Color
    ColorCombo *m_color;

    QPushButton *m_resetToDefaults;

    // ComboBox update routines for updateWidgets2().
    void updatePlaybackDevice(DeviceId deviceId);
    void updateInstrument(const Instrument *instrument);
    void updateRecordingDevice(DeviceId deviceId);
    /// Update all widgets from RosegardenDocument.
    /**
     * This new routine will replace all other update routines.  It will
     * update all widgets, but do it efficiently so that it can be called
     * in any situation without worrying about performance.
     *
     * See MIDIInstrumentParameterPanel::updateWidgets().
     */
    void updateWidgets2();
};


}

#endif
