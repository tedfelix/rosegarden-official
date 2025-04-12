/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2024 the Rosegarden development team.

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
    m_compositionView(nullptr),
    m_compositionModel(nullptr),
    m_trackCellHeight(0),
    m_trackButtons(nullptr),
    m_trackButtonScroll(nullptr),
    m_showTrackLabels(showTrackLabels),
    m_rulerScale(rulerScale),
    m_tempoRuler(nullptr),
    m_chordNameRuler(nullptr),
    m_topStandardRuler(nullptr),
    m_bottomStandardRuler(nullptr)
    //m_canvasWidth(0)
{
    // Accept objects dragged and dropped onto this widget.
    setAcceptDrops(true);

    // ??? Only called here.  Inline it here.
    init(mainViewWidget);

    updateCanvasSize();

    Composition &comp = m_doc->getComposition();
    m_playTracking = comp.getMainFollowPlayback();
}

void
TrackEditor::init(RosegardenMainViewWidget *mainViewWidget)
{
    QSettings settings;
    settings.beginGroup(GeneralOptionsConfigGroup);
    int trackSize = settings.value("track_size", 0).toInt();
    // Write it back out so we can find it.
    settings.setValue("track_size", trackSize);

    int sizeFactor = 100;     // 0: small (default)
    if (trackSize == 1)       // 1: medium
        sizeFactor = 125;
    else if (trackSize == 2)  // 2: large
        sizeFactor = 150;
    else if (trackSize == 3)  // 3: extra large
        sizeFactor = 194;

    m_trackCellHeight = 24 * sizeFactor / 100;

    QGridLayout *grid = new QGridLayout(this);
    grid->setContentsMargins(0, 0, 0, 0);
    grid->setSpacing(0);

    // Top Rulers

    m_chordNameRuler = new ChordNameRuler(m_rulerScale,
                                          m_doc,
                                          20,
                                          this);
    grid->addWidget(m_chordNameRuler, 0, 1);

    m_tempoRuler = new TempoRuler(m_rulerScale,
                                  m_doc,
                                  24,
                                  true);
    grid->addWidget(m_tempoRuler, 1, 1);

    m_topStandardRuler = new StandardRuler(m_doc,
                                     m_rulerScale,
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

    const int canvasHeight = m_trackCellHeight *
                       std::max(40u, m_doc->getComposition().getNbTracks());

    m_trackButtons = new TrackButtons(m_trackCellHeight,
                                      m_showTrackLabels,
                                      canvasHeight,  // overall height
                                      m_trackButtonScroll);  // parent

    m_trackButtonScroll->setWidget(m_trackButtons);

    // Connections

    //connect(m_trackButtons, SIGNAL(widthChanged()),
    //        this, SLOT(slotTrackButtonsWidthChanged()));

    connect(m_trackButtons, &TrackButtons::trackSelected,
            mainViewWidget, &RosegardenMainViewWidget::slotSelectTrackSegments);

    connect(this, &TrackEditor::stateChange,
            mainViewWidget, &RosegardenMainViewWidget::stateChange);

    // No such signal.  Was there ever?
//    connect(m_trackButtons, SIGNAL(modified()),
//            m_doc, SLOT(slotDocumentModified()));

    // Connect for all standard ruler mouse move starts and stops.  This
    // allows for auto-scroll while the user drags (pointer or loop) in
    // a StandardRuler.
    connect(m_topStandardRuler->getLoopRuler(), &LoopRuler::startMouseMove,
            this, &TrackEditor::slotSRStartMouseMove);
    connect(m_topStandardRuler->getLoopRuler(), &LoopRuler::stopMouseMove,
            this, &TrackEditor::slotSRStopMouseMove);
    connect(m_bottomStandardRuler->getLoopRuler(), &LoopRuler::startMouseMove,
            this, &TrackEditor::slotSRStartMouseMove);
    connect(m_bottomStandardRuler->getLoopRuler(), &LoopRuler::stopMouseMove,
            this, &TrackEditor::slotSRStopMouseMove);

    // Connect for TempoRuler mouse press/release to allow for
    // auto-scroll while the user drags in the TempoRuler.
    connect(m_tempoRuler, &TempoRuler::mousePress,
            this, &TrackEditor::slotTRMousePress);
    connect(m_tempoRuler, &TempoRuler::mouseRelease,
            this, &TrackEditor::slotTRMouseRelease);

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
    connect(m_compositionView->verticalScrollBar(), &QAbstractSlider::valueChanged,
            this, &TrackEditor::slotVerticalScrollTrackButtons);
    connect(m_compositionView->verticalScrollBar(), &QAbstractSlider::sliderMoved,
            this, &TrackEditor::slotVerticalScrollTrackButtons);

    // Connect scrolling with the mouse wheel in the TrackButtons to
    // scrolling the CompositionView.
    connect(m_trackButtonScroll, &DeferScrollArea::gotWheelEvent,
            m_compositionView, &CompositionView::slotExternalWheelEvent);

    // Synchronize the rulers with the horizontal scrollbar.

    connect(m_compositionView->horizontalScrollBar(), &QAbstractSlider::valueChanged,
            m_topStandardRuler, &StandardRuler::slotScrollHoriz);
    connect(m_compositionView->horizontalScrollBar(), &QAbstractSlider::sliderMoved,
            m_topStandardRuler, &StandardRuler::slotScrollHoriz);

    connect(m_compositionView->horizontalScrollBar(), &QAbstractSlider::valueChanged,
            m_bottomStandardRuler, &StandardRuler::slotScrollHoriz);
    connect(m_compositionView->horizontalScrollBar(), &QAbstractSlider::sliderMoved,
            m_bottomStandardRuler, &StandardRuler::slotScrollHoriz);

    connect(m_compositionView->horizontalScrollBar(), &QAbstractSlider::valueChanged,
            m_tempoRuler, &TempoRuler::slotScrollHoriz);
    connect(m_compositionView->horizontalScrollBar(), &QAbstractSlider::sliderMoved,
            m_tempoRuler, &TempoRuler::slotScrollHoriz);

    connect(m_compositionView->horizontalScrollBar(), &QAbstractSlider::valueChanged,
            m_chordNameRuler, &ChordNameRuler::slotScrollHoriz);
    connect(m_compositionView->horizontalScrollBar(), &QAbstractSlider::sliderMoved,
            m_chordNameRuler, &ChordNameRuler::slotScrollHoriz);

    // Was only emitted from dead code.
    //connect(this, SIGNAL(needUpdate()),
    //        m_compositionView, SLOT(slotUpdateAll()));

    connect(m_compositionView->getModel(),
            &CompositionModelImpl::selectionChanged,
            mainViewWidget,
            &RosegardenMainViewWidget::slotSelectedSegments);

    connect(m_compositionView, &RosegardenScrollView::viewportResize,
            this, &TrackEditor::slotViewportResize);
    connect(m_compositionView, &RosegardenScrollView::zoomIn,
            RosegardenMainWindow::self(), &RosegardenMainWindow::slotZoomIn);
    connect(m_compositionView, &RosegardenScrollView::zoomOut,
            RosegardenMainWindow::self(), &RosegardenMainWindow::slotZoomOut);

    connect(CommandHistory::getInstance(), &CommandHistory::commandExecuted,
            this, &TrackEditor::slotCommandExecuted);

    connect(m_doc, &RosegardenDocument::pointerPositionChanged,
            this, &TrackEditor::slotSetPointerPosition);

    // Top/Bottom ruler pointer drag.
    connect(m_topStandardRuler, &StandardRuler::dragPointerToPosition,
            this, &TrackEditor::slotPointerDraggedToPosition);
    connect(m_bottomStandardRuler, &StandardRuler::dragPointerToPosition,
            this, &TrackEditor::slotPointerDraggedToPosition);
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

void TrackEditor::addTrack(InstrumentId id,
                           int position)
{
    addCommandToHistory(new AddTracksCommand(id, position));

    updateCanvasSize();
}

void TrackEditor::deleteTracks(std::vector<TrackId> tracks)
{
    MacroCommand *macro = new MacroCommand(tr("Delete Tracks"));

    Composition &comp = m_doc->getComposition();
    const SegmentMultiSet &segments = comp.getSegments();

    // Delete the segments.

    // for each track we are deleting
    for (size_t i = 0; i < tracks.size(); ++i) {
        const TrackId trackId = tracks[i];

        // for each segment in the composition
        for (SegmentMultiSet::const_iterator segmentIter = segments.begin();
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

        m_compositionView->drawPointer(newPosition);

    }
}

void
TrackEditor::slotPointerDraggedToPosition(timeT position)
{
    if (!m_rulerScale)
        return;

    double newPosition = m_rulerScale->getXForTime(position);

    m_compositionView->drawPointer(static_cast<int>(newPosition));
}

void
TrackEditor::slotSRStartMouseMove()
{
    m_compositionView->setFollowMode(FOLLOW_HORIZONTAL);
    m_compositionView->startAutoScroll();
}

void TrackEditor::slotSRStopMouseMove()
{
    m_compositionView->stopAutoScroll();
}

void
TrackEditor::slotTRMousePress()
{
    m_compositionView->setFollowMode(FOLLOW_HORIZONTAL);
    m_compositionView->startAutoScroll();
}

void
TrackEditor::slotTRMouseRelease()
{
    m_compositionView->stopAutoScroll();
}

void
TrackEditor::scrollToFollow()
{
    m_playTracking = !m_playTracking;
    Composition &comp = m_doc->getComposition();
    comp.setMainFollowPlayback(m_playTracking);

}

void
TrackEditor::addCommandToHistory(Command *command)
{
    CommandHistory::getInstance()->addCommand(command);
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

    // ??? RosegardenMainViewWidget also connects
    //     CompositionView::slotUpdateAll() to commandExecuted().

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
        // ??? CompositionModelImpl now does this in response to doc
        //     modified.  It is likely that this is redundant.
        m_compositionView->deleteCachedPreviews();
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
        RG_DEBUG << "HINT: Unaccepted MimeFormat in TrackEditor::dragEnterEvent : " << formats;
    }
}

/* unused
void TrackEditor::dragMoveEvent(QDragMoveEvent *)
{
    // Prevent QWidget from handling this.
    // QWidget::dragMoveEvent() does nothing, so this isn't really necessary.
}
*/

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
            RG_DEBUG << "dropEvent() urls: " << e->mimeData()->urls();

            // Note: December 2015, with Qt5, links dragged from Chromium
            //       and dropped onto rg end up as a list containing an
            //       empty URL and a URL that consists of the link text
            //       (e.g. "Click Here!") rather than the link itself.
            //       Firefox does not have this problem.  With Qt4, Chromium
            //       works fine.  I'm assuming the issue is with either
            //       Chromium or Qt5 and will eventually get fixed.

            QList<QUrl> uList = e->mimeData()->urls();
            if (!uList.isEmpty()) {
                for (int i = 0; i < uList.size(); ++i)  {
                    uriList.append(QString::fromLocal8Bit(uList.value(i).toEncoded().data()));
                }
            }

            RG_DEBUG << "dropEvent() uri list: " << uriList;
        }

        if (e->mimeData()->hasText()) {
            RG_DEBUG << "dropEvent() text: " << e->mimeData()->text();

            text = e->mimeData()->text();
        }
    }

    if (uriList.empty() && text == "") {
        RG_DEBUG << "dropEvent(): Nothing dropped";
        return;
    }

#if (QT_VERSION >= QT_VERSION_CHECK(6, 0, 0))
    QPoint cpoint = m_compositionView->mapFrom(this, e->position().toPoint());
#else
    QPoint cpoint = m_compositionView->mapFrom(this, e->pos());
#endif

    int trackPos = m_compositionView->grid().getYBin
        (cpoint.y() + m_compositionView->contentsY());

    timeT time = m_compositionView->grid().snapX
        (cpoint.x() + m_compositionView->contentsX());

    RG_DEBUG << "dropEvent() trackPos = " << trackPos << ", time = " << time;

    Track *track = m_doc->getComposition().getTrackByPosition(trackPos);

    bool internal = (e->source() != nullptr); // have a source widget

    if (!internal && !uriList.empty()) {

        // Update code allow multiple audio drops to TrackEditor
        // Old behavior of stopping if .rg or .midi file encountered still works
        // We have a URI, and it didn't come from within RG

        RG_DEBUG << "dropEvent() first URI: " << uriList.first();

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
                // !!! WARNING
                //
                // DO NOT PERFORM ANY OPERATIONS AFTER EMITTING droppedDocument().
                // EMITTING droppedDocument() TRIGGERS THE LOADING OF A NEW DOCUMENT
                // AND AS A CONSEQUENCE THE DELETION OF THIS TrackEditor OBJECT.
                //

            } else {

                if (!track) return;

                RG_DEBUG << "dropEvent() : dropping at track pos = " << trackPos
                         << ", time = " << time
#if (QT_VERSION >= QT_VERSION_CHECK(6, 0, 0))
                         << ", x = " << e->position().toPoint().x();
#else
                         << ", x = " << e->pos().x();
#endif

                QString audioText;
                QTextStream t(&audioText, QIODevice::ReadWrite);
                t << uri << "\n";
                t << track->getId() << "\n";
                t << time << "\n";
                t.flush();

                RG_DEBUG << "dropEvent() audioText = " << audioText;

                emit droppedNewAudio(audioText);
                // connected to RosegardenMainViewWidget::slotDroppedNewAudio()
            }
        }

    } else if (internal && !text.isEmpty()) {

        // We have some text and a source widget: this is an internal
        // drop, which will hopefully turn out to be from the audio
        // file manager

        RG_DEBUG << "dropEvent(): got text info";

        QString tester = text.toLower();

        if (tester.endsWith(".rg") || tester.endsWith(".rgp") ||
            tester.endsWith(".mid") || tester.endsWith(".midi")) {

            // presumably unlikely for an internal drop, but we can
            // handle it so no reason not to
            emit droppedDocument(text);
            return;
            //
            // !!! WARNING
            //
            // DO NOT PERFORM ANY OPERATIONS AFTER EMITTING droppedDocument().
            // EMITTING droppedDocument() TRIGGERS THE LOADING OF A NEW DOCUMENT
            // AND AS A CONSEQUENCE THE DELETION OF THIS TrackEditor OBJECT.
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

            // We know e->source() is non-nullptr, tested it above when
            // setting internal, but no harm in leaving this check in
            QString sourceName = "nullptr";
            if (e->source()) sourceName = e->source()->objectName();

            RG_DEBUG << "dropEvent() event source: " << sourceName;

            if (sourceName == "AudioListView") { // only create something if this is data from the right client

                RG_DEBUG << "dropEvent() : dropping at track pos = " << trackPos
                         << ", time = " << time
#if (QT_VERSION >= QT_VERSION_CHECK(6, 0, 0))
                         << ", x = " << e->position().toPoint().x();
#else
                         << ", x = " << e->pos().x();
#endif

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
