/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2008 the Rosegarden development team.
 
    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.
 
    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/


#include "TrackEditor.h"
#include <QLayout>
#include <kapplication.h>

#include <klocale.h>
#include <kconfig.h>
#include <kstandarddirs.h>
#include "misc/Debug.h"
#include "document/ConfigGroups.h"
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
#include "segmentcanvas/CompositionModel.h"
#include "segmentcanvas/CompositionModelImpl.h"
#include "segmentcanvas/CompositionView.h"
#include "document/MultiViewCommandHistory.h"
#include "document/RosegardenGUIDoc.h"
#include "gui/application/RosegardenGUIApp.h"
#include "gui/rulers/ChordNameRuler.h"
#include "gui/rulers/TempoRuler.h"
#include "gui/rulers/LoopRuler.h"
#include "gui/widgets/ProgressDialog.h"
#include "gui/widgets/QDeferScrollView.h"
#include "sound/AudioFile.h"
#include "TrackButtons.h"
#include "TrackEditorIface.h"
#include <dcopobject.h>
#include "document/Command.h"
#include <kglobal.h>
#include <kmessagebox.h>
#include <QApplication>
#include <QCursor>
#include <QFont>
#include <QPixmap>
#include <QPoint>
#include <qscrollview.h>
#include <QString>
#include <QStringList>
#include <QStringList>
#include <QWidget>
#include <QValidator>
#include <qdragobject.h>
#include <QTextStream>


namespace Rosegarden
{

TrackEditor::TrackEditor(RosegardenGUIDoc* doc,
                         QWidget* rosegardenguiview,
                         RulerScale *rulerScale,
                         bool showTrackLabels,
                         double initialUnitsPerPixel,
                         QWidget* parent, const char* name,
                         WFlags) :
    DCOPObject("TrackEditorIface"),
    QWidget(parent, name),
    m_doc(doc),
    m_rulerScale(rulerScale),
    m_topStandardRuler(0),
    m_bottomStandardRuler(0),
    m_trackButtons(0),
    m_segmentCanvas(0),
    m_trackButtonScroll(0),
    m_showTrackLabels(showTrackLabels),
    m_canvasWidth(0),
    m_compositionRefreshStatusId(doc->getComposition().getNewRefreshStatusId()),
    m_playTracking(true),
    m_initialUnitsPerPixel(initialUnitsPerPixel)
{
    // accept dnd
    setAcceptDrops(true);

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
    QGridLayout *grid = new QGridLayout(this, 4, 2);

    int trackLabelWidth = 230;
    int barButtonsHeight = 25;

    m_chordNameRuler = new ChordNameRuler(m_rulerScale,
                                          m_doc,
                                          0.0,
                                          20,
                                          this);
    grid->addWidget(m_chordNameRuler, 0, 1);

    m_tempoRuler = new TempoRuler(m_rulerScale,
                                  m_doc,
                                  RosegardenGUIApp::self(),
                                  0.0,
                                  24,
                                  true,
                                  this);

    grid->addWidget(m_tempoRuler, 1, 1);

    m_tempoRuler->connectSignals();

    //
    // Top Bar Buttons
    //
    m_topStandardRuler = new StandardRuler(m_doc,
                                     m_rulerScale,
                                     0,
                                     barButtonsHeight,
                                     false,
                                     this, "topbarbuttons");
    m_topStandardRuler->connectRulerToDocPointer(m_doc);

    grid->addWidget(m_topStandardRuler, 2, 1);

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

    m_segmentCanvas = new CompositionView(m_doc, m_compositionModel, this);

    QSettings confq4;

    confq4.beginGroup( GeneralOptionsConfigGroup );

    // 

    // FIX-manually-(GW), add:

    // confq4.endGroup();		// corresponding to: confq4.beginGroup( GeneralOptionsConfigGroup );

    //  

    if ( qStrToBool( confq4.value("backgroundtextures", "true" ) ) ) {
        QPixmap background;
        QString pixmapDir = KGlobal::dirs()->findResource("appdata", "pixmaps/");
        if (background.load(QString("%1/misc/bg-segmentcanvas.xpm").
                            arg(pixmapDir))) {
            m_segmentCanvas->setBackgroundPixmap(background);
            m_segmentCanvas->viewport()->setBackgroundPixmap(background);
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
                                        m_segmentCanvas, "bottombarbuttons");
    m_bottomStandardRuler->connectRulerToDocPointer(m_doc);

    m_segmentCanvas->setBottomFixedWidget(m_bottomStandardRuler);

    grid->addWidget(m_segmentCanvas, 3, 1);

    grid->setColStretch(1, 10); // to make sure the seg canvas doesn't leave a "blank" grey space when
    // loading a file which has a low zoom factor

    // Track Buttons
    //
    // (must be put in a QScrollView)
    //
    m_trackButtonScroll = new QDeferScrollView(this);
    grid->addWidget(m_trackButtonScroll, 3, 0);

    int canvasHeight = getTrackCellHeight() *
                       std::max(40u, m_doc->getComposition().getNbTracks());

    m_trackButtons = new TrackButtons(m_doc,
                                      getTrackCellHeight(),
                                      trackLabelWidth,
                                      m_showTrackLabels,
                                      canvasHeight,
                                      m_trackButtonScroll->viewport());
    m_trackButtonScroll->addChild(m_trackButtons);
    m_trackButtonScroll->setHScrollBarMode(QScrollView::AlwaysOff);
    m_trackButtonScroll->setVScrollBarMode(QScrollView::AlwaysOff);
    m_trackButtonScroll->setResizePolicy(QScrollView::AutoOneFit);
    m_trackButtonScroll->setBottomMargin(m_bottomStandardRuler->height() +
                                         m_segmentCanvas->horizontalScrollBar()->height());

    connect(m_trackButtons, SIGNAL(widthChanged()),
            this, SLOT(slotTrackButtonsWidthChanged()));

    connect(m_trackButtons, SIGNAL(trackSelected(int)),
            rosegardenguiview, SLOT(slotSelectTrackSegments(int)));

    connect(m_trackButtons, SIGNAL(instrumentSelected(int)),
            rosegardenguiview, SLOT(slotUpdateInstrumentParameterBox(int)));

    connect(this, SIGNAL(stateChange(QString, bool)),
            rosegardenguiview, SIGNAL(stateChange(QString, bool)));

    connect(m_trackButtons, SIGNAL(modified()),
            m_doc, SLOT(slotDocumentModified()));

    connect(m_trackButtons, SIGNAL(muteButton(TrackId, bool)),
            rosegardenguiview, SLOT(slotSetMuteButton(TrackId, bool)));

    // connect loop rulers' follow-scroll signals
    connect(m_topStandardRuler->getLoopRuler(), SIGNAL(startMouseMove(int)),
            m_segmentCanvas, SLOT(startAutoScroll(int)));
    connect(m_topStandardRuler->getLoopRuler(), SIGNAL(stopMouseMove()),
            m_segmentCanvas, SLOT(stopAutoScroll()));
    connect(m_bottomStandardRuler->getLoopRuler(), SIGNAL(startMouseMove(int)),
            m_segmentCanvas, SLOT(startAutoScroll(int)));
    connect(m_bottomStandardRuler->getLoopRuler(), SIGNAL(stopMouseMove()),
            m_segmentCanvas, SLOT(stopAutoScroll()));

    connect(m_segmentCanvas, SIGNAL(contentsMoving(int, int)),
            this, SLOT(slotCanvasScrolled(int, int)));

    // Synchronize bar buttons' scrollview with segment canvas' scrollbar
    //
    connect(m_segmentCanvas->verticalScrollBar(), SIGNAL(valueChanged(int)),
            this, SLOT(slotVerticalScrollTrackButtons(int)));

    connect(m_segmentCanvas->verticalScrollBar(), SIGNAL(sliderMoved(int)),
            this, SLOT(slotVerticalScrollTrackButtons(int)));

    // scrolling with mouse wheel
    connect(m_trackButtonScroll, SIGNAL(gotWheelEvent(QWheelEvent*)),
            m_segmentCanvas, SLOT(slotExternalWheelEvent(QWheelEvent*)));

    // Connect horizontal scrollbar
    //
    connect(m_segmentCanvas->horizontalScrollBar(), SIGNAL(valueChanged(int)),
            m_topStandardRuler, SLOT(slotScrollHoriz(int)));
    connect(m_segmentCanvas->horizontalScrollBar(), SIGNAL(sliderMoved(int)),
            m_topStandardRuler, SLOT(slotScrollHoriz(int)));

    connect(m_segmentCanvas->horizontalScrollBar(), SIGNAL(valueChanged(int)),
            m_bottomStandardRuler, SLOT(slotScrollHoriz(int)));
    connect(m_segmentCanvas->horizontalScrollBar(), SIGNAL(sliderMoved(int)),
            m_bottomStandardRuler, SLOT(slotScrollHoriz(int)));

    connect(m_segmentCanvas->horizontalScrollBar(), SIGNAL(valueChanged(int)),
            m_tempoRuler, SLOT(slotScrollHoriz(int)));
    connect(m_segmentCanvas->horizontalScrollBar(), SIGNAL(sliderMoved(int)),
            m_tempoRuler, SLOT(slotScrollHoriz(int)));

    connect(m_segmentCanvas->horizontalScrollBar(), SIGNAL(valueChanged(int)),
            m_chordNameRuler, SLOT(slotScrollHoriz(int)));
    connect(m_segmentCanvas->horizontalScrollBar(), SIGNAL(sliderMoved(int)),
            m_chordNameRuler, SLOT(slotScrollHoriz(int)));

    connect(this, SIGNAL(needUpdate()), m_segmentCanvas, SLOT(slotUpdateSegmentsDrawBuffer()));

    connect(m_segmentCanvas->getModel(),
            SIGNAL(selectedSegments(const SegmentSelection &)),
            rosegardenguiview,
            SLOT(slotSelectedSegments(const SegmentSelection &)));

    connect(m_segmentCanvas, SIGNAL(zoomIn()),
            RosegardenGUIApp::self(), SLOT(slotZoomIn()));
    connect(m_segmentCanvas, SIGNAL(zoomOut()),
            RosegardenGUIApp::self(), SLOT(slotZoomOut()));

    connect(getCommandHistory(), SIGNAL(commandExecuted()),
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
}

void TrackEditor::slotReadjustCanvasSize()
{
    m_segmentCanvas->slotUpdateSize();
}

void TrackEditor::slotTrackButtonsWidthChanged()
{
    // We need to make sure the trackButtons geometry is fully updated
    //
    ProgressDialog::processEvents();

    m_trackButtonScroll->setMinimumWidth(m_trackButtons->width());
    m_doc->slotDocumentModified();
}

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
    if (isCompositionModified()) {

        slotReadjustCanvasSize();
        m_trackButtons->slotUpdateTracks();
        m_segmentCanvas->clearSegmentRectsCache(true);
        m_segmentCanvas->updateContents();

        Composition &composition = m_doc->getComposition();

        if (composition.getNbSegments() == 0) {
            emit stateChange("have_segments", false); // no segments : reverse state
            emit stateChange("have_selection", false); // no segments : reverse state
        } else {
            emit stateChange("have_segments", true);
            if (m_segmentCanvas->haveSelection())
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

void TrackEditor::slotSegmentOrderChanged(int section, int fromIdx, int toIdx)
{
    RG_DEBUG << QString("TrackEditor::segmentOrderChanged(section : %1, from %2, to %3)")
    .arg(section).arg(fromIdx).arg(toIdx) << endl;

    //!!! how do we get here? need to involve a command
    emit needUpdate();
}

void
TrackEditor::slotCanvasScrolled(int x, int y)
{
    // update the pointer position if the user is dragging it from the loop ruler
    if ((m_topStandardRuler && m_topStandardRuler->getLoopRuler() &&
         m_topStandardRuler->getLoopRuler()->hasActiveMousePress() &&
         !m_topStandardRuler->getLoopRuler()->getLoopingMode()) ||
        (m_bottomStandardRuler && m_bottomStandardRuler->getLoopRuler() &&
         m_bottomStandardRuler->getLoopRuler()->hasActiveMousePress() &&
         !m_bottomStandardRuler->getLoopRuler()->getLoopingMode())) {

        int mx = m_segmentCanvas->viewport()->mapFromGlobal(QCursor::pos()).x();
        m_segmentCanvas->setPointerPos(x + mx);

        // bad idea, creates a feedback loop
        // 	timeT t = m_segmentCanvas->grid().getRulerScale()->getTimeForX(x + mx);
        // 	slotSetPointerPosition(t);
    }
}

void
TrackEditor::slotSetPointerPosition(timeT position)
{
    SimpleRulerScale *ruler =
        dynamic_cast<SimpleRulerScale*>(m_rulerScale);

    if (!ruler)
        return ;

    double pos = m_segmentCanvas->grid().getRulerScale()->getXForTime(position);

    int currentPointerPos = m_segmentCanvas->getPointerPos();

    double distance = pos - currentPointerPos;
    if (distance < 0.0)
        distance = -distance;

    if (distance >= 1.0) {

        if (m_doc && m_doc->getSequenceManager() &&
            (m_doc->getSequenceManager()->getTransportStatus() != STOPPED)) {
            
            if (m_playTracking) {
                getSegmentCanvas()->slotScrollHoriz(int(double(position) / ruler->getUnitsPerPixel()));
            }
        } else if (!getSegmentCanvas()->isAutoScrolling()) {
            int newpos = int(double(position) / ruler->getUnitsPerPixel());
            //             RG_DEBUG << "TrackEditor::slotSetPointerPosition("
            //                      << position
            //                      << ") : calling canvas->slotScrollHoriz() "
            //                      << newpos << endl;
            getSegmentCanvas()->slotScrollHoriz(newpos);
        }

        m_segmentCanvas->setPointerPos(pos);
    }

}

void
TrackEditor::slotPointerDraggedToPosition(timeT position)
{
    int currentPointerPos = m_segmentCanvas->getPointerPos();

    double newPosition;

    if (handleAutoScroll(currentPointerPos, position, newPosition))
        m_segmentCanvas->setPointerPos(int(newPosition));
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

    newPosition = m_segmentCanvas->grid().getRulerScale()->getXForTime(newTimePosition);

    double distance = fabs(newPosition - currentPosition);

    bool moveDetected = distance >= 1.0;

    if (moveDetected) {

        if (m_doc && m_doc->getSequenceManager() &&
                (m_doc->getSequenceManager()->getTransportStatus() != STOPPED)) {

            if (m_playTracking) {
                getSegmentCanvas()->slotScrollHoriz(int(double(newTimePosition) / ruler->getUnitsPerPixel()));
            }
        } else {
            int newpos = int(double(newTimePosition) / ruler->getUnitsPerPixel());
            getSegmentCanvas()->slotScrollHorizSmallSteps(newpos);
            getSegmentCanvas()->doAutoScroll();
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

MultiViewCommandHistory*
TrackEditor::getCommandHistory()
{
    return m_doc->getCommandHistory();
}

void
TrackEditor::addCommandToHistory(Command *command)
{
    getCommandHistory()->addCommand(command);
}

void
TrackEditor::slotScrollToTrack(int track)
{
    // Find the vertical track pos
    int newY = track * getTrackCellHeight();

    RG_DEBUG << "TrackEditor::scrollToTrack(" << track <<
    ") scrolling to Y " << newY << endl;

    // Scroll the segment view; it will scroll tracks by connected signals
    //    slotVerticalScrollTrackButtons(newY);
    m_segmentCanvas->slotScrollVertSmallSteps(newY);
}

void
TrackEditor::slotDeleteSelectedSegments()
{
    MacroCommand *macro = new MacroCommand("Delete Segments");

    SegmentSelection segments =
        m_segmentCanvas->getSelectedSegments();

    if (segments.size() == 0)
        return ;

    SegmentSelection::iterator it;

    // Clear the selection before erasing the Segments
    // the selection points to
    //
    m_segmentCanvas->getModel()->clearSelected();

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
        m_segmentCanvas->getSelectedSegments();

    if (segments.size() == 0)
        return ;

    QString text;

    if (segments.size() == 1)
        text = i18n("Turn Repeating Segment into Real Copies");
    else
        text = i18n("Turn Repeating Segments into Real Copies");

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
TrackEditor::slotVerticalScrollTrackButtons(int y)
{
    m_trackButtonScroll->setContentsPos(0, y);
}

void TrackEditor::dragEnterEvent(QDragEnterEvent *event)
{
    event->accept(QUriDrag::canDecode(event) ||
                  QTextDrag::canDecode(event));
}

void TrackEditor::dropEvent(QDropEvent* event)
{
    QStrList uri;
    QString text;

    int heightAdjust = 0;
    //int widthAdjust = 0;

    // Adjust any drop event height position by visible rulers
    //
    if (m_topStandardRuler && m_topStandardRuler->isVisible())
        heightAdjust += m_topStandardRuler->height();

    if (m_tempoRuler && m_tempoRuler->isVisible())
        heightAdjust += m_tempoRuler->height();

    if (m_chordNameRuler && m_chordNameRuler->isVisible())
        heightAdjust += m_chordNameRuler->height();

    QPoint posInSegmentCanvas =
        m_segmentCanvas->viewportToContents
        (m_segmentCanvas->
         viewport()->mapFrom(this, event->pos()));

    int trackPos = m_segmentCanvas->grid().getYBin(posInSegmentCanvas.y());

    timeT time =
//        m_segmentCanvas->grid().getRulerScale()->
//        getTimeForX(posInSegmentCanvas.x());
        m_segmentCanvas->grid().snapX(posInSegmentCanvas.x());


    if (QUriDrag::decode(event, uri)) {
        RG_DEBUG << "TrackEditor::dropEvent() : got URI :"
        << uri.first() << endl;
        QString uriPath = uri.first();

        if (uriPath.endsWith(".rg")) {
            emit droppedDocument(uriPath);
        } else {

            QStrList uris;
            QString uri;
            if (QUriDrag::decode(event, uris)) uri = uris.first();
//            QUriDrag::decodeLocalFiles(event, files);
//            QString filePath = files.first();

            RG_DEBUG << "TrackEditor::dropEvent() : got URI: "
            << uri << endl;

            RG_DEBUG << "TrackEditor::dropEvent() : dropping at track pos = "
            << trackPos
            << ", time = "
            << time
            << ", x = "
            << event->pos().x()
            << ", mapped x = "
            << posInSegmentCanvas.x()
            << endl;

            Track* track = m_doc->getComposition().getTrackByPosition(trackPos);
            if (track) {
                QString audioText;
                QTextOStream t(&audioText);

                t << uri << "\n";
                t << track->getId() << "\n";
                t << time << "\n";

                emit droppedNewAudio(audioText);
            }

        }

    } else if (QTextDrag::decode(event, text)) {
        RG_DEBUG << "TrackEditor::dropEvent() : got text info " << endl;
        //<< text << endl;

        if (text.endsWith(".rg")) {
            emit droppedDocument(text);
            //
            // WARNING
            //
            // DO NOT PERFORM ANY OPERATIONS AFTER THAT
            // EMITTING THIS SIGNAL TRIGGERS THE LOADING OF A NEW DOCUMENT
            // AND AS A CONSEQUENCE THE DELETION OF THIS TrackEditor OBJECT
            //
        } else {

            QTextIStream s(&text);

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

            if (id == "AudioFileManager") { // only create something if this is data from the right client


                // Drop this audio segment if we have a valid track number
                // (could also check for time limits too)
                //
                Track* track = m_doc->getComposition().getTrackByPosition(trackPos);
                if (track) {

                    RG_DEBUG << "TrackEditor::dropEvent() : dropping at track pos = "
                    << trackPos
                    << ", time = "
                    << time
                    << ", x = "
                    << event->pos().x()
                    << ", map = "
                    << posInSegmentCanvas.x()
                    << endl;

                    QString audioText;
                    QTextOStream t(&audioText);
                    t << audioFileId << "\n";
                    t << track->getId() << "\n";
                    t << time << "\n"; // time on canvas
                    t << startTime.sec << "\n";
                    t << startTime.nsec << "\n";
                    t << endTime.sec << "\n";
                    t << endTime.nsec << "\n";

                    emit droppedAudio(audioText);
                }

            } else {

                KMessageBox::sorry(this, i18n("You can't drop files into Rosegarden from this client.  Try using Konqueror instead."));

            }

        }

        // SEE WARNING ABOVE - DON'T DO ANYTHING, THIS OBJECT MAY NOT
        // EXIST AT THIS POINT.

    }
}

}
#include "TrackEditor.moc"
