/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2018 the Rosegarden development team.

    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#ifndef RG_ROSEGARDENGUIVIEW_H
#define RG_ROSEGARDENGUIVIEW_H

#include "base/Event.h"
#include "base/MidiProgram.h"
#include "base/Selection.h"
#include "base/Track.h"
#include "sound/AudioFile.h"
#include "gui/editors/segment/TrackEditor.h"
#include <QString>
#include <QWidget>
#include <QVBoxLayout>


class QWidget;
class QObject;
class LevelInfo;
class Command;


namespace Rosegarden
{

class TrackParameterBox;
class TrackEditor;
class SimpleRulerScale;
class SegmentParameterBox;
class Segment;
class RosegardenDocument;
class RosegardenParameterArea;
class RealTime;
class NotationView;
class PitchTrackerView;
class MatrixView;
class MappedEvent;
class InstrumentParameterBox;
class EventView;
class Composition;
class LevelInfo;

/// Parent for the TrackEditor
/**
 * The RosegardenMainViewWidget class provides the view widget for the
 * RosegardenMainWindow instance.  The View instance inherits QWidget as a
 * base class and represents the view object of a QMainWindow. As
 * RosegardenMainViewWidget is part of the document-view model, it needs a
 * reference to the document object connected with it by the
 * RosegardenMainWindow class to manipulate and display the document
 * structure provided by the RosegardenDocument class.
 *
 * An instance of this class is owned by RosegardenMainWindow and can be
 * accessed via RosegardenMainWindow::getView().  This class is
 * primarily responsible for containing the TrackEditor instance which
 * can be accessed via getTrackEditor().
 *
 * @author Guillaume Laurent with KDevelop 0.4
 */
class RosegardenMainViewWidget : public QWidget
{
    Q_OBJECT
public:

    /**
     * Constructor for the main view
     */
    RosegardenMainViewWidget(bool showTrackLabels,
                             SegmentParameterBox*,
                             InstrumentParameterBox*,
                             TrackParameterBox*,
                             RosegardenParameterArea* parameterArea,
                             QWidget *parent = nullptr);

    /**
     * Destructor for the main view
     */
    ~RosegardenMainViewWidget() override;

    /**
     * returns a pointer to the document connected to the view
     * instance. Mind that this method requires a RosegardenMainWindow
     * instance as a parent widget to get to the window document
     * pointer by calling the RosegardenMainWindow::getDocument() method.
     *
     * @see RosegardenMainWindow#getDocument
     */
    RosegardenDocument* getDocument() const;

    TrackEditor* getTrackEditor() { return m_trackEditor; }

    // the following aren't slots because they're called from
    // RosegardenMainWindow

    /**
     * Select a tool at the CompositionView
     */
    void selectTool(QString toolName);

    void updateMeters();
    void updateMonitorMeters();

    /**
     * Change zoom size -- set the RulerScale's units-per-pixel to size
     */
    void setZoomSize(double size);

    void initChordNameRuler();
    
    bool haveSelection();
    SegmentSelection getSelection();
    void updateSelectedSegments();

    TrackParameterBox *getTrackParameterBox()
            { return m_trackParameterBox; }

    enum ExternalControllerWindow { Main, AudioMixer, MidiMixer };

public slots:
    void slotEditSegment(Segment*);
    void slotEditSegmentNotation(Segment*);
    void slotEditSegmentsNotation(std::vector<Segment*>);
    void slotEditSegmentMatrix(Segment*);
    void slotEditSegmentsMatrix(std::vector<Segment*>);
    void slotEditSegmentPercussionMatrix(Segment*);
    void slotEditSegmentsPercussionMatrix(std::vector<Segment*>);
    void slotEditSegmentEventList(Segment*);
    void slotEditSegmentsEventList(std::vector<Segment*>);
    void slotEditSegmentPitchTracker(Segment*);
    void slotEditSegmentsPitchTracker(std::vector<Segment*>);
    void slotEditTriggerSegment(int);
    void slotEditSegmentAudio(Segment*);
    void slotSegmentAutoSplit(Segment*);
    void slotEditRepeat(Segment*, timeT);

    /**
     * Highlight all the Segments on a Track because the Track has
     * been selected * We have to ensure we create a Selector object
     * before we can highlight * these tracks.
     *
     * Called by signal from Track selection routine to highlight
     * all available Segments on a Track
     */
    void slotSelectTrackSegments(int);

    void slotSelectAllSegments();

    /*
     * This is called from the canvas (actually the selector tool) moving out
     */
    void slotSelectedSegments(const SegmentSelection &segments);

    /*
     * And this one from the user interface going down
     */
    void slotPropagateSegmentSelection(const SegmentSelection &segments);

    void slotShowRulers(bool);

    void slotShowTempoRuler(bool);

    void slotShowChordNameRuler(bool);

    void slotShowPreviews(bool);

    void slotShowSegmentLabels(bool);

    void slotAddTracks(unsigned int count, InstrumentId instrument, int position);

    void slotDeleteTracks(std::vector<TrackId> tracks);

    void slotAddAudioSegmentCurrentPosition(AudioFileId,
                                            const RealTime &startTime,
                                            const RealTime &endTime);

    void slotAddAudioSegmentDefaultPosition(AudioFileId,
                                            const RealTime &startTime,
                                            const RealTime &endTime);

    void slotAddAudioSegment(AudioFileId audioId,
                             TrackId trackId,
                             timeT position,
                             const RealTime &startTime,
                             const RealTime &endTime);

    void slotDroppedAudio(QString audioDesc);
    void slotDroppedNewAudio(QString audioDesc);

    /**
     * Commands
     *
     */
    void slotAddCommandToHistory(Command *command);

    /**
     * Change the Track Label
     */
//    void slotChangeTrackLabel(TrackId id, QString label);

    /// Set the record state for an instrument.
    void slotSetRecord(InstrumentId, bool);

    /// Set the solo state for an instrument.
    void slotSetSolo(InstrumentId, bool);

    /**
     * To indicate that we should track the recording segment (despite
     * no commands being issued on it)
     */
    void slotUpdateRecordingSegment(Segment *segment,
                                    timeT updatedFrom);

    /**
     * A manual fudgy way of creating a view update for certain
     * semi-static data (devices/instrument labels mainly)
     */
    void slotSynchroniseWithComposition();

    /**
     * To indicate that an edit view, mixer, etc (something that might
     * want to receive MIDI input) has become active.  We only send
     * inputs such as MIDI to a single one of these, in most cases,
     * and it's whichever was most recently made active.  (It doesn't
     * have to still _be_ active -- we want to allow moving focus to
     * another application entirely but still receiving MIDI etc in
     * Rosegarden.)
     */
    void slotActiveMainWindowChanged(ExternalControllerWindow window);

    /// Handle events from the external controller port.
    /**
     * This routine handles remote control events received from a
     * device connected to the "external controller" port.
     *
     * This routine handles controller 81 which opens or
     * brings to the top various windows based on the value.
     */
    void slotExternalControllerMain(MappedEvent *);

    /// Handle events from the external controller port.
    /**
     * This routine handles remote control events received from a
     * device connected to the "external controller" port.
     *
     * This routine handles various controllers and Program Changes.
     * Controller 82 selects the current track.  Other controllers,
     * like volume and pan, cause changes to the corresponding controller
     * setting for the current track.  Program Changes change the program
     * for the current track.  For audio tracks, MIDI volume and pan
     * controllers control the track's volume and pan.
     *
     * @see MidiMixerWindow::slotExternalController()
     * @see AudioMixerWindow2::slotExternalController()
     */
    void slotExternalController(
            MappedEvent *, ExternalControllerWindow window);

signals:
    void activateTool(QString toolName);

    void stateChange(QString, bool);

    /**
     * Inform that we've got a SegmentSelection
     */
    void segmentsSelected(const SegmentSelection&);

    void toggleSolo(bool);

    /**
     * Current used to dispatch things like track select changes, solo, etc...
     * to edit views
     */
    void compositionStateUpdate();
    

    /**
     * This signal is used to dispatch a notification for a request to
     * set the step-by-step-editing target window to all candidate targets,
     * so that they can either know that their request has been granted
     * (if they match the QObject passed) or else deactivate any step-by-
     * step editing currently active in their own window (otherwise).
     */
    void stepByStepTargetRequested(QObject *);

    /*
     * Add an audio file at the sequencer - when we drop a new file
     * on the segment canvas.
     */
    void addAudioFile(AudioFileId);

    void instrumentLevelsChanged(InstrumentId,
                                 const LevelInfo &);

    void externalController(
            MappedEvent *event,
            ExternalControllerWindow window);

private:

    NotationView *createNotationView(std::vector<Segment *>);
    MatrixView *createMatrixView(std::vector<Segment *>, bool drumMode);
    EventView *createEventView(std::vector<Segment *>);
    PitchTrackerView *createPitchTrackerView(std::vector<Segment *>);

    static bool hasNonAudioSegment(const SegmentSelection &segments);

    //--------------- Data members ---------------------------------

    SimpleRulerScale  *m_rulerScale;
    TrackEditor                   *m_trackEditor;

    SegmentParameterBox           *m_segmentParameterBox;
    InstrumentParameterBox        *m_instrumentParameterBox;
    TrackParameterBox             *m_trackParameterBox;

    static ExternalControllerWindow m_lastActiveMainWindow;
};


}

#endif
