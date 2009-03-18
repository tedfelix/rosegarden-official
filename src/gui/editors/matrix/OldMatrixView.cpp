/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2009 the Rosegarden development team.
 
    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.
 
    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/
#ifdef NO_LONGER_USED

#include <Q3Canvas>
#include <Q3CanvasItem>
#include <Q3CanvasPixmap>
#include <Q3CanvasView>

#include "MatrixView.h"

#include "base/BaseProperties.h"
#include "misc/Debug.h"
#include "misc/Strings.h"
#include "base/AudioLevel.h"
#include "base/Clipboard.h"
#include "base/Composition.h"
#include "base/Event.h"
#include "base/Instrument.h"
#include "base/LayoutEngine.h"
#include "base/MidiProgram.h"
#include "base/NotationTypes.h"
#include "base/Profiler.h"
#include "base/PropertyName.h"
#include "base/BasicQuantizer.h"
#include "base/LegatoQuantizer.h"
#include "base/RealTime.h"
#include "base/RulerScale.h"
#include "base/Segment.h"
#include "base/Selection.h"
#include "base/SnapGrid.h"
#include "base/Staff.h"
#include "base/Studio.h"
#include "base/Track.h"
#include "commands/edit/ChangeVelocityCommand.h"
#include "commands/edit/ClearTriggersCommand.h"
#include "commands/edit/CollapseNotesCommand.h"
#include "commands/edit/CopyCommand.h"
#include "commands/edit/CutCommand.h"
#include "commands/edit/EraseCommand.h"
#include "commands/edit/EventQuantizeCommand.h"
#include "commands/edit/EventUnquantizeCommand.h"
#include "commands/edit/PasteEventsCommand.h"
#include "commands/edit/SelectionPropertyCommand.h"
#include "commands/edit/SetTriggerCommand.h"
#include "commands/matrix/MatrixInsertionCommand.h"
#include "document/RosegardenGUIDoc.h"
#include "document/ConfigGroups.h"
#include "gui/application/RosegardenGUIApp.h"
#include "gui/dialogs/EventFilterDialog.h"
#include "gui/dialogs/EventParameterDialog.h"
#include "gui/dialogs/QuantizeDialog.h"
#include "gui/dialogs/TriggerSegmentDialog.h"
#include "gui/editors/guitar/Chord.h"
#include "gui/editors/notation/NotationElement.h"
#include "gui/editors/notation/NotationStrings.h"
#include "gui/editors/notation/NotePixmapFactory.h"
#include "gui/editors/parameters/InstrumentParameterBox.h"
#include "gui/rulers/StandardRuler.h"
//!!!#include "gui/general/ActiveItem.h"
#include "gui/general/EditViewBase.h"
#include "gui/general/EditView.h"
#include "gui/general/GUIPalette.h"
#include "gui/general/MidiPitchLabel.h"
#include "gui/general/IconLoader.h"
#include "gui/general/ResourceFinder.h"
#include "gui/kdeext/KTmpStatusMsg.h"
#include "gui/rulers/ControlRuler.h"
#include "gui/rulers/ChordNameRuler.h"
#include "gui/rulers/LoopRuler.h"
#include "gui/rulers/PercussionPitchRuler.h"
#include "gui/rulers/PitchRuler.h"
#include "gui/rulers/PropertyBox.h"
#include "gui/rulers/PropertyViewRuler.h"
#include "gui/rulers/TempoRuler.h"
#include "gui/studio/StudioControl.h"
#include "gui/widgets/QDeferScrollView.h"
#include "MatrixCanvasView.h"
#include "MatrixElement.h"
#include "MatrixEraser.h"
#include "MatrixHLayout.h"
#include "MatrixMover.h"
#include "MatrixPainter.h"
#include "MatrixResizer.h"
#include "MatrixVelocity.h"
#include "MatrixSelector.h"
#include "MatrixStaff.h"
#include "MatrixToolBox.h"
#include "MatrixVLayout.h"
#include "PianoKeyboard.h"
#include "sound/MappedEvent.h"
#include "sound/SequencerDataBlock.h"

#include <QAction>
#include <QComboBox>
#include <QSettings>
#include <QDockWidget>
#include <QMessageBox>
#include <QCursor>
#include <QDialog>
#include <QLayout>
#include <QIcon>
#include <QLabel>
#include <QPixmap>
#include <QPoint>
#include <QScrollArea>
#include <QSize>
#include <QSlider>
#include <QString>
#include <QWidget>
#include <QMatrix>
#include <QMouseEvent>
#include <QStatusBar>
#include <QToolBar>
#include <QShortcut>
#include <QKeySequence>



namespace Rosegarden
{

static double xorigin = 0.0;


MatrixView::MatrixView(RosegardenGUIDoc *doc,
                       std::vector<Segment *> segments,
                       QWidget *parent,
                       bool drumMode)
        : EditView(doc, segments, 3, parent, "matrixview"),
        m_hlayout(&doc->getComposition()),
        m_vlayout(),
        m_snapGrid(new SnapGrid(&m_hlayout)),
        m_lastEndMarkerTime(0),
        m_hoveredOverAbsoluteTime(0),
        m_hoveredOverNoteName(0),
        m_selectionCounter(0),
        m_insertModeLabel(0),
        m_haveHoveredOverNote(false),
        m_previousEvPitch(0),
        m_dockLeft(0),
        m_canvasView(0),
        m_pianoView(0),
        m_localMapping(0),
        m_lastNote(0),
        m_quantizations(BasicQuantizer::getStandardQuantizations()),
        m_chordNameRuler(0),
        m_tempoRuler(0),
        m_referenceRuler(new ZoomableMatrixHLayoutRulerScale(m_hlayout)),
        m_playTracking(true),
        m_dockVisible(true),
        m_drumMode(drumMode),
        m_mouseInCanvasView(false)
{
    RG_DEBUG << "MatrixView ctor: drumMode " << drumMode << "\n";

//     QString pixmapDir = KGlobal::dirs()->findResource("appdata", "pixmaps/toolbar");
//     QPixmap matrixPixmap(pixmapDir + "/matrix.xpm");
	IconLoader il;
	QPixmap matrixPixmap = il.loadPixmap( "matrix" );
	
	
	
	// note: toobars added by parseAction script (?)
	//
/*
	m_actionsToolBar = new QToolBar( "Actions Toolbar", this );
	m_actionsToolBar->setToolTip( "Actions Toolbar" );
	m_actionsToolBar->setObjectName( "Actions Toolbar" );
	
	m_zoomToolBar = new QToolBar( "Zoom Toolbar", this );
	m_zoomToolBar->setToolTip( "Zoom Toolbar" );
	m_zoomToolBar->setObjectName( "Zoom Toolbar" );
// 	m_zoomToolBar = new QSlider( Qt::Horizontal, this );
	
	addToolBar( m_actionsToolBar );
	addToolBar( m_zoomToolBar );
	
	m_actionsToolBar->setMinimumHeight( 32 );
	m_actionsToolBar->setMinimumWidth( 80 );
	
	m_zoomToolBar->setMinimumHeight( 32 );
	m_zoomToolBar->setMinimumWidth( 80 );
*/	
	
	m_dockLeft = new QDockWidget( tr("Instrument Parameters"),  this );
	m_dockLeft->setAllowedAreas( Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea );
	m_dockLeft->setFeatures( QDockWidget::AllDockWidgetFeatures );
	// set min/max size for InstrumentPanelWidget instead
// 	m_dockLeft->setMaximumSize( 300, 460 );
// 	m_dockLeft->setMinimumSize( 140, 200 );
	this->addDockWidget( Qt::LeftDockWidgetArea, m_dockLeft ); // optional 3. param: Qt::Horizontal
	
	/*
	// old:
    connect(m_dockLeft, SIGNAL(iMBeingClosed()),
            this, SLOT(slotParametersClosed()));
    connect(m_dockLeft, SIGNAL(hasUndocked()),
            this, SLOT(slotParametersClosed()));
	*/
	
	/*
	// new qt4:
	//&&& dockLeft connection - not required ?
	connect(m_dockLeft, SIGNAL(visibilityChanged(bool)),
			this, SLOT(slotParametersClosed()));
	
	connect(m_dockLeft, SIGNAL(topLevelChanged(bool)),
			this, SLOT(slotParametersClosed()));
	*/
	
	
    // Apparently, hasUndocked() is emitted when the dock widget's
    // 'close' button on the dock handle is clicked.
	connect(m_mainDockWidget, SIGNAL(docking(QDockWidget*, Qt::DockWidgetAreas )),
			this, SLOT(slotParametersDockedBack(QDockWidget*, Qt::DockWidgetAreas )));
	//### qt4 note: docking signal is now:| void QDockWidget::topLevelChanged ( bool topLevel ) 
	
	
    Composition &comp = doc->getComposition();

	// note:  BaseToolBox : maintains a single instance of each registered tool
    m_toolBox = new MatrixToolBox(this);
	
	
    initStatusBar();

    connect(m_toolBox, SIGNAL(showContextHelp(const QString &)),
            this, SLOT(slotToolHelpChanged(const QString &)));

    Q3Canvas *tCanvas = new Q3Canvas(this);

    QSettings settings;
    settings.beginGroup( MatrixViewConfigGroup );
	
// 	IconLoader il;

    if ( qStrToBool( settings.value("backgroundtextures-1.6-plus", "true" ) ) ) {
	// We now use a lined background for the non-percussion matrix,
	// suggested and supplied by Alessandro Preziosi
	QString backgroundPixmap = isDrumMode() ? "bg-paper-white" : "bg-matrix-lines";
        QPixmap background = il.loadPixmap(backgroundPixmap);
        tCanvas->setBackgroundPixmap(background);
    }

    MATRIX_DEBUG << "MatrixView : creating staff\n";

    Track *track =
        comp.getTrackById(segments[0]->getTrack());

    Instrument *instr = getDocument()->getStudio().
                        getInstrumentById(track->getInstrument());

    int resolution = 8;

    if (isDrumMode() && instr && instr->getKeyMapping()) {
        resolution = 11;
    }

    for (unsigned int i = 0; i < segments.size(); ++i) {
        m_staffs.push_back(new MatrixStaff(tCanvas,
                                           segments[i],
                                           m_snapGrid,
                                           i,
                                           resolution,
                                           this));
        // staff has one too many rows to avoid a half-row at the top:
        m_staffs[i]->setY( -resolution / 2);
        //!!!	if (isDrumMode()) m_staffs[i]->setX(resolution);
        if (i == 0)
            m_staffs[i]->setCurrent(true);
    }

    MATRIX_DEBUG << "MatrixView : creating canvas view\n";

    const MidiKeyMapping *mapping = 0;

    if (instr) {
        mapping = instr->getKeyMapping();
        if (mapping) {
            RG_DEBUG << "MatrixView: Instrument has key mapping: "
            << mapping->getName() << endl;
            m_localMapping = new MidiKeyMapping(*mapping);
            extendKeyMapping();
        } else {
            RG_DEBUG << "MatrixView: Instrument has no key mapping\n";
        }
    }

	m_pianoView = new QDeferScrollView(getCentralWidget());
//	m_pianoView = new QScrollArea(getCentralWidget());

    QWidget* vport = m_pianoView->viewport();

    if (isDrumMode() && mapping &&
            !m_localMapping->getMap().empty()) {
        m_pitchRuler = new PercussionPitchRuler(vport,
                                                m_localMapping,
                                                resolution); // line spacing
    } else {
        m_pitchRuler = new PianoKeyboard(vport);
    }

    m_pianoView->setVScrollBarMode(Q3ScrollView::AlwaysOff);
    m_pianoView->setHScrollBarMode(Q3ScrollView::AlwaysOff);
    m_pianoView->addChild(m_pitchRuler);
    m_pianoView->setFixedWidth(m_pianoView->contentsWidth());
/*
	// new:
// 	m_pianoView->verticalScrollBar()->setEnabled( false );
// 	m_pianoView->horizontalScrollBar()->setEnabled( false );
	m_pianoView->setVerticalScrollBarPolicy( Qt::ScrollBarAlwaysOff );
	m_pianoView->setHorizontalScrollBarPolicy( Qt::ScrollBarAlwaysOff );
	
// 	m_pianoView->addChild(m_pitchRuler);

	QWidget *scrollMainWidget = new QWidget();
	scrollMainWidget->setLayout( new QHBoxLayout() );

	// widget to scroll
	m_pianoView->setWidget(scrollMainWidget);

	
	
// 	m_pitchRuler->setWidth( 45 );
	m_pitchRuler->setMinimumWidth( 45 );
	m_pitchRuler->setMaximumWidth( 45 );
	
	scrollMainWidget->layout()->addWidget( m_pitchRuler );
// 	m_pianoView->setFixedWidth( m_pianoView->contentsWidth() );
// 	m_pianoView->setFixedWidth( m_pianoView->maximumViewportSize().width() );
*/		
	
    m_grid->addWidget(m_pianoView, CANVASVIEW_ROW, 1);

    m_parameterBox = new InstrumentParameterBox(getDocument(), m_dockLeft);
//	m_parameterBox->setMaximumSize( 300, 460 );
//	m_parameterBox->setMinimumSize( 140, 200 );
	
    m_dockLeft->setWidget(m_parameterBox);

    RosegardenGUIApp *app = RosegardenGUIApp::self();
    connect(app,
            SIGNAL(pluginSelected(InstrumentId, int, int)),
            m_parameterBox,
            SLOT(slotPluginSelected(InstrumentId, int, int)));
    connect(app,
            SIGNAL(pluginBypassed(InstrumentId, int, bool)),
            m_parameterBox,
            SLOT(slotPluginBypassed(InstrumentId, int, bool)));
    connect(app,
            SIGNAL(instrumentParametersChanged(InstrumentId)),
            m_parameterBox,
            SLOT(slotInstrumentParametersChanged(InstrumentId)));
    connect(m_parameterBox,
            SIGNAL(instrumentParametersChanged(InstrumentId)),
            app,
            SIGNAL(instrumentParametersChanged(InstrumentId)));
    connect(m_parameterBox,
            SIGNAL(selectPlugin(QWidget *, InstrumentId, int)),
            app,
            SLOT(slotShowPluginDialog(QWidget *, InstrumentId, int)));
    connect(m_parameterBox,
            SIGNAL(showPluginGUI(InstrumentId, int)),
            app,
            SLOT(slotShowPluginGUI(InstrumentId, int)));
    connect(parent,  // RosegardenGUIView
            SIGNAL(checkTrackAssignments()),
            this,
            SLOT(slotCheckTrackAssignments()));

    // Assign the instrument
    //
    m_parameterBox->useInstrument(instr);

    if (m_drumMode) {
        connect(m_parameterBox,
                SIGNAL(instrumentPercussionSetChanged(Instrument *)),
                this,
                SLOT(slotPercussionSetChanged(Instrument *)));
    }

    // Set the snap grid from the stored size in the segment
    //
    int snapGridSize = m_staffs[0]->getSegment().getSnapGridSize();

    MATRIX_DEBUG << "MatrixView : Snap Grid Size = " << snapGridSize << endl;

    if (snapGridSize != -1) {
        m_snapGrid->setSnapTime(snapGridSize);
    } else {
        //###settings.beginGroup( MatrixViewConfigGroup );
        snapGridSize = settings.value("Snap Grid Size", static_cast<int>(SnapGrid::SnapToBeat) ).toInt();
        m_snapGrid->setSnapTime( snapGridSize );
        m_staffs[0]->getSegment().setSnapGridSize(snapGridSize);
    }

    m_canvasView = new MatrixCanvasView(*m_staffs[0],
                                        m_snapGrid,
                                        m_drumMode,
                                        tCanvas,
                                        getCentralWidget());
    setCanvasView(m_canvasView);

    // do this after we have a canvas
    setupActions();
    setupAddControlRulerMenu();

    leaveActionState("parametersbox_closed");

    // tool bars
    initActionsToolbar();
    initZoomToolbar();

    // Connect vertical scrollbars between matrix and piano
    //
    connect(m_canvasView->verticalScrollBar(), SIGNAL(valueChanged(int)),
            this, SLOT(slotVerticalScrollPianoKeyboard(int)));

    connect(m_canvasView->verticalScrollBar(), SIGNAL(sliderMoved(int)),
            this, SLOT(slotVerticalScrollPianoKeyboard(int)));

    connect(m_canvasView, SIGNAL(zoomIn()), this, SLOT(slotZoomIn()));
    connect(m_canvasView, SIGNAL(zoomOut()), this, SLOT(slotZoomOut()));

    connect(m_pianoView, SIGNAL(gotWheelEvent(QWheelEvent*)),
            m_canvasView, SLOT(slotExternalWheelEvent(QWheelEvent*)));

    // ensure the piano keyb keeps the right margins when the user toggles
    // the canvas view rulers
    //
    connect(m_canvasView, SIGNAL(bottomWidgetHeightChanged(int)),
            this, SLOT(slotCanvasBottomWidgetHeightChanged(int)));

    connect(m_canvasView, SIGNAL(mouseEntered()),
            this, SLOT(slotMouseEnteredCanvasView()));

    connect(m_canvasView, SIGNAL(mouseLeft()),
            this, SLOT(slotMouseLeftCanvasView()));

    /*
    QObject::connect
        (getCanvasView(), SIGNAL(activeItemPressed(QMouseEvent*, Q3CanvasItem*)),
         this,            SLOT  (activeItemPressed(QMouseEvent*, Q3CanvasItem*)));
         */

    QObject::connect
    (getCanvasView(),
     SIGNAL(mousePressed(timeT,
                         int, QMouseEvent*, MatrixElement*)),
     this,
     SLOT(slotMousePressed(timeT,
                           int, QMouseEvent*, MatrixElement*)));

    QObject::connect
    (getCanvasView(),
     SIGNAL(mouseMoved(timeT, int, QMouseEvent*)),
     this,
     SLOT(slotMouseMoved(timeT, int, QMouseEvent*)));

    QObject::connect
    (getCanvasView(),
     SIGNAL(mouseReleased(timeT, int, QMouseEvent*)),
     this,
     SLOT(slotMouseReleased(timeT, int, QMouseEvent*)));

    QObject::connect
    (getCanvasView(), SIGNAL(hoveredOverNoteChanged(int, bool, timeT)),
     this, SLOT(slotHoveredOverNoteChanged(int, bool, timeT)));

    QObject::connect
    (m_pitchRuler, SIGNAL(hoveredOverKeyChanged(unsigned int)),
     this, SLOT (slotHoveredOverKeyChanged(unsigned int)));

    QObject::connect
    (m_pitchRuler, SIGNAL(keyPressed(unsigned int, bool)),
     this, SLOT (slotKeyPressed(unsigned int, bool)));

    QObject::connect
    (m_pitchRuler, SIGNAL(keySelected(unsigned int, bool)),
     this, SLOT (slotKeySelected(unsigned int, bool)));

    QObject::connect
    (m_pitchRuler, SIGNAL(keyReleased(unsigned int, bool)),
     this, SLOT (slotKeyReleased(unsigned int, bool)));

    QObject::connect
    (getCanvasView(), SIGNAL(hoveredOverAbsoluteTimeChanged(unsigned int)),
     this, SLOT (slotHoveredOverAbsoluteTimeChanged(unsigned int)));

    QObject::connect
    (doc, SIGNAL(pointerPositionChanged(timeT)),
     this, SLOT(slotSetPointerPosition(timeT)));

    MATRIX_DEBUG << "MatrixView : applying layout\n";

    bool layoutApplied = applyLayout();
    if (!layoutApplied)
        /* was sorry */ QMessageBox::warning(0, "", tr("Couldn't apply piano roll layout"));
    else {
        MATRIX_DEBUG << "MatrixView : rendering elements\n";
        for (unsigned int i = 0; i < m_staffs.size(); ++i) {

            m_staffs[i]->positionAllElements();
            m_staffs[i]->getSegment().getRefreshStatus
            (m_segmentsRefreshStatusIds[i]).setNeedsRefresh(false);
        }
    }

    StandardRuler *topStandardRuler = new StandardRuler(getDocument(),
                                &m_hlayout, int(xorigin), 25,
                                false, getCentralWidget());
    topStandardRuler->setSnapGrid(m_snapGrid);
    setTopStandardRuler(topStandardRuler);

    StandardRuler *bottomStandardRuler = new StandardRuler(getDocument(),
                                   &m_hlayout, 0, 25,
                                   true, getBottomWidget());
	
    getBottomWidget()->layout()->addWidget(bottomStandardRuler);
    bottomStandardRuler->setSnapGrid(m_snapGrid);
    setBottomStandardRuler(bottomStandardRuler);

    topStandardRuler->connectRulerToDocPointer(doc);
    bottomStandardRuler->connectRulerToDocPointer(doc);

    // Disconnect the default connections for this signal from the
    // top ruler, and connect our own instead

    QObject::disconnect
    (topStandardRuler->getLoopRuler(),
     SIGNAL(setPointerPosition(timeT)), 0, 0);

    QObject::connect
    (topStandardRuler->getLoopRuler(),
     SIGNAL(setPointerPosition(timeT)),
     this, SLOT(slotSetInsertCursorPosition(timeT)));

    QObject::connect
    (topStandardRuler,
     SIGNAL(dragPointerToPosition(timeT)),
     this, SLOT(slotSetInsertCursorPosition(timeT)));

    topStandardRuler->getLoopRuler()->setBackgroundColor
    (GUIPalette::getColour(GUIPalette::InsertCursorRuler));

    connect(topStandardRuler->getLoopRuler(), SIGNAL(startMouseMove(int)),
            m_canvasView, SLOT(startAutoScroll(int)));
    connect(topStandardRuler->getLoopRuler(), SIGNAL(stopMouseMove()),
            m_canvasView, SLOT(stopAutoScroll()));

    connect(bottomStandardRuler->getLoopRuler(), SIGNAL(startMouseMove(int)),
            m_canvasView, SLOT(startAutoScroll(int)));
    connect(bottomStandardRuler->getLoopRuler(), SIGNAL(stopMouseMove()),
            m_canvasView, SLOT(stopAutoScroll()));
    connect(m_bottomStandardRuler, SIGNAL(dragPointerToPosition(timeT)),
            this, SLOT(slotSetPointerPosition(timeT)));

    // Force height for the moment
    //
    m_pitchRuler->setFixedHeight(canvas()->height());


    updateViewCaption();

    // Add a velocity ruler
    //
    //!!!    addPropertyViewRuler(BaseProperties::VELOCITY);

    m_chordNameRuler = new ChordNameRuler
                       (m_referenceRuler, doc, segments, 0, 20, getCentralWidget());
    m_chordNameRuler->setStudio(&getDocument()->getStudio());
    addRuler(m_chordNameRuler);

    m_tempoRuler = new TempoRuler
                   (m_referenceRuler, doc, this, 0, 24, false, getCentralWidget());
    static_cast<TempoRuler *>(m_tempoRuler)->connectSignals();
    addRuler(m_tempoRuler);

    leaveActionState("have_selection");
    slotTestClipboard();

    timeT start = doc->getComposition().getLoopStart();
    timeT end = doc->getComposition().getLoopEnd();
    m_topStandardRuler->getLoopRuler()->slotSetLoopMarker(start, end);
    m_bottomStandardRuler->getLoopRuler()->slotSetLoopMarker(start, end);

    setCurrentSelection(0, false);

    // Change this if the matrix view ever has its own page
    // in the config dialog.
    setConfigDialogPageIndex(0);

    // default zoom
    //###settings.beginGroup( MatrixViewConfigGroup );

    if (m_hZoomSlider) {
        double zoom = settings.value("Zoom Level",
                                     m_hZoomSlider->getCurrentSize()).toDouble();
        m_hZoomSlider->setSize(zoom);
        m_referenceRuler->setHScaleFactor(zoom);
    }
    
    // Scroll view to centre middle-C and warp to pointer position
    //
    m_canvasView->scrollBy(0, m_staffs[0]->getCanvasYForHeight(60) / 2);

    slotSetPointerPosition(comp.getPosition());

    // All toolbars should be created before this is called
//     setAutoSaveSettings("MatrixView", true);	//&&& does not exist. use saveGeometry() ?? + custum var autoSave

    readOptions();
    setOutOfCtor();

    // Property and Control Rulers
    //
    if (getCurrentSegment()->getViewFeatures())
        slotShowVelocityControlRuler();
    setupControllerTabs();

    setRewFFwdToAutoRepeat();
    slotCompositionStateUpdate();

    settings.endGroup();
}

MatrixView::~MatrixView()
{
    slotSaveOptions();

    delete m_chordNameRuler;

    for (unsigned int i = 0; i < m_staffs.size(); ++i) {
        delete m_staffs[i]; // this will erase all "notes" canvas items
    }

    // This looks silly but the reason is that on destruction of the
    // MatrixCanvasView, setCanvas() is called (this is in
    // ~Q3CanvasView so we can't do anything about it). This calls
    // Q3CanvasView::updateContentsSize(), which in turn updates the
    // view's scrollbars, hence calling QScrollBar::setValue(), and
    // sending the QSCrollbar::valueChanged() signal. But we have a
    // slot connected to that signal
    // (MatrixView::slotVerticalScrollPianoKeyboard), which scrolls
    // the pianoView. However at this stage the pianoView has already
    // been deleted, so a likely outcome is a crash.
    //
    // A solution is to zero out m_pianoView here, and to check if
    // it's non null in slotVerticalScrollPianoKeyboard.
    //
    m_pianoView = 0;

    delete m_snapGrid;

    if (m_localMapping)
        delete m_localMapping;
}

void MatrixView::slotSaveOptions()
{
    QSettings settings;
    settings.beginGroup( MatrixViewConfigGroup );

    settings.setValue("Show Chord Name Ruler", findAction("show_chords_ruler")->isChecked());
    settings.setValue("Show Tempo Ruler", findAction("show_tempo_ruler")->isChecked());
    settings.setValue("Show Parameters", m_dockVisible);
    //findAction("m_dockLeft->isVisible());

    settings.sync();
    settings.endGroup();

}

void MatrixView::readOptions()
{
    EditView::readOptions();
    QSettings settings;
    settings.beginGroup( MatrixViewConfigGroup );

    bool opt = false;

    opt = qStrToBool( settings.value("Show Chord Name Ruler", "false" ) ) ;
    findAction("show_chords_ruler")->setChecked(opt);
    slotToggleChordsRuler();

    opt = qStrToBool( settings.value("Show Tempo Ruler", "true" ) ) ;
    findAction("show_tempo_ruler")->setChecked(opt);
    slotToggleTempoRuler();

    opt = qStrToBool( settings.value("Show Parameters", "true" ) ) ;
    if (!opt) {
//         m_dockLeft->undock();
//         m_dockLeft->hide();
        m_dockLeft->setFloating( false );
        m_dockLeft->setVisible( false );
        enterActionState("parametersbox_closed");
        m_dockVisible = false;
    }

    settings.endGroup();
}

void MatrixView::setupActions()
{
    EditViewBase::setupActions("matrix.rc");
    EditView::setupActions();

    createAction("select", SLOT(slotSelectSelected()));
    createAction("draw", SLOT(slotPaintSelected()));
    createAction("erase", SLOT(slotEraseSelected()));
    createAction("move", SLOT(slotMoveSelected()));
    createAction("resize", SLOT(slotResizeSelected()));
    createAction("chord_mode", SLOT(slotUpdateInsertModeStatus()));
    createAction("toggle_step_by_step", SLOT(slotToggleStepByStep()));
    createAction("quantize", SLOT(slotTransformsQuantize()));
    createAction("repeat_quantize", SLOT(slotTransformsRepeatQuantize()));
    createAction("collapse_notes", SLOT(slotTransformsCollapseNotes()));
    createAction("legatoize", SLOT(slotTransformsLegato()));
    createAction("velocity_up", SLOT(slotVelocityUp()));
    createAction("velocity_down", SLOT(slotVelocityDown()));
    createAction("set_to_current_velocity", SLOT(slotSetVelocitiesToCurrent()));
    createAction("set_velocities", SLOT(slotSetVelocities()));
    createAction("trigger_segment", SLOT(slotTriggerSegment()));
    createAction("remove_trigger", SLOT(slotRemoveTriggers()));
    createAction("select_all", SLOT(slotSelectAll()));
    createAction("delete", SLOT(slotEditDelete()));
    createAction("cursor_back", SLOT(slotStepBackward()));
    createAction("cursor_forward", SLOT(slotStepForward()));
    createAction("cursor_back_bar", SLOT(slotJumpBackward()));
    createAction("cursor_forward_bar", SLOT(slotJumpForward()));
    createAction("extend_selection_backward", SLOT(slotExtendSelectionBackward()));
    createAction("extend_selection_forward", SLOT(slotExtendSelectionForward()));
    createAction("extend_selection_backward_bar", SLOT(slotExtendSelectionBackwardBar()));
    createAction("extend_selection_forward_bar", SLOT(slotExtendSelectionForwardBar()));
    createAction("cursor_start", SLOT(slotJumpToStart()));
    createAction("cursor_end", SLOT(slotJumpToEnd()));
    createAction("cursor_to_playback_pointer", SLOT(slotJumpCursorToPlayback()));
    //&&& NB Play has two shortcuts (Enter and Ctrl+Return) -- need to
    // ensure both get carried across somehow
    createAction("play", SIGNAL(play()));
    createAction("stop", SIGNAL(stop()));
    createAction("playback_pointer_back_bar", SIGNAL(rewindPlayback()));
    createAction("playback_pointer_forward_bar", SIGNAL(fastForwardPlayback()));
    createAction("playback_pointer_start", SIGNAL(rewindPlaybackToBeginning()));
    createAction("playback_pointer_end", SIGNAL(fastForwardPlaybackToEnd()));
    createAction("playback_pointer_to_cursor", SLOT(slotJumpPlaybackToCursor()));
    createAction("toggle_solo", SLOT(slotToggleSolo()));
    createAction("toggle_tracking", SLOT(slotToggleTracking()));
    createAction("panic", SIGNAL(panic()));
    createAction("preview_selection", SLOT(slotPreviewSelection()));
    createAction("clear_loop", SLOT(slotClearLoop()));
    createAction("clear_selection", SLOT(slotClearSelection()));
    createAction("filter_selection", SLOT(slotFilterSelection()));

    timeT crotchetDuration = Note(Note::Crotchet).getDuration();
    m_snapValues.clear();
    m_snapValues.push_back(SnapGrid::NoSnap);
    m_snapValues.push_back(SnapGrid::SnapToUnit);
    m_snapValues.push_back(crotchetDuration / 16);
    m_snapValues.push_back(crotchetDuration / 12);
    m_snapValues.push_back(crotchetDuration / 8);
    m_snapValues.push_back(crotchetDuration / 6);
    m_snapValues.push_back(crotchetDuration / 4);
    m_snapValues.push_back(crotchetDuration / 3);
    m_snapValues.push_back(crotchetDuration / 2);
    m_snapValues.push_back((crotchetDuration * 3) / 4);
    m_snapValues.push_back(crotchetDuration);
    m_snapValues.push_back((crotchetDuration * 3) / 2);
    m_snapValues.push_back(crotchetDuration * 2);
    m_snapValues.push_back(SnapGrid::SnapToBeat);
    m_snapValues.push_back(SnapGrid::SnapToBar);

    for (unsigned int i = 0; i < m_snapValues.size(); i++) {

        timeT d = m_snapValues[i];

        if (d == SnapGrid::NoSnap) {
            createAction("snap_none", SLOT(slotSetSnapFromAction()));
        } else if (d == SnapGrid::SnapToUnit) {
        } else if (d == SnapGrid::SnapToBeat) {
            createAction("snap_beat", SLOT(slotSetSnapFromAction()));
        } else if (d == SnapGrid::SnapToBar) {
            createAction("snap_bar", SLOT(slotSetSnapFromAction()));
        } else {
            QString actionName = QString("snap_%1").arg(int((crotchetDuration * 4) / d));
            if (d == (crotchetDuration * 3) / 4) actionName = "snap_dotted_8";
            if (d == (crotchetDuration * 3) / 2) actionName = "snap_dotted_4";
            createAction(actionName, SLOT(slotSetSnapFromAction()));
        }
    }

    createAction("show_inst_parameters", SLOT(slotDockParametersBack()));
    createAction("show_chords_ruler", SLOT(slotToggleChordsRuler()));
    createAction("show_tempo_ruler", SLOT(slotToggleTempoRuler()));

    createGUI(getRCFileName());

    if (getSegmentsOnlyRestsAndClefs()) {
        findAction("draw")->trigger();
    } else {
        findAction("select")->trigger();
    }
}

bool
MatrixView::isInChordMode()
{
    return findAction("chord_mode")->isChecked();
}

void MatrixView::slotDockParametersBack()
{
//     m_dockLeft->dockBack();		//&&& not required ?
}

void MatrixView::slotParametersClosed()
{
    enterActionState("parametersbox_closed");
    m_dockVisible = false;
}

void MatrixView::slotParametersDockedBack(QDockWidget* dw, int )//QDockWidget::DockPosition)
{
    if (dw == m_dockLeft) {
        leaveActionState("parametersbox_closed");
        m_dockVisible = true;
    }
}

void MatrixView::slotCheckTrackAssignments()
{
    Track *track =
        m_staffs[0]->getSegment().getComposition()->
        getTrackById(m_staffs[0]->getSegment().getTrack());

    Instrument *instr = getDocument()->getStudio().
                        getInstrumentById(track->getInstrument());

    m_parameterBox->useInstrument(instr);
}

void MatrixView::initStatusBar()
{
    QStatusBar* sb = statusBar();

    m_hoveredOverAbsoluteTime = new QLabel(sb);
    m_hoveredOverNoteName = new QLabel(sb);

    m_hoveredOverAbsoluteTime->setMinimumWidth(175);
    m_hoveredOverNoteName->setMinimumWidth(65);

    sb->addWidget(m_hoveredOverAbsoluteTime);
    sb->addWidget(m_hoveredOverNoteName);

    m_insertModeLabel = new QLabel(sb);
    m_insertModeLabel->setMinimumWidth(20);
    sb->addWidget(m_insertModeLabel);

	/*
    sb->addItem(KTmpStatusMsg::getDefaultMsg(),
                   KTmpStatusMsg::getDefaultId(), 1);
    sb->setItemAlignment(KTmpStatusMsg::getDefaultId(),
                         AlignLeft | AlignVCenter);
	*/
	
    m_selectionCounter = new QLabel(sb);
    sb->addWidget(m_selectionCounter);
}

void MatrixView::slotToolHelpChanged(const QString &s)
{
    QString msg = " " + s;
    if (m_toolContextHelp == msg) return;
    m_toolContextHelp = msg;

    QSettings settings;
    settings.beginGroup( GeneralOptionsConfigGroup );

    if (! qStrToBool( settings.value("toolcontexthelp", "true" ) ) ) {
        settings.endGroup();
        return;
    }
    settings.endGroup();

// 	if (m_mouseInCanvasView) statusBar()->changeItem(m_toolContextHelp, 1);
	if (m_mouseInCanvasView) statusBar()->showMessage(m_toolContextHelp);
}

void MatrixView::slotMouseEnteredCanvasView()
{
    QSettings settings;
    settings.beginGroup( GeneralOptionsConfigGroup );

    if (! qStrToBool( settings.value("toolcontexthelp", "true" ) ) ) {
        settings.endGroup();
        return;
    }
    settings.endGroup();

    m_mouseInCanvasView = true;
//     statusBar()->changeItem(m_toolContextHelp, 1);
	statusBar()->showMessage(m_toolContextHelp);
}

void MatrixView::slotMouseLeftCanvasView()
{
    m_mouseInCanvasView = false;
//     statusBar()->changeItem(KTmpStatusMsg::getDefaultMsg(), 1);
	statusBar()->showMessage( "default message" );	//### fix default message
}

bool MatrixView::applyLayout(int staffNo,
                             timeT startTime,
                             timeT endTime)
{
    Profiler profiler("MatrixView::applyLayout", true);

    m_hlayout.reset();
    m_vlayout.reset();

    for (unsigned int i = 0; i < m_staffs.size(); ++i) {

        if (staffNo >= 0 && (int)i != staffNo)
            continue;

        m_hlayout.scanStaff(*m_staffs[i], startTime, endTime);
        m_vlayout.scanStaff(*m_staffs[i], startTime, endTime);
    }

    m_hlayout.finishLayout();
    m_vlayout.finishLayout();

    if (m_staffs[0]->getSegment().getEndMarkerTime() != m_lastEndMarkerTime ||
            m_lastEndMarkerTime == 0 ||
            isCompositionModified()) {
        readjustCanvasSize();
        m_lastEndMarkerTime = m_staffs[0]->getSegment().getEndMarkerTime();
    }

    return true;
}

void MatrixView::refreshSegment(Segment *segment,
                                timeT startTime, timeT endTime)
{
    Profiler profiler("MatrixView::refreshSegment", true);

    MATRIX_DEBUG << "MatrixView::refreshSegment(" << startTime
    << ", " << endTime << ")\n";

    applyLayout( -1, startTime, endTime);

    if (!segment)
        segment = m_segments[0];

    if (endTime == 0)
        endTime = segment->getEndTime();
    else if (startTime == endTime) {
        startTime = segment->getStartTime();
        endTime = segment->getEndTime();
    }

    m_staffs[0]->positionElements(startTime, endTime);
    repaintRulers();
}

QSize MatrixView::getViewSize()
{
    return canvas()->size();
}

void MatrixView::setViewSize(QSize s)
{
    MATRIX_DEBUG << "MatrixView::setViewSize() w = " << s.width() << endl;

    canvas()->resize(getXbyInverseWorldMatrix(s.width()), s.height());
    getCanvasView()->resizeContents(s.width(), s.height());

    MATRIX_DEBUG << "MatrixView::setViewSize() contentsWidth = " << getCanvasView()->contentsWidth() << endl;
}

void MatrixView::repaintRulers()
{
    for (unsigned int i = 0; i != m_propertyViewRulers.size(); i++)
        m_propertyViewRulers[i].first->repaint();
}

void MatrixView::updateView()
{
    canvas()->update();
}

void MatrixView::setCurrentSelection(EventSelection* s, bool preview,
                                     bool redrawNow)
{
    //!!! rather too much here shared with notationview -- could much of
    // this be in editview?

    /// The assignment of event selection to the ruler needs to be first
    /// otherwise we get some unsafe thread usages that i had hard to solve.
    ///  Let's leave this alone as is for now...
    ControlRuler *ruler=EditView::getCurrentControlRuler();
    if(ruler)
	ruler->assignEventSelection(s);

	
    if (m_currentEventSelection == s)	{
        updateQuantizeCombo();
        return ;
    }

    if (m_currentEventSelection) {
        getStaff(0)->positionElements(m_currentEventSelection->getStartTime(),
                                      m_currentEventSelection->getEndTime());
    }

    EventSelection *oldSelection = m_currentEventSelection;
    m_currentEventSelection = s;

    timeT startA, endA, startB, endB;

    if (oldSelection) {
        startA = oldSelection->getStartTime();
        endA = oldSelection->getEndTime();
        startB = s ? s->getStartTime() : startA;
        endB = s ? s->getEndTime() : endA;
    } else {
        // we know they can't both be null -- first thing we tested above
        startA = startB = s->getStartTime();
        endA = endB = s->getEndTime();
    }

    // refreshSegment takes start==end to mean refresh everything
    if (startA == endA)
        ++endA;
    if (startB == endB)
        ++endB;

    bool updateRequired = true;

    if (s) {
         bool foundNewEvent = false;

        for (EventSelection::eventcontainer::iterator i =
                    s->getSegmentEvents().begin();
                i != s->getSegmentEvents().end(); ++i) {

            if (oldSelection && oldSelection->getSegment() == s->getSegment()
                    && oldSelection->contains(*i))
                continue;

            foundNewEvent = true;

            if (preview) {
                long pitch;
                if ((*i)->get<Int>(BaseProperties::PITCH, pitch)) {
                    long velocity = -1;
                    (void)((*i)->get<Int>(BaseProperties::VELOCITY, velocity));
                    if (!((*i)->has(BaseProperties::TIED_BACKWARD) &&
                          (*i)->get<Bool>(BaseProperties::TIED_BACKWARD)))
                        playNote(s->getSegment(), pitch, velocity);
                }
            }
        }

        if (!foundNewEvent) {
            if (oldSelection &&
                    oldSelection->getSegment() == s->getSegment() &&
                    oldSelection->getSegmentEvents().size() ==
                    s->getSegmentEvents().size())
                updateRequired = false;
        }
    }

    if (updateRequired) {

        if ((endA >= startB && endB >= startA) &&
                (!s || !oldSelection ||
                 oldSelection->getSegment() == s->getSegment())) {

            Segment &segment(s ? s->getSegment() :
                             oldSelection->getSegment());

            if (redrawNow) {
                // recolour the events now
                getStaff(segment)->positionElements(std::min(startA, startB),
                                                    std::max(endA, endB));
            } else {
                // mark refresh status and then request a repaint
                segment.getRefreshStatus
                (m_segmentsRefreshStatusIds
                 [getStaff(segment)->getId()]).
                push(std::min(startA, startB), std::max(endA, endB));
            }

        } else {
            // do two refreshes, one for each -- here we know neither is null

            if (redrawNow) {
                // recolour the events now
                getStaff(oldSelection->getSegment())->positionElements(startA,
                        endA);

                getStaff(s->getSegment())->positionElements(startB, endB);
            } else {
                // mark refresh status and then request a repaint

                oldSelection->getSegment().getRefreshStatus
                (m_segmentsRefreshStatusIds
                 [getStaff(oldSelection->getSegment())->getId()]).
                push(startA, endA);

                s->getSegment().getRefreshStatus
                (m_segmentsRefreshStatusIds
                 [getStaff(s->getSegment())->getId()]).
                push(startB, endB);
            }
        }
    }

    delete oldSelection;

    if (s) {

        int eventsSelected = s->getSegmentEvents().size();
        m_selectionCounter->setText
        (tr("  %n event(s) selected ", "", eventsSelected));

    } else {
        m_selectionCounter->setText(tr("  No selection "));
    }

    m_selectionCounter->update();

    slotSetCurrentVelocityFromSelection();

    // Clear states first, then enter only those ones that apply
    // (so as to avoid ever clearing one after entering another, in
    // case the two overlap at all)
    leaveActionState("have_selection");
    leaveActionState("have_notes_in_selection");
    leaveActionState("have_rests_in_selection");

    if (s) {
        enterActionState("have_selection");
        if (s->contains(Note::EventType)) {
            enterActionState("have_notes_in_selection");
        }
        if (s->contains(Note::EventRestType)) {
            enterActionState("have_rests_in_selection");
        }
    }

    updateQuantizeCombo();

    if (redrawNow)
        updateView();
    else
        update();
}

void MatrixView::updateQuantizeCombo()
{
    timeT unit = 0;

    if (m_currentEventSelection) {
        unit =
            BasicQuantizer::getStandardQuantization
            (m_currentEventSelection);
    } else {
        unit =
            BasicQuantizer::getStandardQuantization
            (&(m_staffs[0]->getSegment()));
    }

    for (unsigned int i = 0; i < m_quantizations.size(); ++i) {
        if (unit == m_quantizations[i]) {
            m_quantizeCombo->setCurrentIndex(i);
            return ;
        }
    }

	if( ! m_quantizeCombo ){
		MATRIX_DEBUG << "ERROR: m_quantizeCombo is NULL in MatrixView::updateQuantizeCombo() \n";
	}
    m_quantizeCombo->setCurrentIndex(m_quantizeCombo->count() - 1); // "Off"
}

void MatrixView::slotPaintSelected()
{
    EditTool* painter = m_toolBox->getTool(MatrixPainter::ToolName);

    setTool(painter);
}

void MatrixView::slotEraseSelected()
{
    EditTool* eraser = m_toolBox->getTool(MatrixEraser::ToolName);

    setTool(eraser);
}

void MatrixView::slotSelectSelected()
{
    EditTool* selector = m_toolBox->getTool(MatrixSelector::ToolName);

    connect(selector, SIGNAL(gotSelection()),
            this, SLOT(slotNewSelection()));

    connect(selector, SIGNAL(editTriggerSegment(int)),
            this, SIGNAL(editTriggerSegment(int)));

    setTool(selector);
}

void MatrixView::slotMoveSelected()
{
    EditTool* mover = m_toolBox->getTool(MatrixMover::ToolName);

    setTool(mover);
}

void MatrixView::slotResizeSelected()
{
    EditTool* resizer = m_toolBox->getTool(MatrixResizer::ToolName);

    setTool(resizer);
}

void MatrixView::slotVelocityChangeSelected()
{
    EditTool* velocity = m_toolBox->getTool(MatrixVelocity::ToolName);

    setTool(velocity);
}

void MatrixView::slotTransformsQuantize()
{
    if (!m_currentEventSelection)
        return ;

    QuantizeDialog dialog(this);

    if (dialog.exec() == QDialog::Accepted) {
        KTmpStatusMsg msg(tr("Quantizing..."), this);
        addCommandToHistory(new EventQuantizeCommand
                            (*m_currentEventSelection,
                             dialog.getQuantizer()));
    }
}

void MatrixView::slotTransformsRepeatQuantize()
{
    if (!m_currentEventSelection)
        return ;

    KTmpStatusMsg msg(tr("Quantizing..."), this);
    addCommandToHistory(new EventQuantizeCommand
                        (*m_currentEventSelection,
                         "Quantize Dialog Grid", false)); // no i18n (config group name)
}

void MatrixView::slotTransformsCollapseNotes()
{
    if (!m_currentEventSelection)
        return ;
    KTmpStatusMsg msg(tr("Collapsing notes..."), this);

    addCommandToHistory(new CollapseNotesCommand
                        (*m_currentEventSelection));
}

void MatrixView::slotTransformsLegato()
{
    if (!m_currentEventSelection)
        return ;

    KTmpStatusMsg msg(tr("Making legato..."), this);
    addCommandToHistory(new EventQuantizeCommand
                        (*m_currentEventSelection,
                         new LegatoQuantizer(0))); // no quantization
}

void MatrixView::slotMousePressed(timeT time, int pitch,
                                  QMouseEvent* e, MatrixElement* el)
{
    MATRIX_DEBUG << "MatrixView::mousePressed at pitch "
    << pitch << ", time " << time << endl;

    // Don't allow moving/insertion before the beginning of the
    // segment
    timeT curSegmentStartTime = getCurrentSegment()->getStartTime();
    if (curSegmentStartTime > time)
        time = curSegmentStartTime;

    m_tool->handleMousePress(time, pitch, 0, e, el);

    if (e->button() != Qt::RightButton) {
        getCanvasView()->startAutoScroll();
    }

    // play a preview
    //playPreview(pitch);
}

void MatrixView::slotMouseMoved(timeT time, int pitch, QMouseEvent* e)
{
    // Don't allow moving/insertion before the beginning of the
    // segment
    timeT curSegmentStartTime = getCurrentSegment()->getStartTime();
    if (curSegmentStartTime > time)
        time = curSegmentStartTime;

/*!!!
    if (activeItem()) {
        activeItem()->handleMouseMove(e);
        updateView();
    } else {
*/
        int follow = m_tool->handleMouseMove(time, pitch, e);
        getCanvasView()->setScrollDirectionConstraint(follow);

        //        if (follow != RosegardenCanvasView::NoFollow) {
        //            getCanvasView()->doAutoScroll();
        //        }

        // play a preview
        if (pitch != m_previousEvPitch) {
            //playPreview(pitch);
            m_previousEvPitch = pitch;
        }
//!!!    }

}

void MatrixView::slotMouseReleased(timeT time, int pitch, QMouseEvent* e)
{
    // Don't allow moving/insertion before the beginning of the
    // segment
    timeT curSegmentStartTime = getCurrentSegment()->getStartTime();
    if (curSegmentStartTime > time)
        time = curSegmentStartTime;
/*!!!
    if (activeItem()) {
        activeItem()->handleMouseRelease(e);
        setActiveItem(0);
        updateView();
    }
*/
    // send the real event time now (not adjusted for beginning of bar)
    m_tool->handleMouseRelease(time, pitch, e);
    m_previousEvPitch = 0;
    getCanvasView()->stopAutoScroll();
}

void
MatrixView::slotHoveredOverNoteChanged(int evPitch,
                                       bool haveEvent,
                                       timeT evTime)
{
    MidiPitchLabel label(evPitch);

    if (haveEvent) {

        m_haveHoveredOverNote = true;

        int bar, beat, fraction, remainder;
        getDocument()->getComposition().getMusicalTimeForAbsoluteTime
        (evTime, bar, beat, fraction, remainder);

        RealTime rt =
            getDocument()->getComposition().getElapsedRealTime(evTime);
        long ms = rt.msec();

        QString msg = tr("Note: %1 (%2.%3s)")
                   .arg(QString("%1-%2-%3-%4")
                       .arg(QString("%1").arg(bar + 1).rightJustify(3, '0'))
                       .arg(QString("%1").arg(beat).rightJustify(2, '0'))
                       .arg(QString("%1").arg(fraction).rightJustify(2, '0'))
                       .arg(QString("%1").arg(remainder).rightJustify(2, '0')))
                   .arg(rt.sec)
                   .arg(QString("%1").arg(ms).rightJustify(3, '0'));

        m_hoveredOverAbsoluteTime->setText(msg);
    }

    m_haveHoveredOverNote = false;

    m_hoveredOverNoteName->setText(tr("%1 (%2)")
                                    .arg(label.getQString())
                                    .arg(evPitch));

    m_pitchRuler->drawHoverNote(evPitch);
}

void
MatrixView::slotHoveredOverKeyChanged(unsigned int y)
{
    MatrixStaff& staff = *(m_staffs[0]);

    int evPitch = staff.getHeightAtCanvasCoords( -1, y);

    if (evPitch != m_previousEvPitch) {
        MidiPitchLabel label(evPitch);
        m_hoveredOverNoteName->setText(QString("%1 (%2)").
                                       arg(label.getQString()).arg(evPitch));
        m_previousEvPitch = evPitch;
    }
}

void
MatrixView::slotHoveredOverAbsoluteTimeChanged(unsigned int time)
{
    if (m_haveHoveredOverNote) return;

    timeT t = time;

    int bar, beat, fraction, remainder;
    getDocument()->getComposition().getMusicalTimeForAbsoluteTime
    (t, bar, beat, fraction, remainder);

    RealTime rt =
        getDocument()->getComposition().getElapsedRealTime(t);
    long ms = rt.msec();

    // At the advice of doc.trolltech.com/3.0/qstring.html#sprintf
    // we replaced this    QString format("%ld (%ld.%03lds)");
    // to support Unicode

    QString message = tr("Time: %1 (%2.%3s)")
                       .arg(QString("%1-%2-%3-%4")
                           .arg(QString("%1").arg(bar + 1).rightJustify(3, '0'))
                           .arg(QString("%1").arg(beat).rightJustify(2, '0'))
                           .arg(QString("%1").arg(fraction).rightJustify(2, '0'))
                           .arg(QString("%1").arg(remainder).rightJustify(2, '0')))
                       .arg(rt.sec)
                       .arg(QString("%1").arg(ms).rightJustify(3, '0'));

    m_hoveredOverAbsoluteTime->setText(message);
}

void
MatrixView::slotSetPointerPosition(timeT time)
{
    slotSetPointerPosition(time, m_playTracking);
}

void
MatrixView::slotSetPointerPosition(timeT time, bool scroll)
{
    Composition &comp = getDocument()->getComposition();
    int barNo = comp.getBarNumber(time);

    if (barNo >= m_hlayout.getLastVisibleBarOnStaff(*m_staffs[0])) {

        Segment &seg = m_staffs[0]->getSegment();

        if (seg.isRepeating() && time < seg.getRepeatEndTime()) {
            time =
                seg.getStartTime() +
                ((time - seg.getStartTime()) %
                 (seg.getEndMarkerTime() - seg.getStartTime()));
            m_staffs[0]->setPointerPosition(m_hlayout, time);
        } else {
            m_staffs[0]->hidePointer();
            scroll = false;
        }
    } else if (barNo < m_hlayout.getFirstVisibleBarOnStaff(*m_staffs[0])) {
        m_staffs[0]->hidePointer();
        scroll = false;
    } else {
        m_staffs[0]->setPointerPosition(m_hlayout, time);
    }

    if (scroll && !getCanvasView()->isAutoScrolling())
        getCanvasView()->slotScrollHoriz(static_cast<int>(getXbyWorldMatrix(m_hlayout.getXForTime(time))));

    updateView();
}

void
MatrixView::slotSetInsertCursorPosition(timeT time, bool scroll)
{
    //!!! For now.  Probably unlike slotSetPointerPosition this one
    // should snap to the nearest event or grid line.

    Segment &s = m_staffs[0]->getSegment();

    if (time < s.getStartTime())     time = s.getStartTime();
    if (time > s.getEndMarkerTime()) time = s.getEndMarkerTime();
    
    m_staffs[0]->setInsertCursorPosition(m_hlayout, time);

    if (scroll && !getCanvasView()->isAutoScrolling()) {
        getCanvasView()->slotScrollHoriz
            (static_cast<int>(getXbyWorldMatrix(m_hlayout.getXForTime(time))));
    }

    updateView();
}

void MatrixView::slotEditCut()
{
    MATRIX_DEBUG << "MatrixView::slotEditCut()\n";

    if (!m_currentEventSelection)
        return ;
    KTmpStatusMsg msg(tr("Cutting selection to clipboard..."), this);

    addCommandToHistory(new CutCommand(*m_currentEventSelection,
                                       getDocument()->getClipboard()));
}

void MatrixView::slotEditCopy()
{
    if (!m_currentEventSelection)
        return ;
    KTmpStatusMsg msg(tr("Copying selection to clipboard..."), this);

    addCommandToHistory(new CopyCommand(*m_currentEventSelection,
                                        getDocument()->getClipboard()));

    emit usedSelection();
}

void MatrixView::slotEditPaste()
{
    if (getDocument()->getClipboard()->isEmpty()) {
        slotStatusHelpMsg(tr("Clipboard is empty"));
        return ;
    }

    KTmpStatusMsg msg(tr("Inserting clipboard contents..."), this);

    PasteEventsCommand *command = new PasteEventsCommand
                                  (m_staffs[0]->getSegment(), getDocument()->getClipboard(),
                                   getInsertionTime(), PasteEventsCommand::MatrixOverlay);

    if (!command->isPossible()) {
        slotStatusHelpMsg(tr("Couldn't paste at this point"));
    } else {
        addCommandToHistory(command);
        setCurrentSelection(new EventSelection(command->getPastedEvents()));
    }
}

void MatrixView::slotEditDelete()
{
    if (!m_currentEventSelection)
        return ;
    KTmpStatusMsg msg(tr("Deleting selection..."), this);

    addCommandToHistory(new EraseCommand(*m_currentEventSelection));

    // clear and clear
    setCurrentSelection(0, false);
}

void MatrixView::slotKeyPressed(unsigned int y, bool repeating)
{
    slotHoveredOverKeyChanged(y);

    getCanvasView()->slotScrollVertSmallSteps(y);

    Composition &comp = getDocument()->getComposition();
    Studio &studio = getDocument()->getStudio();

    MatrixStaff& staff = *(m_staffs[0]);
    MidiByte evPitch = staff.getHeightAtCanvasCoords( -1, y);

    // Don't do anything if we're part of a run up the keyboard
    // and the pitch hasn't changed
    //
    if (m_lastNote == evPitch && repeating)
        return ;

    // Save value
    m_lastNote = evPitch;
    if (!repeating)
        m_firstNote = evPitch;

    Track *track = comp.getTrackById(
                       staff.getSegment().getTrack());

    Instrument *ins =
        studio.getInstrumentById(track->getInstrument());

    // check for null instrument
    //
    if (ins == 0)
        return ;

    MappedEvent mE(ins->getId(),
                   MappedEvent::MidiNote,
                   evPitch + staff.getSegment().getTranspose(),
                   MidiMaxValue,
                   RealTime::zeroTime,
                   RealTime::zeroTime,
                   RealTime::zeroTime);
    StudioControl::sendMappedEvent(mE);

}

void MatrixView::slotKeySelected(unsigned int y, bool repeating)
{
    slotHoveredOverKeyChanged(y);

    getCanvasView()->slotScrollVertSmallSteps(y);

    MatrixStaff& staff = *(m_staffs[0]);
    Segment &segment(staff.getSegment());
    MidiByte evPitch = staff.getHeightAtCanvasCoords( -1, y);

    // Don't do anything if we're part of a run up the keyboard
    // and the pitch hasn't changed
    //
    if (m_lastNote == evPitch && repeating)
        return ;

    // Save value
    m_lastNote = evPitch;
    if (!repeating)
        m_firstNote = evPitch;

    EventSelection *s = new EventSelection(segment);

    for (Segment::iterator i = segment.begin();
            segment.isBeforeEndMarker(i); ++i) {

        if ((*i)->isa(Note::EventType) &&
                (*i)->has(BaseProperties::PITCH)) {

            MidiByte p = (*i)->get
                         <Int>
                         (BaseProperties::PITCH);
            if (p >= std::min(m_firstNote, evPitch) &&
                    p <= std::max(m_firstNote, evPitch)) {
                s->addEvent(*i);
            }
        }
    }

    if (m_currentEventSelection) {
        // allow addFromSelection to deal with eliminating duplicates
        s->addFromSelection(m_currentEventSelection);
    }

    setCurrentSelection(s, false);

    // now play the note as well

    Composition &comp = getDocument()->getComposition();
    Studio &studio = getDocument()->getStudio();
    Track *track = comp.getTrackById(segment.getTrack());
    Instrument *ins =
        studio.getInstrumentById(track->getInstrument());

    // check for null instrument
    //
    if (ins == 0)
        return ;

    MappedEvent mE(ins->getId(),
                   MappedEvent::MidiNoteOneShot,
                   evPitch + segment.getTranspose(),
                   MidiMaxValue,
                   RealTime::zeroTime,
                   RealTime(0, 250000000),
                   RealTime::zeroTime);
    StudioControl::sendMappedEvent(mE);
}

void MatrixView::slotKeyReleased(unsigned int y, bool repeating)
{
    MatrixStaff& staff = *(m_staffs[0]);
    int evPitch = staff.getHeightAtCanvasCoords(-1, y);

    if (m_lastNote == evPitch && repeating)
        return;

    Rosegarden::Segment &segment(staff.getSegment());

    // send note off (note on at zero velocity)

    Rosegarden::Composition &comp = getDocument()->getComposition();
    Rosegarden::Studio &studio = getDocument()->getStudio();
    Rosegarden::Track *track = comp.getTrackById(segment.getTrack());
    Rosegarden::Instrument *ins =
        studio.getInstrumentById(track->getInstrument());

    // check for null instrument
    //
    if (ins == 0)
        return;

    evPitch = evPitch + segment.getTranspose();
    if (evPitch < 0 || evPitch > 127) return;

    Rosegarden::MappedEvent mE(ins->getId(),
                               Rosegarden::MappedEvent::MidiNote,
                               evPitch,
                               0,
                               Rosegarden::RealTime::zeroTime,
                               Rosegarden::RealTime::zeroTime,
                               Rosegarden::RealTime::zeroTime);
    Rosegarden::StudioControl::sendMappedEvent(mE);
}

void MatrixView::slotVerticalScrollPianoKeyboard(int y)
{
    if (m_pianoView){ // check that the piano view still exists (see dtor)
         m_pianoView->setContentsPos(0, y);
    }
}

void MatrixView::slotInsertNoteFromAction()
{
    const QObject *s = sender();
    QString name = s->objectName();

    Segment &segment = *getCurrentSegment();
    int pitch = 0;

    Accidental accidental =
        Accidentals::NoAccidental;

    timeT time(getInsertionTime());
    if (time >= segment.getEndMarkerTime()) {
        MATRIX_DEBUG << "WARNING: off end of segment" << endl;
        return ;
    }
    ::Rosegarden::Key key = segment.getKeyAtTime(time);
    Clef clef = segment.getClefAtTime(time);

    try {

        pitch = getPitchFromNoteInsertAction(name, accidental, clef, key);

    } catch (...) {

        /* was sorry */ QMessageBox::warning
        (this, "", tr("Unknown note insert action %1").arg(name) );
        return ;
    }

    KTmpStatusMsg msg(tr("Inserting note"), this);

    MATRIX_DEBUG << "Inserting note at pitch " << pitch << endl;

    Event modelEvent(Note::EventType, 0, 1);
    modelEvent.set<Int>(BaseProperties::PITCH, pitch);
    modelEvent.set<String>(BaseProperties::ACCIDENTAL, accidental);
    timeT endTime(time + m_snapGrid->getSnapTime(time));

    MatrixInsertionCommand* command =
        new MatrixInsertionCommand(segment, time, endTime, &modelEvent);

    addCommandToHistory(command);

    if (!isInChordMode()) {
        slotSetInsertCursorPosition(endTime);
    }
}

void MatrixView::closeWindow()
{
    delete this;
}

bool MatrixView::canPreviewAnotherNote()
{
    static time_t lastCutOff = 0;
    static int sinceLastCutOff = 0;

    time_t now = time(0);
    ++sinceLastCutOff;

    if ((now - lastCutOff) > 0) {
        sinceLastCutOff = 0;
        lastCutOff = now;
    } else {
        if (sinceLastCutOff >= 20) {
            // don't permit more than 20 notes per second, to avoid
            // gungeing up the sound drivers
            MATRIX_DEBUG << "Rejecting preview (too busy)" << endl;
            return false;
        }
    }

    return true;
}

void MatrixView::playNote(Event *event)
{
    // Only play note events
    //
    if (!event->isa(Note::EventType))
        return ;

    Composition &comp = getDocument()->getComposition();
    Studio &studio = getDocument()->getStudio();

    // Get the Instrument
    //
    Track *track = comp.getTrackById(
                       m_staffs[0]->getSegment().getTrack());

    Instrument *ins =
        studio.getInstrumentById(track->getInstrument());

    if (ins == 0)
        return ;

    if (!canPreviewAnotherNote())
        return ;

    // Get a velocity
    //
    MidiByte velocity = MidiMaxValue / 4; // be easy on the user's ears
    long eventVelocity = 0;
    if (event->get
            <Int>(BaseProperties::VELOCITY, eventVelocity))
        velocity = eventVelocity;

    RealTime duration =
        comp.getElapsedRealTime(event->getDuration());

    // create
    MappedEvent mE(ins->getId(),
                   MappedEvent::MidiNoteOneShot,
                   (MidiByte)
                   event->get
                   <Int>
                   (BaseProperties::PITCH) +
                   m_staffs[0]->getSegment().getTranspose(),
                   velocity,
                   RealTime::zeroTime,
                   duration,
                   RealTime::zeroTime);

    StudioControl::sendMappedEvent(mE);
}

void MatrixView::playNote(const Segment &segment, int pitch,
                          int velocity)
{
    Composition &comp = getDocument()->getComposition();
    Studio &studio = getDocument()->getStudio();

    Track *track = comp.getTrackById(segment.getTrack());

    Instrument *ins =
        studio.getInstrumentById(track->getInstrument());

    // check for null instrument
    //
    if (ins == 0)
        return ;

    if (velocity < 0)
        velocity = getCurrentVelocity();

    MappedEvent mE(ins->getId(),
                   MappedEvent::MidiNoteOneShot,
                   pitch + segment.getTranspose(),
                   velocity,
                   RealTime::zeroTime,
                   RealTime(0, 250000000),
                   RealTime::zeroTime);

    StudioControl::sendMappedEvent(mE);
}

MatrixStaff*
MatrixView::getStaff(const Segment &segment)
{
    for (unsigned int i = 0; i < m_staffs.size(); ++i) {
        if (&(m_staffs[i]->getSegment()) == &segment)
            return m_staffs[i];
    }

    return 0;
}

void
MatrixView::setSingleSelectedEvent(int staffNo, Event *event,
                                   bool preview, bool redrawNow)
{
    setSingleSelectedEvent(getStaff(staffNo)->getSegment(), event,
                           preview, redrawNow);
}

void
MatrixView::setSingleSelectedEvent(Segment &segment,
                                   Event *event,
                                   bool preview, bool redrawNow)
{
    setCurrentSelection(0, false);

    EventSelection *selection = new EventSelection(segment);
    selection->addEvent(event);

    //!!!
    // this used to say
    //   setCurrentSelection(selection, true)
    // since the default arg for preview is false, this changes the
    // default semantics -- test what circumstance this matters in
    // and choose an acceptable solution for both matrix & notation
    setCurrentSelection(selection, preview, redrawNow);
}

void
MatrixView::slotNewSelection()
{
    MATRIX_DEBUG << "MatrixView::slotNewSelection\n";

    //    m_parameterBox->setSelection(m_currentEventSelection);
}

void
MatrixView::slotSetSnapFromIndex(int s)
{
    slotSetSnap(m_snapValues[s]);
}

void
MatrixView::slotSetSnapFromAction()
{
    const QObject *s = sender();
    QString name = s->objectName();

    if (name.left(5) == "snap_") {
        int snap = name.right(name.length() - 5).toInt();
        if (snap > 0) {
            slotSetSnap(Note(Note::Semibreve).getDuration() / snap);
        } else if (name.left(12) == "snap_dotted_") {
            snap = name.right(name.length() - 12).toInt();
            slotSetSnap((3*Note(Note::Semibreve).getDuration()) / (2*snap));
        } else if (name == "snap_none") {
            slotSetSnap(SnapGrid::NoSnap);
        } else if (name == "snap_beat") {
            slotSetSnap(SnapGrid::SnapToBeat);
        } else if (name == "snap_bar") {
            slotSetSnap(SnapGrid::SnapToBar);
        } else if (name == "snap_unit") {
            slotSetSnap(SnapGrid::SnapToUnit);
        } else {
            MATRIX_DEBUG << "Warning: MatrixView::slotSetSnapFromAction: unrecognised action " << name << endl;
        }
    }
}

void
MatrixView::slotSetSnap(timeT t)
{
    MATRIX_DEBUG << "MatrixView::slotSetSnap: time is " << t << endl;
    m_snapGrid->setSnapTime(t);

    for (unsigned int i = 0; i < m_snapValues.size(); ++i) {
        if (m_snapValues[i] == t) {
            m_snapGridCombo->setCurrentIndex(i);
            break;
        }
    }

    for (unsigned int i = 0; i < m_staffs.size(); ++i)
        m_staffs[i]->sizeStaff(m_hlayout);

    m_segments[0]->setSnapGridSize(t);

    QSettings settings;
    settings.beginGroup( MatrixViewConfigGroup );

    settings.setValue("Snap Grid Size", static_cast<unsigned long long>(t));
    settings.endGroup();

    updateView();
}

void
MatrixView::slotQuantizeSelection(int q)
{
    MATRIX_DEBUG << "MatrixView::slotQuantizeSelection\n";

    timeT unit =
        ((unsigned int)q < m_quantizations.size() ? m_quantizations[q] : 0);

    Quantizer *quant =
        new BasicQuantizer
        (unit ? unit :
         Note(Note::Shortest).getDuration(), false);

    if (unit) {
        KTmpStatusMsg msg(tr("Quantizing..."), this);
        if (m_currentEventSelection &&
                m_currentEventSelection->getAddedEvents()) {
            addCommandToHistory(new EventQuantizeCommand
                                (*m_currentEventSelection, quant));
        } else {
            Segment &s = m_staffs[0]->getSegment();
            addCommandToHistory(new EventQuantizeCommand
                                (s, s.getStartTime(), s.getEndMarkerTime(),
                                 quant));
        }
    } else {
        KTmpStatusMsg msg(tr("Unquantizing..."), this);
        if (m_currentEventSelection &&
                m_currentEventSelection->getAddedEvents()) {
            addCommandToHistory(new EventUnquantizeCommand
                                (*m_currentEventSelection, quant));
        } else {
            Segment &s = m_staffs[0]->getSegment();
            addCommandToHistory(new EventUnquantizeCommand
                                (s, s.getStartTime(), s.getEndMarkerTime(),
                                 quant));
        }
    }
}

void
MatrixView::initActionsToolbar()
{
    MATRIX_DEBUG << "MatrixView::initActionsToolbar" << endl;

    QToolBar *actionsToolbar = findToolbar("Actions Toolbar");
//	QToolBar *actionsToolbar = m_actionsToolBar;
	//actionsToolbar->setLayout( new QHBoxLayout(actionsToolbar) );
	
    if (!actionsToolbar) {
        MATRIX_DEBUG << "MatrixView::initActionsToolbar - "
        << "tool bar not found" << endl;
        return ;
    }

    // The SnapGrid combo and Snap To... menu items
    //
    QLabel *sLabel = new QLabel(tr(" Grid: "), actionsToolbar, "kde toolbar widget");
    sLabel->setIndent(10);
	actionsToolbar->addWidget( sLabel );

    QPixmap noMap = NotePixmapFactory::makeToolbarPixmap("menu-no-note");

    m_snapGridCombo = new QComboBox(actionsToolbar);
	actionsToolbar->addWidget( m_snapGridCombo );

    for (unsigned int i = 0; i < m_snapValues.size(); i++) {

        timeT d = m_snapValues[i];

        if (d == SnapGrid::NoSnap) {
            m_snapGridCombo->addItem(tr("None"));
        } else if (d == SnapGrid::SnapToUnit) {
            m_snapGridCombo->addItem(tr("Unit"));
        } else if (d == SnapGrid::SnapToBeat) {
            m_snapGridCombo->addItem(tr("Beat"));
        } else if (d == SnapGrid::SnapToBar) {
            m_snapGridCombo->addItem(tr("Bar"));
        } else {
            timeT err = 0;
            QString label = NotationStrings::makeNoteMenuLabel(d, true, err);
            QPixmap pixmap = NotePixmapFactory::makeNoteMenuPixmap(d, err);
            m_snapGridCombo->addItem((err ? noMap : pixmap), label);
        }

        if (d == m_snapGrid->getSnapSetting()) {
            m_snapGridCombo->setCurrentIndex(m_snapGridCombo->count() - 1);
        }
    }

    connect(m_snapGridCombo, SIGNAL(activated(int)),
            this, SLOT(slotSetSnapFromIndex(int)));

    // Velocity combo.  Not a spin box, because the spin box is too
    // slow to use unless we make it typeable into, and then it takes
    // focus away from our more important widgets

    QLabel *vlabel = new QLabel(tr(" Velocity: "), actionsToolbar, "kde toolbar widget");
    vlabel->setIndent(10);
	actionsToolbar->addWidget( vlabel );
    
    m_velocityCombo = new QComboBox(actionsToolbar);
	actionsToolbar->addWidget( m_velocityCombo );
	
	for (int i = 0; i <= 127; ++i) {
        m_velocityCombo->addItem(QString("%1").arg(i));
    }
    m_velocityCombo->setCurrentIndex(100); //!!! associate with segment

    // Quantize combo
    //
    QLabel *qLabel = new QLabel(tr(" Quantize: "), actionsToolbar, "kde toolbar widget");
    qLabel->setIndent(10);
	actionsToolbar->addWidget( qLabel );

    m_quantizeCombo = new QComboBox(actionsToolbar);
	actionsToolbar->addWidget( m_quantizeCombo );

    for (unsigned int i = 0; i < m_quantizations.size(); ++i) {

        timeT time = m_quantizations[i];
        timeT error = 0;
        QString label = NotationStrings::makeNoteMenuLabel(time, true, error);
        QPixmap pmap = NotePixmapFactory::makeNoteMenuPixmap(time, error);
        m_quantizeCombo->addItem(error ? noMap : pmap, label);
    }

    m_quantizeCombo->addItem(noMap, tr("Off"));

    connect(m_quantizeCombo, SIGNAL(activated(int)),
            this, SLOT(slotQuantizeSelection(int)));
}


void
MatrixView::initZoomToolbar()
{
    MATRIX_DEBUG << "MatrixView::initZoomToolbar" << endl;

     QToolBar *zoomToolbar = findToolbar("Zoom Toolbar");
// 	QSlider *zoomToolbar = m_zoomToolBar;
//	QToolBar *zoomToolbar = m_zoomToolBar;

    if (!zoomToolbar) {
        MATRIX_DEBUG << "MatrixView::initZoomToolbar - "
        << "tool bar not found" << endl;
        return ;
    }

    std::vector<double> zoomSizes; // in units-per-pixel

    //double defaultBarWidth44 = 100.0;
    //double duration44 = TimeSignature(4,4).getBarDuration();

    static double factors[] = { 0.025, 0.05, 0.1, 0.2, 0.5,
                                1.0, 1.5, 2.5, 5.0, 10.0, 20.0 };
    // Zoom labels
    //
    for (unsigned int i = 0; i < sizeof(factors) / sizeof(factors[0]); ++i) {
//         zoomSizes.push_back(duration44 / (defaultBarWidth44 * factors[i]));

//         zoomSizes.push_back(factors[i] / 2); // GROSS HACK - see in matrixstaff.h - BREAKS MATRIX VIEW, see bug 1000595
        zoomSizes.push_back(factors[i]);
    }
	
	// ZoomSlider( sizes, default val, orientation, parent, name );
    m_hZoomSlider = new ZoomSlider<double>
                    (zoomSizes, -1, Qt::Horizontal, zoomToolbar, "kde toolbar widget");
    m_hZoomSlider->setTracking(true);
    m_hZoomSlider->setFocusPolicy(Qt::NoFocus);

    QLabel *label = new QLabel(tr("  Zoom:  "));
    zoomToolbar->addWidget(label);

    m_zoomLabel = new QLabel();
    m_zoomLabel->setIndent(10);
    m_zoomLabel->setFixedWidth(80);
    m_zoomLabel->setText(tr("%1%").arg(m_hZoomSlider->getCurrentSize()*100.0));

    connect(m_hZoomSlider,
            SIGNAL(valueChanged(int)),
            SLOT(slotChangeHorizontalZoom(int)));

    zoomToolbar->addWidget(m_hZoomSlider);
    zoomToolbar->addWidget(m_zoomLabel);
}

void
MatrixView::slotChangeHorizontalZoom(int)
{
    double zoomValue = m_hZoomSlider->getCurrentSize();

    //     m_zoomLabel->setText(tr("%1%").arg(zoomValue*100.0 * 2)); // GROSS HACK - see in matrixstaff.h - BREAKS MATRIX VIEW, see bug 1000595
    m_zoomLabel->setText(tr("%1%").arg(zoomValue*100.0));

    MATRIX_DEBUG << "MatrixView::slotChangeHorizontalZoom() : zoom factor = "
    << zoomValue << endl;

    m_referenceRuler->setHScaleFactor(zoomValue);
    
    if (m_tempoRuler)
        m_tempoRuler->repaint();
    if (m_chordNameRuler)
        m_chordNameRuler->repaint();

    // Set zoom matrix
    //
    QMatrix zoomMatrix;
    zoomMatrix.scale(zoomValue, 1.0);
    m_canvasView->setWorldMatrix(zoomMatrix);

    // make control rulers zoom too
    //
    setControlRulersZoom(zoomMatrix);

    if (m_topStandardRuler)
        m_topStandardRuler->setHScaleFactor(zoomValue);
    if (m_bottomStandardRuler)
        m_bottomStandardRuler->setHScaleFactor(zoomValue);

    for (unsigned int i = 0; i < m_propertyViewRulers.size(); ++i) {
        m_propertyViewRulers[i].first->setHScaleFactor(zoomValue);
        m_propertyViewRulers[i].first->repaint();
    }

    if (m_topStandardRuler)
        m_topStandardRuler->update();
    if (m_bottomStandardRuler)
        m_bottomStandardRuler->update();

    QSettings settings;
    settings.beginGroup( MatrixViewConfigGroup );

    settings.setValue("Zoom Level", zoomValue);
    settings.endGroup();

    // If you do adjust the viewsize then please remember to
    // either re-center() or remember old scrollbar position
    // and restore.
    //

    int newWidth = computePostLayoutWidth();

    //     int newWidth = int(getXbyWorldMatrix(getCanvasView()->canvas()->width()));

    // We DO NOT resize the canvas(), only the area it's displaying on
    //
    getCanvasView()->resizeContents(newWidth, getViewSize().height());

    // This forces a refresh of the h. scrollbar, even if the canvas width
    // hasn't changed
    //
    getCanvasView()->polish();

    getCanvasView()->slotScrollHoriz
        (getXbyWorldMatrix(m_staffs[0]->getLayoutXOfInsertCursor()));
}

void
MatrixView::slotZoomIn()
{
    m_hZoomSlider->increment();
}

void
MatrixView::slotZoomOut()
{
    m_hZoomSlider->decrement();
}

void
MatrixView::scrollToTime(timeT t)
{
    double layoutCoord = m_hlayout.getXForTime(t);
    getCanvasView()->slotScrollHoriz(int(layoutCoord));
}

int
MatrixView::getCurrentVelocity() const
{
    return m_velocityCombo->currentIndex();
}

void
MatrixView::slotSetCurrentVelocity(int value)
{
    m_velocityCombo->setCurrentIndex(value);
}


void
MatrixView::slotSetCurrentVelocityFromSelection()
{
    if (!m_currentEventSelection) return;

    float totalVelocity = 0;
    int count = 0;

    for (EventSelection::eventcontainer::iterator i =
             m_currentEventSelection->getSegmentEvents().begin();
         i != m_currentEventSelection->getSegmentEvents().end(); ++i) {

        if ((*i)->has(BaseProperties::VELOCITY)) {
            totalVelocity += (*i)->get<Int>(BaseProperties::VELOCITY);
            ++count;
        }
    }

    if (count > 0) {
        slotSetCurrentVelocity((totalVelocity / count) + 0.5);
    }
}

unsigned int
MatrixView::addPropertyViewRuler(const PropertyName &property)
{
    // Try and find this controller if it exists
    //
    for (unsigned int i = 0; i != m_propertyViewRulers.size(); i++) {
        if (m_propertyViewRulers[i].first->getPropertyName() == property)
            return i;
    }

    int height = 20;

    PropertyViewRuler *newRuler = new PropertyViewRuler(&m_hlayout,
                                  m_segments[0],
                                  property,
                                  xorigin,
                                  height,
                                  getCentralWidget());

    addRuler(newRuler);

    PropertyBox *newControl = new PropertyBox(strtoqstr(property),
                              m_parameterBox->width() + m_pitchRuler->width(),
                              height,
                              getCentralWidget());

    addPropertyBox(newControl);

    m_propertyViewRulers.push_back(
        std::pair<PropertyViewRuler*, PropertyBox*>(newRuler, newControl));

    return m_propertyViewRulers.size() - 1;
}

bool
MatrixView::removePropertyViewRuler(unsigned int number)
{
    if (number > m_propertyViewRulers.size() - 1)
        return false;

    std::vector<std::pair<PropertyViewRuler*, PropertyBox*> >::iterator it
    = m_propertyViewRulers.begin();
    while (number--)
        it++;

    delete it->first;
    delete it->second;
    m_propertyViewRulers.erase(it);

    return true;
}

RulerScale*
MatrixView::getHLayout()
{
    return &m_hlayout;
}

Staff*
MatrixView::getCurrentStaff()
{
    return getStaff(0);
}

Segment *
MatrixView::getCurrentSegment()
{
    MatrixStaff *staff = getStaff(0);
    return (staff ? &staff->getSegment() : 0);
}

timeT
MatrixView::getInsertionTime()
{
    MatrixStaff *staff = m_staffs[0];
    return staff->getInsertCursorTime(m_hlayout);
}

void
MatrixView::slotStepBackward()
{
    timeT time(getInsertionTime());
    slotSetInsertCursorPosition(SnapGrid(&m_hlayout).snapTime
                                (time - 1,
                                 SnapGrid::SnapLeft));
}

void
MatrixView::slotStepForward()
{
    timeT time(getInsertionTime());
    slotSetInsertCursorPosition(SnapGrid(&m_hlayout).snapTime
                                (time + 1,
                                 SnapGrid::SnapRight));
}

void
MatrixView::slotJumpCursorToPlayback()
{
    slotSetInsertCursorPosition(getDocument()->getComposition().getPosition());
}

void
MatrixView::slotJumpPlaybackToCursor()
{
    emit jumpPlaybackTo(getInsertionTime());
}

void
MatrixView::slotToggleTracking()
{
    m_playTracking = !m_playTracking;
}

void
MatrixView::slotSelectAll()
{
    Segment *segment = m_segments[0];
    Segment::iterator it = segment->begin();
    EventSelection *selection = new EventSelection(*segment);

    for (; segment->isBeforeEndMarker(it); it++)
        if ((*it)->isa(Note::EventType))
            selection->addEvent(*it);

    setCurrentSelection(selection, false);
}

void MatrixView::slotPreviewSelection()
{
    if (!m_currentEventSelection)
        return ;

    getDocument()->slotSetLoop(m_currentEventSelection->getStartTime(),
                               m_currentEventSelection->getEndTime());
}

void MatrixView::slotClearLoop()
{
    getDocument()->slotSetLoop(0, 0);
}

void MatrixView::slotClearSelection()
{
    // Actually we don't clear the selection immediately: if we're
    // using some tool other than the select tool, then the first
    // press switches us back to the select tool.

    MatrixSelector *selector = dynamic_cast<MatrixSelector *>(m_tool);

    if (!selector) {
        slotSelectSelected();
    } else {
        setCurrentSelection(0);
    }
}

void MatrixView::slotFilterSelection()
{
    RG_DEBUG << "MatrixView::slotFilterSelection" << endl;

    Segment *segment = getCurrentSegment();
    EventSelection *existingSelection = m_currentEventSelection;
    if (!segment || !existingSelection)
        return ;

    EventFilterDialog dialog(this);
    if (dialog.exec() == QDialog::Accepted) {
        RG_DEBUG << "slotFilterSelection- accepted" << endl;

        bool haveEvent = false;

        EventSelection *newSelection = new EventSelection(*segment);
        EventSelection::eventcontainer &ec =
            existingSelection->getSegmentEvents();
        for (EventSelection::eventcontainer::iterator i =
                    ec.begin(); i != ec.end(); ++i) {
            if (dialog.keepEvent(*i)) {
                haveEvent = true;
                newSelection->addEvent(*i);
            }
        }

        if (haveEvent)
            setCurrentSelection(newSelection);
        else
            setCurrentSelection(0);
    }
}

void
MatrixView::readjustCanvasSize()
{
    int maxHeight = 0;

    for (unsigned int i = 0; i < m_staffs.size(); ++i) {

        MatrixStaff &staff = *m_staffs[i];

        staff.sizeStaff(m_hlayout);

        //         if (staff.getTotalWidth() + staff.getX() > maxWidth) {
        //             maxWidth = staff.getTotalWidth() + staff.getX() + 1;
        //         }

        if (staff.getTotalHeight() + staff.getY() > maxHeight) {
            if (isDrumMode()) {
                maxHeight = staff.getTotalHeight() + staff.getY() + 5;
            } else {
                maxHeight = staff.getTotalHeight() + staff.getY() + 1;
            }
        }

    }

    int newWidth = computePostLayoutWidth();

    // now get the EditView to do the biz
    readjustViewSize(QSize(newWidth, maxHeight), true);

    repaintRulers();
}

void MatrixView::slotVelocityUp()
{
    if (!m_currentEventSelection)
        return ;
    KTmpStatusMsg msg(tr("Raising velocities..."), this);

    addCommandToHistory
    (new ChangeVelocityCommand(10, *m_currentEventSelection));

    slotSetCurrentVelocityFromSelection();
}

void MatrixView::slotVelocityDown()
{
    if (!m_currentEventSelection)
        return ;
    KTmpStatusMsg msg(tr("Lowering velocities..."), this);

    addCommandToHistory
    (new ChangeVelocityCommand( -10, *m_currentEventSelection));

    slotSetCurrentVelocityFromSelection();
}

void
MatrixView::slotSetVelocities()
{
    if (!m_currentEventSelection)
        return ;

    EventParameterDialog dialog(this,
                                tr("Set Event Velocities"),
                                BaseProperties::VELOCITY,
                                getCurrentVelocity());

    if (dialog.exec() == QDialog::Accepted) {
        KTmpStatusMsg msg(tr("Setting Velocities..."), this);
        addCommandToHistory(new SelectionPropertyCommand
                            (m_currentEventSelection,
                             BaseProperties::VELOCITY,
                             dialog.getPattern(),
                             dialog.getValue1(),
                             dialog.getValue2()));
    }
}

void
MatrixView::slotSetVelocitiesToCurrent()
{
    if (!m_currentEventSelection) return;

    addCommandToHistory(new SelectionPropertyCommand
                        (m_currentEventSelection,
                         BaseProperties::VELOCITY,
                         FlatPattern,
                         getCurrentVelocity(),
                         getCurrentVelocity()));
}

void
MatrixView::slotTriggerSegment()
{
    if (!m_currentEventSelection)
        return ;

    TriggerSegmentDialog dialog(this, &getDocument()->getComposition());
    if (dialog.exec() != QDialog::Accepted)
        return ;

    addCommandToHistory(new SetTriggerCommand(*m_currentEventSelection,
                        dialog.getId(),
                        true,
                        dialog.getRetune(),
                        dialog.getTimeAdjust(),
                        Marks::NoMark,
                        tr("Trigger Segment")));
}

void
MatrixView::slotRemoveTriggers()
{
    if (!m_currentEventSelection)
        return ;

    addCommandToHistory(new ClearTriggersCommand(*m_currentEventSelection,
                        tr("Remove Triggers")));
}

void
MatrixView::slotToggleChordsRuler()
{
    toggleWidget(m_chordNameRuler, "show_chords_ruler");
}

void
MatrixView::slotToggleTempoRuler()
{
    toggleWidget(m_tempoRuler, "show_tempo_ruler");
}

void
MatrixView::paintEvent(QPaintEvent* e)
{
    //!!! There's a lot of code shared between matrix and notation for
    // dealing with step recording (the insertable note event stuff).
    // It should probably be factored out into a base class, but I'm
    // not sure I wouldn't rather wait until the functionality is all
    // sorted in both matrix and notation so we can be sure how much
    // of it is actually common.

    EditView::paintEvent(e);

    // now deal with any backlog of insertable notes that appeared
    // during paint (because it's not safe to modify a segment from
    // within a sub-event-loop in a processEvents call from a paint)
    if (!m_pendingInsertableNotes.empty()) {
        std::vector<std::pair<int, int> > notes = m_pendingInsertableNotes;
        m_pendingInsertableNotes.clear();
        for (unsigned int i = 0; i < notes.size(); ++i) {
            slotInsertableNoteEventReceived(notes[i].first, notes[i].second, true);
        }
    }
}

void
MatrixView::updateViewCaption()
{
    // Set client label
    //
    QString view = tr("Matrix");
    if (isDrumMode())
        view = tr("Percussion");

    if (m_segments.size() == 1) {

        TrackId trackId = m_segments[0]->getTrack();
        Track *track =
            m_segments[0]->getComposition()->getTrackById(trackId);

        int trackPosition = -1;
        if (track)
            trackPosition = track->getPosition();

        setWindowTitle(tr("%1 - Segment Track #%2 - %3")
                    .arg(getDocument()->getTitle())
                    .arg(trackPosition + 1)
                    .arg(view));

    } else if (m_segments.size() == getDocument()->getComposition().getNbSegments()) {

        setWindowTitle(tr("%1 - All Segments - %2")
                    .arg(getDocument()->getTitle())
                    .arg(view));

    } else {

        setWindowTitle(tr("%1 - %n Segment(s) - %2", "",
                        m_segments.size())
                    .arg(getDocument()->getTitle())
                    .arg(view));
    }
}

int MatrixView::computePostLayoutWidth()
{
    Segment *segment = m_segments[0];
    Composition *composition = segment->getComposition();
    int endX = int(m_hlayout.getXForTime
                   (composition->getBarEndForTime
                    (segment->getEndMarkerTime())));
    int startX = int(m_hlayout.getXForTime
                     (composition->getBarStartForTime
                      (segment->getStartTime())));

    int newWidth = int(getXbyWorldMatrix(endX - startX));

    MATRIX_DEBUG << "MatrixView::readjustCanvasSize() : startX = "
    << startX
    << " endX = " << endX
    << " newWidth = " << newWidth
    << " endmarkertime : " << segment->getEndMarkerTime()
    << " barEnd for time : " << composition->getBarEndForTime(segment->getEndMarkerTime())
    << endl;

    newWidth += 12;
    if (isDrumMode())
        newWidth += 12;

    return newWidth;
}

bool MatrixView::getMinMaxPitches(int& minPitch, int& maxPitch)
{
    minPitch = MatrixVLayout::maxMIDIPitch + 1;
    maxPitch = MatrixVLayout::minMIDIPitch - 1;

    std::vector<MatrixStaff*>::iterator sit;
    for (sit = m_staffs.begin(); sit != m_staffs.end(); ++sit) {

        MatrixElementList *mel = (*sit)->getViewElementList();
        MatrixElementList::iterator eit;
        for (eit = mel->begin(); eit != mel->end(); ++eit) {

            NotationElement *el = static_cast<NotationElement*>(*eit);
            if (el->isNote()) {
                Event* ev = el->event();
                int pitch = ev->get
                            <Int>
                            (BaseProperties::PITCH);
                if (minPitch > pitch)
                    minPitch = pitch;
                if (maxPitch < pitch)
                    maxPitch = pitch;
            }
        }
    }

    return maxPitch >= minPitch;
}

void MatrixView::extendKeyMapping()
{
    int minStaffPitch, maxStaffPitch;
    if (getMinMaxPitches(minStaffPitch, maxStaffPitch)) {
        int minKMPitch = m_localMapping->getPitchForOffset(0);
        int maxKMPitch = m_localMapping->getPitchForOffset(0)
                         + m_localMapping->getPitchExtent() - 1;
        if (minStaffPitch < minKMPitch)
            m_localMapping->getMap()[minStaffPitch] = std::string("");
        if (maxStaffPitch > maxKMPitch)
            m_localMapping->getMap()[maxStaffPitch] = std::string("");
    }
}

void
MatrixView::slotInsertableNoteEventReceived(int pitch, int velocity, bool noteOn)
{
    // hjj:
    // The default insertion mode is implemented equivalently in
    // notationviewslots.cpp:
    //  - proceed if notes do not overlap
    //  - make the chord if notes do overlap, and do not proceed

    static int numberOfNotesOn = 0;
    static time_t lastInsertionTime = 0;
    if (!noteOn) {
        numberOfNotesOn--;
        return ;
    }

    QAction *action = findAction("toggle_step_by_step");

    if (!action) {
        MATRIX_DEBUG << "WARNING: No toggle_step_by_step action" << endl;
        return ;
    }
    if (!action->isChecked())
        return ;

    if (m_inPaintEvent) {
        m_pendingInsertableNotes.push_back(std::pair<int, int>(pitch, velocity));
        return ;
    }

    Segment &segment = *getCurrentSegment();

    // If the segment is transposed, we want to take that into
    // account.  But the note has already been played back to the user
    // at its untransposed pitch, because that's done by the MIDI THRU
    // code in the sequencer which has no way to know whether a note
    // was intended for step recording.  So rather than adjust the
    // pitch for playback according to the transpose setting, we have
    // to adjust the stored pitch in the opposite direction.

    pitch -= segment.getTranspose();

    KTmpStatusMsg msg(tr("Inserting note"), this);

    MATRIX_DEBUG << "Inserting note at pitch " << pitch << endl;

    Event modelEvent(Note::EventType, 0, 1);
    modelEvent.set<Int>(BaseProperties::PITCH, pitch);
    static timeT insertionTime(getInsertionTime());
    if (insertionTime >= segment.getEndMarkerTime()) {
        MATRIX_DEBUG << "WARNING: off end of segment" << endl;
        return ;
    }
    time_t now;
    time (&now);
    double elapsed = difftime(now, lastInsertionTime);
    time (&lastInsertionTime);

    if (numberOfNotesOn <= 0 || elapsed > 10.0 ) {
        numberOfNotesOn = 0;
        insertionTime = getInsertionTime();
    }
    numberOfNotesOn++;
    timeT endTime(insertionTime + m_snapGrid->getSnapTime(insertionTime));

    if (endTime <= insertionTime) {
        static bool showingError = false;
        if (showingError)
            return ;
        showingError = true;
        /* was sorry */ QMessageBox::warning(this, "", tr("Can't insert note: No grid duration selected"));
        showingError = false;
        return ;
    }

    MatrixInsertionCommand* command =
        new MatrixInsertionCommand(segment, insertionTime, endTime, &modelEvent);

    addCommandToHistory(command);

    if (!isInChordMode()) {
        slotSetInsertCursorPosition(endTime);
    }
}

void
MatrixView::slotInsertableNoteOnReceived(int pitch, int velocity)
{
    MATRIX_DEBUG << "MatrixView::slotInsertableNoteOnReceived: " << pitch << endl;
    slotInsertableNoteEventReceived(pitch, velocity, true);
}

void
MatrixView::slotInsertableNoteOffReceived(int pitch, int velocity)
{
    MATRIX_DEBUG << "MatrixView::slotInsertableNoteOffReceived: " << pitch << endl;
    slotInsertableNoteEventReceived(pitch, velocity, false);
}

void
MatrixView::slotToggleStepByStep()
{
    QAction *action = findAction("toggle_step_by_step");
        
    if (!action) {
        MATRIX_DEBUG << "WARNING: No toggle_step_by_step action" << endl;
        return ;
    }
    if (action->isChecked()) { // after toggling, that is
        emit stepByStepTargetRequested(this);
    } else {
        emit stepByStepTargetRequested(0);
    }
}

void
MatrixView::slotUpdateInsertModeStatus()
{
    QString message;
    if (isInChordMode()) {
        message = tr(" Chord ");
    } else {
        message = "";
    }
    m_insertModeLabel->setText(message);
}

void
MatrixView::slotStepByStepTargetRequested(QObject *obj)
{
    QAction *action = findAction("toggle_step_by_step");
	
    if (!action) {
        MATRIX_DEBUG << "WARNING: No toggle_step_by_step action" << endl;
        return ;
    }
    action->setChecked(obj == this);
}

void
MatrixView::slotInstrumentLevelsChanged(InstrumentId id,
                                        const LevelInfo &info)
{
    if (!m_parameterBox)
        return ;

    Composition &comp = getDocument()->getComposition();

    Track *track =
        comp.getTrackById(m_staffs[0]->getSegment().getTrack());
    if (!track || track->getInstrument() != id)
        return ;

    Instrument *instr = getDocument()->getStudio().
                        getInstrumentById(track->getInstrument());
    if (!instr || instr->getType() != Instrument::SoftSynth)
        return ;

    float dBleft = AudioLevel::fader_to_dB
                   (info.level, 127, AudioLevel::LongFader);
    float dBright = AudioLevel::fader_to_dB
                    (info.levelRight, 127, AudioLevel::LongFader);

    m_parameterBox->setAudioMeter(dBleft, dBright,
                                  AudioLevel::DB_FLOOR,
                                  AudioLevel::DB_FLOOR);
}

void
MatrixView::slotPercussionSetChanged(Instrument * newInstr)
{
    // Must be called only when in drum mode
    assert(m_drumMode);

    int resolution = 8;
    if (newInstr && newInstr->getKeyMapping()) {
        resolution = 11;
    }

    const MidiKeyMapping *mapping = 0;
    if (newInstr) {
        mapping = newInstr->getKeyMapping();
    }

    // Construct a local new keymapping :
    if (m_localMapping)
        delete m_localMapping;
    if (mapping) {
        m_localMapping = new MidiKeyMapping(*mapping);
        extendKeyMapping();
    } else {
        m_localMapping = 0;
    }

    m_staffs[0]->setResolution(resolution);

    delete m_pitchRuler;

    QWidget *vport = m_pianoView->viewport();

    // Create a new pitchruler widget
    PitchRuler *pitchRuler;
    if (newInstr && newInstr->getKeyMapping() &&
            !newInstr->getKeyMapping()->getMap().empty()) {
        pitchRuler = new PercussionPitchRuler(vport,
                                              m_localMapping,
                                              resolution); // line spacing
    } else {
        pitchRuler = new PianoKeyboard(vport);
    }


    QObject::connect
    (pitchRuler, SIGNAL(hoveredOverKeyChanged(unsigned int)),
     this, SLOT (slotHoveredOverKeyChanged(unsigned int)));

    QObject::connect
    (pitchRuler, SIGNAL(keyPressed(unsigned int, bool)),
     this, SLOT (slotKeyPressed(unsigned int, bool)));

    QObject::connect
    (pitchRuler, SIGNAL(keySelected(unsigned int, bool)),
     this, SLOT (slotKeySelected(unsigned int, bool)));

    QObject::connect
    (pitchRuler, SIGNAL(keyReleased(unsigned int, bool)),
     this, SLOT (slotKeyReleased(unsigned int, bool)));

    // Replace the old pitchruler widget
    m_pitchRuler = pitchRuler;
 	m_pianoView->addChild(m_pitchRuler);
//	m_pianoView->layout()->addWidget(m_pitchRuler);
	m_pitchRuler->show();
    m_pianoView->setFixedWidth(pitchRuler->sizeHint().width());

    // Update matrix canvas
    readjustCanvasSize();
    bool layoutApplied = applyLayout();
    if (!layoutApplied)
        /* was sorry */ QMessageBox::warning(0, "", tr("Couldn't apply piano roll layout"));
    else {
        MATRIX_DEBUG << "MatrixView : rendering elements\n";
        m_staffs[0]->positionAllElements();
        m_staffs[0]->getSegment().getRefreshStatus
        (m_segmentsRefreshStatusIds[0]).setNeedsRefresh(false);
        update();
    }
}

void
MatrixView::slotCanvasBottomWidgetHeightChanged(int newHeight)
{
	int newH;
	newH = newHeight + m_canvasView->horizontalScrollBar()->height();
     m_pianoView->setBottomMargin( newH );
//	m_pianoView->setContentsMargins ( 0, 0, 0, newH );
}

MatrixCanvasView* MatrixView::getCanvasView()
{
    return dynamic_cast<MatrixCanvasView *>(m_canvasView);
}

}
#include "MatrixView.moc"
#endif
