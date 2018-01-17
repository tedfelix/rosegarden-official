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

#ifndef RG_TRACKBUTTONS_H
#define RG_TRACKBUTTONS_H

#include "base/Composition.h"
#include "base/MidiProgram.h"
#include "base/Track.h"
#include "gui/application/RosegardenMainWindow.h"
#include "TrackLabel.h"
#include <QFrame>
#include <QString>
#include <QSharedPointer>
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
class InstrumentStaticSignals;

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
 * TrackEditor owns the single instance of this class.  See
 * TrackEditor::m_trackButtons.
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

    /// Ctor.
    /**
     * @param[in] trackCellHeight Height of each track including the gap
     *                            between tracks.  See m_cellSize.
     * @param[in] trackLabelWidth Width of the TrackLabel portion.  See
     *                            m_trackLabelWidth.
     * @param[in] showTrackLabels true => track names are displayed.
     *     false => instrument names are displayed.  See
     *     m_labelDisplayMode and changeLabelDisplayMode().
     * @param[in] overallHeight   Height of the entire TrackButtons frame.
     */
    TrackButtons(RosegardenDocument* doc,
                 int trackCellHeight,
                 int trackLabelWidth,
                 bool showTrackLabels,
                 int overallHeight,
                 QWidget* parent = 0);

    ~TrackButtons();

    /// Return a vector of highlighted track positions
    /// @see selectTrack()
    // unused.
//    std::vector<int> getHighlightedTracks();

    /// Change the track labels between track or instrument.
    /**
     * Menu: View > Show Track Labels (show_tracklabels action)
     */
    void changeLabelDisplayMode(TrackLabel::DisplayMode mode);

    /// Fill the instrument popup menu with the available instruments.
    /**
     * @see RosegardenMainWindow::slotPopulateTrackInstrumentPopup()
     */
    void populateInstrumentPopup(Instrument *thisTrackInstr, QMenu* instrumentPopup);

    /// Toggle solo on the selected track.
    /**
     * Used by the solo buttons/menu items on the various editors.
     */
    void toggleSolo();

    void selectInstrument(Track *track, Instrument *instrument);

signals:
    /// Emitted when a track button has been clicked.
    /**
     * Emitted by slotTrackSelected().
     * Handled by RosegardenMainViewWidget::slotSelectTrackSegments().
     *
     * @see slotLabelSelected()
     */
    void trackSelected(int trackId);

    /// Emitted when a track's name changes.
    /**
     * Appears unneeded as the width of the track button never changes.
     *
     * @see changeTrackName()
     * @see TrackEditor::slotTrackButtonsWidthChanged()
     */
    //void widthChanged();

public slots:

    /// Toggles the record state for the track at the given position.
    void slotToggleRecord(int position);

    /// Full sync of the track buttons with the composition.
    /**
     * Adds or deletes track buttons as needed and updates the labels and
     * LEDs on all tracks.
     *
     * @see slotSynchroniseWithComposition()
     * @see populateButtons()
     */
    void slotUpdateTracks();

    /// Rename the Track in the Composition.
    /**
     * Connected to TrackLabel::renameTrack() to respond to the user changing
     * the name of the track (by double-clicking on the label).
     *
     * @see changeTrackName()
     */
    void slotRenameTrack(QString longLabel, QString shortLabel, TrackId trackId);

    /// Sets the level of the VU meter on a track.
    /**
     * Suggestion: Reverse the argument order.  Also, this is never used as
     * a slot.  Move it to public.
     *
     * @see slotSetMetersByInstrument()
     * @see RosegardenMainViewWidget::updateMeters()
     */
    void slotSetTrackMeter(float value, int position);
    /// Sets the VU meter level on all tracks that use a specific instrument.
    /**
     * Suggestion: Reverse the argument order.
     *
     * @see slotSetTrackMeter()
     */
    void slotSetMetersByInstrument(float value, InstrumentId id);

    /// Brings up the instrument selection popup menu.
    /**
     * Called in response to a right-click on a track label.  Brings up
     * the popup menu so that the user can select an instrument for the
     * given track.
     *
     * Warning: This uses an "int" rather than TrackId because TrackId is
     * unsigned, and this will be connected to a signal that passes an int.
     * trackId must be an "int".
     *
     * @see slotInstrumentSelected()
     */
    void slotInstrumentMenu(int trackId);

    /// Does the actual work for the other slotInstrumentSelected().
    void slotInstrumentSelected(int instrumentIndex);		// old kde3

    /// Handles the user clicking on a selection in the instrument menu.
    /**
     * Delegates to the overloaded version that takes an int.
     *
     * @see slotInstrumentMenu()
     */
    void slotInstrumentSelected(QAction*);		// old kde3

    /// Handles instrument changes from the TrackParameterBox.
    /**
     * Connected to TrackParameterBox::instrumentSelected().  This is called
     * when the user changes the instrument via the Track Parameters box.
     */
    void slotTPBInstrumentSelected(TrackId trackId, int item);
    
    /// Ensure track buttons on the UI match the Composition.
    /**
     * @see updateUI()
     */
    void slotSynchroniseWithComposition();

    /// Convert a positional selection into a track ID selection and emit
    /// trackSelected().
    // unused
//    void slotLabelSelected(int position);

protected slots:
    /// Toggles the mute state for the track at the given position.
    /**
     * Called when the user clicks on a mute button.
     * @see m_muteSigMapper
     */
    void slotToggleMute(int position);

    /// Toggles the solo state for the track at the given position.
    /**
     * Called when the user clicks on a solo button.
     * @see m_soloSigMapper
     */
    void slotToggleSolo(int position);

protected:

    /// Initializes the instrument names.
    /**
     * @see TrackLabel
     */
    void initInstrumentNames(Instrument *ins, TrackLabel *label);

    /// Updates a track button from its associated Track.
    void updateUI(Track *track);

    /// Updates the buttons from the composition.
    /**
     * @see slotUpdateTracks()
     */
    void populateButtons();

    /// Remove buttons for a position.
    void removeButtons(int position);

    /// Set record button - UI only.
    /**
     * @see slotSynchroniseWithComposition()
     */
    void setRecordButton(int position, bool record);

    /// Creates and syncs the buttons for all the tracks.
    void makeButtons();

    /// Creates all the widgets for a single track.
    QFrame* makeButton(Track *track);

    // Dead Code.
//    QString getPresentationName(Instrument *);

    /// Used to associate TrackLabel signals with their track ID.
    /**
     * @see QSignalMapper::setMapping()
     */
    void setButtonMapping(TrackLabel* obj, TrackId trackId);

    /// Gets the proper color for an instrument based on its type.
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
    virtual void trackChanged(const Composition *, Track*);
    virtual void tracksDeleted(const Composition *, std::vector<TrackId> &trackIds);
    virtual void trackSelectionChanged(const Composition *, TrackId trackId);
    virtual void segmentRemoved(const Composition *, Segment *);

    int labelWidth();
    int trackHeight(TrackId trackId);


    //--------------- Data members ---------------------------------

    RosegardenDocument               *m_doc;

    /// Layout used to stack the trackHBoxes vertically
    QVBoxLayout                      *m_layout;

    // --- The widgets
    // These vectors are indexed by track position.
    /**
     * The TrackVUMeter appears as the track number when there is no MIDI
     * activity on a track.  It is to the left of the Mute LED.
     */
    std::vector<TrackVUMeter *>       m_trackMeters;
    std::vector<LedButton *>          m_muteLeds;
    std::vector<LedButton *>          m_recordLeds;
    std::vector<LedButton *>          m_soloLeds;
    std::vector<TrackLabel *>         m_trackLabels;

    /**
     * Each HBox contains the widgets (TrackVUMeter, muteLed, recordLed, and
     * Label) for a track.  m_trackHBoxes is indexed by track position.
     */
    std::vector<QFrame *>             m_trackHBoxes;

    QSignalMapper                    *m_recordSigMapper;
    QSignalMapper                    *m_muteSigMapper;
    QSignalMapper                    *m_soloSigMapper;
    QSignalMapper                    *m_clickedSigMapper;
    QSignalMapper                    *m_instListSigMapper;

    /// The number of tracks on our view.
    int                               m_tracks;

    // The pixel offset from the top - just to overcome
    // the borders
    // unused
//    int                               m_offset;

    /// The height of the cells
    int                               m_cellSize;

    int                               m_trackLabelWidth;

    /// Position of the track that is showing the instrument popup menu.
    int                               m_popupTrackPos;

    TrackLabel::DisplayMode           m_labelDisplayMode;

    /// Position of the last selected track.
    int m_lastSelected;

    // Constants
    /// gaps between elements vertically
    static const int m_borderGap;
    static const int m_buttonGap;
    static const int m_vuWidth;
    static const int m_vuSpacing;

private slots:
    /// Handles clicks from m_clickedSigMapper.
    void slotTrackSelected(int trackId);

    /// Handles a change to the Program in the Instrument Parameters box.
    void slotInstrumentChanged(Instrument *instrument);

private:
    // Hide copy ctor and op=
    TrackButtons(const TrackButtons &);
    TrackButtons &operator=(const TrackButtons &);

    /// Select the given track.  This displays it with a highlight.
    void selectTrack(int position);
};



}

#endif
