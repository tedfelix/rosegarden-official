/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2015 the Rosegarden development team.
 
    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.
 
    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/



#define RG_MODULE_STRING "[TrackEditor]"

#include "TrackEditor.h"
#include "TrackButtons.h"

#include "misc/Strings.h"
#include "misc/Debug.h"
#include "misc/ConfigGroups.h"
#include "gui/seqmanager/SequenceManager.h"
#include "gui/rulers/StandardRuler.h"
#include "base/Composition.h"
#include "base/MidiProgram.h"
#include "base/RealTime.h"
#include "base/RulerScale.h"
#include "base/Segment.h"
#include "base/Selection.h"
#include "commands/segment/AddTracksCommand.h"
#include "commands/segment/DeleteTracksCommand.h"
#include "commands/segment/SegmentEraseCommand.h"
#include "commands/segment/SegmentInsertCommand.h"
#include "commands/segment/SegmentRepeatToCopyCommand.h"
#include "commands/segment/SegmentLinkToCopyCommand.h"
#include "compositionview/CompositionModelImpl.h"
#include "compositionview/CompositionView.h"
#include "document/CommandHistory.h"
#include "document/RosegardenDocument.h"
#include "gui/application/RosegardenMainWindow.h"
#include "gui/application/RosegardenMainViewWidget.h"
#include "gui/rulers/ChordNameRuler.h"
#include "gui/rulers/TempoRuler.h"
#include "gui/rulers/LoopRuler.h"
#include "gui/widgets/ProgressDialog.h"
#include "gui/widgets/DeferScrollArea.h"
#include "sound/AudioFile.h"
#include "document/Command.h"

#include <QSettings>
#include <QLayout>
#include <QApplication>
#include <QMessageBox>
#include <QApplication>
#include <QCursor>
#include <QFont>
#include <QPixmap>
#include <QPoint>
#include <QScrollBar>
#include <QString>
#include <QStringList>
#include <QStringList>
#include <QWidget>
#include <QValidator>
#include <QTextStream>
#include <QEvent>
#include <QDragEnterEvent>
#include <QDragLeaveEvent>
#include <QDragMoveEvent>
#include <QDropEvent>
#include <QMimeData>

#include <vector>
#include <algorithm>
#include <math.h>

namespace Rosegarden
{


TrackEditor::TrackEditor(RosegardenDocument *doc,
                         RosegardenMainViewWidget *mainViewWidget,
                         SimpleRulerScale *rulerScale,
                         bool showTrackLabels) :
    QWidget(mainViewWidget),
    m_doc(doc),
    m_compositionRefreshStatusId(doc->getComposition().getNewRefreshStatusId()),
    m_compositionView(0),
    m_compositionModel(0),
    m_playTracking(true),
    m_trackCellHeight(0),
    m_trackButtons(0),
    m_trackButtonScroll(0),
    m_showTrackLabels(showTrackLabels),
    m_rulerScale(rulerScale),
    m_tempoRuler(0),
    m_chordNameRuler(0),
    m_topStandardRuler(0),
    m_bottomStandardRuler(0)
    //m_canvasWidth(0)
{
    // Accept objects dragged and dropped onto this widget.
    setAcceptDrops(true);

    // ??? Only called here.  Inline it here.
    init(mainViewWidget);

    updateCanvasSize();
}

void
TrackEditor::init(RosegardenMainViewWidget *mainViewWidget)
{
    QFontMetrics fontMetrics(QApplication::font(this));
    m_trackCellHeight = std::min(fontMetrics.height() + 9, 24);

    QGridLayout *grid = new QGridLayout(this);
    grid->setMargin(0);
    grid->setSpacing(0);

    // Height for top and bottom standard rulers.
    // rename: standardRulerHeight
    const int barButtonsHeight = 25;

    // Top Rulers
    
    m_chordNameRuler = new ChordNameRuler(m_rulerScale,
                                          m_doc,
                                          0.0,
                                          20,
                                          this);
    grid->addWidget(m_chordNameRuler, 0, 1);

    m_tempoRuler = new TempoRuler(m_rulerScale,
                                  m_doc,
                                  RosegardenMainWindow::self(),
                                  0.0,
                                  24,
                                  true,
                                  this);
    m_tempoRuler->connectSignals();
    grid->addWidget(m_tempoRuler, 1, 1);

    m_topStandardRuler = new StandardRuler(m_doc,
                                     m_rulerScale,
                                     0,
                                     barButtonsHeight,
                                     false,
                                     true,
                                     this);
    m_topStandardRuler->connectRulerToDocPointer(m_doc);
    grid->addWidget(m_topStandardRuler, 2, 1);

    // Segment Canvas (CompositionView)

    m_compositionModel =
            new CompositionModelImpl(this,
                                     m_doc->getComposition(),
                                     m_doc->getStudio(),
                                     m_rulerScale, m_trackCellHeight);

    m_compositionView = new CompositionView(m_doc, m_compositionModel, this);
    m_compositionView->verticalScrollBar()->setSingleStep(m_trackCellHeight);

    // Bottom Ruler

    m_bottomStandardRuler = new StandardRuler(m_doc,
                                        m_rulerScale,
                                        0,
                                        barButtonsHeight,
                                        true,
                                        true,
                                        m_compositionView);
    m_bottomStandardRuler->connectRulerToDocPointer(m_doc);

    m_compositionView->setBottomRuler(m_bottomStandardRuler);

    // Span 2 rows (3 and 4) so that there is an extra cell at the bottom of
    // column 0 to take up space below the TrackButtons.
    grid->addWidget(m_compositionView, 3, 1, 2, 1);

    // Reserve space at the bottom below the TrackButtons to make sure they
    // don't extend down too far.
    // ??? This looks great when there is no horizontal scrollbar (zoom all
    //     the way out).  It extends too far down when there *is* a
    //     scrollbar.  Need to figure out a way to update this when the
    //     scrollbar shows/hides.  Use m_compositionView->viewport()
    //     to do the computations since it takes the scrollbar and ruler
    //     into account.
    grid->setRowMinimumHeight(4, m_bottomStandardRuler->sizeHint().height());

    // Make sure the segment canvas doesn't leave a "blank" grey space when
    // loading a file which has a low zoom factor.
    grid->setColumnStretch(1, 10);

    // Track Buttons

    m_trackButtonScroll = new DeferScrollArea(this);
    // Scroll bars always off
    m_trackButtonScroll->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_trackButtonScroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    grid->addWidget(m_trackButtonScroll, 3, 0);

    const int trackLabelWidth = 200;
    const int canvasHeight = m_trackCellHeight *
                       std::max(40u, m_doc->getComposition().getNbTracks());

    m_trackButtons = new TrackButtons(m_doc,
                                      m_trackCellHeight,
                                      trackLabelWidth,
                                      m_showTrackLabels,
                                      canvasHeight,
                                      m_trackButtonScroll);

    m_trackButtons->setObjectName("TRACK_BUTTONS"); // permit styling; internal string; no tr()

    m_trackButtonScroll->setWidget(m_trackButtons);

    // Connections

    //connect(m_trackButtons, SIGNAL(widthChanged()),
    //        this, SLOT(slotTrackButtonsWidthChanged()));

    connect(m_trackButtons, SIGNAL(trackSelected(int)),
            mainViewWidget, SLOT(slotSelectTrackSegments(int)));

    connect(m_trackButtons, SIGNAL(instrumentSelected(int)),
            mainViewWidget, SLOT(slotUpdateInstrumentParameterBox(int)));

    connect(this, SIGNAL(stateChange(QString, bool)),
            mainViewWidget, SIGNAL(stateChange(QString, bool)));

    // No such signal.  Was there ever?
//    connect(m_trackButtons, SIGNAL(modified()),
//            m_doc, SLOT(slotDocumentModified()));

    // connect loop rulers' follow-scroll signals
    connect(m_topStandardRuler->getLoopRuler(), SIGNAL(startMouseMove(int)),
            m_compositionView, SLOT(slotStartAutoScroll(int)));
    connect(m_topStandardRuler->getLoopRuler(), SIGNAL(stopMouseMove()),
            m_compositionView, SLOT(slotStopAutoScroll()));
    connect(m_bottomStandardRuler->getLoopRuler(), SIGNAL(startMouseMove(int)),
            m_compositionView, SLOT(slotStartAutoScroll(int)));
    connect(m_bottomStandardRuler->getLoopRuler(), SIGNAL(stopMouseMove()),
            m_compositionView, SLOT(slotStopAutoScroll()));

    //&&&  Interesting one here.  Q(3)ScrollArea had a contentsMoving signal we
    // used to grab for some purpose.  Q(Abstract)ScrollArea has no usable
    // signals whatsoever.  I think this is why autoscrolling is still slightly
    // wonky in Thorn, but I don't reckon there's much to do about this one
    // unless we write a custom widget or something.  
    //
    //connect(m_compositionView, SIGNAL(contentsMoving(int, int)),
    //        this, SLOT(slotCanvasScrolled(int, int)));

    // Synchronize TrackButtons scroll area (m_trackButtonScroll) with
    // segment canvas's vertical scrollbar.
    connect(m_compositionView->verticalScrollBar(), SIGNAL(valueChanged(int)),
            this, SLOT(slotVerticalScrollTrackButtons(int)));
    connect(m_compositionView->verticalScrollBar(), SIGNAL(sliderMoved(int)),
            this, SLOT(slotVerticalScrollTrackButtons(int)));

    // Connect scrolling with the mouse wheel in the TrackButtons to
    // scrolling the CompositionView.
    connect(m_trackButtonScroll, SIGNAL(gotWheelEvent(QWheelEvent*)),
            m_compositionView, SLOT(slotExternalWheelEvent(QWheelEvent*)));

    // Synchronize the rulers with the horizontal scrollbar.

    connect(m_compositionView->horizontalScrollBar(), SIGNAL(valueChanged(int)),
            m_topStandardRuler, SLOT(slotScrollHoriz(int)));
    connect(m_compositionView->horizontalScrollBar(), SIGNAL(sliderMoved(int)),
            m_topStandardRuler, SLOT(slotScrollHoriz(int)));

    connect(m_compositionView->horizontalScrollBar(), SIGNAL(valueChanged(int)),
            m_bottomStandardRuler, SLOT(slotScrollHoriz(int)));
    connect(m_compositionView->horizontalScrollBar(), SIGNAL(sliderMoved(int)),
            m_bottomStandardRuler, SLOT(slotScrollHoriz(int)));

    connect(m_compositionView->horizontalScrollBar(), SIGNAL(valueChanged(int)),
            m_tempoRuler, SLOT(slotScrollHoriz(int)));
    connect(m_compositionView->horizontalScrollBar(), SIGNAL(sliderMoved(int)),
            m_tempoRuler, SLOT(slotScrollHoriz(int)));

    connect(m_compositionView->horizontalScrollBar(), SIGNAL(valueChanged(int)),
            m_chordNameRuler, SLOT(slotScrollHoriz(int)));
    connect(m_compositionView->horizontalScrollBar(), SIGNAL(sliderMoved(int)),
            m_chordNameRuler, SLOT(slotScrollHoriz(int)));

    // Was only emitted from dead code.
    //connect(this, SIGNAL(needUpdate()),
    //        m_compositionView, SLOT(slotUpdateAll()));

    connect(m_compositionView->getModel(),
            SIGNAL(selectedSegments(const SegmentSelection &)),
            mainViewWidget,
            SLOT(slotSelectedSegments(const SegmentSelection &)));

    connect(m_compositionView, SIGNAL(viewportResize()),
            this, SLOT(slotViewportResize()));
    connect(m_compositionView, SIGNAL(zoomIn()),
            RosegardenMainWindow::self(), SLOT(slotZoomIn()));
    connect(m_compositionView, SIGNAL(zoomOut()),
            RosegardenMainWindow::self(), SLOT(slotZoomOut()));

    connect(CommandHistory::getInstance(), SIGNAL(commandExecuted()),
            this, SLOT(slotCommandExecuted()));

    connect(m_doc, SIGNAL(pointerPositionChanged(timeT)),
            this, SLOT(slotSetPointerPosition(timeT)));

    // Top/Bottom ruler pointer drag.
    connect(m_topStandardRuler, SIGNAL(dragPointerToPosition(timeT)),
            this, SLOT(slotPointerDraggedToPosition(timeT)));
    connect(m_bottomStandardRuler, SIGNAL(dragPointerToPosition(timeT)),
            this, SLOT(slotPointerDraggedToPosition(timeT)));

    // Top/Bottom ruler loop drag.
    connect(m_topStandardRuler, SIGNAL(dragLoopToPosition(timeT)),
            this, SLOT(slotLoopDraggedToPosition(timeT)));
    connect(m_bottomStandardRuler, SIGNAL(dragLoopToPosition(timeT)),
            this, SLOT(slotLoopDraggedToPosition(timeT)));

    connect(m_doc, SIGNAL(loopChanged(timeT, timeT)),
            this, SLOT(slotSetLoop(timeT, timeT)));
}

void TrackEditor::updateCanvasSize()
{
    m_compositionView->slotUpdateSize();
}

#if 0
void TrackEditor::slotTrackButtonsWidthChanged()
{
    // We need to make sure the trackButtons geometry is fully updated
    //
//    ProgressDialog::processEvents();

    m_trackButtonScroll->setMinimumWidth(m_trackButtons->width());
    m_doc->slotDocumentModified();
}
#endif

void TrackEditor::updateRulers()
{
    if (m_tempoRuler)
        m_tempoRuler->update();

    if (m_chordNameRuler)
        m_chordNameRuler->update();

    if (m_topStandardRuler)
        m_topStandardRuler->update();

    if (m_bottomStandardRuler)
        m_bottomStandardRuler->update();
}

void TrackEditor::addTracks(unsigned int nbNewTracks,
                            InstrumentId id,
                            int position)
{
    Composition &comp = m_doc->getComposition();

    addCommandToHistory(new AddTracksCommand(&comp, nbNewTracks, id, position));

    updateCanvasSize();
}

void TrackEditor::deleteTracks(std::vector<TrackId> tracks)
{
    MacroCommand *macro = new MacroCommand(tr("Delete Tracks"));

    Composition &comp = m_doc->getComposition();
    const segmentcontainer &segments = comp.getSegments();

    // Delete the segments.

    // for each track we are deleting
    for (size_t i = 0; i < tracks.size(); ++i) {
        const TrackId trackId = tracks[i];

        // for each segment in the composition
        for (segmentcontainer::const_iterator segmentIter = segments.begin();
             segmentIter != segments.end();
             ++segmentIter) {
            Segment *segment = *segmentIter;

            // if this segment is in the track
            if (segment->getTrack() == trackId) {
                macro->addCommand(new SegmentEraseCommand(
                        segment, &m_doc->getAudioFileManager()));
            }
        }
    }

    // Delete the tracks.
    macro->addCommand(new DeleteTracksCommand(&comp, tracks));

    addCommandToHistory(macro);
}

#if 0
void TrackEditor::addSegment(int track, int time, unsigned int duration)
{
    if (!m_doc)
        return ; // sanity check

    SegmentInsertCommand *command =
        new SegmentInsertCommand(m_doc, track, time, duration);

    addCommandToHistory(command);
}
#endif

#if 0
// Dead Code.
void TrackEditor::slotSegmentOrderChanged(int section, int fromIdx, int toIdx)
{
    RG_DEBUG << QString("TrackEditor::segmentOrderChanged(section : %1, from %2, to %3)")
    .arg(section).arg(fromIdx).arg(toIdx) << endl;

    //!!! how do we get here? need to involve a command
    emit needUpdate();
}
#endif

#if 0
void
TrackEditor::slotCanvasScrolled(int x, int /*y*/)
{
    // update the pointer position if the user is dragging it from the loop ruler
    if ((m_topStandardRuler && m_topStandardRuler->getLoopRuler() &&
         m_topStandardRuler->getLoopRuler()->hasActiveMousePress() &&
         !m_topStandardRuler->getLoopRuler()->getLoopingMode()) ||
        (m_bottomStandardRuler && m_bottomStandardRuler->getLoopRuler() &&
         m_bottomStandardRuler->getLoopRuler()->hasActiveMousePress() &&
         !m_bottomStandardRuler->getLoopRuler()->getLoopingMode())) {

        int mx = m_compositionView->viewport()->mapFromGlobal(QCursor::pos()).x();
        m_compositionView->setPointerPos(x + mx);

        // bad idea, creates a feedback loop
        //     timeT t = m_compositionView->grid().getRulerScale()->getTimeForX(x + mx);
        //     slotSetPointerPosition(t);
    }
}
#endif

void
TrackEditor::slotSetPointerPosition(timeT pointerTime)
{
    if (!m_rulerScale)
        return;

    const double newPosition = m_rulerScale->getXForTime(pointerTime);
    const int currentPosition = m_compositionView->getPointerPos();
    const double distance = fabs(newPosition - currentPosition);

    // If we're moving at least one pixel
    if (distance >= 1.0) {

        if (m_doc  &&  m_doc->getSequenceManager()  &&
            m_doc->getSequenceManager()->getTransportStatus() != STOPPED) {
            
            if (m_playTracking) {
                m_compositionView->scrollHoriz(newPosition);
            }
        } else if (!m_compositionView->isAutoScrolling()) {
            m_compositionView->scrollHoriz(newPosition);
        }

        m_compositionView->setPointerPos(newPosition);

    }
}

void
TrackEditor::slotPointerDraggedToPosition(timeT position)
{
    int currentPointerPos = m_compositionView->getPointerPos();

    double newPosition;

    if (handleAutoScroll(currentPointerPos, position, newPosition))
        m_compositionView->setPointerPos(int(newPosition));
}

void
TrackEditor::slotLoopDraggedToPosition(timeT position)
{
    if (!m_doc)
        return;

    int currentEndLoopPos = m_doc->getComposition().getLoopEnd();
    double dummy;
    handleAutoScroll(currentEndLoopPos, position, dummy);
}

bool TrackEditor::handleAutoScroll(int currentPosition, timeT newTimePosition, double &newPosition)
{
    if (!m_rulerScale)
        return false;

    newPosition = m_rulerScale->getXForTime(newTimePosition);
    const double distance = fabs(newPosition - currentPosition);

    bool moveDetected = (distance >= 1.0);

    if (moveDetected) {

        if (m_doc  &&  m_doc->getSequenceManager()  &&
            m_doc->getSequenceManager()->getTransportStatus() != STOPPED) {

            if (m_playTracking) {
                m_compositionView->scrollHoriz(newPosition);
            }
        } else {
            m_compositionView->doAutoScroll();
        }

    }

    return moveDetected;
}

void
TrackEditor::toggleTracking()
{
    m_playTracking = !m_playTracking;
}

void
TrackEditor::slotSetLoop(timeT start, timeT end)
{
    m_topStandardRuler->getLoopRuler()->slotSetLoopMarker(start, end);
    m_bottomStandardRuler->getLoopRuler()->slotSetLoopMarker(start, end);
}

void
TrackEditor::addCommandToHistory(Command *command)
{
    CommandHistory::getInstance()->addCommand(command);
}

void
TrackEditor::slotScrollToTrack(int track)
{
    // Find the vertical track pos
    int newY = track * m_trackCellHeight;

    m_compositionView->scrollVertSmallSteps(newY);
}

void
TrackEditor::deleteSelectedSegments()
{
    SegmentSelection segments = m_compositionView->getSelectedSegments();

    if (segments.empty())
        return;

    // Clear the selection before erasing the Segments
    // the selection points to
    //
    m_compositionView->getModel()->clearSelected();

    MacroCommand *macro = new MacroCommand(tr("Delete Segments"));

    // For each selected segment
    for (SegmentSelection::iterator it = segments.begin();
         it != segments.end();
         ++it) {
        macro->addCommand(new SegmentEraseCommand(*it,
                          &m_doc->getAudioFileManager()));
    }

    addCommandToHistory(macro);
}

void
TrackEditor::turnRepeatingSegmentToRealCopies()
{
    RG_DEBUG << "turnRepeatingSegmentToRealCopies()";

    SegmentSelection segments = m_compositionView->getSelectedSegments();

    if (segments.empty())
        return;

    QString text = tr("Turn %n Repeating Segment(s) into Real Copies", "", segments.size());

    MacroCommand *macro = new MacroCommand(text);

    // For each selected segment
    for (SegmentSelection::iterator it = segments.begin();
         it != segments.end();
         ++it) {
        if ((*it)->isRepeating()) {
            macro->addCommand(new SegmentRepeatToCopyCommand(*it));
        }
    }

    addCommandToHistory(macro);
}

void
TrackEditor::turnLinkedSegmentsToRealCopies()
{
    RG_DEBUG << "turnLinkedSegmentsToRealCopies()";

    SegmentSelection segments = m_compositionView->getSelectedSegments();

    if (segments.empty())
        return;

    QString text = tr("Turn %n Linked Segment(s) into Real Copies", "", segments.size());

    MacroCommand *macro = new MacroCommand(text);

    // For each selected segment
    for (SegmentSelection::iterator it = segments.begin();
         it != segments.end();
         ++it) {
        if ((*it)->isLinked()) {
            macro->addCommand(new SegmentLinkToCopyCommand(*it));
        }
    }

    addCommandToHistory(macro);
}

void
TrackEditor::slotVerticalScrollTrackButtons(int y)
{
    // Make sure the TrackButtons are scrolled the same amount as the
    // segment canvas (CompositionView).
    m_trackButtonScroll->verticalScrollBar()->setValue(y);
}

void TrackEditor::slotCommandExecuted()
{
    // ??? This routine doesn't belong here.  It is doing things for other
    //     objects.  Those objects should take care of themselves.

    // ??? Is there a way to do this at a more relevant time?  Like when
    //     the Composition changes, instead for every command that is
    //     executed regardless of whether that command changes the
    //     Composition?

    bool compositionNeedsRefresh = m_doc->getComposition().
            getRefreshStatus(m_compositionRefreshStatusId).needsRefresh();

    // If the composition has changed, redraw the CompositionView's
    // contents.
    if (compositionNeedsRefresh) {

        // ??? Need to investigate each of these calls and see if we can
        //     implement them in a more appropriate way.  E.g. can
        //     CompositionView take care of itself?
        // ??? It would be more logical if CompositionView redrew its
        //     contents and repainted itself in response to a change
        //     to the Composition.  The change notification would need
        //     to be called responsibly (e.g. not for every single
        //     modification to an event while recording!).  And observers
        //     would need to be smart about detecting relevant changes
        //     and avoiding work.  It's the same old story.

        // In case the composition has grown.
        // ??? CompositionView should take care of this.
        updateCanvasSize();

        // In case any tracks have been added, deleted, or changed.
        // ??? TrackButtons should take care of this.
        m_trackButtons->slotUpdateTracks();

        // Redraw the contents.
        // ??? CompositionView should take care of this.
        m_compositionView->clearSegmentRectsCache(true);
        m_compositionView->updateContents();

        Composition &composition = m_doc->getComposition();

        // ??? Composition should take care of have_segments.
        if (composition.getNbSegments() == 0) {
            emit stateChange("have_segments", false);
            emit stateChange("have_selection", false);
        } else {
            emit stateChange("have_segments", true);
            // ??? CompositionView should take care of have_selection.
            if (m_compositionView->haveSelection())
                emit stateChange("have_selection", true);
            else
                emit stateChange("have_selection", false);
        }

        // Clear the composition refresh flag.
        m_doc->getComposition().
                getRefreshStatus(m_compositionRefreshStatusId).
                setNeedsRefresh(false);
    }

    // Send a paint event to all children.
    // ??? Painting every time a command is executed is overkill.  We should
    //     only paint when relevant changes are made to the Composition.
    // ??? The children should paint themselves in response to changes.
    //     TrackEditor shouldn't care.
    update();
}

void TrackEditor::slotViewportResize()
{
    QGridLayout *grid = dynamic_cast<QGridLayout *>(layout());
    if (!grid)
        return;

    // The height of the segment canvas.
    int viewportHeight = m_compositionView->viewport()->height();

    // Force TrackButtons to the same size as the viewport.
    // The only way to do this with a grid is to adjust the minimum height of
    // the row below TrackButtons to take up the appropriate amount of space.
    // In this case, that's the height of the bottom ruler and the horizontal
    // scrollbar, if it exists.
    grid->setRowMinimumHeight(4, m_compositionView->height() - viewportHeight);
}

void TrackEditor::dragEnterEvent(QDragEnterEvent *e)
{
    const QMimeData *mime = e->mimeData();

    if (mime->hasUrls()  ||  mime->hasText()) {

        if (e->proposedAction() & Qt::CopyAction) {
            e->acceptProposedAction();
        } else {
            e->setDropAction(Qt::CopyAction);
            e->accept();
        }
    } else {
        QStringList formats(mime->formats());
        RG_DEBUG << "HINT: Unaccepted MimeFormat in TrackEditor::dragEnterEvent : " << formats << endl;
    }
}


void TrackEditor::dragMoveEvent(QDragMoveEvent *)
{
    // Prevent QWidget from handling this.
    // QWidget::dragMoveEvent() does nothing, so this isn't really necessary.
}


void TrackEditor::dropEvent(QDropEvent *e)
{
    QStringList uriList;
    QString text;

    if (e->mimeData()->hasUrls() || e->mimeData()->hasText()) {

        if (e->proposedAction() & Qt::CopyAction) {
            e->acceptProposedAction();
        } else {
            e->setDropAction(Qt::CopyAction);
            e->accept();
        }

        if (e->mimeData()->hasUrls()) {
            QList<QUrl> uList = e->mimeData()->urls();
            if (!uList.isEmpty()) {
                for (int i = 0; i < uList.size(); ++i)  {
                    uriList.append(QString::fromLocal8Bit(uList.value(i).toEncoded().data()));
               }
            }
        }

        if (e->mimeData()->hasText()) {
            text = e->mimeData()->text();
        }
    }

    if (uriList.empty() && text == "") {
        RG_DEBUG << "TrackEditor::dropEvent: Nothing dropped" << endl;
        return;
    }

    RG_DEBUG << "TrackEditor::dropEvent: uri list is " << uriList
             << ", text is " << text << endl;

    QPoint cpoint = m_compositionView->mapFrom(this, e->pos());

    int trackPos = m_compositionView->grid().getYBin
        (cpoint.y() + m_compositionView->contentsY());

    timeT time = m_compositionView->grid().snapX
        (cpoint.x() + m_compositionView->contentsX());

    RG_DEBUG << "trackPos = " << trackPos << ", time = " << time << endl;

    Track *track = m_doc->getComposition().getTrackByPosition(trackPos);

    bool internal = (e->source() != 0); // have a source widget

    if (!internal && !uriList.empty()) {

        // Update code allow multiple audio drops to TrackEditor
        // Old behavior of stopping if .rg or .midi file encountered still works
        // We have a URI, and it didn't come from within RG

        RG_DEBUG << "TrackEditor::dropEvent() : got URI :" << uriList.first() << endl;
        
        QStringList::const_iterator ci;
        for (ci = uriList.constBegin(); ci != uriList.constEnd(); ++ci) {

            QString uri = *ci;
            QString tester = uri.toLower();

            if (tester.endsWith(".rg") || tester.endsWith(".rgp") ||
                tester.endsWith(".mid") || tester.endsWith(".midi")) {

                // is a rosegarden document or project

                emit droppedDocument(uri);
                return;
                //
                // WARNING
                //
                // DO NOT PERFORM ANY OPERATIONS AFTER THAT
                // EMITTING THIS SIGNAL TRIGGERS THE LOADING OF A NEW DOCUMENT
                // AND AS A CONSEQUENCE THE DELETION OF THIS TrackEditor OBJECT
                //

            } else {
            
                if (!track) return;

                RG_DEBUG << "TrackEditor::dropEvent() : dropping at track pos = "
                         << trackPos
                         << ", time = " << time
                         << ", x = " << e->pos().x()
                         << endl;

                QString audioText;
                QTextStream t(&audioText, QIODevice::ReadWrite);
                t << uri << "\n";
                t << track->getId() << "\n";
                t << time << "\n";
                t.flush();

                RG_DEBUG << "TrackEditor::dropEvent() audioText = \n " << audioText << "\n";

                emit droppedNewAudio(audioText);
                // connected to RosegardenMainViewWidget::slotDroppedNewAudio()
            }
        }

    } else if (internal && !text.isEmpty()) {

        // We have some text and a source widget: this is an internal
        // drop, which will hopefully turn out to be from the audio
        // file manager

        RG_DEBUG << "TrackEditor::dropEvent() : got text info " << endl;
        
        QString tester = text.toLower();

        if (tester.endsWith(".rg") || tester.endsWith(".rgp") ||
            tester.endsWith(".mid") || tester.endsWith(".midi")) {

            // presumably unlikely for an internal drop, but we can
            // handle it so no reason not to
            emit droppedDocument(text);
            return;
            //
            // WARNING
            //
            // DO NOT PERFORM ANY OPERATIONS AFTER THAT
            // EMITTING THIS SIGNAL TRIGGERS THE LOADING OF A NEW DOCUMENT
            // AND AS A CONSEQUENCE THE DELETION OF THIS TrackEditor OBJECT
            //

        } else {

            if (!track) return;

            // if it's a RG-internal drag-drop, use the text data provided

            QTextStream s(&text);  //qt4

            QString id;
            AudioFileId audioFileId;
            RealTime startTime, endTime;

            // read the audio info checking for end of stream
            s >> id;
            s >> audioFileId;
            s >> startTime.sec;
            s >> startTime.nsec;
            s >> endTime.sec;
            s >> endTime.nsec;

            // We know e->source() is non-NULL, tested it above when
            // setting internal, but no harm in leaving this check in
            QString sourceName = "NULL";
            if (e->source()) sourceName = e->source()->objectName();
            
            RG_DEBUG << "TrackEditor::dropEvent() - event source : " << sourceName << endl;
            
            if (sourceName == "AudioListView") { // only create something if this is data from the right client
                
                RG_DEBUG << "TrackEditor::dropEvent() : dropping at track pos = "
                         << trackPos
                         << ", time = " << time
                         << ", x = " << e->pos().x()
                         << endl;

                QString audioText;
                QTextStream t(&audioText);
                t << audioFileId << "\n";
                t << track->getId() << "\n";
                t << time << "\n"; // time on canvas
                t << startTime.sec << "\n";
                t << startTime.nsec << "\n";
                t << endTime.sec << "\n";
                t << endTime.nsec << "\n";
                
                emit droppedAudio(audioText);

            } else {
                // data is not from AudioFileManager

                QMessageBox::warning
                    (this, tr("Rosegarden"),
                     tr("Rosegarden cannot accept dropped files of this type."));
            }
        }
    }
}


} // end namespace Rosegarden

#include "TrackEditor.moc"
