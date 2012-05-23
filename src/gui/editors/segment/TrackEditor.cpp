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
#include "compositionview/CompositionModel.h"
#include "compositionview/CompositionModelImpl.h"
#include "compositionview/CompositionView.h"
#include "document/CommandHistory.h"
#include "document/RosegardenDocument.h"
#include "gui/application/RosegardenMainWindow.h"
#include "gui/rulers/ChordNameRuler.h"
#include "gui/rulers/TempoRuler.h"
#include "gui/rulers/LoopRuler.h"
#include "gui/general/IconLoader.h"
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


namespace Rosegarden
{

TrackEditor::TrackEditor(RosegardenDocument* doc,
                         QWidget* rosegardenguiview,
                         RulerScale *rulerScale,
                         bool showTrackLabels,
                         double initialUnitsPerPixel,
                         QWidget* parent) :
    QWidget(parent),
    m_doc(doc),
    m_rulerScale(rulerScale),
    m_topStandardRuler(0),
    m_bottomStandardRuler(0),
    m_trackButtons(0),
    m_compositionView(0),
    m_trackButtonScroll(0),
    m_showTrackLabels(showTrackLabels),
    m_canvasWidth(0),
    m_compositionRefreshStatusId(doc->getComposition().getNewRefreshStatusId()),
    m_playTracking(true),
    m_initialUnitsPerPixel(initialUnitsPerPixel)
{
    // accept dnd
    setAcceptDrops(true);
    //setDragEnabled(true);
    
    
    init(rosegardenguiview);
    slotReadjustCanvasSize();
}

TrackEditor::~TrackEditor()
{
    delete m_chordNameRuler;
    delete m_compositionModel;
}

void
TrackEditor::init(QWidget* rosegardenguiview)
{
    QGridLayout *grid = new QGridLayout(this);
    grid->setMargin(0);
    grid->setSpacing(0);

    int trackLabelWidth = 200;
    int barButtonsHeight = 25;
    
    m_chordNameRuler = new ChordNameRuler(m_rulerScale,
                                          m_doc,
                                          0.0,
                                          20,
                                          this);
    grid->addWidget(m_chordNameRuler, 0, 1);

//    m_chordNameRuler->hide();

    m_tempoRuler = new TempoRuler(m_rulerScale,
                                  m_doc,
                                  RosegardenMainWindow::self(),
                                  0.0,
                                  24,
                                  true,
                                  this);

    grid->addWidget(m_tempoRuler, 1, 1);

    m_tempoRuler->connectSignals();

//    m_tempoRuler->hide();

    //
    // Top Bar Buttons
    //
    m_topStandardRuler = new StandardRuler(m_doc,
                                     m_rulerScale,
                                     0,
                                     barButtonsHeight,
                                     false,
                                     true,
                                     this);
    m_topStandardRuler->connectRulerToDocPointer(m_doc);
    m_topStandardRuler->setContentsMargins(2,0,0,0);

    grid->addWidget(m_topStandardRuler, 2, 1);

//    m_topStandardRuler->hide();

    //
    // Segment Canvas
    //
    m_compositionModel = new CompositionModelImpl(m_doc->getComposition(),
                         m_doc->getStudio(),
                         m_rulerScale, getTrackCellHeight());

    connect(rosegardenguiview, SIGNAL(instrumentParametersChanged(InstrumentId)),
            m_compositionModel, SLOT(slotInstrumentParametersChanged(InstrumentId)));
    connect(rosegardenguiview->parent(), SIGNAL(instrumentParametersChanged(InstrumentId)),
            m_compositionModel, SLOT(slotInstrumentParametersChanged(InstrumentId)));

    m_compositionView = new CompositionView(m_doc, m_compositionModel, this);

    IconLoader il;
    
    QSettings settings;
    settings.beginGroup( GeneralOptionsConfigGroup );

    if ( qStrToBool( settings.value("backgroundtextures", "true" ) ) ) {

        QPixmap background = il.loadPixmap("bg-segmentcanvas");

        if (!background.isNull()) {
            m_compositionView->setBackgroundPixmap(background);
//            m_compositionView->viewport()->setBackgroundPixmap(background);
            QPalette palette;
            palette.setBrush(m_compositionView->backgroundRole(), QBrush(background));
            palette.setBrush(m_compositionView->viewport()->backgroundRole(), QBrush(background));
            m_compositionView->setPalette(palette);
            m_compositionView->viewport()->setPalette(palette);
        }
    }

    //
    // Bottom Bar Buttons
    //
    m_bottomStandardRuler = new StandardRuler(m_doc,
                                        m_rulerScale,
                                        0,
                                        barButtonsHeight,
                                        true,
                                        true,
                                        m_compositionView);
    m_bottomStandardRuler->connectRulerToDocPointer(m_doc);
    m_bottomStandardRuler->setContentsMargins(2,0,0,0);

    m_compositionView->setBottomFixedWidget(m_bottomStandardRuler);

    grid->addWidget(m_compositionView, 3, 1, 2, 1); // Multi-cell widget FromRow, FromCol, RowSpan, ColSpan

    QSize hsbSize = m_compositionView->horizontalScrollBar()->sizeHint();
    grid->setRowMinimumHeight(4,hsbSize.height() + m_bottomStandardRuler->height());

    grid->setColumnStretch(1, 10); // to make sure the seg canvas doesn't leave a "blank" grey space when
    // loading a file which has a low zoom factor

    // Track Buttons
    //
    m_trackButtonScroll = new DeferScrollArea(this);
    // Scroll bars always off
    m_trackButtonScroll->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_trackButtonScroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    grid->addWidget(m_trackButtonScroll, 3, 0);


    int canvasHeight = getTrackCellHeight() *
                       std::max(40u, m_doc->getComposition().getNbTracks());

    m_trackButtons = new TrackButtons(m_doc,
                                      getTrackCellHeight(),
                                      trackLabelWidth,
                                      m_showTrackLabels,
                                      canvasHeight,
                                      m_trackButtonScroll);

    m_trackButtons->setObjectName("TRACK_BUTTONS"); // permit styling; internal string; no tr()

    m_trackButtonScroll->setWidget(m_trackButtons);

    //connect(m_trackButtons, SIGNAL(widthChanged()),
    //        this, SLOT(slotTrackButtonsWidthChanged()));

    connect(m_trackButtons, SIGNAL(trackSelected(int)),
            rosegardenguiview, SLOT(slotSelectTrackSegments(int)));

    connect(m_trackButtons, SIGNAL(instrumentSelected(int)),
            rosegardenguiview, SLOT(slotUpdateInstrumentParameterBox(int)));

    connect(this, SIGNAL(stateChange(QString, bool)),
            rosegardenguiview, SIGNAL(stateChange(QString, bool)));

    // No such signal.  Was there ever?
//    connect(m_trackButtons, SIGNAL(modified()),
//            m_doc, SLOT(slotDocumentModified()));

    // connect loop rulers' follow-scroll signals
    connect(m_topStandardRuler->getLoopRuler(), SIGNAL(startMouseMove(int)),
            m_compositionView, SLOT(startAutoScroll(int)));
    connect(m_topStandardRuler->getLoopRuler(), SIGNAL(stopMouseMove()),
            m_compositionView, SLOT(stopAutoScroll()));
    connect(m_bottomStandardRuler->getLoopRuler(), SIGNAL(startMouseMove(int)),
            m_compositionView, SLOT(startAutoScroll(int)));
    connect(m_bottomStandardRuler->getLoopRuler(), SIGNAL(stopMouseMove()),
            m_compositionView, SLOT(stopAutoScroll()));

    //&&&  Interesting one here.  Q(3)ScrollArea had a contentsMoving signal we
    // used to grab for some purpose.  Q(Abstract)ScrollArea has no usable
    // signals whatsoever.  I think this is why autoscrolling is still slightly
    // wonky in Thorn, but I don't reckon there's much to do about this one
    // unless we write a custom widget or something.  
    //
    //connect(m_compositionView, SIGNAL(contentsMoving(int, int)),
    //        this, SLOT(slotCanvasScrolled(int, int)));

    // Synchronize bar buttons' scrollview with segment canvas' scrollbar
    //
    connect(m_compositionView->verticalScrollBar(), SIGNAL(valueChanged(int)),
            this, SLOT(slotVerticalScrollTrackButtons(int)));
//    connect(m_vertScrollBar, SIGNAL(valueChanged(int)),
//        this, SLOT(slotVerticalScrollTrackButtons(int)));

    connect(m_compositionView->verticalScrollBar(), SIGNAL(sliderMoved(int)),
            this, SLOT(slotVerticalScrollTrackButtons(int)));

    // scrolling with mouse wheel
    connect(m_trackButtonScroll, SIGNAL(gotWheelEvent(QWheelEvent*)),
            m_compositionView, SLOT(slotExternalWheelEvent(QWheelEvent*)));

    // Connect horizontal scrollbar
    //
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
    //        m_compositionView, SLOT(slotUpdateSegmentsDrawBuffer()));

    connect(m_compositionView->getModel(),
            SIGNAL(selectedSegments(const SegmentSelection &)),
            rosegardenguiview,
            SLOT(slotSelectedSegments(const SegmentSelection &)));

    connect(m_compositionView, SIGNAL(zoomIn()),
            RosegardenMainWindow::self(), SLOT(slotZoomIn()));
    connect(m_compositionView, SIGNAL(zoomOut()),
            RosegardenMainWindow::self(), SLOT(slotZoomOut()));

    connect(CommandHistory::getInstance(), SIGNAL(commandExecuted()),
            this, SLOT(update()));

    connect(m_doc, SIGNAL(pointerPositionChanged(timeT)),
            this, SLOT(slotSetPointerPosition(timeT)));

    //
    // pointer and loop drag signals from top and bottom bar buttons (loop rulers actually)
    //
    connect(m_topStandardRuler, SIGNAL(dragPointerToPosition(timeT)),
            this, SLOT(slotPointerDraggedToPosition(timeT)));
    connect(m_bottomStandardRuler, SIGNAL(dragPointerToPosition(timeT)),
            this, SLOT(slotPointerDraggedToPosition(timeT)));

    connect(m_topStandardRuler, SIGNAL(dragLoopToPosition(timeT)),
            this, SLOT(slotLoopDraggedToPosition(timeT)));
    connect(m_bottomStandardRuler, SIGNAL(dragLoopToPosition(timeT)),
            this, SLOT(slotLoopDraggedToPosition(timeT)));

    connect(m_doc, SIGNAL(loopChanged(timeT,
                                      timeT)),
            this, SLOT(slotSetLoop(timeT, timeT)));

    settings.endGroup();
}

void TrackEditor::slotReadjustCanvasSize()
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

int TrackEditor::getTrackCellHeight() const
{
    int size;
    static QFont defaultFont;

    // do some scrabbling around for a reasonable size
    //
    size = defaultFont.pixelSize();

    if (size < 8) {
        if (QApplication::font(this).pixelSize() < 8)
            size = 12;
        else
            size = QApplication::font(this).pixelSize();
    }

    return size + 12;
}

bool TrackEditor::isCompositionModified()
{
    return m_doc->getComposition().getRefreshStatus
           (m_compositionRefreshStatusId).needsRefresh();
}

void TrackEditor::setCompositionModified(bool c)
{
    m_doc->getComposition().getRefreshStatus
    (m_compositionRefreshStatusId).setNeedsRefresh(c);
}

void TrackEditor::updateRulers()
{
    if (getTempoRuler() != 0)
        getTempoRuler()->update();

    if (getChordNameRuler() != 0)
        getChordNameRuler()->update();

    getTopStandardRuler()->update();
    getBottomStandardRuler()->update();
}

void TrackEditor::paintEvent(QPaintEvent* e)
{
#if 0
    RG_DEBUG << "TrackEditor::paintEvent()";
    static QTime t;
    RG_DEBUG << "  elapsed: " << t.restart();
    QRect wr = rect();
    RG_DEBUG << "  widget rect: (" << wr.x() << "," << wr.y() << ") - (" << wr.right() << "," << wr.bottom() << ")";
    QRect r = e->rect();
    RG_DEBUG << "  pe rect: (" << r.x() << "," << r.y() << ") - (" << r.right() << "," << r.bottom() << ")";
    QVector<QRect> rects = e->region().rects();
    for (unsigned i = 0; i < (unsigned)rects.size(); ++i) {
        QRect &s = rects[i];
        RG_DEBUG << "  pe region rect #" << i+1 << " (" << s.x() << "," << s.y() << ") - (" << s.right() << "," << s.bottom() << ")";
    }
#endif

    // ??? If we are asked to paint, we should paint.  It shouldn't matter
    //   whether anything has changed.  This may be here as a way to stop the
    //   endless loop that would otherwise be created by the
    //   m_compositionView->updateContents() call inside this "if".
    if (isCompositionModified()) {

        //RG_DEBUG << "TrackEditor::paintEvent: Composition modified";

        // !!! These calls from within a paintEvent look ugly
        // ??? Need to investigate each of these calls and see if we can
        //     implement them in a more appropriate way.  Some are causing
        //     serious CPU issues.  None belong here.

        slotReadjustCanvasSize();

        // ??? At one time, this was linked to excessive CPU usage while
        //     recording MIDI.  Now, it doesn't seem to make much difference.
        m_trackButtons->slotUpdateTracks();

        m_compositionView->clearSegmentRectsCache(true);

        // ??? This is directly contrary to the advice in the Qt docs.  This
        //   is asking a child QWidget to paint itself.  From the Qt docs for
        //   QWidget::paintEvent():
        //     "Generally, you should refrain from calling update() or
        //      repaint() inside a paintEvent(). For example, calling
        //      update() or repaint() on children inside a paintevent()
        //      results in undefined behavior; the child may or may not get
        //      a paint event."
        m_compositionView->updateContents();

        //m_compositionView->update();

        Composition &composition = m_doc->getComposition();

        if (composition.getNbSegments() == 0) {
            emit stateChange("have_segments", false); // no segments : reverse state
            emit stateChange("have_selection", false); // no segments : reverse state
        } else {
            emit stateChange("have_segments", true);
            if (m_compositionView->haveSelection())
                emit stateChange("have_selection", true);
            else
                emit stateChange("have_selection", false); // no selection : reverse state
        }

        setCompositionModified(false);
    }

    QWidget::paintEvent(e);
}

void TrackEditor::slotAddTracks(unsigned int nbNewTracks,
                                InstrumentId id,
                                int position)
{
    Composition &comp = m_doc->getComposition();

    AddTracksCommand* command = new AddTracksCommand(&comp, nbNewTracks, id,
                                                     position);
    addCommandToHistory(command);
    slotReadjustCanvasSize();
}

void TrackEditor::slotDeleteTracks(std::vector<TrackId> tracks)
{
    Composition &comp = m_doc->getComposition();

    DeleteTracksCommand* command = new DeleteTracksCommand(&comp, tracks);
    addCommandToHistory(command);
}

void TrackEditor::addSegment(int track, int time, unsigned int duration)
{
    if (!m_doc)
        return ; // sanity check

    SegmentInsertCommand *command =
        new SegmentInsertCommand(m_doc, track, time, duration);

    addCommandToHistory(command);
}

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

void
TrackEditor::slotSetPointerPosition(timeT position)
{
    SimpleRulerScale *ruler =
        dynamic_cast<SimpleRulerScale*>(m_rulerScale);

    if (!ruler)
        return ;

    double pos = m_compositionView->grid().getRulerScale()->getXForTime(position);

    int currentPointerPos = m_compositionView->getPointerPos();

    double distance = pos - currentPointerPos;
    if (distance < 0.0)
        distance = -distance;

    if (distance >= 1.0) {

        if (m_doc && m_doc->getSequenceManager() &&
            (m_doc->getSequenceManager()->getTransportStatus() != STOPPED)) {
            
            if (m_playTracking) {
                getCompositionView()->slotScrollHoriz(int(double(position) / ruler->getUnitsPerPixel()));
            }
        } else if (!getCompositionView()->isAutoScrolling()) {
            int newpos = int(double(position) / ruler->getUnitsPerPixel());
            //             RG_DEBUG << "TrackEditor::slotSetPointerPosition("
            //                      << position
            //                      << ") : calling canvas->slotScrollHoriz() "
            //                      << newpos << endl;
            getCompositionView()->slotScrollHoriz(newpos);
        }

        m_compositionView->setPointerPos(pos);
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
    if (m_doc) {
        int currentEndLoopPos = m_doc->getComposition().getLoopEnd();
        double dummy;
        handleAutoScroll(currentEndLoopPos, position, dummy);
    }
}

bool TrackEditor::handleAutoScroll(int currentPosition, timeT newTimePosition, double &newPosition)
{
    SimpleRulerScale *ruler =
        dynamic_cast<SimpleRulerScale*>(m_rulerScale);

    if (!ruler)
        return false;

    newPosition = m_compositionView->grid().getRulerScale()->getXForTime(newTimePosition);

    double distance = fabs(newPosition - currentPosition);

    bool moveDetected = distance >= 1.0;

    if (moveDetected) {

        if (m_doc && m_doc->getSequenceManager() &&
                (m_doc->getSequenceManager()->getTransportStatus() != STOPPED)) {

            if (m_playTracking) {
                getCompositionView()->slotScrollHoriz(int(double(newTimePosition) / ruler->getUnitsPerPixel()));
            }
        } else {
            int newpos = int(double(newTimePosition) / ruler->getUnitsPerPixel());
            getCompositionView()->slotScrollHorizSmallSteps(newpos);
            getCompositionView()->doAutoScroll();
        }

    }

    return moveDetected;
}

void
TrackEditor::slotToggleTracking()
{
    m_playTracking = !m_playTracking;
}

void
TrackEditor::slotSetLoop(timeT start, timeT end)
{
    getTopStandardRuler()->getLoopRuler()->slotSetLoopMarker(start, end);
    getBottomStandardRuler()->getLoopRuler()->slotSetLoopMarker(start, end);
}

void
TrackEditor::addCommandToHistory(Command *command)
{
    CommandHistory::getInstance()->addCommand(command);
}

void
TrackEditor::slotScrollToTrack(int track)
{
    ///!!! Reconfigure to use m_compositionmodel to return y value for track number
    // Find the vertical track pos
    int newY = track * getTrackCellHeight();

    RG_DEBUG << "TrackEditor::scrollToTrack(" << track <<
    ") scrolling to Y " << newY << endl;

    // Scroll the segment view; it will scroll tracks by connected signals
    //    slotVerticalScrollTrackButtons(newY);

    // This is currently a bit broke
    m_compositionView->slotScrollVertSmallSteps(newY);

    // This works but is basic McBasic
    //m_compositionView->setContentsPos(m_compositionView->contentsX(),newY);
}

void
TrackEditor::slotDeleteSelectedSegments()
{
    MacroCommand *macro = new MacroCommand("Delete Segments");

    SegmentSelection segments =
        m_compositionView->getSelectedSegments();

    if (segments.size() == 0)
        return ;

    SegmentSelection::iterator it;

    // Clear the selection before erasing the Segments
    // the selection points to
    //
    m_compositionView->getModel()->clearSelected();

    // Create the compound command
    //
    for (it = segments.begin(); it != segments.end(); it++) {
        macro->addCommand(new SegmentEraseCommand(*it,
                          &m_doc->getAudioFileManager()));
    }

    addCommandToHistory(macro);

}

void
TrackEditor::slotTurnRepeatingSegmentToRealCopies()
{
    RG_DEBUG << "TrackEditor::slotTurnRepeatingSegmentToRealCopies" << endl;

    SegmentSelection segments =
        m_compositionView->getSelectedSegments();

    if (segments.size() == 0)
        return ;

    QString text = tr("Turn %n Repeating Segment(s) into Real Copies", "", segments.size());

    MacroCommand *macro = new MacroCommand(text);

    SegmentSelection::iterator it = segments.begin();
    for (; it != segments.end(); it++) {
        if ((*it)->isRepeating()) {
            macro->addCommand(new SegmentRepeatToCopyCommand(*it));
        }
    }

    addCommandToHistory(macro);

}

void
TrackEditor::slotTurnLinkedSegmentsToRealCopies()
{
    RG_DEBUG << "TrackEditor::slotTurnLinkedSegmentsToRealCopies" << endl;

    SegmentSelection segments =
        m_compositionView->getSelectedSegments();

    if (segments.size() == 0) return ;

    QString text = tr("Turn %n Linked Segment(s) into Real Copies", "", segments.size());

    MacroCommand *macro = new MacroCommand(text);

    SegmentSelection::iterator it = segments.begin();
    for (; it != segments.end(); it++) {
        if ((*it)->isLinked()) {
            macro->addCommand(new SegmentLinkToCopyCommand(*it));
        }
    }

    addCommandToHistory(macro);

}

void
TrackEditor::slotVerticalScrollTrackButtons(int y)
{
//     m_trackButtonScroll->setContentsPos(0, y);
    m_trackButtonScroll->verticalScrollBar()->setValue(y);
    
//     ensureVisible ( int x, int y, int xmargin = 50, int ymargin = 50 )
//    m_trackButtonScroll->ensureVisible ( 0, y, 50, 20 );
}

void TrackEditor::dragEnterEvent(QDragEnterEvent *e)
{
    const QMimeData *mime;
    mime = e->mimeData();
    QStringList formats(mime->formats());
    
    if (mime->hasFormat("text/uri-list") || mime->hasFormat("text/plain")) {

        if (e->proposedAction() & Qt::CopyAction) {
            e->acceptProposedAction();
        } else {
            e->setDropAction(Qt::CopyAction);
            e->accept();
        }
    }else{
        RG_DEBUG << "HINT: Unaccepted MimeFormat in TrackEditor::dragEnterEvent : " << formats << endl;
    }
}



void TrackEditor::dragMoveEvent(QDragMoveEvent *){
    // pass
}



void TrackEditor::dropEvent(QDropEvent *e)
{
    QStringList uriList;
    QString text;
    QString audioText;

    if (e->provides("text/uri-list") || e->provides("text/plain")) {

        if (e->proposedAction() & Qt::CopyAction) {
            e->acceptProposedAction();
        } else {
            e->setDropAction(Qt::CopyAction);
            e->accept();
        }

        if (e->provides("text/uri-list")) {
            // note: we could also do
            // QList<QUrl> uList = e->mimeData()->urls();
            uriList =
                QString::fromLocal8Bit(e->encodedData("text/uri-list").data())
                .split(QRegExp("[\\r\\n]+"), QString::SkipEmptyParts);
        }
        if (e->provides("text/plain")) {
            text = e->mimeData()->text();
            //text = QString::fromLocal8Bit(e->encodedData("text/plain").data());
            //uriList << text;
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
        // Old behavoir of stopping if .rg or .midi file encountered still works
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

                audioText.clear();
                QTextStream t(&audioText, QIODevice::ReadWrite);
                t << uri << "\n";
                t << track->getId() << "\n";
                t << time << "\n";
                t.flush();
                RG_DEBUG << "TrackEditor::dropEvent() audioText = \n " << audioText << "\n";
                emit droppedNewAudio(QString(audioText));
                // connected to RosegardenMainViewWidget::droppedNewAudio()
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
