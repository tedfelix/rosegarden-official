
/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.

    This program is Copyright 2000-2006
        Guillaume Laurent   <glaurent@telegraph-road.org>,
        Chris Cannam        <cannam@all-day-breakfast.com>,
        Richard Bown        <richard.bown@ferventsoftware.com>

    The moral rights of Guillaume Laurent, Chris Cannam, and Richard
    Bown to claim authorship of this work have been asserted.

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

#include "base/MidiProgram.h"
#include "base/Track.h"
#include "gui/application/RosegardenGUIApp.h"
#include "TrackLabel.h"
#include <qframe.h>
#include <qstring.h>
#include <vector>


class QWidget;
class QVBoxLayout;
class QSignalMapper;
class QPopupMenu;
class QObject;


namespace Rosegarden
{

class TrackVUMeter;
class RosegardenGUIDoc;
class KLedButton;
class Instrument;


class TrackButtons : public QFrame
{
    Q_OBJECT
public:

    TrackButtons(RosegardenGUIDoc* doc,
                 unsigned int trackCellHeight,
                 unsigned int trackLabelWidth,
                 bool showTrackLabels,
                 int overallHeight,
                 QWidget* parent = 0,
                 const char* name = 0,
                 WFlags f=0);

    ~TrackButtons();

    /// Return a vector of muted tracks
    std::vector<int> mutedTracks();

    /// Return a vector of highlighted tracks
    std::vector<int> getHighlightedTracks();

    void changeTrackInstrumentLabels(TrackLabel::InstrumentTrackLabels label);

    /**
     * Change the instrument label to something else like
     * an actual program name rather than a meaningless
     * device number and midi channel
     */
    void changeInstrumentLabel(InstrumentId id, QString label);

    void changeTrackLabel(TrackId id, QString label);

    // Select a label from outside this class by position
    //
    void selectLabel(int trackId);

    /*
     * Set the mute button down or up
     */
    void setMuteButton(TrackId track, bool value);

    /*
     * Make this available so that others can set record buttons down
     */
    void setRecordTrack(int position, bool value);

    /**
     * Precalculate the Instrument popup so we don't have to every
     * time it appears
     * not protected because also used by the RosegardenGUIApp
     *
     * @see RosegardenGUIApp#slotPopulateTrackInstrumentPopup()
     */
    void populateInstrumentPopup(Instrument *thisTrackInstr, QPopupMenu* instrumentPopup);

signals:
    // to emit what Track has been selected
    //
    void trackSelected(int);
    void instrumentSelected(int);

    void widthChanged();

    // to tell the notation canvas &c when a name changes
    //
    void nameChanged();

    // document modified (mute button)
    //
    void modified();

    // A record button has been pressed - if we're setting to an audio
    // track we need to tell the sequencer for live monitoring
    // purposes.
    //
    void recordButton(TrackId track, bool state);

    // A mute button has been pressed
    //
    void muteButton(TrackId track, bool state);

public slots:

    void slotToggleRecordTrack(int position);
    void slotToggleMutedTrack(int mutedTrack);
    void slotUpdateTracks();
    void slotRenameTrack(QString newName, TrackId trackId);
    void slotSetTrackMeter(float value, int position);
    void slotSetMetersByInstrument(float value, InstrumentId id);

    void slotInstrumentSelection(int);
    void slotInstrumentPopupActivated(int);
    void slotTrackInstrumentSelection(TrackId, int);
    
    // ensure track buttons match the Composition
    //
    void slotSynchroniseWithComposition();

    // Convert a positional selection into a track selection and re-emit
    //
    void slotLabelSelected(int position);

protected:

    /**
     * Populate the track buttons themselves with Instrument information
     */
    void populateButtons();

    /**
     * Remove buttons and clear iterators for a position
     */
    void removeButtons(unsigned int position);

    /**
     * Set record button - graphically only
     */
    void setRecordButton(int position, bool down);

    /**
     *  buttons, starting at the specified index
     */
    void makeButtons();

    QFrame* makeButton(TrackId trackId);
    QString getPresentationName(Instrument *);

    void setButtonMapping(QObject*, TrackId);

    //--------------- Data members ---------------------------------

    RosegardenGUIDoc                 *m_doc;

    QVBoxLayout                      *m_layout;

    std::vector<KLedButton *>         m_muteLeds;
    std::vector<KLedButton *>         m_recordLeds;
    std::vector<TrackLabel *>         m_trackLabels;
    std::vector<TrackVUMeter *>       m_trackMeters;
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

    // gaps between elements
    //
    int                               m_borderGap;

    int                               m_trackLabelWidth;
    int                               m_popupItem;

    TrackLabel::InstrumentTrackLabels             m_trackInstrumentLabels;
    int m_lastSelected;
};



}

#endif
