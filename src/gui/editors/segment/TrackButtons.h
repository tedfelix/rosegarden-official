/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2012 the Rosegarden development team.

    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#ifndef _RG_TRACKBUTTONS_H_
#define _RG_TRACKBUTTONS_H_

#include "base/Composition.h"
#include "base/MidiProgram.h"
#include "base/Track.h"
#include "gui/application/RosegardenMainWindow.h"
#include "TrackLabel.h"
#include <QFrame>
#include <QString>
#include <vector>


class QWidget;
class QVBoxLayout;
class QSignalMapper;
class QPopupMenu;
class QObject;


namespace Rosegarden
{

class TrackVUMeter;
class RosegardenDocument;
class LedButton;
class Instrument;

/// The QFrame containing the various widgets for each track.
/**
 * The TrackButtons class is a QFrame that runs vertically along the left
 * side of the tracks (CompositionView).  It contains an "HBox" QFrame for
 * each track.  That HBox contains the widgets that appear to the left of
 * each track:
 *
 *   - The TrackVUMeter (which doubles as the track number)
 *   - The Mute LedButton
 *   - The Record LedButton
 *   - The TrackLabel (defaults to "<untitled>")
 *
 * These widgets are created based on the RosegardenDocument.
 *
 * Suggestion: This class appears to be the focus for track related changes.
 * It would probably be better to have the system make direct changes to
 * Composition, then call a notification routine which would trigger an
 * update to this UI.  This should result in simpler code.
 */
class TrackButtons : public QFrame, CompositionObserver
{
    Q_OBJECT
public:

    /**
     * @param[in] trackCellHeight Height of each track including the gap
     *                            between tracks.  See m_cellSize.
     * @param[in] trackLabelWidth Width of the TrackLabel portion.  See
     *                            m_trackLabelWidth.
     * @param[in] showTrackLabels true => track names are displayed.
     *     false => instrument names are displayed.  See
     *     m_trackInstrumentLabels and changeTrackInstrumentLabels().
     * @param[in] overallHeight   Height of the entire TrackButtons frame.
     */
    TrackButtons(RosegardenDocument* doc,
                 unsigned int trackCellHeight,
                 unsigned int trackLabelWidth,
                 bool showTrackLabels,
                 int overallHeight,
                 QWidget* parent = 0);

    ~TrackButtons();

    /// Return a vector of muted track positions
    std::vector<int> mutedTracks();

    /// Return a vector of highlighted track positions
    /// @see selectLabel()
    std::vector<int> getHighlightedTracks();

    /// Change the track labels between track, instrument, or both.
    /// rename: changeLabelDisplayMode()
    void changeTrackInstrumentLabels(TrackLabel::DisplayMode mode);

    /**
     * Change the instrument label to something else like
     * an actual program name rather than a meaningless
     * device number and MIDI channel.
     */
    void changeInstrumentLabel(InstrumentId id, QString label);

    /// Set the label (track name) that is normally displayed.
    /// @see slotRenameTrack()
    void changeTrackLabel(TrackId id, QString label);

    /// Select the given track.  This displays it with a highlight.
    /// @see getHighlightedTracks()
    void selectLabel(int position);

    /// Set the mute button down or up.
    void setMuteButton(TrackId track, bool value);

    /// Set the record button down or up.
    void setRecordTrack(int position, bool value);

    /**
     * Precalculate the Instrument popup so we don't have to every
     * time it appears.
     *
     * @see RosegardenMainWindow::slotPopulateTrackInstrumentPopup()
     */
    void populateInstrumentPopup(Instrument *thisTrackInstr, QMenu* instrumentPopup);

signals:
    /// Emitted when a track has been selected.
    /// @see slotLabelSelected()
    void trackSelected(int trackId);
    /// Emitted when an instrument is selected from the popup.
    /// @see slotInstrumentPopupActivated()
    void instrumentSelected(int instrumentId);

    /// Emitted when the track label changes.
    /// @see changeTrackLabel()
    void widthChanged();

    /// Emitted when a track label changes.
    /// Tell the notation canvas &c when a name changes.
    /// @see changeTrackLabel()
    void nameChanged();

    // document modified (mute button)
    // Dead Code.
//    void modified();

    /// Emitted when a record button's state has changed.
    /// If we're setting to an audio
    /// track we need to tell the sequencer for live monitoring
    /// purposes.
    /// @see slotToggleRecordTrack()
    void recordButton(TrackId trackId, bool state);

    /// Emitted when a mute button's state has changed.
    /// @see slotToggleMutedTrack()
    void muteButton(TrackId trackId, bool state);

public slots:

    /// Toggles the record state for the track at the given position.
    void slotToggleRecordTrack(int position);
    /// Toggles the mute state for the track at the given position.
    void slotToggleMutedTrack(int position);

    /// Full sync of the track buttons with the composition.
    /// Adds or deletes track buttons as needed and updates the labels and
    /// LEDs on all tracks.
    /// @see slotSynchroniseWithComposition()
    /// @see populateButtons()
    void slotUpdateTracks();

    /// Connected to TrackLabel::renameTrack() to respond to the user changing
    /// the name of the track.
    /// @see changeTrackLabel()
    void slotRenameTrack(QString newName, TrackId trackId);
    /// Sets the level of the VU meter on a track.
    /// @see slotSetMetersByInstrument()
    /// @see RosegardenMainViewWidget::updateMeters()
    void slotSetTrackMeter(float value, int position);
    /// Sets the level of the VU meter on all tracks that use a specific
    /// instrument.
    /// @see slotSetTrackMeter()
    void slotSetMetersByInstrument(float value, InstrumentId id);

    /// Brings up the popup menu so that the user can select an instrument
    /// for the given track.
    void slotInstrumentSelection(int trackId);
    /// Sets the instrument for the track once the user has made a selection
    /// in the instrument popup.
    /// @see slotTrackInstrumentSelection()
    void slotInstrumentPopupActivated(int item);		// old kde3
    /// Delegates to the overloaded version that takes an int.
    void slotInstrumentPopupActivated(QAction*);		// old kde3

    /// Selects the instrument for a track given the track ID and the popup
    /// menu item for the instrument.
    void slotTrackInstrumentSelection(TrackId trackId, int item);
    
    /// Ensure track buttons match the Composition.
    /// This routine only makes sure the mute and record buttons are synced.
    /// slotUpdateTracks() does a more thorough sync.
    void slotSynchroniseWithComposition();

    /// Convert a positional selection into a track ID selection and emit
    /// trackSelected().
    void slotLabelSelected(int position);

protected:

    /// Updates the buttons from the composition.
    /// @see slotUpdateTracks()
    void populateButtons();

    /// Remove buttons for a position.
    void removeButtons(unsigned int position);

    /// Set record button - graphically only.
    /// @see slotSynchroniseWithComposition()
    /// @see setRecordTrack()
    void setRecordButton(int position, bool down);

    /// Creates the buttons for all the tracks, then calls populateButtons()
    /// to sync them up with the composition.
    void makeButtons();

    /// Creates all the widgets for a single track.
    QFrame* makeButton(Track *track);

    // Dead Code.
//    QString getPresentationName(Instrument *);

    /// Used to associate TrackLabel signals with their track ID.
    /// @see QSignalMapper::setMapping()
    void setButtonMapping(QObject* obj, TrackId trackId);

    /**
     * Return a suitable colour for a record LED for the supplied instrument,
     * based on its type.  If the instrument is invalid, it will return a
     * neutral color.
     *
     * This is a refactoring of several patches of duplicate code, and it adds
     * sanity checking in the form of returning a bad LED if the instrument is
     * invalid, or is of an invalid type, as a visual indication of an
     * underlying problem.  (This may actually prove useful beyond the scope of
     * the bug I'm tracking.  I think broken instruments may be rather common
     * when adding and deleting things with the device manager, and this may
     * help show that up.  Or not.)
     */
    QColor getRecordLedColour(Rosegarden::Instrument *ins);

    // CompositionObserver overrides
    virtual void tracksAdded(const Composition *, std::vector<TrackId> &trackIds);
//    virtual void trackChanged(const Composition *, Track*);
    virtual void tracksDeleted(const Composition *, std::vector<TrackId> &trackIds);

    int labelWidth();
    int trackHeight(TrackId trackId);


    //--------------- Data members ---------------------------------

    RosegardenDocument               *m_doc;

    /// Layout used to stack the trackHBoxes vertically
    QVBoxLayout                      *m_layout;

    // --- The widgets
    // These vectors are indexed by track position.
    std::vector<LedButton *>          m_muteLeds;
    std::vector<LedButton *>          m_recordLeds;
    std::vector<TrackLabel *>         m_trackLabels;
    /// The TrackVUMeter appears as the track number when there is no MIDI
    /// activity on a track.  It is to the left of the Mute LED.
    std::vector<TrackVUMeter *>       m_trackMeters;

    /// Each HBox contains the widgets (TrackVUMeter, muteLed, recordLed, and
    /// Label) for a track.
    std::vector<QFrame *>             m_trackHBoxes;

    QSignalMapper                    *m_recordSigMapper;
    QSignalMapper                    *m_muteSigMapper;
    QSignalMapper                    *m_clickedSigMapper;
    QSignalMapper                    *m_instListSigMapper;

    // Number of tracks on our view
    //
    unsigned int                      m_tracks;

    // The pixel offset from the top - just to overcome
    // the borders
    int                               m_offset;

    // The height of the cells
    //
    int                               m_cellSize;

    // gaps between elements vertically
    //
    int                               m_borderGap;

    int                               m_trackLabelWidth;
    int                               m_popupItem;

    // rename: m_labelDisplayMode
    TrackLabel::DisplayMode           m_trackInstrumentLabels;
    // Position of the last selected track.
    int m_lastSelected;

    // Constants
    static const int buttonGap;
    static const int vuWidth;
    static const int vuSpacing;

private:
    // Hide copy ctor and op=
    TrackButtons(const TrackButtons &);
    TrackButtons &operator=(const TrackButtons &);
};



}

#endif
