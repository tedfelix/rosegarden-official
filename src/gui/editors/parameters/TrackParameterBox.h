/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2016 the Rosegarden development team.

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

#include <QString>

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
class RosegardenDocument;


/// The "Track Parameters" box in the left pane of the main window.
class TrackParameterBox : public RosegardenParameterBox,
                          public CompositionObserver
{
    Q_OBJECT
        
public:
    TrackParameterBox(RosegardenDocument *doc, QWidget *parent = 0);
    
    void setDocument(RosegardenDocument *doc);

    // CompositionObserver overrides.
    virtual void trackChanged(const Composition *comp, Track *track);
    virtual void trackSelectionChanged(const Composition *, TrackId);

public slots:
    /// Connected to RosegardenDocument::docColoursChanged().
    /**
     * ??? This is a selective refresh.  Recommend combining with all others
     *     into a single full refresh (updateWidgets2()) and optimize.
     */
    void slotDocColoursChanged();

    /// Connected to InstrumentStaticSignals::changed().
    /**
     * ??? This is a selective refresh.  Recommend combining with all others
     *     into a single full refresh (updateWidgets2()) and optimize.
     */
    void slotInstrumentChanged(Instrument *instrument);

    /// Refresh the Playback and Recording Device lists.
    /**
     * Connected to DeviceManagerDialog::deviceNamesChanged() and
     * BankEditorDialog::deviceNamesChanged().
     *
     * ??? Is this the updateWidgets() I'm looking for?
     */
    void slotPopulateDeviceLists();

signals:
    /// Connected to TrackButtons::slotTPBInstrumentSelected().
    void instrumentSelected(TrackId, int);

private slots:
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

private:
    RosegardenDocument *m_doc;

    TrackId m_selectedTrackId;
    Track *getTrack();

    // Track number and name
    QLabel *m_trackLabel;
    void selectedTrackNameChanged();

    // --- Playback parameters --------------------------------------

    /// Playback parameters: Device
    QComboBox *m_playbackDevice;
    typedef std::vector<DeviceId> IdsVector;
    IdsVector m_playbackDeviceIds;
    void populatePlaybackDeviceList();
    /// Cache for detecting changes.
    std::vector<DeviceId> m_playbackDeviceIds2;
    /// Cache for detecting changes.
    std::vector<std::string> m_playbackDeviceNames;

    /// Playback parameters: Instrument
    QComboBox *m_instrument;
    std::map<DeviceId, IdsVector> m_instrumentIds;
    std::map<DeviceId, QStringList> m_instrumentNames;
    std::vector<InstrumentId> m_instrumentIds2;
    std::vector<QString> m_instrumentNames2;

    /// Playback parameters: Archive
    QCheckBox *m_archive;

    // --- Recording filters -------------------------------------------

    CollapsingFrame *m_recordingFiltersFrame;

    /// Recording filters: Device
    QComboBox *m_recordingDevice;
    IdsVector m_recordingDeviceIds;
    std::vector<DeviceId> m_recordingDeviceIds2;
    std::vector<QString> m_recordingDeviceNames;
    /// Cache to detect change.
    Instrument::InstrumentType m_lastInstrumentType;
    void populateRecordingDeviceList();

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
    QLabel *m_preset;
    QPushButton *m_load;

    /// Create segments with: Clef
    QComboBox *m_clef;

    /// Create segments with: Transpose
    QComboBox *m_transpose;
    void transposeChanged(int transpose);

    /// Create segments with: Pitch (Lowest/Highest)
    QPushButton *m_lowest;
    QPushButton *m_highest;
    /// MIDI Pitch.
    int m_lowestPlayable;
    /// MIDI Pitch.
    int m_highestPlayable;
    void updateHighLow();

    /// Create segments with: Color
    QComboBox *m_color;

    /**
     * ??? This is a selective refresh.  Recommend combining with all others
     *     into a single full refresh (updateWidgets2()) and optimize.
     */
    void updateWidgets();

    // ComboBox update routines for updateWidgets2().
    void updatePlaybackDevice(DeviceId deviceId);
    void updateInstrument(const Instrument *instrument);
    void updateRecordDevice(DeviceId deviceId);
    /// Update all widgets from RosegardenDocument.
    /**
     * This new routine will replace all other update routines.  It will
     * update all widgets, but do it efficiently so that it can be called
     * in any situation without worrying about performance.
     *
     * See MIDIInstrumentParameterPanel::updateWidgets().
     */
    void updateWidgets2();

    // DEBUG
    // Enables the use of updateWidgets2() for testing.  Use this while
    // updateWidgets2() is in progress.
    bool m_testNewUpdate;
};


}

#endif
