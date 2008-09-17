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


#include <Q3Canvas>
#include <Q3CanvasItem>
#include <Q3CanvasPixmap>
#include <Q3CanvasRectangle>
#include <Q3CanvasText>
#include "NotationView.h"
#include <list>
#include <QLayout>
#include "misc/Debug.h"
#include <QApplication>

#include "gui/editors/segment/TrackEditor.h"
#include "gui/editors/segment/TrackButtons.h"
#include "gui/editors/parameters/TrackParameterBox.h"
#include "gui/general/IconLoader.h"
#include "base/BaseProperties.h"
#include <klocale.h>
#include <kstandarddirs.h>
#include "misc/Strings.h"
#include "base/AnalysisTypes.h"
#include "base/Clipboard.h"
#include "base/Composition.h"
#include "base/CompositionTimeSliceAdapter.h"
#include "base/Configuration.h"
#include "base/Device.h"
#include "base/Event.h"
#include "base/Exception.h"
#include "base/Instrument.h"
#include "base/MidiDevice.h"
#include "base/MidiTypes.h"
#include "base/NotationTypes.h"
#include "base/Profiler.h"
#include "base/PropertyName.h"
#include "base/NotationQuantizer.h"
#include "base/RealTime.h"
#include "base/RulerScale.h"
#include "base/Segment.h"
#include "base/Selection.h"
#include "base/Staff.h"
#include "base/Studio.h"
#include "base/Track.h"
#include "ClefInserter.h"
#include "commands/edit/AddDotCommand.h"
#include "commands/edit/ChangeVelocityCommand.h"
#include "commands/edit/ClearTriggersCommand.h"
#include "commands/edit/CollapseNotesCommand.h"
#include "commands/edit/CopyCommand.h"
#include "commands/edit/CutAndCloseCommand.h"
#include "commands/edit/CutCommand.h"
#include "commands/edit/EraseCommand.h"
#include "commands/edit/EventEditCommand.h"
#include "commands/edit/EventQuantizeCommand.h"
#include "commands/edit/InsertTriggerNoteCommand.h"
#include "commands/edit/PasteEventsCommand.h"
#include "commands/edit/SetLyricsCommand.h"
#include "commands/edit/SetNoteTypeCommand.h"
#include "commands/edit/SetTriggerCommand.h"
#include "commands/edit/TransposeCommand.h"
#include "commands/notation/ClefInsertionCommand.h"
#include "commands/notation/EraseEventCommand.h"
#include "commands/notation/InterpretCommand.h"
#include "commands/notation/KeyInsertionCommand.h"
#include "commands/notation/MultiKeyInsertionCommand.h"
#include "commands/notation/NormalizeRestsCommand.h"
#include "commands/notation/SustainInsertionCommand.h"
#include "commands/notation/TextInsertionCommand.h"
#include "commands/notation/TupletCommand.h"
#include "commands/segment/PasteToTriggerSegmentCommand.h"
#include "commands/segment/SegmentSyncCommand.h"
#include "commands/segment/SegmentTransposeCommand.h"
#include "commands/segment/RenameTrackCommand.h"
#include "document/RosegardenGUIDoc.h"
#include "document/ConfigGroups.h"
#include "document/io/LilyPondExporter.h"
#include "GuitarChordInserter.h"
#include "gui/application/SetWaitCursor.h"
#include "gui/application/RosegardenGUIView.h"
#include "gui/application/RosegardenGUIApp.h"
#include "gui/dialogs/ClefDialog.h"
#include "gui/dialogs/EventEditDialog.h"
#include "gui/dialogs/EventParameterDialog.h"
#include "gui/dialogs/InterpretDialog.h"
#include "gui/dialogs/IntervalDialog.h"
#include "gui/dialogs/KeySignatureDialog.h"
#include "gui/dialogs/LilyPondOptionsDialog.h"
#include "gui/dialogs/LyricEditDialog.h"
#include "gui/dialogs/MakeOrnamentDialog.h"
#include "gui/dialogs/PasteNotationDialog.h"
#include "gui/dialogs/QuantizeDialog.h"
#include "gui/dialogs/SimpleEventEditDialog.h"
#include "gui/dialogs/TextEventDialog.h"
#include "gui/dialogs/TupletDialog.h"
#include "gui/dialogs/UseOrnamentDialog.h"
#include "gui/rulers/StandardRuler.h"
#include "gui/general/ActiveItem.h"
#include "gui/general/ClefIndex.h"
#include "gui/general/EditViewBase.h"
#include "gui/general/EditView.h"
#include "gui/general/GUIPalette.h"
#include "gui/general/LinedStaff.h"
#include "gui/general/LinedStaffManager.h"
#include "gui/general/ProgressReporter.h"
#include "gui/general/PresetHandlerDialog.h"
#include "gui/general/RosegardenCanvasView.h"
#include "gui/kdeext/KTmpStatusMsg.h"
#include "gui/kdeext/QCanvasSimpleSprite.h"
#include "gui/rulers/ChordNameRuler.h"
#include "gui/rulers/RawNoteRuler.h"
#include "gui/rulers/TempoRuler.h"
#include "gui/rulers/LoopRuler.h"
#include "gui/studio/StudioControl.h"
#include "gui/dialogs/EventFilterDialog.h"
#include "gui/widgets/ProgressBar.h"
#include "gui/widgets/ProgressDialog.h"
#include "gui/widgets/ScrollBoxDialog.h"
#include "gui/widgets/ScrollBox.h"
#include "gui/widgets/QDeferScrollView.h"
#include "NotationCanvasView.h"
#include "NotationElement.h"
#include "NotationEraser.h"
#include "NotationHLayout.h"
#include "NotationProperties.h"
#include "NotationSelector.h"
#include "NotationStaff.h"
#include "NotationStrings.h"
#include "NotationToolBox.h"
#include "NotationVLayout.h"
#include "NoteFontFactory.h"
#include "NoteInserter.h"
#include "NotePixmapFactory.h"
#include "NoteStyleFactory.h"
#include "NoteStyle.h"
#include "RestInserter.h"
#include "sound/MappedEvent.h"
#include "TextInserter.h"
#include "NotationCommandRegistry.h"
#include "HeadersGroup.h"
#include <QAction>
#include <QComboBox>
#include <QSettings>
#include <kglobal.h>
#include <klineeditdlg.h>
#include <QMessageBox>
#include <kprinter.h>
#include <QProcess>
#include <QProgressBar>
#include <QProgressDialog>
#include <kstatusbar.h>
#include <kstandardaction.h>
#include <ktoolbar.h>
#include <kxmlguiclient.h>
#include <QBrush>
#include <Q3Canvas>
#include <QCursor>
#include <QDialog>
#include <QEvent>
#include <QFont>
#include <QFontMetrics>
#include <QIcon>
#include <QLabel>
#include <QObject>
#include <qpaintdevicemetrics.h>
#include <QPainter>
#include <QPixmap>
#include <QPoint>
#include <QPrinter>
#include <QRect>
#include <QRegExp>
#include <QSize>
#include <QString>
#include <QTimer>
#include <QWidget>
#include <QHBoxLayout>
#include <QValidator>
#include <algorithm>
#include <QPushButton>
#include <QToolTip>


namespace Rosegarden
{

class NoteActionData
{
public:
    NoteActionData();
    NoteActionData(const QString& _title,
		   QString _actionName,
		   QString _pixmapName,
		   int _keycode,
		   bool _rest,
		   Note::Type _noteType,
		   int _dots);
    
    QString title;
    QString actionName;
    QString pixmapName;
    int keycode;
    bool rest;
    Note::Type noteType;
    int dots;
};

NoteActionData::NoteActionData()
    : title(0),
      actionName(0),
      pixmapName(0),
      keycode(0),
      rest(false),
      noteType(0),
      dots(0)
{
}

NoteActionData::NoteActionData(const QString& _title,
			       QString _actionName,
			       QString _pixmapName,
			       int _keycode,
			       bool _rest,
			       Note::Type _noteType,
			       int _dots)
    : title(_title),
      actionName(_actionName),
      pixmapName(_pixmapName),
      keycode(_keycode),
      rest(_rest),
      noteType(_noteType),
      dots(_dots)
{
}


class NoteChangeActionData
{
public:
    NoteChangeActionData();
    NoteChangeActionData(const QString &_title,
			 QString _actionName,
			 QString _pixmapName,
			 int _keycode,
			 bool _notationOnly,
			 Note::Type _noteType);

    QString title;
    QString actionName;
    QString pixmapName;
    int keycode;
    bool notationOnly;
    Note::Type noteType;
};

NoteChangeActionData::NoteChangeActionData()
    : title(0),
      actionName(0),
      pixmapName(0),
      keycode(0),
      notationOnly(false),
      noteType(0)
{
}

NoteChangeActionData::NoteChangeActionData(const QString& _title,
					   QString _actionName,
					   QString _pixmapName,
					   int _keycode,
					   bool _notationOnly,
					   Note::Type _noteType)
    : title(_title),
      actionName(_actionName),
      pixmapName(_pixmapName),
      keycode(_keycode),
      notationOnly(_notationOnly),
      noteType(_noteType)
{
}


NotationView::NotationView(RosegardenGUIDoc *doc,
                           std::vector<Segment *> segments,
                           QWidget *parent,
                           bool showProgressive) :
        EditView(doc, segments, 2, parent, "notationview"),
        m_properties(getViewLocalPropertyPrefix()),
        m_selectionCounter(0),
        m_insertModeLabel(0),
        m_annotationsLabel(0),
        m_lilyPondDirectivesLabel(0),
        m_progressBar(0),
        m_currentNotePixmap(0),
        m_hoveredOverNoteName(0),
        m_hoveredOverAbsoluteTime(0),
        m_currentStaff( -1),
        m_lastFinishingStaff( -1),
        m_title(0),
        m_subtitle(0),
        m_composer(0),
        m_copyright(0),
        m_insertionTime(0),
        m_deferredCursorMove(NoCursorMoveNeeded),
        m_lastNoteAction("crotchet"),
        m_fontName(NoteFontFactory::getDefaultFontName()),
        m_fontSize(NoteFontFactory::getDefaultSize(m_fontName)),
        m_pageMode(LinedStaff::LinearMode),
        m_leftGutter(20),
        m_notePixmapFactory(new NotePixmapFactory(m_fontName, m_fontSize)),
        m_hlayout(new NotationHLayout(&doc->getComposition(), m_notePixmapFactory,
                                      m_properties, this)),
        m_vlayout(new NotationVLayout(&doc->getComposition(), m_notePixmapFactory,
                                      m_properties, this)),
        m_chordNameRuler(0),
        m_tempoRuler(0),
        m_rawNoteRuler(0),
        m_annotationsVisible(false),
        m_lilyPondDirectivesVisible(false),
        m_selectDefaultNote(0),
        m_fontCombo(0),
        m_fontSizeCombo(0),
        m_spacingCombo(0),
        m_fontSizeActionMenu(0),
        m_pannerDialog(new ScrollBoxDialog(this, ScrollBox::FixHeight)),
        m_renderTimer(0),
        m_playTracking(true),
        m_progressDisplayer(PROGRESS_NONE),
        m_inhibitRefresh(true),
        m_ok(false),
        m_printMode(false),
        m_printSize(8), // set in positionStaffs
        m_showHeadersGroup(0),
        m_headersGroupView(0),
        m_headersGroup(0),
        m_headersTopFrame(0),
        m_showHeadersMenuEntry(0)
{
    initActionDataMaps(); // does something only the 1st time it's called

    m_toolBox = new NotationToolBox(this);

    assert(segments.size() > 0);
    NOTATION_DEBUG << "NotationView ctor" << endl;


    // Initialise the display-related defaults that will be needed
    // by both the actions and the layout toolbar

    QSettings settings;

    settings.beginGroup( NotationViewConfigGroup );

    m_showHeadersGroup = settings.value("shownotationheader",
                                                HeadersGroup::DefaultShowMode).toInt() ;

    m_fontName = qstrtostr(settings.value("notefont",
            strtoqstr(NoteFontFactory::getDefaultFontName())).toString());

    try
    {
        (void)NoteFontFactory::getFont
        (m_fontName,
         NoteFontFactory::getDefaultSize(m_fontName));
    } catch (Exception e)
    {
        m_fontName = NoteFontFactory::getDefaultFontName();
    }

    m_fontSize = settings.value((segments.size() > 1 ? "multistaffnotesize"
            : "singlestaffnotesize"),
            NoteFontFactory::getDefaultSize(m_fontName)).toUInt();

    int defaultSpacing = settings.value("spacing", 100).toInt();
    m_hlayout->setSpacing(defaultSpacing);

    int defaultProportion = settings.value("proportion", 60).toInt();
    m_hlayout->setProportion(defaultProportion);

    delete m_notePixmapFactory;
    m_notePixmapFactory = new NotePixmapFactory(m_fontName, m_fontSize);
    m_hlayout->setNotePixmapFactory(m_notePixmapFactory);
    m_vlayout->setNotePixmapFactory(m_notePixmapFactory);

    m_commandRegistry = new NotationCommandRegistry(this);

    setupActions();
    //     setupAddControlRulerMenu(); - too early for notation, moved to end of ctor.

    initLayoutToolbar();
    initStatusBar();

    setBackgroundMode(PaletteBase);

    Q3Canvas *tCanvas = new Q3Canvas(this);
    tCanvas->resize(width() * 2, height() * 2);

    setCanvasView(new NotationCanvasView(*this, tCanvas, getCentralWidget()));

    updateViewCaption();

    m_chordNameRuler = new ChordNameRuler
                       (m_hlayout, doc, segments, m_leftGutter, 20, getCentralWidget());
    addRuler(m_chordNameRuler);
    if (showProgressive)
        m_chordNameRuler->show();

    m_tempoRuler = new TempoRuler
                   (m_hlayout, doc, this, m_leftGutter, 24, false, getCentralWidget());
    addRuler(m_tempoRuler);
    m_tempoRuler->hide();
    static_cast<TempoRuler *>(m_tempoRuler)->connectSignals();

    m_rawNoteRuler = new RawNoteRuler
                     (m_hlayout, segments[0], m_leftGutter, 20, getCentralWidget());
    addRuler(m_rawNoteRuler);
    m_rawNoteRuler->show();

    // All toolbars should be created before this is called
    setAutoSaveSettings("NotationView", true);

    // All rulers must have been created before this is called,
    // or the program will crash
    readOptions();

    StandardRuler *standardRuler = new StandardRuler(getDocument(), m_hlayout,
                                                     m_leftGutter, 25, true,
                                                     getBottomWidget());
    getBottomWidget()->layout()->addWidget(standardRuler);
    setBottomStandardRuler(standardRuler);

    for (unsigned int i = 0; i < segments.size(); ++i)
    {
        m_staffs.push_back(new NotationStaff
                           (canvas(), segments[i], 0,  // snap
                            i, this,
                            m_fontName, m_fontSize));
    }


    // HeadersGroup ctor must not be called before m_staffs initialization
    m_headersGroupView = new QDeferScrollView(getCentralWidget());
    QWidget * vport = m_headersGroupView->viewport();
    m_headersGroup = new HeadersGroup(vport, this, &doc->getComposition());
    m_headersGroupView->setVScrollBarMode(QScrollView::AlwaysOff);
    m_headersGroupView->setHScrollBarMode(QScrollView::AlwaysOff);
    m_headersGroupView->setFixedWidth(m_headersGroupView->contentsWidth());
    m_canvasView->setLeftFixedWidget(m_headersGroupView);

    // Add a close button just above the track headers.
    // The grid layout is only here to maintain the button in a
    // right place
    m_headersTopFrame = new QFrame(getCentralWidget());
    QGridLayout * headersTopGrid
        = new QGridLayout(m_headersTopFrame, 2, 2);
    QString pixmapDir = KGlobal::dirs()->findResource("appdata", "pixmaps/");
    Q3CanvasPixmap pixmap(pixmapDir + "/misc/close.xpm");
    QPushButton * hideHeadersButton
        = new QPushButton(m_headersTopFrame);
    headersTopGrid->addWidget(hideHeadersButton, 1, 1,
                                        Qt::AlignRight | Qt::AlignBottom);
    hideHeadersButton->setIconSet(QIcon(pixmap));
    hideHeadersButton->setFlat(true);
    QToolTip::add(hideHeadersButton, i18n("Close track headers"));
    headersTopGrid->setMargin(4);
    setTopStandardRuler(new StandardRuler(getDocument(),
                                    m_hlayout, m_leftGutter, 25,
                                    false, getCentralWidget()), m_headersTopFrame);

    m_topStandardRuler->getLoopRuler()->setBackgroundColor
        (GUIPalette::getColour(GUIPalette::InsertCursorRuler));

    connect(m_topStandardRuler->getLoopRuler(), SIGNAL(startMouseMove(int)),
            m_canvasView, SLOT(startAutoScroll(int)));
    connect(m_topStandardRuler->getLoopRuler(), SIGNAL(stopMouseMove()),
            m_canvasView, SLOT(stopAutoScroll()));

    connect(m_bottomStandardRuler->getLoopRuler(), SIGNAL(startMouseMove(int)),
            m_canvasView, SLOT(startAutoScroll(int)));
    connect(m_bottomStandardRuler->getLoopRuler(), SIGNAL(stopMouseMove()),
            m_canvasView, SLOT(stopAutoScroll()));

    // Following connection have to be done before calling setPageMode())
    connect(m_headersGroup, SIGNAL(headersResized(int)),
            this, SLOT(slotHeadersWidthChanged(int)));


    //
    // layout
    //
    ProgressDialog* progressDlg = 0;

    if (showProgressive)
    {
        show();
        ProgressDialog::processEvents();

        NOTATION_DEBUG << "NotationView : setting up progress dialog" << endl;

        progressDlg = new ProgressDialog(i18n("Starting..."),
                                         100, this);
        progressDlg->setAutoClose(false);
        progressDlg->setAutoReset(true);
        progressDlg->setMinimumDuration(1000);
        setupProgress(progressDlg);

        m_progressDisplayer = PROGRESS_DIALOG;
    }

    m_chordNameRuler->setStudio(&getDocument()->getStudio());

    m_currentStaff = 0;
    m_staffs[0]->setCurrent(true);

    //###settings.beginGroup( NotationViewConfigGroup );
    int layoutMode = settings.value("layoutmode", 0).toInt() ;
    settings.endGroup();

    try
    {

        LinedStaff::PageMode mode = LinedStaff::LinearMode;
        if (layoutMode == 1)
            mode = LinedStaff::ContinuousPageMode;
        else if (layoutMode == 2)
            mode = LinedStaff::MultiPageMode;

        setPageMode(mode);

        for (unsigned int i = 0; i < m_staffs.size(); ++i) {
            m_staffs[i]->getSegment().getRefreshStatus
            (m_segmentsRefreshStatusIds[i]).setNeedsRefresh(false);
        }

        m_ok = true;

    } catch (ProgressReporter::Cancelled c)
    {
        // when cancelled, m_ok is false -- checked by calling method
        NOTATION_DEBUG << "NotationView ctor : layout Cancelled" << endl;
    }

    NOTATION_DEBUG << "NotationView ctor : m_ok = " << m_ok << endl;

    delete progressDlg;

    // at this point we can return if operation was cancelled
    if (!isOK())
    {
        setOutOfCtor();
        return ;
    }


    // otherwise, carry on
    setupDefaultProgress();

    //
    // Connect signals
    //

    QObject::connect
    (getCanvasView(), SIGNAL(renderRequired(double, double)),
     this, SLOT(slotCheckRendered(double, double)));

    m_topStandardRuler->connectRulerToDocPointer(doc);
    m_bottomStandardRuler->connectRulerToDocPointer(doc);

    // Disconnect the default connection for this signal from the
    // top ruler, and connect our own instead

    QObject::disconnect
    (m_topStandardRuler->getLoopRuler(),
     SIGNAL(setPointerPosition(timeT)), 0, 0);

    QObject::connect
    (m_topStandardRuler->getLoopRuler(),
     SIGNAL(setPointerPosition(timeT)),
     this, SLOT(slotSetInsertCursorPosition(timeT)));

    QObject::connect
    (m_topStandardRuler,
     SIGNAL(dragPointerToPosition(timeT)),
     this, SLOT(slotSetInsertCursorPosition(timeT)));

    connect(m_bottomStandardRuler, SIGNAL(dragPointerToPosition(timeT)),
            this, SLOT(slotSetPointerPosition(timeT)));

    QObject::connect
    (getCanvasView(), SIGNAL(itemPressed(int, int, QMouseEvent*, NotationElement*)),
     this, SLOT (slotItemPressed(int, int, QMouseEvent*, NotationElement*)));

    QObject::connect
    (getCanvasView(), SIGNAL(activeItemPressed(QMouseEvent*, Q3CanvasItem*)),
     this, SLOT (slotActiveItemPressed(QMouseEvent*, Q3CanvasItem*)));

    QObject::connect
    (getCanvasView(), SIGNAL(nonNotationItemPressed(QMouseEvent*, Q3CanvasItem*)),
     this, SLOT (slotNonNotationItemPressed(QMouseEvent*, Q3CanvasItem*)));

    QObject::connect
    (getCanvasView(), SIGNAL(textItemPressed(QMouseEvent*, Q3CanvasItem*)),
     this, SLOT (slotTextItemPressed(QMouseEvent*, Q3CanvasItem*)));

    QObject::connect
    (getCanvasView(), SIGNAL(mouseMoved(QMouseEvent*)),
     this, SLOT (slotMouseMoved(QMouseEvent*)));

    QObject::connect
    (getCanvasView(), SIGNAL(mouseReleased(QMouseEvent*)),
     this, SLOT (slotMouseReleased(QMouseEvent*)));

    QObject::connect
    (getCanvasView(), SIGNAL(hoveredOverNoteChanged(const QString&)),
     this, SLOT (slotHoveredOverNoteChanged(const QString&)));

    QObject::connect
    (getCanvasView(), SIGNAL(hoveredOverAbsoluteTimeChanged(unsigned int)),
     this, SLOT (slotHoveredOverAbsoluteTimeChanged(unsigned int)));

    QObject::connect
    (getCanvasView(), SIGNAL(zoomIn()), this, SLOT(slotZoomIn()));

    QObject::connect
    (getCanvasView(), SIGNAL(zoomOut()), this, SLOT(slotZoomOut()));

    QObject::connect
    (m_pannerDialog->scrollbox(), SIGNAL(valueChanged(const QPoint &)),
     getCanvasView(), SLOT(slotSetScrollPos(const QPoint &)));

    QObject::connect
    (getCanvasView()->horizontalScrollBar(), SIGNAL(valueChanged(int)),
     m_pannerDialog->scrollbox(), SLOT(setViewX(int)));

    QObject::connect
    (getCanvasView()->verticalScrollBar(), SIGNAL(valueChanged(int)),
     m_pannerDialog->scrollbox(), SLOT(setViewY(int)));

    QObject::connect
    (doc, SIGNAL(pointerPositionChanged(timeT)),
     this, SLOT(slotSetPointerPosition(timeT)));

    //
    // Connect vertical scrollbars between canvas and notation header
    QObject::connect
    (getCanvasView()->verticalScrollBar(), SIGNAL(valueChanged(int)),
     this, SLOT(slotVerticalScrollHeadersGroup(int)));

    QObject::connect
    (getCanvasView()->verticalScrollBar(), SIGNAL(sliderMoved(int)),
     this, SLOT(slotVerticalScrollHeadersGroup(int)));

    QObject::connect
    (m_headersGroupView, SIGNAL(gotWheelEvent(QWheelEvent*)),
     getCanvasView(), SLOT(slotExternalWheelEvent(QWheelEvent*)));

    // Ensure notation header keeps the right bottom margin when user
    // toggles the canvas view bottom rulers
    connect(getCanvasView(), SIGNAL(bottomWidgetHeightChanged(int)),
            this, SLOT(slotCanvasBottomWidgetHeightChanged(int)));

    // Signal canvas horizontal scroll to notation header
     QObject::connect
     (getCanvasView(), SIGNAL(contentsMoving(int, int)),
     this, SLOT(slotUpdateHeaders(int, int)));

    // Connect the close notation headers button
    QObject::connect(hideHeadersButton, SIGNAL(clicked()), 
                                  this, SLOT(slotHideHeadersGroup()));

    stateChanged("have_selection", KXMLGUIClient::StateReverse);
    stateChanged("have_notes_in_selection", KXMLGUIClient::StateReverse);
    stateChanged("have_rests_in_selection", KXMLGUIClient::StateReverse);
    stateChanged("have_multiple_staffs",
                 (m_staffs.size() > 1 ? KXMLGUIClient::StateNoReverse :
                  KXMLGUIClient::StateReverse));
    stateChanged("rest_insert_tool_current", KXMLGUIClient::StateReverse);
    slotTestClipboard();

    if (getSegmentsOnlyRestsAndClefs())
    {
        m_selectDefaultNote->activate();
        stateChanged("note_insert_tool_current",
                     KXMLGUIClient::StateNoReverse);
    } else
    {
        actionCollection()->action("select")->activate();
        stateChanged("note_insert_tool_current",
                     KXMLGUIClient::StateReverse);
    }

    timeT start = doc->getComposition().getLoopStart();
    timeT end = doc->getComposition().getLoopEnd();
    m_topStandardRuler->getLoopRuler()->slotSetLoopMarker(start, end);
    m_bottomStandardRuler->getLoopRuler()->slotSetLoopMarker(start, end);

    slotSetInsertCursorPosition(0);
    slotSetPointerPosition(doc->getComposition().getPosition());
    setCurrentSelection(0, false, true);
    slotUpdateInsertModeStatus();
    m_chordNameRuler->repaint();
    m_tempoRuler->repaint();
    m_rawNoteRuler->repaint();
    m_inhibitRefresh = false;

    //    slotCheckRendered(0, getCanvasView()->visibleWidth());
    //    getCanvasView()->repaintContents();
    updateView();

    QObject::connect
    (this, SIGNAL(renderComplete()),
     getCanvasView(), SLOT(slotRenderComplete()));

    if (parent)
    {
        const TrackButtons * trackLabels =
            ((RosegardenGUIView*)parent)->getTrackEditor()->getTrackButtons();
        QObject::connect
        (trackLabels, SIGNAL(nameChanged()),
         this, SLOT(slotUpdateStaffName()));
    }

    setConfigDialogPageIndex(3);
    setOutOfCtor();

    // Property and Control Rulers
    //
    if (getCurrentSegment()->getViewFeatures())
        slotShowVelocityControlRuler();
    setupControllerTabs();

    setupAddControlRulerMenu();
    setRewFFwdToAutoRepeat();

    slotCompositionStateUpdate();

    NOTATION_DEBUG << "NotationView ctor exiting" << endl;
}

NotationView::NotationView(RosegardenGUIDoc *doc,
                           std::vector<Segment *> segments,
                           QWidget *parent,
                           NotationView *referenceView)
        : EditView(doc, segments, 1, 0, "printview"),
        m_properties(getViewLocalPropertyPrefix()),
        m_selectionCounter(0),
        m_currentNotePixmap(0),
        m_hoveredOverNoteName(0),
        m_hoveredOverAbsoluteTime(0),
        m_lastFinishingStaff( -1),
        m_title(0),
        m_subtitle(0),
        m_composer(0),
        m_copyright(0),
        m_insertionTime(0),
        m_deferredCursorMove(NoCursorMoveNeeded),
        m_lastNoteAction("crotchet"),
        m_fontName(NoteFontFactory::getDefaultFontName()),
        m_fontSize(NoteFontFactory::getDefaultSize(m_fontName)),
        m_pageMode(LinedStaff::LinearMode),
        m_leftGutter(0),
        m_notePixmapFactory(new NotePixmapFactory(m_fontName, m_fontSize)),
        m_hlayout(new NotationHLayout(&doc->getComposition(), m_notePixmapFactory,
                                      m_properties, this)),
        m_vlayout(new NotationVLayout(&doc->getComposition(), m_notePixmapFactory,
                                      m_properties, this)),
        m_chordNameRuler(0),
        m_tempoRuler(0),
        m_rawNoteRuler(0),
        m_annotationsVisible(false),
        m_lilyPondDirectivesVisible(false),
        m_selectDefaultNote(0),
        m_fontCombo(0),
        m_fontSizeCombo(0),
        m_spacingCombo(0),
        m_fontSizeActionMenu(0),
        m_pannerDialog(0),
        m_renderTimer(0),
        m_playTracking(false),
        m_progressDisplayer(PROGRESS_NONE),
        m_inhibitRefresh(true),
        m_ok(false),
        m_printMode(true),
        m_printSize(8), // set in positionStaffs
        m_showHeadersGroup(0),
        m_headersGroupView(0),
        m_headersGroup(0),
        m_headersTopFrame(0),
        m_showHeadersMenuEntry(0)
{
    assert(segments.size() > 0);
    NOTATION_DEBUG << "NotationView print ctor" << endl;


    // Initialise the display-related defaults that will be needed
    // by both the actions and the layout toolbar

    QSettings settings;

    settings.beginGroup( NotationViewConfigGroup );

    if (referenceView)
    {
        m_fontName = referenceView->m_fontName;
    } else
    {
        m_fontName = qstrtostr(settings.value("notefont",
                strtoqstr(NoteFontFactory::getDefaultFontName())).toString());
    }


    // Force largest font size
    std::vector<int> sizes = NoteFontFactory::getAllSizes(m_fontName);
    m_fontSize = sizes[sizes.size() - 1];

    if (referenceView)
    {
        m_hlayout->setSpacing(referenceView->m_hlayout->getSpacing());
        m_hlayout->setProportion(referenceView->m_hlayout->getProportion());
    } else
    {
        int defaultSpacing = settings.value("spacing", 100).toInt();
        m_hlayout->setSpacing(defaultSpacing);
        int defaultProportion = settings.value("proportion", 60).toInt();
        m_hlayout->setProportion(defaultProportion);
    }
    settings.endGroup();

    delete m_notePixmapFactory;
    m_notePixmapFactory = new NotePixmapFactory(m_fontName, m_fontSize);
    m_hlayout->setNotePixmapFactory(m_notePixmapFactory);
    m_vlayout->setNotePixmapFactory(m_notePixmapFactory);

    setBackgroundMode(PaletteBase);

    Q3Canvas *tCanvas = new Q3Canvas(this);
    tCanvas->resize(width() * 2, height() * 2); //!!!

    setCanvasView(new NotationCanvasView(*this, tCanvas, getCentralWidget()));
    canvas()->retune(128); // tune for larger canvas

    for (unsigned int i = 0; i < segments.size(); ++i)
    {
        m_staffs.push_back(new NotationStaff(canvas(), segments[i], 0,  // snap
                                             i, this,
                                             m_fontName, m_fontSize));
    }

    m_currentStaff = 0;
    m_staffs[0]->setCurrent(true);

    ProgressDialog* progressDlg = 0;

    if (parent)
    {

        ProgressDialog::processEvents();

        NOTATION_DEBUG << "NotationView : setting up progress dialog" << endl;

        progressDlg = new ProgressDialog(i18n("Preparing to print..."),
                                         100, parent);
        progressDlg->setAutoClose(false);
        progressDlg->setAutoReset(true);
        progressDlg->setMinimumDuration(1000);
        setupProgress(progressDlg);

        m_progressDisplayer = PROGRESS_DIALOG;
    }

    try
    {

        setPageMode(LinedStaff::MultiPageMode); // also positions and renders the staffs!

        for (unsigned int i = 0; i < m_staffs.size(); ++i) {
            m_staffs[i]->getSegment().getRefreshStatus
            (m_segmentsRefreshStatusIds[i]).setNeedsRefresh(false);
        }

        m_ok = true;

    } catch (ProgressReporter::Cancelled c)
    {
        // when cancelled, m_ok is false -- checked by calling method
        NOTATION_DEBUG << "NotationView ctor : layout Cancelled" << endl;
    }

    NOTATION_DEBUG << "NotationView ctor : m_ok = " << m_ok << endl;

    delete progressDlg;

    if (!isOK())
    {
        setOutOfCtor();
        return ; // In case more code is added there later
    }

    setOutOfCtor(); // keep this as last call in the ctor
}

NotationView::~NotationView()
{
    NOTATION_DEBUG << "-> ~NotationView()" << endl;

    if (!m_printMode && m_ok)
        slotSaveOptions();

    delete m_chordNameRuler;

    delete m_renderTimer;

    for (unsigned int i = 0; i < m_staffs.size(); ++i) {
        for (Segment::iterator j = m_staffs[i]->getSegment().begin();
                j != m_staffs[i]->getSegment().end(); ++j) {
            removeViewLocalProperties(*j);
        }
        delete m_staffs[i]; // this will erase all "notes" canvas items
    }

    PixmapArrayGC::deleteAll();
    Profiles::getInstance()->dump();

    NOTATION_DEBUG << "<- ~NotationView()" << endl;
}

void
NotationView::removeViewLocalProperties(Event *e)
{
    Event::PropertyNames names(e->getPropertyNames());
    std::string prefix(getViewLocalPropertyPrefix());

    for (Event::PropertyNames::iterator i = names.begin();
            i != names.end(); ++i) {
        if (i->getName().substr(0, prefix.size()) == prefix) {
            e->unset(*i);
        }
    }
}

const NotationProperties &
NotationView::getProperties() const
{
    return m_properties;
}

void NotationView::positionStaffs()
{
    NOTATION_DEBUG << "NotationView::positionStaffs" << endl;

    QSettings settings;
    settings.beginGroup( NotationViewConfigGroup );

    m_printSize = settings.value("printingnotesize", 5).toUInt() ;

    int minTrack = 0, maxTrack = 0;
    bool haveMinTrack = false;
    typedef std::map<int, int> TrackIntMap;
    TrackIntMap trackHeights;
    TrackIntMap trackCoords;

    int pageWidth, pageHeight, leftMargin, topMargin;
    pageWidth = getPageWidth();
    pageHeight = getPageHeight();
    leftMargin = 0, topMargin = 0;
    getPageMargins(leftMargin, topMargin);

    int accumulatedHeight;
    int rowsPerPage = 1;
    int legerLines = 8;
    if (m_pageMode != LinedStaff::LinearMode)
        legerLines = 7;
    int rowGapPercent = (m_staffs.size() > 1 ? 40 : 10);
    int aimFor = -1;

    bool done = false;

    int titleHeight = 0;

    if (m_title)
        delete m_title;
    if (m_subtitle)
        delete m_subtitle;
    if (m_composer)
        delete m_composer;
    if (m_copyright)
        delete m_copyright;
    m_title = m_subtitle = m_composer = m_copyright = 0;

    if (m_pageMode == LinedStaff::MultiPageMode) {

        const Configuration &metadata =
            getDocument()->getComposition().getMetadata();

        QFont defaultFont(NotePixmapFactory::defaultSerifFontFamily);

        //###settings.beginGroup( NotationViewConfigGroup );

        QFont font = settings.value("textfont", &defaultFont).toString();

        font.setPixelSize(m_fontSize * 5);
        QFontMetrics metrics(font);

        if (metadata.has(CompositionMetadataKeys::Title)) {
            QString title(strtoqstr(metadata.get<String>
                                    (CompositionMetadataKeys::Title)));
            m_title = new Q3CanvasText(title, font, canvas());
            m_title->setX(m_leftGutter + pageWidth / 2 - metrics.width(title) / 2);
            m_title->setY(20 + topMargin / 4 + metrics.ascent());
            m_title->show();
            titleHeight += metrics.height() * 3 / 2 + topMargin / 4;
        }

        font.setPixelSize(m_fontSize * 3);
        metrics = QFontMetrics(font);

        if (metadata.has(CompositionMetadataKeys::Subtitle)) {
            QString subtitle(strtoqstr(metadata.get<String>
                                       (CompositionMetadataKeys::Subtitle)));
            m_subtitle = new Q3CanvasText(subtitle, font, canvas());
            m_subtitle->setX(m_leftGutter + pageWidth / 2 - metrics.width(subtitle) / 2);
            m_subtitle->setY(20 + titleHeight + metrics.ascent());
            m_subtitle->show();
            titleHeight += metrics.height() * 3 / 2;
        }

        if (metadata.has(CompositionMetadataKeys::Composer)) {
            QString composer(strtoqstr(metadata.get<String>
                                       (CompositionMetadataKeys::Composer)));
            m_composer = new Q3CanvasText(composer, font, canvas());
            m_composer->setX(m_leftGutter + pageWidth - metrics.width(composer) - leftMargin);
            m_composer->setY(20 + titleHeight + metrics.ascent());
            m_composer->show();
            titleHeight += metrics.height() * 3 / 2;
        }

        font.setPixelSize(m_fontSize * 2);
        metrics = QFontMetrics(font);

        if (metadata.has(CompositionMetadataKeys::Copyright)) {
            QString copyright(strtoqstr(metadata.get<String>
                                        (CompositionMetadataKeys::Copyright)));
            m_copyright = new Q3CanvasText(copyright, font, canvas());
            m_copyright->setX(m_leftGutter + leftMargin);
            m_copyright->setY(20 + pageHeight - topMargin - metrics.descent());
            m_copyright->show();
        }
    }
    settings.endGroup();

    while (1) {

        accumulatedHeight = 0;
        int maxTrackHeight = 0;

        trackHeights.clear();

        for (unsigned int i = 0; i < m_staffs.size(); ++i) {

            m_staffs[i]->setLegerLineCount(legerLines);

            int height = m_staffs[i]->getHeightOfRow();
            TrackId trackId = m_staffs[i]->getSegment().getTrack();
            Track *track =
                m_staffs[i]->getSegment().getComposition()->
                getTrackById(trackId);

            if (!track)
                continue; // This Should Not Happen, My Friend

            int trackPosition = track->getPosition();

            TrackIntMap::iterator hi = trackHeights.find(trackPosition);
            if (hi == trackHeights.end()) {
                trackHeights.insert(TrackIntMap::value_type
                                    (trackPosition, height));
            } else if (height > hi->second) {
                hi->second = height;
            }

            if (height > maxTrackHeight)
                maxTrackHeight = height;

            if (trackPosition < minTrack || !haveMinTrack) {
                minTrack = trackPosition;
                haveMinTrack = true;
            }
            if (trackPosition > maxTrack) {
                maxTrack = trackPosition;
            }
        }

        for (int i = minTrack; i <= maxTrack; ++i) {
            TrackIntMap::iterator hi = trackHeights.find(i);
            if (hi != trackHeights.end()) {
                trackCoords[i] = accumulatedHeight;
                accumulatedHeight += hi->second;
            }
        }

        accumulatedHeight += maxTrackHeight * rowGapPercent / 100;

        if (done)
            break;

        if (m_pageMode != LinedStaff::MultiPageMode) {

            rowsPerPage = 0;
            done = true;
            break;

        } else {

            // Check how well all this stuff actually fits on the
            // page.  If things don't fit as well as we'd like, modify
            // at most one parameter so as to save some space, then
            // loop around again and see if it worked.  This iterative
            // approach is inefficient but the time spent here is
            // neglible in context, and it's a simple way to code it.

            int staffPageHeight = pageHeight - topMargin * 2 - titleHeight;
            rowsPerPage = staffPageHeight / accumulatedHeight;

            if (rowsPerPage < 1) {

                if (legerLines > 5)
                    --legerLines;
                else if (rowGapPercent > 20)
                    rowGapPercent -= 10;
                else if (legerLines > 4)
                    --legerLines;
                else if (rowGapPercent > 0)
                    rowGapPercent -= 10;
                else if (legerLines > 3)
                    --legerLines;
                else if (m_printSize > 3)
                    --m_printSize;
                else { // just accept that we'll have to overflow
                    rowsPerPage = 1;
                    done = true;
                }

            } else {

                if (aimFor == rowsPerPage) {

                    titleHeight +=
                        (staffPageHeight - (rowsPerPage * accumulatedHeight)) / 2;

                    done = true;

                } else {

                    if (aimFor == -1)
                        aimFor = rowsPerPage + 1;

                    // we can perhaps accommodate another row, with care
                    if (legerLines > 5)
                        --legerLines;
                    else if (rowGapPercent > 20)
                        rowGapPercent -= 10;
                    else if (legerLines > 3)
                        --legerLines;
                    else if (rowGapPercent > 0)
                        rowGapPercent -= 10;
                    else { // no, we can't
                        rowGapPercent = 0;
                        legerLines = 8;
                        done = true;
                    }
                }
            }
        }
    }

    m_hlayout->setPageWidth(pageWidth - leftMargin * 2);

    int topGutter = 0;

    if (m_pageMode == LinedStaff::MultiPageMode) {

        topGutter = 20;

    } else if (m_pageMode == LinedStaff::ContinuousPageMode) {

        // fewer leger lines above staff than in linear mode --
        // compensate for this on the top staff
        topGutter = m_notePixmapFactory->getLineSpacing() * 2;
    }

    for (unsigned int i = 0; i < m_staffs.size(); ++i) {

        TrackId trackId = m_staffs[i]->getSegment().getTrack();
        Track *track =
            m_staffs[i]->getSegment().getComposition()->
            getTrackById(trackId);

        if (!track)
            continue; // Once Again, My Friend, You Should Never See Me Here

        int trackPosition = track->getPosition();

        m_staffs[i]->setTitleHeight(titleHeight);
        m_staffs[i]->setRowSpacing(accumulatedHeight);

        if (trackPosition < maxTrack) {
            m_staffs[i]->setConnectingLineLength(trackHeights[trackPosition]);
        }

        if (trackPosition == minTrack &&
                m_pageMode != LinedStaff::LinearMode) {
            m_staffs[i]->setBarNumbersEvery(5);
        } else {
            m_staffs[i]->setBarNumbersEvery(0);
        }

        m_staffs[i]->setX(m_leftGutter);
        m_staffs[i]->setY(topGutter + trackCoords[trackPosition] + topMargin);
        m_staffs[i]->setPageWidth(pageWidth - leftMargin * 2);
        m_staffs[i]->setRowsPerPage(rowsPerPage);
        m_staffs[i]->setPageMode(m_pageMode);
        m_staffs[i]->setMargin(leftMargin);

        NOTATION_DEBUG << "NotationView::positionStaffs: set staff's page width to "
        << (pageWidth - leftMargin * 2) << endl;

    }


    if (!m_printMode) {
        // Destroy then recreate all track headers
        hideHeadersGroup();
        m_headersGroup->removeAllHeaders();
        if (m_pageMode == LinedStaff::LinearMode) {
            for (int i = minTrack; i <= maxTrack; ++i) {
                TrackIntMap::iterator hi = trackHeights.find(i);
                if (hi != trackHeights.end()) {
                    TrackId trackId = getDocument()->getComposition()
                                            .getTrackByPosition(i)->getId();
                    m_headersGroup->addHeader(trackId, trackHeights[i],
                                              trackCoords[i], getCanvasLeftX());
                }
            }

            m_headersGroup->completeToHeight(canvas()->height());

            m_headersGroupView->addChild(m_headersGroup);

            getCanvasView()->updateLeftWidgetGeometry();

            if (    (m_showHeadersGroup == HeadersGroup::ShowAlways)
                || (    (m_showHeadersGroup == HeadersGroup::ShowWhenNeeded)
                      && (m_headersGroup->getUsedHeight()
                              > getCanvasView()->visibleHeight()))) {
                m_headersGroup->slotUpdateAllHeaders(getCanvasLeftX(), 0, true);
                showHeadersGroup();

                // Disable menu entry when headers are shown
                m_showHeadersMenuEntry->setEnabled(false);
            } else {
                // Enable menu entry when headers are hidden
                m_showHeadersMenuEntry->setEnabled(true);
            }
        } else {
            // Disable menu entry when not in linear mode
            m_showHeadersMenuEntry->setEnabled(false);
        }
    }
}

void NotationView::slotCanvasBottomWidgetHeightChanged(int newHeight)
{
    getCanvasView()->updateLeftWidgetGeometry();
}

void NotationView::positionPages()
{
    if (m_printMode)
        return ;

    QPixmap background;
    QPixmap deskBackground;
    bool haveBackground = false;

    QSettings settings;
    settings.beginGroup( NotationViewConfigGroup );

    if ( qStrToBool( settings.value("backgroundtextures", "true" ) ) ) {
        QString pixmapDir =
            KGlobal::dirs()->findResource("appdata", "pixmaps/");
        if (background.load(QString("%1/misc/bg-paper-cream.xpm").
                            arg(pixmapDir))) {
            haveBackground = true;
        }
        // we're happy to ignore errors from this one:
        deskBackground.load(QString("%1/misc/bg-desktop.xpm").arg(pixmapDir));
    }
    settings.endGroup();

    int pageWidth = getPageWidth();
    int pageHeight = getPageHeight();
    int leftMargin = 0, topMargin = 0;
    getPageMargins(leftMargin, topMargin);
    int maxPageCount = 1;

    for (unsigned int i = 0; i < m_staffs.size(); ++i) {
        int pageCount = m_staffs[i]->getPageCount();
        if (pageCount > maxPageCount)
            maxPageCount = pageCount;
    }

    for (unsigned int i = 0; i < m_pages.size(); ++i) {
        delete m_pages[i];
        delete m_pageNumbers[i];
    }
    m_pages.clear();
    m_pageNumbers.clear();

    if (m_pageMode != LinedStaff::MultiPageMode) {
        if (haveBackground) {
            canvas()->setBackgroundPixmap(background);
            getCanvasView()->setBackgroundMode(Qt::FixedPixmap);
            getCanvasView()->setPaletteBackgroundPixmap(background);
            getCanvasView()->setErasePixmap(background);
        }
    } else {
        if (haveBackground) {
            canvas()->setBackgroundPixmap(deskBackground);
            getCanvasView()->setBackgroundMode(Qt::FixedPixmap);
            getCanvasView()->setPaletteBackgroundPixmap(background);
            getCanvasView()->setErasePixmap(background);
        }

        QFont pageNumberFont;
        pageNumberFont.setPixelSize(m_fontSize * 2);
        QFontMetrics pageNumberMetrics(pageNumberFont);

        for (int page = 0; page < maxPageCount; ++page) {

            int x = m_leftGutter + pageWidth * page + leftMargin / 4;
            int y = 20;
            int w = pageWidth - leftMargin / 2;
            int h = pageHeight;

            QString str = QString("%1").arg(page + 1);
            Q3CanvasText *text = new Q3CanvasText(str, pageNumberFont, canvas());
            text->setX(m_leftGutter + pageWidth * page + pageWidth - pageNumberMetrics.width(str) - leftMargin);
            text->setY(y + h - pageNumberMetrics.descent() - topMargin);
            text->setZ( -999);
            text->show();
            m_pageNumbers.push_back(text);

            Q3CanvasRectangle *rect = new Q3CanvasRectangle(x, y, w, h, canvas());
            if (haveBackground)
                rect->setBrush(QBrush(QColor(Qt::white), background));
            rect->setPen(QColor(Qt::black));
            rect->setZ( -1000);
            rect->show();
            m_pages.push_back(rect);
        }

        updateThumbnails(false);
    }
}

void NotationView::slotUpdateStaffName()
{
    LinedStaff *staff = getLinedStaff(m_currentStaff);
    staff->drawStaffName();
    m_headersGroup->slotUpdateAllHeaders(getCanvasLeftX(), 0, true);
}

void NotationView::slotSaveOptions()
{
    QSettings settings;
    settings.beginGroup( NotationViewConfigGroup );

    settings.setValue("Show Chord Name Ruler", getToggleAction("show_chords_ruler")->isChecked());
    settings.setValue("Show Raw Note Ruler", getToggleAction("show_raw_note_ruler")->isChecked());
    settings.setValue("Show Tempo Ruler", getToggleAction("show_tempo_ruler")->isChecked());
    settings.setValue("Show Annotations", m_annotationsVisible);
    settings.setValue("Show LilyPond Directives", m_lilyPondDirectivesVisible);

    settings.sync();

    settings.endGroup();
}

void NotationView::setOneToolbar(const char *actionName,
                                 const char *toolbarName)
{
    /* was toggle */ QAction *action = getToggleAction(actionName);
    if (!action) {
        std::cerr << "WARNING: No such action as " << actionName << std::endl;
        return ;
    }
    QWidget *toolbar = toolBar(toolbarName);
    if (!toolbar) {
        std::cerr << "WARNING: No such toolbar as " << toolbarName << std::endl;
        return ;
    }
    action->setChecked(!toolbar->isHidden());
}

void NotationView::readOptions()
{
    EditView::readOptions();

    setOneToolbar("show_tools_toolbar", "Tools Toolbar");
    setOneToolbar("show_notes_toolbar", "Notes Toolbar");
    setOneToolbar("show_rests_toolbar", "Rests Toolbar");
    setOneToolbar("show_clefs_toolbar", "Clefs Toolbar");
    setOneToolbar("show_group_toolbar", "Group Toolbar");
    setOneToolbar("show_marks_toolbar", "Marks Toolbar");
    setOneToolbar("show_layout_toolbar", "Layout Toolbar");
    setOneToolbar("show_transport_toolbar", "Transport Toolbar");
    setOneToolbar("show_accidentals_toolbar", "Accidentals Toolbar");
    setOneToolbar("show_meta_toolbar", "Meta Toolbar");

    QSettings settings;
    settings.beginGroup( NotationViewConfigGroup );

    bool opt;

    opt = qStrToBool( settings.value("Show Chord Name Ruler", "false" ) ) ;
    getToggleAction("show_chords_ruler")->setChecked(opt);
    slotToggleChordsRuler();

    opt = qStrToBool( settings.value("Show Raw Note Ruler", "true" ) ) ;
    getToggleAction("show_raw_note_ruler")->setChecked(opt);
    slotToggleRawNoteRuler();

    opt = qStrToBool( settings.value("Show Tempo Ruler", "true" ) ) ;
    getToggleAction("show_tempo_ruler")->setChecked(opt);
    slotToggleTempoRuler();

    opt = qStrToBool( settings.value("Show Annotations", "true" ) ) ;
    m_annotationsVisible = opt;
    getToggleAction("show_annotations")->setChecked(opt);
    slotUpdateAnnotationsStatus();
    //    slotToggleAnnotations();

    opt = qStrToBool( settings.value("Show LilyPond Directives", "true" ) ) ;
    m_lilyPondDirectivesVisible = opt;
    getToggleAction("show_lilypond_directives")->setChecked(opt);
    slotUpdateLilyPondDirectivesStatus();

    settings.endGroup();
}

void NotationView::setupActions()
{
    KStandardAction::print(this, SLOT(slotFilePrint()), actionCollection());
    KStandardAction::printPreview(this, SLOT(slotFilePrintPreview()),
                             actionCollection());

    QAction *qa_file_print_lilypond = new QAction( "Print &with LilyPond...", dynamic_cast<QObject*>(this) ); //### deallocate action ptr 
			qa_file_print_lilypond->setIconText(0); 
			connect( qa_file_print_lilypond, SIGNAL(triggered()), this, SLOT(slotPrintLilyPond())  );

    QAction *qa_file_preview_lilypond = new QAction( "Preview with Lil&yPond...", dynamic_cast<QObject*>(this) ); //### deallocate action ptr 
			qa_file_preview_lilypond->setIconText(0); 
			connect( qa_file_preview_lilypond, SIGNAL(triggered()), this, SLOT(slotPreviewLilyPond())  );

    EditViewBase::setupActions("notation.rc");
    EditView::setupActions();

    KRadioAction* noteAction = 0;

    // View menu stuff

    KActionMenu *fontActionMenu =
        new KActionMenu(i18n("Note &Font"), this, "note_font_actionmenu");

    std::set
        <std::string> fs(NoteFontFactory::getFontNames());
    std::vector<std::string> f(fs.begin(), fs.end());
    std::sort(f.begin(), f.end());

    for (std::vector<std::string>::iterator i = f.begin(); i != f.end(); ++i) {

        QString fontQName(strtoqstr(*i));

        /* was toggle */ QAction *fontAction =
            new /* was toggle */ QAction
            (fontQName, 0, this, SLOT(slotChangeFontFromAction()),
             actionCollection(), "note_font_" + fontQName);

        fontAction->setChecked(*i == m_fontName);
        fontActionMenu->insert(fontAction);
    }

    actionCollection()->insert(fontActionMenu);

    m_fontSizeActionMenu =
        new KActionMenu(i18n("Si&ze"), this, "note_font_size_actionmenu");
    setupFontSizeMenu();

    actionCollection()->insert(m_fontSizeActionMenu);

    m_showHeadersMenuEntry
        = QAction* qa_show_track_headers = new QAction(  i18n("Show Track Headers"), dynamic_cast<QObject*>(this) );
			connect( qa_show_track_headers, SIGNAL(toggled()), dynamic_cast<QObject*>(this), SLOT(slotShowHeadersGroup()) );
			qa_show_track_headers->setObjectName( "show_track_headers" );		//
			//qa_show_track_headers->setCheckable( true );		//
			qa_show_track_headers->setAutoRepeat( false );	//
			//qa_show_track_headers->setActionGroup( 0 );		// QActionGroup*
			//qa_show_track_headers->setChecked( false );		//
			//### FIX: deallocate QAction ptr
			

    KActionMenu *spacingActionMenu =
        new KActionMenu(i18n("S&pacing"), this, "stretch_actionmenu");

    int defaultSpacing = m_hlayout->getSpacing();
    std::vector<int> spacings = NotationHLayout::getAvailableSpacings();

    for (std::vector<int>::iterator i = spacings.begin();
            i != spacings.end(); ++i) {

        /* was toggle */ QAction *spacingAction =
            new /* was toggle */ QAction
            (QString("%1%").arg(*i), 0, this,
             SLOT(slotChangeSpacingFromAction()),
             actionCollection(), QString("spacing_%1").arg(*i));

        spacingAction->setExclusiveGroup("spacing");
        spacingAction->setChecked(*i == defaultSpacing);
        spacingActionMenu->insert(spacingAction);
    }

    actionCollection()->insert(spacingActionMenu);

    KActionMenu *proportionActionMenu =
        new KActionMenu(i18n("Du&ration Factor"), this, "proportion_actionmenu");

    int defaultProportion = m_hlayout->getProportion();
    std::vector<int> proportions = NotationHLayout::getAvailableProportions();

    for (std::vector<int>::iterator i = proportions.begin();
            i != proportions.end(); ++i) {

        QString name = QString("%1%").arg(*i);
        if (*i == 0)
            name = i18n("None");

        /* was toggle */ QAction *proportionAction =
            new /* was toggle */ QAction
            (name, 0, this,
             SLOT(slotChangeProportionFromAction()),
             actionCollection(), QString("proportion_%1").arg(*i));

        proportionAction->setExclusiveGroup("proportion");
        proportionAction->setChecked(*i == defaultProportion);
        proportionActionMenu->insert(proportionAction);
    }

    actionCollection()->insert(proportionActionMenu);

    KActionMenu *ornamentActionMenu =
        new KActionMenu(i18n("Use Ornament"), this, "ornament_actionmenu");



    new KAction
    (i18n("Insert Rest"), Qt::Key_P, this, SLOT(slotInsertRest()),
     actionCollection(), QString("insert_rest"));

    new KAction
    (i18n("Switch from Note to Rest"), Qt::Key_T, this,
     SLOT(slotSwitchFromNoteToRest()),
     actionCollection(), QString("switch_from_note_to_rest"));

    new KAction
    (i18n("Switch from Rest to Note"), Qt::Key_Y, this,
     SLOT(slotSwitchFromRestToNote()),
     actionCollection(), QString("switch_from_rest_to_note"));


    // setup Notes menu & toolbar
    QIcon icon;

    for (NoteActionDataMap::Iterator actionDataIter = m_noteActionDataMap->begin();
            actionDataIter != m_noteActionDataMap->end();
            ++actionDataIter) {

        NoteActionData noteActionData = **actionDataIter;

        icon = QIcon
               (NotePixmapFactory::toQPixmap(NotePixmapFactory::makeToolbarPixmap
                                             (noteActionData.pixmapName)));
        noteAction = new KRadioAction(noteActionData.title,
                                      icon,
                                      noteActionData.keycode,
                                      this,
                                      SLOT(slotNoteAction()),
                                      actionCollection(),
                                      noteActionData.actionName);
        noteAction->setExclusiveGroup("notes");

        if (noteActionData.noteType == Note::Crotchet &&
                noteActionData.dots == 0 && !noteActionData.rest) {
            m_selectDefaultNote = noteAction;
        }
    }

    // Note duration change actions
    for (NoteChangeActionDataMap::Iterator actionDataIter = m_noteChangeActionDataMap->begin();
            actionDataIter != m_noteChangeActionDataMap->end();
            ++actionDataIter) {

        NoteChangeActionData data = **actionDataIter;

        icon = QIcon
               (NotePixmapFactory::toQPixmap(NotePixmapFactory::makeToolbarPixmap
                                             (data.pixmapName)));

        KAction *action = new KAction(data.title,
                                      icon,
                                      data.keycode,
                                      this,
                                      SLOT(slotNoteChangeAction()),
                                      actionCollection(),
                                      data.actionName);
    }

    //
    // Accidentals
    //
    static QString actionsAccidental[][4] =
        {
            { i18n("No accidental"), "1slotNoAccidental()", "no_accidental", "accidental-none" },
            { i18n("Follow previous accidental"), "1slotFollowAccidental()", "follow_accidental", "accidental-follow" },
            { i18n("Sharp"), "1slotSharp()", "sharp_accidental", "accidental-sharp" },
            { i18n("Flat"), "1slotFlat()", "flat_accidental", "accidental-flat" },
            { i18n("Natural"), "1slotNatural()", "natural_accidental", "accidental-natural" },
            { i18n("Double sharp"), "1slotDoubleSharp()", "double_sharp_accidental", "accidental-doublesharp" },
            { i18n("Double flat"), "1slotDoubleFlat()", "double_flat_accidental", "accidental-doubleflat" }
        };

    IconLoader il;

    for (unsigned int i = 0;
            i < sizeof(actionsAccidental) / sizeof(actionsAccidental[0]); ++i) {

        icon = il.load(actionsAccidental[i][3]);
        noteAction = new KRadioAction(actionsAccidental[i][0], icon, 0, this,
                                      actionsAccidental[i][1],
                                      actionCollection(), actionsAccidental[i][2]);
        noteAction->setExclusiveGroup("accidentals");
    }


    //
    // Clefs
    //

    // Treble
    icon = il.load("clef-treble");
    noteAction = new KRadioAction(i18n("&Treble Clef"), icon, 0, this,
                                  SLOT(slotTrebleClef()),
                                  actionCollection(), "treble_clef");
    noteAction->setExclusiveGroup("notes");

    // Alto
    icon = il.load("clef-alto");
    noteAction = new KRadioAction(i18n("&Alto Clef"), icon, 0, this,
                                  SLOT(slotAltoClef()),
                                  actionCollection(), "alto_clef");
    noteAction->setExclusiveGroup("notes");

    // Tenor
    icon = il.load("clef-tenor");
    noteAction = new KRadioAction(i18n("Te&nor Clef"), icon, 0, this,
                                  SLOT(slotTenorClef()),
                                  actionCollection(), "tenor_clef");
    noteAction->setExclusiveGroup("notes");

    // Bass
    icon = il.load("clef-bass");
    noteAction = new KRadioAction(i18n("&Bass Clef"), icon, 0, this,
                                  SLOT(slotBassClef()),
                                  actionCollection(), "bass_clef");
    noteAction->setExclusiveGroup("notes");


    icon = il.load("text");
    noteAction = new KRadioAction(i18n("&Text"), icon, Qt::Key_F8, this,
                                  SLOT(slotText()),
                                  actionCollection(), "text");
    noteAction->setExclusiveGroup("notes");

    icon = il.load("guitarchord");
    noteAction = new KRadioAction(i18n("&Guitar Chord"), icon, Qt::Key_F9, this,
                                  SLOT(slotGuitarChord()),
                                  actionCollection(), "guitarchord");
    noteAction->setExclusiveGroup("notes");

    /*    icon = il.load("lilypond");
        noteAction = new KRadioAction(i18n("Lil&ypond Directive"), icon, Qt::Key_F9, this,
                                      SLOT(slotLilyPondDirective()),
                                      actionCollection(), "lilypond_directive");
        noteAction->setExclusiveGroup("notes"); */


    //
    // Edition tools (eraser, selector...)
    //
    noteAction = new KRadioAction(i18n("&Erase"), "eraser", Qt::Key_F4,
                                  this, SLOT(slotEraseSelected()),
                                  actionCollection(), "erase");
    noteAction->setExclusiveGroup("notes");

    icon = il.load("select");
    noteAction = new KRadioAction(i18n("&Select and Edit"), icon, Qt::Key_F2,
                                  this, SLOT(slotSelectSelected()),
                                  actionCollection(), "select");
    noteAction->setExclusiveGroup("notes");

    icon = il.load("step_by_step");
    QAction* qa_toggle_step_by_step = new QAction( icon, i18n("Ste&p Recording"), dynamic_cast<QObject*>(0) );
	connect( qa_toggle_step_by_step, SIGNAL(toggled()), dynamic_cast<QObject*>(0), this );
	qa_toggle_step_by_step->setObjectName( "toggle_step_by_step" );	//### FIX: deallocate QAction ptr
	qa_toggle_step_by_step->setCheckable( true );	//
	qa_toggle_step_by_step->setAutoRepeat( false );	//
	//qa_toggle_step_by_step->setActionGroup( 0 );	// QActionGroup*
	qa_toggle_step_by_step->setChecked( false );	//
	// ;


    // Edit menu
    QAction* qa_select_from_start = new QAction(  i18n("Select from Sta&rt"), dynamic_cast<QObject*>(this) );
			connect( qa_select_from_start, SIGNAL(toggled()), dynamic_cast<QObject*>(this), SLOT(slotEditSelectFromStart()) );
			qa_select_from_start->setObjectName( "select_from_start" );		//
			//qa_select_from_start->setCheckable( true );		//
			qa_select_from_start->setAutoRepeat( false );	//
			//qa_select_from_start->setActionGroup( 0 );		// QActionGroup*
			//qa_select_from_start->setChecked( false );		//
			//### FIX: deallocate QAction ptr
			

    QAction* qa_select_to_end = new QAction(  i18n("Select to &End"), dynamic_cast<QObject*>(this) );
			connect( qa_select_to_end, SIGNAL(toggled()), dynamic_cast<QObject*>(this), SLOT(slotEditSelectToEnd()) );
			qa_select_to_end->setObjectName( "select_to_end" );		//
			//qa_select_to_end->setCheckable( true );		//
			qa_select_to_end->setAutoRepeat( false );	//
			//qa_select_to_end->setActionGroup( 0 );		// QActionGroup*
			//qa_select_to_end->setChecked( false );		//
			//### FIX: deallocate QAction ptr
			

    QAction* qa_select_whole_staff = new QAction(  i18n("Select Whole St&aff"), dynamic_cast<QObject*>(this) );
			connect( qa_select_whole_staff, SIGNAL(toggled()), dynamic_cast<QObject*>(this), SLOT(slotEditSelectWholeStaff()) );
			qa_select_whole_staff->setObjectName( "select_whole_staff" );		//
			//qa_select_whole_staff->setCheckable( true );		//
			qa_select_whole_staff->setAutoRepeat( false );	//
			//qa_select_whole_staff->setActionGroup( 0 );		// QActionGroup*
			//qa_select_whole_staff->setChecked( false );		//
			//### FIX: deallocate QAction ptr
			

    QAction* qa_cut_and_close = new QAction(  i18n("C&ut and Close"), dynamic_cast<QObject*>(this) );
			connect( qa_cut_and_close, SIGNAL(toggled()), dynamic_cast<QObject*>(this), SLOT(slotEditCutAndClose()) );
			qa_cut_and_close->setObjectName( "cut_and_close" );		//
			//qa_cut_and_close->setCheckable( true );		//
			qa_cut_and_close->setAutoRepeat( false );	//
			//qa_cut_and_close->setActionGroup( 0 );		// QActionGroup*
			//qa_cut_and_close->setChecked( false );		//
			//### FIX: deallocate QAction ptr
			

    QAction* qa_general_paste = new QAction(  i18n("Pa&ste..."), dynamic_cast<QObject*>(this) );
			connect( qa_general_paste, SIGNAL(toggled()), dynamic_cast<QObject*>(this), SLOT(slotEditGeneralPaste()) );
			qa_general_paste->setObjectName( "general_paste" );		//
			//qa_general_paste->setCheckable( true );		//
			qa_general_paste->setAutoRepeat( false );	//
			//qa_general_paste->setActionGroup( 0 );		// QActionGroup*
			//qa_general_paste->setChecked( false );		//
			//### FIX: deallocate QAction ptr
			

    QAction* qa_delete = new QAction(  i18n("De&lete"), dynamic_cast<QObject*>(this) );
			connect( qa_delete, SIGNAL(toggled()), dynamic_cast<QObject*>(this), SLOT(slotEditDelete()) );
			qa_delete->setObjectName( "delete" );		//
			//qa_delete->setCheckable( true );		//
			qa_delete->setAutoRepeat( false );	//
			//qa_delete->setActionGroup( 0 );		// QActionGroup*
			//qa_delete->setChecked( false );		//
			//### FIX: deallocate QAction ptr
			

    QAction* qa_move_events_up_staff = new QAction(  i18n("Move to Staff Above"), dynamic_cast<QObject*>(this) );
			connect( qa_move_events_up_staff, SIGNAL(toggled()), dynamic_cast<QObject*>(this), SLOT(slotMoveEventsUpStaff()) );
			qa_move_events_up_staff->setObjectName( "move_events_up_staff" );		//
			//qa_move_events_up_staff->setCheckable( true );		//
			qa_move_events_up_staff->setAutoRepeat( false );	//
			//qa_move_events_up_staff->setActionGroup( 0 );		// QActionGroup*
			//qa_move_events_up_staff->setChecked( false );		//
			//### FIX: deallocate QAction ptr
			

    QAction* qa_move_events_down_staff = new QAction(  i18n("Move to Staff TicksBelow"), dynamic_cast<QObject*>(this) );
			connect( qa_move_events_down_staff, SIGNAL(toggled()), dynamic_cast<QObject*>(this), SLOT(slotMoveEventsDownStaff()) );
			qa_move_events_down_staff->setObjectName( "move_events_down_staff" );		//
			//qa_move_events_down_staff->setCheckable( true );		//
			qa_move_events_down_staff->setAutoRepeat( false );	//
			//qa_move_events_down_staff->setActionGroup( 0 );		// QActionGroup*
			//qa_move_events_down_staff->setChecked( false );		//
			//### FIX: deallocate QAction ptr
			

    //
    // Settings menu
    //
    int layoutMode = settings.value("layoutmode", 0).toInt() ;

    QString pixmapDir = KGlobal::dirs()->findResource("appdata", "pixmaps/");

    Q3CanvasPixmap pixmap(pixmapDir + "/toolbar/linear-layout.xpm");
    icon = QIcon(pixmap);
    KRadioAction *linearModeAction = new KRadioAction
                                     (i18n("&Linear Layout"), icon, 0, this, SLOT(slotLinearMode()),
                                      actionCollection(), "linear_mode");
    linearModeAction->setExclusiveGroup("layoutMode");
    if (layoutMode == 0)
        linearModeAction->setChecked(true);

    pixmap.load(pixmapDir + "/toolbar/continuous-page-mode.xpm");
    icon = QIcon(pixmap);
    KRadioAction *continuousPageModeAction = new KRadioAction
            (i18n("&Continuous Page Layout"), icon, 0, this, SLOT(slotContinuousPageMode()),
             actionCollection(), "continuous_page_mode");
    continuousPageModeAction->setExclusiveGroup("layoutMode");
    if (layoutMode == 1)
        continuousPageModeAction->setChecked(true);

    pixmap.load(pixmapDir + "/toolbar/multi-page-mode.xpm");
    icon = QIcon(pixmap);
    KRadioAction *multiPageModeAction = new KRadioAction
                                        (i18n("&Multiple Page Layout"), icon, 0, this, SLOT(slotMultiPageMode()),
                                         actionCollection(), "multi_page_mode");
    multiPageModeAction->setExclusiveGroup("layoutMode");
    if (layoutMode == 2)
        multiPageModeAction->setChecked(true);

    QAction* qa_show_chords_ruler = new QAction( 0, i18n("Show Ch&ord Name Ruler"), dynamic_cast<QObject*>(this) );
	connect( qa_show_chords_ruler, SIGNAL(toggled()), dynamic_cast<QObject*>(this), SLOT(slotToggleChordsRuler()) );
	qa_show_chords_ruler->setObjectName( "show_chords_ruler" );	//### FIX: deallocate QAction ptr
	qa_show_chords_ruler->setCheckable( true );	//
	qa_show_chords_ruler->setAutoRepeat( false );	//
	//qa_show_chords_ruler->setActionGroup( 0 );	// QActionGroup*
	qa_show_chords_ruler->setChecked( false );	//
	// ;

    QAction* qa_show_raw_note_ruler = new QAction( 0, i18n("Show Ra&w Note Ruler"), dynamic_cast<QObject*>(this) );
	connect( qa_show_raw_note_ruler, SIGNAL(toggled()), dynamic_cast<QObject*>(this), SLOT(slotToggleRawNoteRuler()) );
	qa_show_raw_note_ruler->setObjectName( "show_raw_note_ruler" );	//### FIX: deallocate QAction ptr
	qa_show_raw_note_ruler->setCheckable( true );	//
	qa_show_raw_note_ruler->setAutoRepeat( false );	//
	//qa_show_raw_note_ruler->setActionGroup( 0 );	// QActionGroup*
	qa_show_raw_note_ruler->setChecked( false );	//
	// ;

    QAction* qa_show_tempo_ruler = new QAction( 0, i18n("Show &Tempo Ruler"), dynamic_cast<QObject*>(this) );
	connect( qa_show_tempo_ruler, SIGNAL(toggled()), dynamic_cast<QObject*>(this), SLOT(slotToggleTempoRuler()) );
	qa_show_tempo_ruler->setObjectName( "show_tempo_ruler" );	//### FIX: deallocate QAction ptr
	qa_show_tempo_ruler->setCheckable( true );	//
	qa_show_tempo_ruler->setAutoRepeat( false );	//
	//qa_show_tempo_ruler->setActionGroup( 0 );	// QActionGroup*
	qa_show_tempo_ruler->setChecked( false );	//
	// ;

    QAction* qa_show_annotations = new QAction( 0, i18n("Show &Annotations"), dynamic_cast<QObject*>(this) );
	connect( qa_show_annotations, SIGNAL(toggled()), dynamic_cast<QObject*>(this), SLOT(slotToggleAnnotations()) );
	qa_show_annotations->setObjectName( "show_annotations" );	//### FIX: deallocate QAction ptr
	qa_show_annotations->setCheckable( true );	//
	qa_show_annotations->setAutoRepeat( false );	//
	//qa_show_annotations->setActionGroup( 0 );	// QActionGroup*
	qa_show_annotations->setChecked( false );	//
	// ;

    QAction* qa_show_lilypond_directives = new QAction( 0, i18n("Show Lily&Pond Directives"), dynamic_cast<QObject*>(this) );
	connect( qa_show_lilypond_directives, SIGNAL(toggled()), dynamic_cast<QObject*>(this), SLOT(slotToggleLilyPondDirectives()) );
	qa_show_lilypond_directives->setObjectName( "show_lilypond_directives" );	//### FIX: deallocate QAction ptr
	qa_show_lilypond_directives->setCheckable( true );	//
	qa_show_lilypond_directives->setAutoRepeat( false );	//
	//qa_show_lilypond_directives->setActionGroup( 0 );	// QActionGroup*
	qa_show_lilypond_directives->setChecked( false );	//
	// ;

    QAction* qa_lyric_editor = new QAction(  i18n("Open L&yric Editor"), dynamic_cast<QObject*>(this) );
			connect( qa_lyric_editor, SIGNAL(toggled()), dynamic_cast<QObject*>(this), SLOT(slotEditLyrics()) );
			qa_lyric_editor->setObjectName( "lyric_editor" );		//
			//qa_lyric_editor->setCheckable( true );		//
			qa_lyric_editor->setAutoRepeat( false );	//
			//qa_lyric_editor->setActionGroup( 0 );		// QActionGroup*
			//qa_lyric_editor->setChecked( false );		//
			//### FIX: deallocate QAction ptr
			

    //
    // Group menu
    //

    icon = il.load
                                         ("group-simple-tuplet");

    QAction* qa_simple_tuplet = new QAction(  TupletCommand::getGlobalName(true), dynamic_cast<QObject*>(Qt::Key_R + Qt::CTRL) );
			connect( qa_simple_tuplet, SIGNAL(toggled()), dynamic_cast<QObject*>(Qt::Key_R + Qt::CTRL), this );
			qa_simple_tuplet->setObjectName( "simple_tuplet" );		//
			//qa_simple_tuplet->setCheckable( true );		//
			qa_simple_tuplet->setAutoRepeat( false );	//
			//qa_simple_tuplet->setActionGroup( 0 );		// QActionGroup*
			//qa_simple_tuplet->setChecked( false );		//
			//### FIX: deallocate QAction ptr
			

    icon = il.load
                                         ("group-tuplet");

    QAction* qa_tuplet = new QAction(  TupletCommand::getGlobalName(false), dynamic_cast<QObject*>(Qt::Key_T + Qt::CTRL) );
			connect( qa_tuplet, SIGNAL(toggled()), dynamic_cast<QObject*>(Qt::Key_T + Qt::CTRL), this );
			qa_tuplet->setObjectName( "tuplet" );		//
			//qa_tuplet->setCheckable( true );		//
			qa_tuplet->setAutoRepeat( false );	//
			//qa_tuplet->setActionGroup( 0 );		// QActionGroup*
			//qa_tuplet->setChecked( false );		//
			//### FIX: deallocate QAction ptr
			

    icon = il.load("triplet");
    (QAction* qa_triplet_mode = new QAction( icon, i18n("Trip&let Insert Mode"), dynamic_cast<QObject*>(Qt::Key_G) );
	connect( qa_triplet_mode, SIGNAL(toggled()), dynamic_cast<QObject*>(Qt::Key_G), this, SLOT(slotUpdateInsertModeStatus()) );
	qa_triplet_mode->setObjectName( "triplet_mode" );	//### FIX: deallocate QAction ptr
	qa_triplet_mode->setCheckable( true );	//
	qa_triplet_mode->setAutoRepeat( false );	//
	//qa_triplet_mode->setActionGroup( 0 );	// QActionGroup*
	qa_triplet_mode->setChecked( false );	//
	// )->
        setChecked(false);

    icon = il.load("chord");
    (QAction* qa_chord_mode = new QAction( icon, i18n("C&hord Insert Mode"), dynamic_cast<QObject*>(Qt::Key_H) );
	connect( qa_chord_mode, SIGNAL(toggled()), dynamic_cast<QObject*>(Qt::Key_H), this, SLOT(slotUpdateInsertModeStatus()) );
	qa_chord_mode->setObjectName( "chord_mode" );	//### FIX: deallocate QAction ptr
	qa_chord_mode->setCheckable( true );	//
	qa_chord_mode->setAutoRepeat( false );	//
	//qa_chord_mode->setActionGroup( 0 );	// QActionGroup*
	qa_chord_mode->setChecked( false );	//
	// )->
        setChecked(false);

    icon = il.load("group-grace");
    (QAction* qa_grace_mode = new QAction( icon, i18n("Grace Insert Mode"), dynamic_cast<QObject*>(0) );
	connect( qa_grace_mode, SIGNAL(toggled()), dynamic_cast<QObject*>(0), this, SLOT(slotUpdateInsertModeStatus()) );
	qa_grace_mode->setObjectName( "grace_mode" );	//### FIX: deallocate QAction ptr
	qa_grace_mode->setCheckable( true );	//
	qa_grace_mode->setAutoRepeat( false );	//
	//qa_grace_mode->setActionGroup( 0 );	// QActionGroup*
	qa_grace_mode->setChecked( false );	//
	// )->
        setChecked(false);

    // setup Transforms menu
    QAction* qa_normalize_rests = new QAction(  NormalizeRestsCommand::getGlobalName(), dynamic_cast<QObject*>(this) );
			connect( qa_normalize_rests, SIGNAL(toggled()), dynamic_cast<QObject*>(this), SLOT(slotTransformsNormalizeRests()) );
			qa_normalize_rests->setObjectName( "normalize_rests" );		//
			//qa_normalize_rests->setCheckable( true );		//
			qa_normalize_rests->setAutoRepeat( false );	//
			//qa_normalize_rests->setActionGroup( 0 );		// QActionGroup*
			//qa_normalize_rests->setChecked( false );		//
			//### FIX: deallocate QAction ptr
			

    QAction* qa_collapse_notes = new QAction(  CollapseNotesCommand::getGlobalName(), dynamic_cast<QObject*>(this) );
			connect( qa_collapse_notes, SIGNAL(toggled()), dynamic_cast<QObject*>(this), SLOT(slotTransformsCollapseNotes()) );
			qa_collapse_notes->setObjectName( "collapse_notes" );		//
			//qa_collapse_notes->setCheckable( true );		//
			qa_collapse_notes->setAutoRepeat( false );	//
			//qa_collapse_notes->setActionGroup( 0 );		// QActionGroup*
			//qa_collapse_notes->setChecked( false );		//
			//### FIX: deallocate QAction ptr
			

    icon = il.load
                                         ("quantize");

    QAction* qa_quantize = new QAction(  EventQuantizeCommand::getGlobalName(), dynamic_cast<QObject*>(Qt::Key_Equal) );
			connect( qa_quantize, SIGNAL(toggled()), dynamic_cast<QObject*>(Qt::Key_Equal), this );
			qa_quantize->setObjectName( "quantize" );		//
			//qa_quantize->setCheckable( true );		//
			qa_quantize->setAutoRepeat( false );	//
			//qa_quantize->setActionGroup( 0 );		// QActionGroup*
			//qa_quantize->setChecked( false );		//
			//### FIX: deallocate QAction ptr
			

    QAction* qa_interpret = new QAction(  InterpretCommand::getGlobalName(), dynamic_cast<QObject*>(this) );
			connect( qa_interpret, SIGNAL(toggled()), dynamic_cast<QObject*>(this), SLOT(slotTransformsInterpret()) );
			qa_interpret->setObjectName( "interpret" );		//
			//qa_interpret->setCheckable( true );		//
			qa_interpret->setAutoRepeat( false );	//
			//qa_interpret->setActionGroup( 0 );		// QActionGroup*
			//qa_interpret->setChecked( false );		//
			//### FIX: deallocate QAction ptr
			

    QAction* qa_debug_dump = new QAction(  i18n("&Dump selected events to stderr"), dynamic_cast<QObject*>(this) );
			connect( qa_debug_dump, SIGNAL(toggled()), dynamic_cast<QObject*>(this), SLOT(slotDebugDump()) );
			qa_debug_dump->setObjectName( "debug_dump" );		//
			//qa_debug_dump->setCheckable( true );		//
			qa_debug_dump->setAutoRepeat( false );	//
			//qa_debug_dump->setActionGroup( 0 );		// QActionGroup*
			//qa_debug_dump->setChecked( false );		//
			//### FIX: deallocate QAction ptr
			


    icon = il.load
                                         ("text-mark");

    QAction* qa_make_ornament = new QAction(  i18n("Ma&ke Ornament..."), dynamic_cast<QObject*>(this) );
			connect( qa_make_ornament, SIGNAL(toggled()), dynamic_cast<QObject*>(this), SLOT(slotMakeOrnament()) );
			qa_make_ornament->setObjectName( "make_ornament" );		//
			//qa_make_ornament->setCheckable( true );		//
			qa_make_ornament->setAutoRepeat( false );	//
			//qa_make_ornament->setActionGroup( 0 );		// QActionGroup*
			//qa_make_ornament->setChecked( false );		//
			//### FIX: deallocate QAction ptr
			

    QAction* qa_use_ornament = new QAction(  i18n("Trigger &Ornament..."), dynamic_cast<QObject*>(this) );
			connect( qa_use_ornament, SIGNAL(toggled()), dynamic_cast<QObject*>(this), SLOT(slotUseOrnament()) );
			qa_use_ornament->setObjectName( "use_ornament" );		//
			//qa_use_ornament->setCheckable( true );		//
			qa_use_ornament->setAutoRepeat( false );	//
			//qa_use_ornament->setActionGroup( 0 );		// QActionGroup*
			//qa_use_ornament->setChecked( false );		//
			//### FIX: deallocate QAction ptr
			

    QAction* qa_remove_ornament = new QAction(  i18n("Remove Ornament..."), dynamic_cast<QObject*>(this) );
			connect( qa_remove_ornament, SIGNAL(toggled()), dynamic_cast<QObject*>(this), SLOT(slotRemoveOrnament()) );
			qa_remove_ornament->setObjectName( "remove_ornament" );		//
			//qa_remove_ornament->setCheckable( true );		//
			qa_remove_ornament->setAutoRepeat( false );	//
			//qa_remove_ornament->setActionGroup( 0 );		// QActionGroup*
			//qa_remove_ornament->setChecked( false );		//
			//### FIX: deallocate QAction ptr
			

    QAction* qa_add_clef = new QAction(  ClefInsertionCommand::getGlobalName(), dynamic_cast<QObject*>(this) );
			connect( qa_add_clef, SIGNAL(toggled()), dynamic_cast<QObject*>(this), SLOT(slotEditAddClef()) );
			qa_add_clef->setObjectName( "add_clef" );		//
			//qa_add_clef->setCheckable( true );		//
			qa_add_clef->setAutoRepeat( false );	//
			//qa_add_clef->setActionGroup( 0 );		// QActionGroup*
			//qa_add_clef->setChecked( false );		//
			//### FIX: deallocate QAction ptr
			

    QAction* qa_add_key_signature = new QAction(  KeyInsertionCommand::getGlobalName(), dynamic_cast<QObject*>(this) );
			connect( qa_add_key_signature, SIGNAL(toggled()), dynamic_cast<QObject*>(this), SLOT(slotEditAddKeySignature()) );
			qa_add_key_signature->setObjectName( "add_key_signature" );		//
			//qa_add_key_signature->setCheckable( true );		//
			qa_add_key_signature->setAutoRepeat( false );	//
			//qa_add_key_signature->setActionGroup( 0 );		// QActionGroup*
			//qa_add_key_signature->setChecked( false );		//
			//### FIX: deallocate QAction ptr
			

    QAction* qa_add_sustain_down = new QAction(  SustainInsertionCommand::getGlobalName(true), dynamic_cast<QObject*>(this) );
			connect( qa_add_sustain_down, SIGNAL(toggled()), dynamic_cast<QObject*>(this), SLOT(slotEditAddSustainDown()) );
			qa_add_sustain_down->setObjectName( "add_sustain_down" );		//
			//qa_add_sustain_down->setCheckable( true );		//
			qa_add_sustain_down->setAutoRepeat( false );	//
			//qa_add_sustain_down->setActionGroup( 0 );		// QActionGroup*
			//qa_add_sustain_down->setChecked( false );		//
			//### FIX: deallocate QAction ptr
			

    QAction* qa_add_sustain_up = new QAction(  SustainInsertionCommand::getGlobalName(false), dynamic_cast<QObject*>(this) );
			connect( qa_add_sustain_up, SIGNAL(toggled()), dynamic_cast<QObject*>(this), SLOT(slotEditAddSustainUp()) );
			qa_add_sustain_up->setObjectName( "add_sustain_up" );		//
			//qa_add_sustain_up->setCheckable( true );		//
			qa_add_sustain_up->setAutoRepeat( false );	//
			//qa_add_sustain_up->setActionGroup( 0 );		// QActionGroup*
			//qa_add_sustain_up->setChecked( false );		//
			//### FIX: deallocate QAction ptr
			

    QAction* qa_transpose_segment = new QAction(  TransposeCommand::getDiatonicGlobalName(false), dynamic_cast<QObject*>(this) );
			connect( qa_transpose_segment, SIGNAL(toggled()), dynamic_cast<QObject*>(this), SLOT(slotEditTranspose()) );
			qa_transpose_segment->setObjectName( "transpose_segment" );		//
			//qa_transpose_segment->setCheckable( true );		//
			qa_transpose_segment->setAutoRepeat( false );	//
			//qa_transpose_segment->setActionGroup( 0 );		// QActionGroup*
			//qa_transpose_segment->setChecked( false );		//
			//### FIX: deallocate QAction ptr
			

	QAction* qa_switch_preset = new QAction(  i18n("Convert Notation For..."), dynamic_cast<QObject*>(this) );
			connect( qa_switch_preset, SIGNAL(toggled()), dynamic_cast<QObject*>(this), SLOT(slotEditSwitchPreset()) );
			qa_switch_preset->setObjectName( "switch_preset" );		//
			//qa_switch_preset->setCheckable( true );		//
			qa_switch_preset->setAutoRepeat( false );	//
			//qa_switch_preset->setActionGroup( 0 );		// QActionGroup*
			//qa_switch_preset->setChecked( false );		//
			//### FIX: deallocate QAction ptr
			


    // setup Settings menu
    static QString actionsToolbars[][4] =
        {
            { i18n("Show T&ools Toolbar"), "1slotToggleToolsToolBar()", "show_tools_toolbar", "palette-tools" },
            { i18n("Show &Notes Toolbar"), "1slotToggleNotesToolBar()", "show_notes_toolbar", "palette-notes" },
            { i18n("Show &Rests Toolbar"), "1slotToggleRestsToolBar()", "show_rests_toolbar", "palette-rests" },
            { i18n("Show &Accidentals Toolbar"), "1slotToggleAccidentalsToolBar()", "show_accidentals_toolbar", "palette-accidentals" },
            { i18n("Show Cle&fs Toolbar"), "1slotToggleClefsToolBar()", "show_clefs_toolbar",
              "palette-clefs" },
            { i18n("Show &Marks Toolbar"), "1slotToggleMarksToolBar()", "show_marks_toolbar",
              "palette-marks" },
            { i18n("Show &Group Toolbar"), "1slotToggleGroupToolBar()", "show_group_toolbar",
              "palette-group" },
            { i18n("Show &Layout Toolbar"), "1slotToggleLayoutToolBar()", "show_layout_toolbar",
              "palette-font" },
            { i18n("Show Trans&port Toolbar"), "1slotToggleTransportToolBar()", "show_transport_toolbar",
              "palette-transport" },
            { i18n("Show M&eta Toolbar"), "1slotToggleMetaToolBar()", "show_meta_toolbar",
              "palette-meta" }
        };

    for (unsigned int i = 0;
            i < sizeof(actionsToolbars) / sizeof(actionsToolbars[0]); ++i) {

        icon = il.load(actionsToolbars[i][3])));

        new /* was toggle */ QAction(actionsToolbars[i][0], icon, 0,
                          this, actionsToolbars[i][1],
                          actionCollection(), actionsToolbars[i][2]);
    }

    QAction *qa_cursor_back = new QAction( "Cursor &Back", dynamic_cast<QObject*>(this) ); //### deallocate action ptr 
			qa_cursor_back->setIconText(0); 
			connect( qa_cursor_back, SIGNAL(triggered()), this, SLOT(slotStepBackward())  );

    QAction *qa_cursor_forward = new QAction( "Cursor &Forward", dynamic_cast<QObject*>(this) ); //### deallocate action ptr 
			qa_cursor_forward->setIconText(0); 
			connect( qa_cursor_forward, SIGNAL(triggered()), this, SLOT(slotStepForward())  );

    QAction *qa_cursor_back_bar = new QAction( "Cursor Ba&ck Bar", dynamic_cast<QObject*>(this) ); //### deallocate action ptr 
			qa_cursor_back_bar->setIconText(0); 
			connect( qa_cursor_back_bar, SIGNAL(triggered()), this, SLOT(slotJumpBackward())  );

    QAction *qa_cursor_forward_bar = new QAction( "Cursor For&ward Bar", dynamic_cast<QObject*>(this) ); //### deallocate action ptr 
			qa_cursor_forward_bar->setIconText(0); 
			connect( qa_cursor_forward_bar, SIGNAL(triggered()), this, SLOT(slotJumpForward())  );

    QAction* qa_extend_selection_backward = new QAction(  i18n("Cursor Back and Se&lect"), dynamic_cast<QObject*>(this) );
			connect( qa_extend_selection_backward, SIGNAL(toggled()), dynamic_cast<QObject*>(this), SLOT(slotExtendSelectionBackward()) );
			qa_extend_selection_backward->setObjectName( "extend_selection_backward" );		//
			//qa_extend_selection_backward->setCheckable( true );		//
			qa_extend_selection_backward->setAutoRepeat( false );	//
			//qa_extend_selection_backward->setActionGroup( 0 );		// QActionGroup*
			//qa_extend_selection_backward->setChecked( false );		//
			//### FIX: deallocate QAction ptr
			

    QAction* qa_extend_selection_forward = new QAction(  i18n("Cursor Forward and &Select"), dynamic_cast<QObject*>(this) );
			connect( qa_extend_selection_forward, SIGNAL(toggled()), dynamic_cast<QObject*>(this), SLOT(slotExtendSelectionForward()) );
			qa_extend_selection_forward->setObjectName( "extend_selection_forward" );		//
			//qa_extend_selection_forward->setCheckable( true );		//
			qa_extend_selection_forward->setAutoRepeat( false );	//
			//qa_extend_selection_forward->setActionGroup( 0 );		// QActionGroup*
			//qa_extend_selection_forward->setChecked( false );		//
			//### FIX: deallocate QAction ptr
			

    QAction* qa_extend_selection_backward_bar = new QAction(  i18n("Cursor Back Bar and Select"), dynamic_cast<QObject*>(this) );
			connect( qa_extend_selection_backward_bar, SIGNAL(toggled()), dynamic_cast<QObject*>(this), SLOT(slotExtendSelectionBackwardBar()) );
			qa_extend_selection_backward_bar->setObjectName( "extend_selection_backward_bar" );		//
			//qa_extend_selection_backward_bar->setCheckable( true );		//
			qa_extend_selection_backward_bar->setAutoRepeat( false );	//
			//qa_extend_selection_backward_bar->setActionGroup( 0 );		// QActionGroup*
			//qa_extend_selection_backward_bar->setChecked( false );		//
			//### FIX: deallocate QAction ptr
			

    QAction* qa_extend_selection_forward_bar = new QAction(  i18n("Cursor Forward Bar and Select"), dynamic_cast<QObject*>(this) );
			connect( qa_extend_selection_forward_bar, SIGNAL(toggled()), dynamic_cast<QObject*>(this), SLOT(slotExtendSelectionForwardBar()) );
			qa_extend_selection_forward_bar->setObjectName( "extend_selection_forward_bar" );		//
			//qa_extend_selection_forward_bar->setCheckable( true );		//
			qa_extend_selection_forward_bar->setAutoRepeat( false );	//
			//qa_extend_selection_forward_bar->setActionGroup( 0 );		// QActionGroup*
			//qa_extend_selection_forward_bar->setChecked( false );		//
			//### FIX: deallocate QAction ptr
			

    /*!!! not here yet
        QAction* qa_move_selection_left = new QAction(  i18n("Move Selection Left"), dynamic_cast<QObject*>(this) );
			connect( qa_move_selection_left, SIGNAL(toggled()), dynamic_cast<QObject*>(this), SLOT(slotMoveSelectionLeft()) );
			qa_move_selection_left->setObjectName( "move_selection_left" );		//
			//qa_move_selection_left->setCheckable( true );		//
			qa_move_selection_left->setAutoRepeat( false );	//
			//qa_move_selection_left->setActionGroup( 0 );		// QActionGroup*
			//qa_move_selection_left->setChecked( false );		//
			//### FIX: deallocate QAction ptr
			
    */

    new KAction(i18n("Cursor to St&art"), 0,
                /* #1025717: conflicting meanings for ctrl+a - dupe with Select All
                  Qt::Key_A + Qt::CTRL, */ this, 
                SLOT(slotJumpToStart()), actionCollection(),
                "cursor_start");

    QAction *qa_cursor_end = new QAction( "Cursor to &End", dynamic_cast<QObject*>(this) ); //### deallocate action ptr 
			qa_cursor_end->setIconText(0); 
			connect( qa_cursor_end, SIGNAL(triggered()), this, SLOT(slotJumpToEnd())  );

    QAction *qa_cursor_up_staff = new QAction( "Cursor &Up Staff", dynamic_cast<QObject*>(this) ); //### deallocate action ptr 
			qa_cursor_up_staff->setIconText(0); 
			connect( qa_cursor_up_staff, SIGNAL(triggered()), this, SLOT(slotCurrentStaffUp())  );

    QAction *qa_cursor_down_staff = new QAction( "Cursor &Down Staff", dynamic_cast<QObject*>(this) ); //### deallocate action ptr 
			qa_cursor_down_staff->setIconText(0); 
			connect( qa_cursor_down_staff, SIGNAL(triggered()), this, SLOT(slotCurrentStaffDown())  );

    QAction *qa_cursor_prior_segment = new QAction( "Cursor Pre&vious Segment", dynamic_cast<QObject*>(this) ); //### deallocate action ptr 
			qa_cursor_prior_segment->setIconText(0); 
			connect( qa_cursor_prior_segment, SIGNAL(triggered()), this, SLOT(slotCurrentSegmentPrior())  );

    QAction *qa_cursor_next_segment = new QAction( "Cursor Ne&xt Segment", dynamic_cast<QObject*>(this) ); //### deallocate action ptr 
			qa_cursor_next_segment->setIconText(0); 
			connect( qa_cursor_next_segment, SIGNAL(triggered()), this, SLOT(slotCurrentSegmentNext())  );

    icon = il.load
                    ("transport-cursor-to-pointer");
    QAction *qa_cursor_to_playback_pointer = new QAction( "Cursor to &Playback Pointer", dynamic_cast<QObject*>(this) ); //### deallocate action ptr 
			qa_cursor_to_playback_pointer->setIcon(icon); 
			connect( qa_cursor_to_playback_pointer, SIGNAL(triggered()), this, SLOT(slotJumpCursorToPlayback())  );

    icon = il.load
                    ("transport-play");
    QAction *qa_play = new QAction( "&Play", dynamic_cast<QObject*>(this) ); //### deallocate action ptr 
			qa_play->setIcon(icon); 
			connect( qa_play, SIGNAL(triggered()), this, SIGNAL(play())  );
    // Alternative shortcut for Play
    KShortcut playShortcut = play->shortcut();
    playShortcut.append( KKey(Key_Return + Qt::CTRL) );
    qa_play->setShortcut(playShortcut);

    icon = il.load
                    ("transport-stop");
    QAction *qa_stop = new QAction( "&Stop", dynamic_cast<QObject*>(this) ); //### deallocate action ptr 
			qa_stop->setIcon(icon); 
			connect( qa_stop, SIGNAL(triggered()), this, SIGNAL(stop())  );

    icon = il.load
                    ("transport-rewind");
    QAction *qa_playback_pointer_back_bar = new QAction( "Re&wind", dynamic_cast<QObject*>(this) ); //### deallocate action ptr 
			qa_playback_pointer_back_bar->setIcon(icon); 
			connect( qa_playback_pointer_back_bar, SIGNAL(triggered()), this, SIGNAL(rewindPlayback())  );

    icon = il.load
                    ("transport-ffwd");
    QAction *qa_playback_pointer_forward_bar = new QAction( "&Fast Forward", dynamic_cast<QObject*>(this) ); //### deallocate action ptr 
			qa_playback_pointer_forward_bar->setIcon(icon); 
			connect( qa_playback_pointer_forward_bar, SIGNAL(triggered()), this, SIGNAL(fastForwardPlayback())  );

    icon = il.load
                    ("transport-rewind-end");
    QAction *qa_playback_pointer_start = new QAction( "Rewind to &Beginning", dynamic_cast<QObject*>(this) ); //### deallocate action ptr 
			qa_playback_pointer_start->setIcon(icon); 
			connect( qa_playback_pointer_start, SIGNAL(triggered()), this, SIGNAL(rewindPlaybackToBeginning())  );

    icon = il.load
                    ("transport-ffwd-end");
    QAction *qa_playback_pointer_end = new QAction( "Fast Forward to &End", dynamic_cast<QObject*>(this) ); //### deallocate action ptr 
			qa_playback_pointer_end->setIcon(icon); 
			connect( qa_playback_pointer_end, SIGNAL(triggered()), this, SIGNAL(fastForwardPlaybackToEnd())  );

    icon = il.load
                    ("transport-pointer-to-cursor");
    QAction *qa_playback_pointer_to_cursor = new QAction( "Playback Pointer to &Cursor", dynamic_cast<QObject*>(this) ); //### deallocate action ptr 
			qa_playback_pointer_to_cursor->setIcon(icon); 
			connect( qa_playback_pointer_to_cursor, SIGNAL(triggered()), this, SLOT(slotJumpPlaybackToCursor())  );

    icon = il.load
                    ("transport-solo");
    QAction* qa_toggle_solo = new QAction( icon, i18n("&Solo"), dynamic_cast<QObject*>(0) );
	connect( qa_toggle_solo, SIGNAL(toggled()), dynamic_cast<QObject*>(0), this );
	qa_toggle_solo->setObjectName( "toggle_solo" );	//### FIX: deallocate QAction ptr
	qa_toggle_solo->setCheckable( true );	//
	qa_toggle_solo->setAutoRepeat( false );	//
	//qa_toggle_solo->setActionGroup( 0 );	// QActionGroup*
	qa_toggle_solo->setChecked( false );	//
	// ;

    icon = il.load
                    ("transport-tracking");
    (QAction* qa_toggle_tracking = new QAction( icon, i18n("Scro&ll to Follow Playback"), dynamic_cast<QObject*>(Qt::Key_Pause) );
	connect( qa_toggle_tracking, SIGNAL(toggled()), dynamic_cast<QObject*>(Qt::Key_Pause), this );
	qa_toggle_tracking->setObjectName( "toggle_tracking" );	//### FIX: deallocate QAction ptr
	qa_toggle_tracking->setCheckable( true );	//
	qa_toggle_tracking->setAutoRepeat( false );	//
	//qa_toggle_tracking->setActionGroup( 0 );	// QActionGroup*
	qa_toggle_tracking->setChecked( false );	//
	// )->setChecked(m_playTracking);

    icon = il.load
                    ("transport-panic")));
    QAction *qa_panic = new QAction( "Panic", dynamic_cast<QObject*>(this) ); //### deallocate action ptr 
			qa_panic->setIcon(icon); 
			connect( qa_panic, SIGNAL(triggered()), this, SIGNAL(panic())  );

    QAction* qa_preview_selection = new QAction(  i18n("Set Loop to Selection"), dynamic_cast<QObject*>(this) );
			connect( qa_preview_selection, SIGNAL(toggled()), dynamic_cast<QObject*>(this), SLOT(slotPreviewSelection()) );
			qa_preview_selection->setObjectName( "preview_selection" );		//
			//qa_preview_selection->setCheckable( true );		//
			qa_preview_selection->setAutoRepeat( false );	//
			//qa_preview_selection->setActionGroup( 0 );		// QActionGroup*
			//qa_preview_selection->setChecked( false );		//
			//### FIX: deallocate QAction ptr
			

    QAction* qa_clear_loop = new QAction(  i18n("Clear L&oop"), dynamic_cast<QObject*>(this) );
			connect( qa_clear_loop, SIGNAL(toggled()), dynamic_cast<QObject*>(this), SLOT(slotClearLoop()) );
			qa_clear_loop->setObjectName( "clear_loop" );		//
			//qa_clear_loop->setCheckable( true );		//
			qa_clear_loop->setAutoRepeat( false );	//
			//qa_clear_loop->setActionGroup( 0 );		// QActionGroup*
			//qa_clear_loop->setChecked( false );		//
			//### FIX: deallocate QAction ptr
			

    QAction* qa_clear_selection = new QAction(  i18n("Clear Selection"), dynamic_cast<QObject*>(this) );
			connect( qa_clear_selection, SIGNAL(toggled()), dynamic_cast<QObject*>(this), SLOT(slotClearSelection()) );
			qa_clear_selection->setObjectName( "clear_selection" );		//
			//qa_clear_selection->setCheckable( true );		//
			qa_clear_selection->setAutoRepeat( false );	//
			//qa_clear_selection->setActionGroup( 0 );		// QActionGroup*
			//qa_clear_selection->setChecked( false );		//
			//### FIX: deallocate QAction ptr
			

    //    QString pixmapDir =
    //	KGlobal::dirs()->findResource("appdata", "pixmaps/");
    //    icon = QIcon(Q3CanvasPixmap(pixmapDir + "/toolbar/eventfilter.xpm"));
    icon = il.load("eventfilter");
    QAction *qa_filter_selection = new QAction( "&Filter Selection", icon, dynamic_cast<QObject*>(this) ); //### deallocate action ptr 
			qa_filter_selection->setIconText("filter"); 
			connect( qa_filter_selection, SIGNAL(triggered()), this, SLOT(slotFilterSelection())  );

    QAction* qa_velocity_up = new QAction(  ChangeVelocityCommand::getGlobalName(10), dynamic_cast<QObject*>(Qt::Key_Up + Qt::SHIFT) );
			connect( qa_velocity_up, SIGNAL(toggled()), dynamic_cast<QObject*>(Qt::Key_Up + Qt::SHIFT), this );
			qa_velocity_up->setObjectName( "velocity_up" );		//
			//qa_velocity_up->setCheckable( true );		//
			qa_velocity_up->setAutoRepeat( false );	//
			//qa_velocity_up->setActionGroup( 0 );		// QActionGroup*
			//qa_velocity_up->setChecked( false );		//
			//### FIX: deallocate QAction ptr
			

    QAction* qa_velocity_down = new QAction(  ChangeVelocityCommand::getGlobalName( -10), dynamic_cast<QObject*>(Qt::Key_Down + Qt::SHIFT) );
			connect( qa_velocity_down, SIGNAL(toggled()), dynamic_cast<QObject*>(Qt::Key_Down + Qt::SHIFT), this );
			qa_velocity_down->setObjectName( "velocity_down" );		//
			//qa_velocity_down->setCheckable( true );		//
			qa_velocity_down->setAutoRepeat( false );	//
			//qa_velocity_down->setActionGroup( 0 );		// QActionGroup*
			//qa_velocity_down->setChecked( false );		//
			//### FIX: deallocate QAction ptr
			

    QAction* qa_set_velocities = new QAction(  i18n("Set Event &Velocities..."), dynamic_cast<QObject*>(this) );
			connect( qa_set_velocities, SIGNAL(toggled()), dynamic_cast<QObject*>(this), SLOT(slotSetVelocities()) );
			qa_set_velocities->setObjectName( "set_velocities" );		//
			//qa_set_velocities->setCheckable( true );		//
			qa_set_velocities->setAutoRepeat( false );	//
			//qa_set_velocities->setActionGroup( 0 );		// QActionGroup*
			//qa_set_velocities->setChecked( false );		//
			//### FIX: deallocate QAction ptr
			

    QAction* qa_toggle_dot = new QAction(  i18n("Toggle Dot"), dynamic_cast<QObject*>(this) );
			connect( qa_toggle_dot, SIGNAL(toggled()), dynamic_cast<QObject*>(this), SLOT(slotToggleDot()) );
			qa_toggle_dot->setObjectName( "toggle_dot" );		//
			//qa_toggle_dot->setCheckable( true );		//
			qa_toggle_dot->setAutoRepeat( false );	//
			//qa_toggle_dot->setActionGroup( 0 );		// QActionGroup*
			//qa_toggle_dot->setChecked( false );		//
			//### FIX: deallocate QAction ptr
			

    QAction* qa_add_dot = new QAction(  i18n("Add Dot"), dynamic_cast<QObject*>(this) );
			connect( qa_add_dot, SIGNAL(toggled()), dynamic_cast<QObject*>(this), SLOT(slotAddDot()) );
			qa_add_dot->setObjectName( "add_dot" );		//
			//qa_add_dot->setCheckable( true );		//
			qa_add_dot->setAutoRepeat( false );	//
			//qa_add_dot->setActionGroup( 0 );		// QActionGroup*
			//qa_add_dot->setChecked( false );		//
			//### FIX: deallocate QAction ptr
			

    QAction* qa_add_notation_dot = new QAction(  i18n("Add Dot"), dynamic_cast<QObject*>(this) );
			connect( qa_add_notation_dot, SIGNAL(toggled()), dynamic_cast<QObject*>(this), SLOT(slotAddDotNotationOnly()) );
			qa_add_notation_dot->setObjectName( "add_notation_dot" );		//
			//qa_add_notation_dot->setCheckable( true );		//
			qa_add_notation_dot->setAutoRepeat( false );	//
			//qa_add_notation_dot->setActionGroup( 0 );		// QActionGroup*
			//qa_add_notation_dot->setChecked( false );		//
			//### FIX: deallocate QAction ptr
			

    createGUI(getRCFileName(), false);
}

bool
NotationView::isInChordMode()
{
    return ((/* was toggle */ QAction *)actionCollection()->action("chord_mode"))->
           isChecked();
}

bool
NotationView::isInTripletMode()
{
    return ((/* was toggle */ QAction *)actionCollection()->action("triplet_mode"))->
           isChecked();
}

bool
NotationView::isInGraceMode()
{
    return ((/* was toggle */ QAction *)actionCollection()->action("grace_mode"))->
           isChecked();
}

void
NotationView::setupFontSizeMenu(std::string oldFontName)
{
    if (oldFontName != "") {

        std::vector<int> sizes = NoteFontFactory::getScreenSizes(oldFontName);

        for (unsigned int i = 0; i < sizes.size(); ++i) {
            KAction *action =
                actionCollection()->action
                (QString("note_font_size_%1").arg(sizes[i]));
            m_fontSizeActionMenu->remove
            (action);

            // Don't delete -- that could cause a crash when this
            // function is called from the action itself.  Instead
            // we reuse and reinsert existing actions below.
        }
    }

    std::vector<int> sizes = NoteFontFactory::getScreenSizes(m_fontName);

    for (unsigned int i = 0; i < sizes.size(); ++i) {

        QString actionName = QString("note_font_size_%1").arg(sizes[i]);

        /* was toggle */ QAction *sizeAction = dynamic_cast<QAction*>
                                    (actionCollection()->action(actionName));

        if (!sizeAction) {
            sizeAction = new QAction( "%1 pixels", i18np("1 pixel", dynamic_cast<QObject*>(sizes[i])) );
{
            	connect( sizeAction, SIGNAL(toggled()), dynamic_cast<QObject*>(sizes[i])), 0, this ); {
            
{
            	sizeAction->setObjectName(actionCollection(), actionName);
{
            	sizeAction->setCheckable( true );	//
{
            	sizeAction->setAutoRepeat( false );	//
{
            	//sizeAction->setActionGroup( 0 );	// QActionGroup*
        }

        sizeAction->setChecked(sizes[i] == m_fontSize);
        m_fontSizeActionMenu->insert(sizeAction);
    }
}

LinedStaff *
NotationView::getLinedStaff(int i)
{
    return getNotationStaff(i);
}

LinedStaff *
NotationView::getLinedStaff(const Segment &segment)
{
    return getNotationStaff(segment);
}

NotationStaff *
NotationView::getNotationStaff(const Segment &segment)
{
    for (unsigned int i = 0; i < m_staffs.size(); ++i) {
        if (&(m_staffs[i]->getSegment()) == &segment)
            return m_staffs[i];
    }
    return 0;
}

bool NotationView::isCurrentStaff(int i)
{
    return getCurrentSegment() == &(m_staffs[i]->getSegment());
}

void NotationView::initLayoutToolbar()
{
    KToolBar *layoutToolbar = toolBar("Layout Toolbar");

    if (!layoutToolbar) {
        std::cerr
        << "NotationView::initLayoutToolbar() : layout toolbar not found"
        << std::endl;
        return ;
    }

    new QLabel(i18n("  Font:  "), layoutToolbar, "font label");

    //
    // font combo
    //
    m_fontCombo = new QComboBox(layoutToolbar);
    m_fontCombo->setEditable(false);

    std::set
        <std::string> fs(NoteFontFactory::getFontNames());
    std::vector<std::string> f(fs.begin(), fs.end());
    std::sort(f.begin(), f.end());

    bool foundFont = false;

    for (std::vector<std::string>::iterator i = f.begin(); i != f.end(); ++i) {

        QString fontQName(strtoqstr(*i));

        m_fontCombo->addItem(fontQName);
        if (fontQName.toLower() == strtoqstr(m_fontName).toLower()) {
            m_fontCombo->setCurrentIndex(m_fontCombo->count() - 1);
            foundFont = true;
        }
    }

    if (!foundFont) {
        /* was sorry */ QMessageBox::warning
        (this, i18n("Unknown font \"%1\", using default", 
         strtoqstr(m_fontName)));
        m_fontName = NoteFontFactory::getDefaultFontName();
    }

    connect(m_fontCombo, SIGNAL(activated(const QString &)),
            this, SLOT(slotChangeFont(const QString &)));

    new QLabel(i18n("  Size:  "), layoutToolbar, "size label");

    QString value;

    //
    // font size combo
    //
    std::vector<int> sizes = NoteFontFactory::getScreenSizes(m_fontName);
    m_fontSizeCombo = new QComboBox(layoutToolbar, "font size combo");

    for (std::vector<int>::iterator i = sizes.begin(); i != sizes.end(); ++i) {

        value.setNum(*i);
        m_fontSizeCombo->addItem(value);
    }
    // set combo's current value to default
    value.setNum(m_fontSize);
    m_fontSizeCombo->setItemText(value);

    connect(m_fontSizeCombo, SIGNAL(activated(const QString&)),
            this, SLOT(slotChangeFontSizeFromStringValue(const QString&)));

    new QLabel(i18n("  Spacing:  "), layoutToolbar, "spacing label");

    //
    // spacing combo
    //
    int defaultSpacing = m_hlayout->getSpacing();
    std::vector<int> spacings = NotationHLayout::getAvailableSpacings();

    m_spacingCombo = new QComboBox(layoutToolbar, "spacing combo");
    for (std::vector<int>::iterator i = spacings.begin(); i != spacings.end(); ++i) {

        value.setNum(*i);
        value += "%";
        m_spacingCombo->addItem(value);
    }
    // set combo's current value to default
    value.setNum(defaultSpacing);
    value += "%";
    m_spacingCombo->setItemText(value);

    connect(m_spacingCombo, SIGNAL(activated(const QString&)),
            this, SLOT(slotChangeSpacingFromStringValue(const QString&)));
}

void NotationView::initStatusBar()
{
    KStatusBar* sb = statusBar();

    m_hoveredOverNoteName = new QLabel(sb);
    m_hoveredOverNoteName->setMinimumWidth(32);

    m_hoveredOverAbsoluteTime = new QLabel(sb);
    m_hoveredOverAbsoluteTime->setMinimumWidth(160);

    sb->addWidget(m_hoveredOverAbsoluteTime);
    sb->addWidget(m_hoveredOverNoteName);

    QWidget *hbox = new QWidget(sb);
    QHBoxLayout *hboxLayout = new QHBoxLayout;
    m_currentNotePixmap = new QLabel( hbox );
    hboxLayout->addWidget(m_currentNotePixmap);
    m_currentNotePixmap->setMinimumWidth(20);
    m_insertModeLabel = new QLabel( hbox );
    hboxLayout->addWidget(m_insertModeLabel);
    m_annotationsLabel = new QLabel( hbox );
    hboxLayout->addWidget(m_annotationsLabel);
    m_lilyPondDirectivesLabel = new QLabel( hbox );
    hboxLayout->addWidget(m_lilyPondDirectivesLabel);
    hbox->setLayout(hboxLayout);
    sb->addWidget(hbox);

    sb->addItem(KTmpStatusMsg::getDefaultMsg(),
                   KTmpStatusMsg::getDefaultId(), 1);
    sb->setItemAlignment(KTmpStatusMsg::getDefaultId(),
                         AlignLeft | AlignVCenter);

    m_selectionCounter = new QLabel(sb);
    sb->addWidget(m_selectionCounter);

    m_progressBar() = new ProgressBar(100, true, sb);
    m_progressBar()->setMinimumWidth(100);
    sb->addWidget(m_progressBar());
}

QSize NotationView::getViewSize()
{
    return canvas()->size();
}

void NotationView::setViewSize(QSize s)
{
    canvas()->resize(s.width(), s.height());

    if (   (m_pageMode == LinedStaff::LinearMode)
        && (m_showHeadersGroup != HeadersGroup::ShowNever)) {
        m_headersGroup->completeToHeight(s.height());
    }
}

void
NotationView::setPageMode(LinedStaff::PageMode pageMode)
{
    m_pageMode = pageMode;

    if (pageMode != LinedStaff::LinearMode) {
        if (m_topStandardRuler)
            m_topStandardRuler->hide();
        if (m_bottomStandardRuler)
            m_bottomStandardRuler->hide();
        if (m_chordNameRuler)
            m_chordNameRuler->hide();
        if (m_rawNoteRuler)
            m_rawNoteRuler->hide();
        if (m_tempoRuler)
            m_tempoRuler->hide();
        hideHeadersGroup();
    } else {
        if (m_topStandardRuler)
            m_topStandardRuler->show();
        if (m_bottomStandardRuler)
            m_bottomStandardRuler->show();
        if (m_chordNameRuler && getToggleAction("show_chords_ruler")->isChecked())
            m_chordNameRuler->show();
        if (m_rawNoteRuler && getToggleAction("show_raw_note_ruler")->isChecked())
            m_rawNoteRuler->show();
        if (m_tempoRuler && getToggleAction("show_tempo_ruler")->isChecked())
            m_tempoRuler->show();
        showHeadersGroup();
    }

    stateChanged("linear_mode",
                 (pageMode == LinedStaff::LinearMode ? KXMLGUIClient::StateNoReverse :
                  KXMLGUIClient::StateReverse));

    int pageWidth = getPageWidth();
    int topMargin = 0, leftMargin = 0;
    getPageMargins(leftMargin, topMargin);

    m_hlayout->setPageMode(pageMode != LinedStaff::LinearMode);
    m_hlayout->setPageWidth(pageWidth - leftMargin * 2);

    NOTATION_DEBUG << "NotationView::setPageMode: set layout's page width to "
    << (pageWidth - leftMargin * 2) << endl;

    positionStaffs();

    bool layoutApplied = applyLayout();
    if (!layoutApplied)
        /* was sorry */ QMessageBox::warning(0, "Couldn't apply layout");
    else {
        for (unsigned int i = 0; i < m_staffs.size(); ++i) {
            m_staffs[i]->markChanged();
        }
    }

    if (!m_printMode) {
        // Layout is done : Time related to left of canvas should now
        // correctly be determined and track headers contents be drawn.
        m_headersGroup->slotUpdateAllHeaders(0, 0, true);
    }

    positionPages();

    if (!m_printMode) {
        updateView();
        slotSetInsertCursorPosition(getInsertionTime(), false, false);
        slotSetPointerPosition(getDocument()->getComposition().getPosition(), false);
    }

    Profiles::getInstance()->dump();
}

int
NotationView::getPageWidth()
{
    if (m_pageMode != LinedStaff::MultiPageMode) {

        if (isInPrintMode() && getCanvasView() && getCanvasView()->canvas())
            return getCanvasView()->canvas()->width();

        if (getCanvasView()) {
            return
                getCanvasView()->width() -
                getCanvasView()->verticalScrollBar()->width() -
                m_leftGutter - 10;
        }

        return width() - 50;

    } else {

        //!!! For the moment we use A4 for this calculation

        double printSizeMm = 25.4 * ((double)m_printSize / 72.0);
        double mmPerPixel = printSizeMm / (double)m_notePixmapFactory->getSize();
        return (int)(210.0 / mmPerPixel);
    }
}

int
NotationView::getPageHeight()
{
    if (m_pageMode != LinedStaff::MultiPageMode) {

        if (isInPrintMode() && getCanvasView() && getCanvasView()->canvas())
            return getCanvasView()->canvas()->height();

        if (getCanvasView()) {
            return getCanvasView()->height();
        }

        return (height() > 200 ? height() - 100 : height());

    } else {

        //!!! For the moment we use A4 for this calculation

        double printSizeMm = 25.4 * ((double)m_printSize / 72.0);
        double mmPerPixel = printSizeMm / (double)m_notePixmapFactory->getSize();
        return (int)(297.0 / mmPerPixel);
    }
}

void
NotationView::getPageMargins(int &left, int &top)
{
    if (m_pageMode != LinedStaff::MultiPageMode) {

        left = 0;
        top = 0;

    } else {

        //!!! For the moment we use A4 for this calculation

        double printSizeMm = 25.4 * ((double)m_printSize / 72.0);
        double mmPerPixel = printSizeMm / (double)m_notePixmapFactory->getSize();
        left = (int)(20.0 / mmPerPixel);
        top = (int)(15.0 / mmPerPixel);
    }
}

void
NotationView::scrollToTime(timeT t)
{

    double notationViewLayoutCoord = m_hlayout->getXForTime(t);

    // Doesn't appear to matter which staff we use
    //!!! actually it probably does matter, if they don't have the same extents
    double notationViewCanvasCoord =
        getLinedStaff(0)->getCanvasCoordsForLayoutCoords
        (notationViewLayoutCoord, 0).first;

    // HK: I could have sworn I saw a hard-coded scroll happen somewhere
    // (i.e. a default extra scroll to make up for the staff not beginning on
    // the left edge) but now I can't find it.
    getCanvasView()->slotScrollHorizSmallSteps
    (int(notationViewCanvasCoord)); // + DEFAULT_STAFF_OFFSET);
}

RulerScale*
NotationView::getHLayout()
{
    return m_hlayout;
}

void
NotationView::paintEvent(QPaintEvent *e)
{
    m_inPaintEvent = true;

    // This is duplicated here from EditViewBase, because (a) we need
    // to know about the segment being removed before we try to check
    // the staff names etc., and (b) it's not safe to call close()
    // from EditViewBase::paintEvent if we're then going to try to do
    // some more work afterwards in this function

    if (isCompositionModified()) {

        // Check if one of the segments we display has been removed
        // from the composition.
        //
        // For the moment we'll have to close the view if any of the
        // segments we handle has been deleted.

        for (unsigned int i = 0; i < m_segments.size(); ++i) {

            if (!m_segments[i]->getComposition()) {
                // oops, I think we've been deleted
                close();
                return ;
            }
        }
    }

    int topMargin = 0, leftMargin = 0;
    getPageMargins(leftMargin, topMargin);

    if (m_pageMode == LinedStaff::ContinuousPageMode) {
        // relayout if the window width changes significantly in continuous page mode
        int diff = int(getPageWidth() - leftMargin * 2 - m_hlayout->getPageWidth());
        if (diff < -10 || diff > 10) {
            setPageMode(m_pageMode);
            refreshSegment(0, 0, 0);
        }

    } else if (m_pageMode == LinedStaff::LinearMode) {
        // resize canvas again if the window height has changed significantly
        if (getCanvasView() && getCanvasView()->canvas()) {
            int diff = int(getPageHeight() - getCanvasView()->canvas()->height());
            if (diff > 10) {
                readjustCanvasSize();
            }
        }
    }

    // check for staff name changes
    for (unsigned int i = 0; i < m_staffs.size(); ++i) {
        if (!m_staffs[i]->isStaffNameUpToDate()) {
            refreshSegment(0);
            break;
        }
    }

    m_inPaintEvent = false;

    EditView::paintEvent(e);

    m_inPaintEvent = false;

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

    slotSetOperationNameAndStatus(i18n("  Ready."));
}

bool NotationView::applyLayout(int staffNo, timeT startTime, timeT endTime)
{
    slotSetOperationNameAndStatus(i18n("Laying out score..."));
    ProgressDialog::processEvents();

    m_hlayout->setStaffCount(m_staffs.size());

    Profiler profiler("NotationView::applyLayout");
    unsigned int i;

    for (i = 0; i < m_staffs.size(); ++i) {

        if (staffNo >= 0 && (int)i != staffNo)
            continue;

        slotSetOperationNameAndStatus(i18n("Laying out staff %1...", i + 1));
        ProgressDialog::processEvents();

        m_hlayout->resetStaff(*m_staffs[i], startTime, endTime);
        m_vlayout->resetStaff(*m_staffs[i], startTime, endTime);
        m_hlayout->scanStaff(*m_staffs[i], startTime, endTime);
        m_vlayout->scanStaff(*m_staffs[i], startTime, endTime);
    }

    slotSetOperationNameAndStatus(i18n("Reconciling staffs..."));
    ProgressDialog::processEvents();

    m_hlayout->finishLayout(startTime, endTime);
    m_vlayout->finishLayout(startTime, endTime);

    // find the last finishing staff for future use

    timeT lastFinishingStaffEndTime = 0;
    bool haveEndTime = false;
    m_lastFinishingStaff = -1;

    timeT firstStartingStaffStartTime = 0;
    bool haveStartTime = false;
    int firstStartingStaff = -1;

    for (i = 0; i < m_staffs.size(); ++i) {

        timeT thisStartTime = m_staffs[i]->getSegment().getStartTime();
        if (thisStartTime < firstStartingStaffStartTime || !haveStartTime) {
            firstStartingStaffStartTime = thisStartTime;
            haveStartTime = true;
            firstStartingStaff = i;
        }

        timeT thisEndTime = m_staffs[i]->getSegment().getEndTime();
        if (thisEndTime > lastFinishingStaffEndTime || !haveEndTime) {
            lastFinishingStaffEndTime = thisEndTime;
            haveEndTime = true;
            m_lastFinishingStaff = i;
        }
    }

    readjustCanvasSize();
    if (m_topStandardRuler) {
        m_topStandardRuler->update();
    }
    if (m_bottomStandardRuler) {
        m_bottomStandardRuler->update();
    }
    if (m_tempoRuler && m_tempoRuler->isVisible()) {
        m_tempoRuler->update();
    }
    if (m_rawNoteRuler && m_rawNoteRuler->isVisible()) {
        m_rawNoteRuler->update();
    }

    return true;
}

void NotationView::setCurrentSelectedNote(const char *pixmapName,
                                          bool rest, Note::Type n, int dots)
{
    if (rest) {
        RestInserter* restInserter = dynamic_cast<RestInserter*>(m_toolBox->getTool(RestInserter::ToolName));

        restInserter->slotSetNote(n);
        restInserter->slotSetDots(dots);
        setTool(restInserter);
    } else {
        NoteInserter* noteInserter = dynamic_cast<NoteInserter*>(m_toolBox->getTool(NoteInserter::ToolName));

        noteInserter->slotSetNote(n);
        noteInserter->slotSetDots(dots);
        setTool(noteInserter);
    }

    m_currentNotePixmap->setPixmap
        (NotePixmapFactory::toQPixmap
         (NotePixmapFactory::makeToolbarPixmap(pixmapName, true)));

    emit changeCurrentNote(rest, n);
}

void NotationView::setCurrentSelectedNote(const NoteActionData &noteAction)
{
    setCurrentSelectedNote(noteAction.pixmapName,
                           noteAction.rest,
                           noteAction.noteType,
                           noteAction.dots);
}

void NotationView::setCurrentSelection(EventSelection* s, bool preview,
                                       bool redrawNow)
{
    //!!! rather too much here shared with matrixview -- could much of
    // this be in editview?

    if (m_currentEventSelection == s)
        return ;
    NOTATION_DEBUG << "XXX " << endl;

    EventSelection *oldSelection = m_currentEventSelection;
    m_currentEventSelection = s;

    // positionElements is overkill here, but we hope it's not too
    // much overkill (if that's not a contradiction)

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

    // play previews if appropriate -- also permits an optimisation
    // for the case where the selection is unchanged (quite likely
    // when sweeping)

    if (s && preview) {

        bool foundNewEvent = false;

        for (EventSelection::eventcontainer::iterator i =
                    s->getSegmentEvents().begin();
                i != s->getSegmentEvents().end(); ++i) {

            if (oldSelection && oldSelection->getSegment() == s->getSegment()
                    && oldSelection->contains(*i))
                continue;

            foundNewEvent = true;

            long pitch;
            if (!(*i)->get
                    <Int>(BaseProperties::PITCH,
                          pitch)) continue;

            long velocity = -1;
            (void)(*i)->get
            <Int>(BaseProperties::VELOCITY,
                  velocity);

            if (!((*i)->has(BaseProperties::TIED_BACKWARD) &&
                    (*i)->get
                    <Bool>
                    (BaseProperties::TIED_BACKWARD)))
                playNote(s->getSegment(), pitch, velocity);
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

        if (!s || !oldSelection ||
                (endA >= startB && endB >= startA &&
                 oldSelection->getSegment() == s->getSegment())) {

            // the regions overlap: use their union and just do one refresh

            Segment &segment(s ? s->getSegment() :
                             oldSelection->getSegment());

            if (redrawNow) {
                // recolour the events now
                getLinedStaff(segment)->positionElements(std::min(startA, startB),
                        std::max(endA, endB));
            } else {
                // mark refresh status and then request a repaint
                segment.getRefreshStatus
                (m_segmentsRefreshStatusIds
                 [getLinedStaff(segment)->getId()]).
                push(std::min(startA, startB), std::max(endA, endB));
            }

        } else {
            // do two refreshes, one for each -- here we know neither is null

            if (redrawNow) {
                // recolour the events now
                getLinedStaff(oldSelection->getSegment())->positionElements(startA,
                        endA);

                getLinedStaff(s->getSegment())->positionElements(startB, endB);
            } else {
                // mark refresh status and then request a repaint

                oldSelection->getSegment().getRefreshStatus
                (m_segmentsRefreshStatusIds
                 [getLinedStaff(oldSelection->getSegment())->getId()]).
                push(startA, endA);

                s->getSegment().getRefreshStatus
                (m_segmentsRefreshStatusIds
                 [getLinedStaff(s->getSegment())->getId()]).
                push(startB, endB);
            }
        }

        if (s) {
            // make the staff containing the selection current
            int staffId = getLinedStaff(s->getSegment())->getId();
            if (staffId != m_currentStaff)
                slotSetCurrentStaff(staffId);
        }
    }

    delete oldSelection;

    statusBar()->changeItem(KTmpStatusMsg::getDefaultMsg(),
                            KTmpStatusMsg::getDefaultId());

    if (s) {
        int eventsSelected = s->getSegmentEvents().size();
        m_selectionCounter->setText
        (i18np("  1 event selected ",
              "  %1 events selected ", eventsSelected));
    } else {
        m_selectionCounter->setText(i18n("  No selection "));
    }
    m_selectionCounter->update();

    setMenuStates();

    if (redrawNow)
        updateView();
    else
        update();

    NOTATION_DEBUG << "XXX " << endl;
}

void NotationView::setSingleSelectedEvent(int staffNo, Event *event,
        bool preview, bool redrawNow)
{
    setSingleSelectedEvent(getStaff(staffNo)->getSegment(), event,
                           preview, redrawNow);
}

void NotationView::setSingleSelectedEvent(Segment &segment, Event *event,
        bool preview, bool redrawNow)
{
    EventSelection *selection = new EventSelection(segment);
    selection->addEvent(event);
    setCurrentSelection(selection, preview, redrawNow);
}

bool NotationView::canPreviewAnotherNote()
{
    static time_t lastCutOff = 0;
    static int sinceLastCutOff = 0;

    time_t now = time(0);
    ++sinceLastCutOff;

    if ((now - lastCutOff) > 0) {
        sinceLastCutOff = 0;
        lastCutOff = now;
        NOTATION_DEBUG << "NotationView::canPreviewAnotherNote: reset" << endl;
    } else {
        if (sinceLastCutOff >= 20) {
            // don't permit more than 20 notes per second or so, to
            // avoid gungeing up the sound drivers
            NOTATION_DEBUG << "Rejecting preview (too busy)" << endl;
            return false;
        }
        NOTATION_DEBUG << "NotationView::canPreviewAnotherNote: ok" << endl;
    }

    return true;
}

void NotationView::playNote(Segment &s, int pitch, int velocity)
{
    Composition &comp = getDocument()->getComposition();
    Studio &studio = getDocument()->getStudio();
    Track *track = comp.getTrackById(s.getTrack());

    Instrument *ins =
        studio.getInstrumentById(track->getInstrument());

    // check for null instrument
    //
    if (ins == 0)
        return ;

    if (!canPreviewAnotherNote())
        return ;

    if (velocity < 0)
        velocity = MidiMaxValue;

    MappedEvent mE(ins->getId(),
                   MappedEvent::MidiNoteOneShot,
                   pitch + s.getTranspose(),
                   velocity,
                   RealTime::zeroTime,
                   RealTime(0, 250000000),
                   RealTime::zeroTime);

    StudioControl::sendMappedEvent(mE);
}

void NotationView::showPreviewNote(int staffNo, double layoutX,
                                   int pitch, int height,
                                   const Note &note, bool grace,
                                   int velocity)
{
    m_staffs[staffNo]->showPreviewNote(layoutX, height, note, grace);
    playNote(m_staffs[staffNo]->getSegment(), pitch, velocity);
}

void NotationView::clearPreviewNote()
{
    for (unsigned int i = 0; i < m_staffs.size(); ++i) {
        m_staffs[i]->clearPreviewNote();
    }
}

void NotationView::setNotePixmapFactory(NotePixmapFactory* f)
{
    delete m_notePixmapFactory;
    m_notePixmapFactory = f;
    if (m_hlayout)
        m_hlayout->setNotePixmapFactory(m_notePixmapFactory);
    if (m_vlayout)
        m_vlayout->setNotePixmapFactory(m_notePixmapFactory);
}

Segment *
NotationView::getCurrentSegment()
{
    Staff *staff = getCurrentStaff();
    return (staff ? &staff->getSegment() : 0);
}

bool
NotationView::hasSegment(Segment *segment)
{
    for (unsigned int i = 0; i < m_segments.size(); ++i) {
	if (segment == m_segments[i]) return true;
    }
    return false;
}


LinedStaff *
NotationView::getCurrentLinedStaff()
{
    return getLinedStaff(m_currentStaff);
}

LinedStaff *
NotationView::getStaffAbove()
{
    if (m_staffs.size() < 2) return 0;

    Composition *composition =
        m_staffs[m_currentStaff]->getSegment().getComposition();

    Track *track = composition->
        getTrackById(m_staffs[m_currentStaff]->getSegment().getTrack());
    if (!track) return 0;

    int position = track->getPosition();
    Track *newTrack = 0;

    while ((newTrack = composition->getTrackByPosition(--position))) {
        for (unsigned int i = 0; i < m_staffs.size(); ++i) {
            if (m_staffs[i]->getSegment().getTrack() == newTrack->getId()) {
                return m_staffs[i];
            }
        }
    }

    return 0;
}

LinedStaff *
NotationView::getStaffBelow()
{
    if (m_staffs.size() < 2) return 0;

    Composition *composition =
        m_staffs[m_currentStaff]->getSegment().getComposition();

    Track *track = composition->
        getTrackById(m_staffs[m_currentStaff]->getSegment().getTrack());
    if (!track) return 0;

    int position = track->getPosition();
    Track *newTrack = 0;

    while ((newTrack = composition->getTrackByPosition(++position))) {
        for (unsigned int i = 0; i < m_staffs.size(); ++i) {
            if (m_staffs[i]->getSegment().getTrack() == newTrack->getId()) {
                return m_staffs[i];
            }
        }
    }

    return 0;
}

timeT
NotationView::getInsertionTime()
{
    return m_insertionTime;
}

timeT
NotationView::getInsertionTime(Clef &clef,
                               Rosegarden::Key &key)
{
    // This fuss is solely to recover the clef and key: we already
    // set m_insertionTime to the right value when we first placed
    // the insert cursor.  We could get clef and key directly from
    // the segment but the staff has a more efficient lookup

    LinedStaff *staff = m_staffs[m_currentStaff];
    double layoutX = staff->getLayoutXOfInsertCursor();
    if (layoutX < 0) layoutX = 0;
    Event *clefEvt = 0, *keyEvt = 0;
    (void)staff->getElementUnderLayoutX(layoutX, clefEvt, keyEvt);

    if (clefEvt) clef = Clef(*clefEvt);
    else clef = Clef();

    if (keyEvt) key = Rosegarden::Key(*keyEvt);
    else key = Rosegarden::Key();

    return m_insertionTime;
}

LinedStaff*
NotationView::getStaffForCanvasCoords(int x, int y) const
{
    // (i)  Do not change staff, if mouse was clicked within the current staff.
    LinedStaff *s = m_staffs[m_currentStaff];
    if (s->containsCanvasCoords(x, y)) {
        LinedStaff::LinedStaffCoords coords =
            s->getLayoutCoordsForCanvasCoords(x, y);

        timeT t = m_hlayout->getTimeForX(coords.first);
	// In order to find the correct starting and ending bar of the segment,
	// make infinitesimal shifts (+1 and -1) towards its center.
	timeT t0 = getDocument()->getComposition().getBarStartForTime(m_staffs[m_currentStaff]->getSegment().getStartTime()+1);
	timeT t1 = getDocument()->getComposition().getBarEndForTime(m_staffs[m_currentStaff]->getSegment().getEndTime()-1);
        if (t >= t0 && t < t1) {
            return m_staffs[m_currentStaff];
        }
    }
    // (ii) Find staff under cursor, if clicked outside the current staff.
    for (unsigned int i = 0; i < m_staffs.size(); ++i) {

        LinedStaff *s = m_staffs[i];

        if (s->containsCanvasCoords(x, y)) {

            LinedStaff::LinedStaffCoords coords =
                s->getLayoutCoordsForCanvasCoords(x, y);

	    timeT t = m_hlayout->getTimeForX(coords.first);
	    // In order to find the correct starting and ending bar of the segment,
	    // make infinitesimal shifts (+1 and -1) towards its center.
	    timeT t0 = getDocument()->getComposition().getBarStartForTime(m_staffs[i]->getSegment().getStartTime()+1);
	    timeT t1 = getDocument()->getComposition().getBarEndForTime(m_staffs[i]->getSegment().getEndTime()-1);
	    if (t >= t0 && t < t1) {
                return m_staffs[i];
            }
        }
    }

    return 0;
}

void NotationView::updateView()
{
    slotCheckRendered
    (getCanvasView()->contentsX(),
     getCanvasView()->contentsX() + getCanvasView()->visibleWidth());
    canvas()->update();
}

void NotationView::print(bool previewOnly)
{
    if (m_staffs.size() == 0) {
        QMessageBox::critical(0, "", i18n("Nothing to print"));
        return ;
    }

    Profiler profiler("NotationView::print");

    // We need to be in multi-page mode at this point

    int pageWidth = getPageWidth();
    int pageHeight = getPageHeight();
    int leftMargin = 0, topMargin = 0;
    getPageMargins(leftMargin, topMargin);
    int maxPageCount = 1;

    for (unsigned int i = 0; i < m_staffs.size(); ++i) {
        int pageCount = m_staffs[i]->getPageCount();
        NOTATION_DEBUG << "NotationView::print(): staff " << i << " reports " << pageCount << " pages " << endl;
        if (pageCount > maxPageCount)
            maxPageCount = pageCount;
    }

    KPrinter printer(true, QPrinter::HighResolution);

    printer.setPageSelection(KPrinter::ApplicationSide);
    printer.setMinMax(1, maxPageCount + 1);

    if (previewOnly)
        printer.setPreviewOnly(true);
    else if (!printer.setup((QWidget *)parent()))
        return ;

    QPaintDeviceMetrics pdm(&printer);
    QPainter printpainter(&printer);

    // Ideally we should aim to retain the aspect ratio and to move the
    // staffs so as to be centred after scaling.  But because we haven't
    // got around to the latter, let's lose the former too and just
    // expand to fit.

    // Retain aspect ratio when scaling
    double ratioX = (double)pdm.width() / (double)(pageWidth - leftMargin * 2),
                    ratioY = (double)pdm.height() / (double)(pageHeight - topMargin * 2);
    double ratio = std::min(ratioX, ratioY);
    printpainter.scale(ratio, ratio);

    //    printpainter.scale((double)pdm.width()  / (double)(pageWidth - leftMargin*2),
    //		       (double)pdm.height() / (double)(pageHeight - topMargin*2));
    printpainter.translate( -leftMargin, -topMargin);

    QLinkedList<int> pages = printer.pageList();

    for (QLinkedList<int>::Iterator pli = pages.begin();
            pli != pages.end(); ) { // incremented just below

        int page = *pli - 1;
        ++pli;
        if (page < 0 || page >= maxPageCount)
            continue;

        NOTATION_DEBUG << "Printing page " << page << endl;

        QRect pageRect(m_leftGutter + leftMargin + pageWidth * page,
                       topMargin,
                       pageWidth - leftMargin,
                       pageHeight - topMargin);

        for (size_t i = 0; i < m_staffs.size(); ++i) {

            LinedStaff *staff = m_staffs[i];

            LinedStaff::LinedStaffCoords cc0 = staff->getLayoutCoordsForCanvasCoords
                                               (pageRect.x(), pageRect.y());

            LinedStaff::LinedStaffCoords cc1 = staff->getLayoutCoordsForCanvasCoords
                                               (pageRect.x() + pageRect.width(), pageRect.y() + pageRect.height());

            timeT t0 = m_hlayout->getTimeForX(cc0.first);
            timeT t1 = m_hlayout->getTimeForX(cc1.first);

            m_staffs[i]->setPrintPainter(&printpainter);
            m_staffs[i]->checkRendered(t0, t1);
        }

        // Supplying doublebuffer==true to this method appears to
        // slow down printing considerably but without it we get
        // all sorts of horrible artifacts (possibly related to
        // mishandling of pixmap masks?) in qt-3.0.  Let's permit
        // it as a "hidden" option.

        QSettings settings;
        settings.beginGroup( NotationViewConfigGroup );

        NOTATION_DEBUG << "NotationView::print: calling Q3Canvas::drawArea" << endl;

        {
            Profiler profiler("NotationView::print(Q3Canvas::drawArea)");

            if ( qStrToBool( settings.value("forcedoublebufferprinting", "false" ) ) ) {
                getCanvasView()->canvas()->drawArea(pageRect, &printpainter, true);
            } else {
#if QT_VERSION >= 0x030100
                getCanvasView()->canvas()->drawArea(pageRect, &printpainter, false);
#else

                getCanvasView()->canvas()->drawArea(pageRect, &printpainter, true);
#endif

            }

        }
        settings.endGroup();

        NOTATION_DEBUG << "NotationView::print: Q3Canvas::drawArea done" << endl;

        for (size_t i = 0; i < m_staffs.size(); ++i) {

            LinedStaff *staff = m_staffs[i];

            LinedStaff::LinedStaffCoords cc0 = staff->getLayoutCoordsForCanvasCoords
                                               (pageRect.x(), pageRect.y());

            LinedStaff::LinedStaffCoords cc1 = staff->getLayoutCoordsForCanvasCoords
                                               (pageRect.x() + pageRect.width(), pageRect.y() + pageRect.height());

            timeT t0 = m_hlayout->getTimeForX(cc0.first);
            timeT t1 = m_hlayout->getTimeForX(cc1.first);

            m_staffs[i]->renderPrintable(t0, t1);
        }

        printpainter.translate( -pageWidth, 0);

        if (pli != pages.end() && *pli - 1 < maxPageCount)
            printer.newPage();

        for (size_t i = 0; i < m_staffs.size(); ++i) {
            m_staffs[i]->markChanged(); // recover any memory used for this page
            PixmapArrayGC::deleteAll();
        }
    }

    for (size_t i = 0; i < m_staffs.size(); ++i) {
        for (Segment::iterator j = m_staffs[i]->getSegment().begin();
                j != m_staffs[i]->getSegment().end(); ++j) {
            removeViewLocalProperties(*j);
        }
        delete m_staffs[i];
    }
    m_staffs.clear();

    printpainter.end();

    Profiles::getInstance()->dump();
}

void
NotationView::updateThumbnails(bool complete)
{
    if (m_pageMode != LinedStaff::MultiPageMode)
        return ;

    int pageWidth = getPageWidth();
    int pageHeight = getPageHeight();
    int leftMargin = 0, topMargin = 0;
    getPageMargins(leftMargin, topMargin);
    int maxPageCount = 1;

    for (unsigned int i = 0; i < m_staffs.size(); ++i) {
        int pageCount = m_staffs[i]->getPageCount();
        if (pageCount > maxPageCount)
            maxPageCount = pageCount;
    }

    int thumbScale = 20;
    QPixmap thumbnail(canvas()->width() / thumbScale,
                      canvas()->height() / thumbScale);
    thumbnail.fill(QColor(Qt::white));
    QPainter thumbPainter(&thumbnail);

    if (complete) {

        thumbPainter.scale(1.0 / double(thumbScale), 1.0 / double(thumbScale));
        thumbPainter.setPen(QColor(Qt::black));
        thumbPainter.setBrush(QColor(Qt::white));

        /*
        	Q3Canvas *canvas = getCanvasView()->canvas();
        	canvas->drawArea(QRect(0, 0, canvas->width(), canvas->height()),
        			 &thumbPainter, false);
        */ 
        // hide small texts, as we get a crash in Xft when trying to
        // render them at this scale
        if (m_title)
            m_title->hide();
        if (m_subtitle)
            m_subtitle->hide();
        if (m_composer)
            m_composer->hide();
        if (m_copyright)
            m_copyright->hide();

        for (size_t page = 0; page < static_cast<size_t>(maxPageCount); ++page) {

            bool havePageNumber = ((m_pageNumbers.size() > page) &&
                                   (m_pageNumbers[page] != 0));
            if (havePageNumber)
                m_pageNumbers[page]->hide();

            QRect pageRect(m_leftGutter + leftMargin * 2 + pageWidth * page,
                           topMargin * 2,
                           pageWidth - leftMargin*3,
                           pageHeight - topMargin*3);

            Q3Canvas *canvas = getCanvasView()->canvas();
            canvas->drawArea(pageRect, &thumbPainter, false);

            if (havePageNumber)
                m_pageNumbers[page]->show();
        }

        if (m_title)
            m_title->show();
        if (m_subtitle)
            m_subtitle->show();
        if (m_composer)
            m_composer->show();
        if (m_copyright)
            m_copyright->show();

    } else {

        thumbPainter.setPen(QColor(Qt::black));

        for (int page = 0; page < maxPageCount; ++page) {

            int x = m_leftGutter + pageWidth * page + leftMargin / 4;
            int y = 20;
            int w = pageWidth - leftMargin / 2;
            int h = pageHeight;

            QString str = QString("%1").arg(page + 1);

            thumbPainter.drawRect(x / thumbScale, y / thumbScale,
                                  w / thumbScale, h / thumbScale);

            int tx = (x + w / 2) / thumbScale, ty = (y + h / 2) / thumbScale;
            tx -= thumbPainter.fontMetrics().width(str) / 2;
            thumbPainter.drawText(tx, ty, str);
        }
    }

    thumbPainter.end();
    if (m_pannerDialog)
        m_pannerDialog->scrollbox()->setThumbnail(thumbnail);
}

void NotationView::refreshSegment(Segment *segment,
                                  timeT startTime, timeT endTime)
{
    NOTATION_DEBUG << "*** " << endl;

    if (m_inhibitRefresh)
        return ;
    NOTATION_DEBUG << "NotationView::refreshSegment(" << segment << "," << startTime << "," << endTime << ")" << endl;
    Profiler foo("NotationView::refreshSegment");

    emit usedSelection();

    if (segment) {
        LinedStaff *staff = getLinedStaff(*segment);
        if (staff)
            applyLayout(staff->getId(), startTime, endTime);
    } else {
        applyLayout( -1, startTime, endTime);
    }

    for (unsigned int i = 0; i < m_staffs.size(); ++i) {

        Segment *ssegment = &m_staffs[i]->getSegment();
        bool thisStaff = (ssegment == segment || segment == 0);
        m_staffs[i]->markChanged(startTime, endTime, !thisStaff);
    }

    PixmapArrayGC::deleteAll();

    statusBar()->changeItem(KTmpStatusMsg::getDefaultMsg(),
                            KTmpStatusMsg::getDefaultId());

    Event::dumpStats(std::cerr);
    if (m_deferredCursorMove == NoCursorMoveNeeded) {
        slotSetInsertCursorPosition(getInsertionTime(), false, false);
    } else {
        doDeferredCursorMove();
    }
    slotSetPointerPosition(getDocument()->getComposition().getPosition(), false);

    if (m_currentEventSelection &&
            m_currentEventSelection->getSegmentEvents().size() == 0) {
        delete m_currentEventSelection;
        m_currentEventSelection = 0;
        //!!!??? was that the right thing to do?
    }

    setMenuStates();
    slotSetOperationNameAndStatus(i18n("  Ready."));
    NOTATION_DEBUG << "*** " << endl;
}

void NotationView::setMenuStates()
{
    // 1. set selection-related states

    // Clear states first, then enter only those ones that apply
    // (so as to avoid ever clearing one after entering another, in
    // case the two overlap at all)
    stateChanged("have_selection", KXMLGUIClient::StateReverse);
    stateChanged("have_notes_in_selection", KXMLGUIClient::StateReverse);
    stateChanged("have_rests_in_selection", KXMLGUIClient::StateReverse);

    if (m_currentEventSelection) {

        NOTATION_DEBUG << "NotationView::setMenuStates: Have selection; it's " << m_currentEventSelection << " covering range from " << m_currentEventSelection->getStartTime() << " to " << m_currentEventSelection->getEndTime() << " (" << m_currentEventSelection->getSegmentEvents().size() << " events)" << endl;

        stateChanged("have_selection", KXMLGUIClient::StateNoReverse);
        if (m_currentEventSelection->contains
                (Note::EventType)) {
            stateChanged("have_notes_in_selection",
                         KXMLGUIClient::StateNoReverse);
        }
        if (m_currentEventSelection->contains
                (Note::EventRestType)) {
            stateChanged("have_rests_in_selection",
                         KXMLGUIClient::StateNoReverse);
        }
    }

    // 2. set inserter-related states

    // #1372863 -- RestInserter is a subclass of NoteInserter, so we
    // need to test dynamic_cast<RestInserter *> before
    // dynamic_cast<NoteInserter *> (which will succeed for both)

    if (dynamic_cast<RestInserter *>(m_tool)) {
        NOTATION_DEBUG << "Have rest inserter " << endl;
        stateChanged("note_insert_tool_current", StateReverse);
        stateChanged("rest_insert_tool_current", StateNoReverse);
    } else if (dynamic_cast<NoteInserter *>(m_tool)) {
        NOTATION_DEBUG << "Have note inserter " << endl;
        stateChanged("note_insert_tool_current", StateNoReverse);
        stateChanged("rest_insert_tool_current", StateReverse);
    } else {
        NOTATION_DEBUG << "Have neither inserter " << endl;
        stateChanged("note_insert_tool_current", StateReverse);
        stateChanged("rest_insert_tool_current", StateReverse);
    }
}

#define UPDATE_PROGRESS(n) \
	progressCount += (n); \
	if (progressTotal > 0) { \
	    emit setValue(progressCount * 100 / progressTotal); \
	    ProgressDialog::processEvents(); \
	}

void NotationView::readjustCanvasSize()
{
    Profiler profiler("NotationView::readjustCanvasSize");

    double maxWidth = 0.0;
    int maxHeight = 0;

    slotSetOperationNameAndStatus(i18n("Sizing and allocating canvas..."));
    ProgressDialog::processEvents();

    int progressTotal = m_staffs.size() + 2;
    int progressCount = 0;

    for (unsigned int i = 0; i < m_staffs.size(); ++i) {

        LinedStaff &staff = *m_staffs[i];

        staff.sizeStaff(*m_hlayout);
        UPDATE_PROGRESS(1);

        if (staff.getTotalWidth() + staff.getX() > maxWidth) {
            maxWidth = staff.getTotalWidth() + staff.getX() + 1;
        }

        if (staff.getTotalHeight() + staff.getY() > maxHeight) {
            maxHeight = staff.getTotalHeight() + staff.getY() + 1;
        }
    }

    int topMargin = 0, leftMargin = 0;
    getPageMargins(leftMargin, topMargin);

    int pageWidth = getPageWidth();
    int pageHeight = getPageHeight();

    NOTATION_DEBUG << "NotationView::readjustCanvasSize: maxHeight is "
    << maxHeight << ", page height is " << pageHeight << endl
    << " - maxWidth is " << maxWidth << ", page width is " << pageWidth << endl;


    if (m_pageMode == LinedStaff::LinearMode) {
        maxWidth = ((maxWidth / pageWidth) + 1) * pageWidth;
        if (maxHeight < pageHeight)
            maxHeight = pageHeight;
    } else {
        if (maxWidth < pageWidth)
            maxWidth = pageWidth;
        if (maxHeight < pageHeight + topMargin*2)
            maxHeight = pageHeight + topMargin * 2;
    }

    // now get the EditView to do the biz
    readjustViewSize(QSize(int(maxWidth), maxHeight), true);

    UPDATE_PROGRESS(2);

    if (m_pannerDialog) {

        if (m_pageMode != LinedStaff::MultiPageMode) {
            m_pannerDialog->hide();

        } else {

            m_pannerDialog->show();

            m_pannerDialog->setPageSize
            (QSize(canvas()->width(),
                   canvas()->height()));

            m_pannerDialog->scrollbox()->setViewSize
            (QSize(getCanvasView()->width(),
                   getCanvasView()->height()));
        }
    }

    // Give a correct vertical alignment to track headers
    if ((m_pageMode == LinedStaff::LinearMode) && m_showHeadersGroup) {
        m_headersGroupView->setContentsPos(0, getCanvasView()->contentsY());
    }
}

void NotationView::slotNoteAction()
{
    const QObject* sigSender = sender();

    NoteActionDataMap::Iterator noteAct =
        m_noteActionDataMap->find(sigSender->objectName());

    if (noteAct != m_noteActionDataMap->end()) {
        m_lastNoteAction = sigSender->objectName();
        setCurrentSelectedNote(**noteAct);
        setMenuStates();
    } else {
        std::cerr << "NotationView::slotNoteAction() : couldn't find NoteActionData named '"
        << sigSender->objectName() << "'\n";
    }
}

void NotationView::slotLastNoteAction()
{
    KAction *action = actionCollection()->action(m_lastNoteAction);
    if (!action)
        action = actionCollection()->action("crotchet");

    if (action) {
        action->activate();
    } else {
        std::cerr << "NotationView::slotNoteAction() : couldn't find action named '"
        << m_lastNoteAction << "' or 'crotchet'\n";
    }
}

void NotationView::slotNoteChangeAction()
{
    const QObject* sigSender = sender();

    NoteChangeActionDataMap::Iterator noteAct =
        m_noteChangeActionDataMap->find(sigSender->objectName());

    if (noteAct != m_noteChangeActionDataMap->end()) {
        slotSetNoteDurations((**noteAct).noteType, (**noteAct).notationOnly);
    } else {
        std::cerr << "NotationView::slotNoteChangeAction() : couldn't find NoteChangeAction named '"
        << sigSender->objectName() << "'\n";
    }
}

void NotationView::initActionDataMaps()
{
    static bool called = false;
    static int keys[] =
        { Qt::Key_0, Qt::Key_3, Qt::Key_6, Qt::Key_8, Qt::Key_4, Qt::Key_2, Qt::Key_1, Qt::Key_5 };

    if (called)
        return ;
    called = true;

    m_noteActionDataMap = new NoteActionDataMap;

    for (int rest = 0; rest < 2; ++rest) {
        for (int dots = 0; dots < 2; ++dots) {
            for (int type = Note::Longest; type >= Note::Shortest; --type) {
                if (dots && (type == Note::Longest))
                    continue;

                QString refName
                (NotationStrings::getReferenceName(Note(type, dots), rest == 1));

                QString shortName(refName);
                shortName.replace(QRegExp("-"), "_");

                QString titleName
                (NotationStrings::getNoteName(Note(type, dots)));

                titleName = titleName.left(1).toUpper() +
                            titleName.right(titleName.length() - 1);

                if (rest) {
                    titleName.replace(QRegExp(i18n("note")), i18n("rest"));
                }

                int keycode = keys[type - Note::Shortest];
                if (dots) // keycode += Qt::CTRL; -- used below for note change action
                    keycode = 0;
                if (rest) // keycode += Qt::SHIFT; -- can't do shift+numbers
                    keycode = 0;

                m_noteActionDataMap->insert
                    (shortName, new NoteActionData
                     (titleName, shortName, refName, keycode,
                      rest > 0, type, dots));
            }
        }
    }

    m_noteChangeActionDataMap = new NoteChangeActionDataMap;

    for (int notationOnly = 0; notationOnly <= 1; ++notationOnly) {
        for (int type = Note::Longest; type >= Note::Shortest; --type) {

            QString refName
            (NotationStrings::getReferenceName(Note(type, 0), false));

            QString shortName(QString("change_%1%2")
                              .arg(notationOnly ? "notation_" : "").arg(refName));
            shortName.replace(QRegExp("-"), "_");

            QString titleName
            (NotationStrings::getNoteName(Note(type, 0)));

            titleName = titleName.left(1).toUpper() +
                        titleName.right(titleName.length() - 1);

            int keycode = keys[type - Note::Shortest];
            keycode += Qt::CTRL;
            if (notationOnly)
                keycode += ALT;

            m_noteChangeActionDataMap->insert
                (shortName, new NoteChangeActionData
                 (titleName, shortName, refName, keycode,
                  notationOnly ? true : false, type));
        }
    }
}

void NotationView::setupProgress(QProgressBar* bar)
{
    if (bar) {
        NOTATION_DEBUG << "NotationView::setupProgress(bar)\n";

        connect(m_hlayout, SIGNAL(setValue(int)),
                bar, SLOT(setValue(int)));

        connect(m_hlayout, SIGNAL(incrementProgress(int)),
                bar, SLOT(advance(int)));

        connect(this, SIGNAL(setValue(int)),
                bar, SLOT(setValue(int)));

        connect(this, SIGNAL(incrementProgress(int)),
                bar, SLOT(advance(int)));

        for (unsigned int i = 0; i < m_staffs.size(); ++i) {
            connect(m_staffs[i], SIGNAL(setValue(int)),
                    bar, SLOT(setValue(int)));

            connect(m_staffs[i], SIGNAL(incrementProgress(int)),
                    bar, SLOT(advance(int)));
        }
    }

}

void NotationView::setupProgress(ProgressDialog* dialog)
{
    NOTATION_DEBUG << "NotationView::setupProgress(dialog)" << endl;
    disconnectProgress();

    if (dialog) {
        setupProgress(dialog->progressBar());

        connect(dialog, SIGNAL(cancelClicked()),
                m_hlayout, SLOT(slotCancel()));

        for (unsigned int i = 0; i < m_staffs.size(); ++i) {
            connect(m_staffs[i], SIGNAL(setOperationName(QString)),
                    this, SLOT(slotSetOperationNameAndStatus(QString)));

            connect(dialog, SIGNAL(cancelClicked()),
                    m_staffs[i], SLOT(slotCancel()));
        }

        connect(this, SIGNAL(setOperationName(QString)),
                dialog, SLOT(slotSetOperationName(QString)));
        m_progressDisplayer = PROGRESS_DIALOG;
    }

}

void NotationView::slotSetOperationNameAndStatus(QString name)
{
    emit setOperationName(name);
    statusBar()->changeItem(QString("  %1").arg(name),
                            KTmpStatusMsg::getDefaultId());
}

void NotationView::disconnectProgress()
{
    NOTATION_DEBUG << "NotationView::disconnectProgress()" << endl;

    m_hlayout->disconnect();
    disconnect(SIGNAL(setValue(int)));
    disconnect(SIGNAL(incrementProgress(int)));
    disconnect(SIGNAL(setOperationName(QString)));

    for (unsigned int i = 0; i < m_staffs.size(); ++i) {
        m_staffs[i]->disconnect();
    }
}

void NotationView::setupDefaultProgress()
{
    if (m_progressDisplayer != PROGRESS_BAR) {
        NOTATION_DEBUG << "NotationView::setupDefaultProgress()" << endl;
        disconnectProgress();
        setupProgress(m_progressBar());
        m_progressDisplayer = PROGRESS_BAR;
    }
}

void NotationView::updateViewCaption()
{
    if (m_segments.size() == 1) {

        TrackId trackId = m_segments[0]->getTrack();
        Track *track =
            m_segments[0]->getComposition()->getTrackById(trackId);

        int trackPosition = -1;
        if (track)
            trackPosition = track->getPosition();
        //	std::cout << std::endl << std::endl << std::endl << "DEBUG TITLE BAR: " << getDocument()->getTitle() << std::endl << std::endl << std::endl;
        setCaption(i18n("%1 - Segment Track #%2 - Notation",
                    getDocument()->getTitle(),
                    trackPosition + 1));

    } else if (m_segments.size() == getDocument()->getComposition().getNbSegments()) {

        setCaption(i18n("%1 - All Segments - Notation",
                    getDocument()->getTitle()));

    } else {

        setCaption(i18np("%2 - Segment - Notation", "%2 - %1 Segments - Notation", m_segments.size(),
                    getDocument()->getTitle()));

    }
}

NotationView::NoteActionDataMap* NotationView::m_noteActionDataMap = 0;

NotationView::NoteChangeActionDataMap* NotationView::m_noteChangeActionDataMap = 0;


/// SLOTS


void
NotationView::slotUpdateInsertModeStatus()
{
    QString tripletMessage = i18n("Triplet");
    QString chordMessage = i18n("Chord");
    QString graceMessage = i18n("Grace");
    QString message;

    if (isInTripletMode()) {
        message = i18n("%1 %2", message, tripletMessage);
    }

    if (isInChordMode()) {
        message = i18n("%1 %2", message, chordMessage);
    }

    if (isInGraceMode()) {
        message = i18n("%1 %2", message, graceMessage);
    }

    m_insertModeLabel->setText(message);
}

void
NotationView::slotUpdateAnnotationsStatus()
{
    if (!areAnnotationsVisible()) {
        for (int i = 0; i < getStaffCount(); ++i) {
            Segment &s = getStaff(i)->getSegment();
            for (Segment::iterator j = s.begin(); j != s.end(); ++j) {
                if ((*j)->isa(Text::EventType) &&
                        ((*j)->get<String>(Text::TextTypePropertyName)
                         == Text::Annotation)) {
                    m_annotationsLabel->setText(i18n("Hidden annotations"));
                    return ;
                }
            }
        }
    }
    m_annotationsLabel->setText("");
    getToggleAction("show_annotations")->setChecked(areAnnotationsVisible());
}

void
NotationView::slotUpdateLilyPondDirectivesStatus()
{
    if (!areLilyPondDirectivesVisible()) {
        for (int i = 0; i < getStaffCount(); ++i) {
            Segment &s = getStaff(i)->getSegment();
            for (Segment::iterator j = s.begin(); j != s.end(); ++j) {
                if ((*j)->isa(Text::EventType) &&
                        ((*j)->get
                         <String>
                         (Text::TextTypePropertyName)
                         == Text::LilyPondDirective)) {
                    m_lilyPondDirectivesLabel->setText(i18n("Hidden LilyPond directives"));
                    return ;
                }
            }
        }
    }
    m_lilyPondDirectivesLabel->setText("");
    getToggleAction("show_lilypond_directives")->setChecked(areLilyPondDirectivesVisible());
}

void
NotationView::slotChangeSpacingFromStringValue(const QString& spacingT)
{
    // spacingT has a '%' at the end
    //
    int spacing = spacingT.left(spacingT.length() - 1).toInt();
    slotChangeSpacing(spacing);
}

void
NotationView::slotChangeSpacingFromAction()
{
    const QObject *s = sender();
    QString name = s->objectName();

    if (name.left(8) == "spacing_") {
        int spacing = name.right(name.length() - 8).toInt();

        if (spacing > 0)
            slotChangeSpacing(spacing);

    } else {
        /* was sorry */ QMessageBox::warning
        (this, i18n("Unknown spacing action %1", name));
    }
}

void
NotationView::slotChangeSpacing(int spacing)
{
    if (m_hlayout->getSpacing() == spacing)
        return ;

    m_hlayout->setSpacing(spacing);

    //     m_spacingSlider->setSize(spacing);

    /* was toggle */ QAction *action = dynamic_cast<QAction*>
                            (actionCollection()->action(QString("spacing_%1").arg(spacing)));
    if (action)
        action->setChecked(true);
    else {
        std::cerr
        << "WARNING: Expected action \"spacing_" << spacing
        << "\" to be a QAction, but it isn't (or doesn't exist)"
        << std::endl;
    }

    positionStaffs();
    applyLayout();

    for (unsigned int i = 0; i < m_staffs.size(); ++i) {
        m_staffs[i]->markChanged();
    }

    positionPages();
    updateControlRulers(true);
    updateView();
}

void
NotationView::slotChangeProportionFromIndex(int n)
{
    std::vector<int> proportions = m_hlayout->getAvailableProportions();
    if (n >= (int)proportions.size())
        n = proportions.size() - 1;
    slotChangeProportion(proportions[n]);
}

void
NotationView::slotChangeProportionFromAction()
{
    const QObject *s = sender();
    QString name = s->objectName();

    if (name.left(11) == "proportion_") {
        int proportion = name.right(name.length() - 11).toInt();
        slotChangeProportion(proportion);

    } else {
        /* was sorry */ QMessageBox::warning
        (this, i18n("Unknown proportion action %1", name));
    }
}

void
NotationView::slotChangeProportion(int proportion)
{
    if (m_hlayout->getProportion() == proportion)
        return ;

    m_hlayout->setProportion(proportion);

    //    m_proportionSlider->setSize(proportion);

    /* was toggle */ QAction *action = dynamic_cast<QAction*>
                            (actionCollection()->action(QString("proportion_%1").arg(proportion)));
    if (action)
        action->setChecked(true);
    else {
        std::cerr
        << "WARNING: Expected action \"proportion_" << proportion
        << "\" to be a QAction, but it isn't (or doesn't exist)"
        << std::endl;
    }

    positionStaffs();
    applyLayout();

    for (unsigned int i = 0; i < m_staffs.size(); ++i) {
        m_staffs[i]->markChanged();
    }

    positionPages();
    updateControlRulers(true);
    updateView();
}

void
NotationView::slotChangeFontFromAction()
{
    const QObject *s = sender();
    QString name = s->objectName();
    if (name.left(10) == "note_font_") {
        name = name.right(name.length() - 10);
        slotChangeFont(name);
    } else {
        /* was sorry */ QMessageBox::warning
        (this, i18n("Unknown font action %1", name));
    }
}

void
NotationView::slotChangeFontSizeFromAction()
{
    const QObject *s = sender();
    QString name = s->objectName();

    if (name.left(15) == "note_font_size_") {
        name = name.right(name.length() - 15);
        bool ok = false;
        int size = name.toInt(&ok);
        if (ok)
            slotChangeFont(m_fontName, size);
        else {
            /* was sorry */ QMessageBox::warning
            (this, i18n("Unknown font size %1", name));
        }
    } else {
        /* was sorry */ QMessageBox::warning
        (this, i18n("Unknown font size action %1", name));
    }
}

void
NotationView::slotChangeFont(const QString &newName)
{
    NOTATION_DEBUG << "changeFont: " << newName << endl;
    slotChangeFont(std::string(newName.toUtf8()));
}

void
NotationView::slotChangeFont(std::string newName)
{
    int newSize = m_fontSize;

    if (!NoteFontFactory::isAvailableInSize(newName, newSize)) {

        int defaultSize = NoteFontFactory::getDefaultSize(newName);

        //@@@ JAS Check here first for errors.  Added .begingGroup()
        QSettings settings;
        settings.beginGroup( NotationViewConfigGroup );

        newSize = settings.value((getStaffCount() > 1 ?
                "multistaffnotesize" : "singlestaffnotesize"), defaultSize).toInt();
        settings.endGroup();

        if (!NoteFontFactory::isAvailableInSize(newName, newSize)) {
            newSize = defaultSize;
        }
    }

    slotChangeFont(newName, newSize);
}

void
NotationView::slotChangeFontSize(int newSize)
{
    slotChangeFont(m_fontName, newSize);
}

void
NotationView::slotChangeFontSizeFromStringValue(const QString& sizeT)
{
    int size = sizeT.toInt();
    slotChangeFont(m_fontName, size);
}

void
NotationView::slotZoomIn()
{
    std::vector<int> sizes = NoteFontFactory::getScreenSizes(m_fontName);
    for (int i = 0; i + 1 < sizes.size(); ++i) {
        if (sizes[i] == m_fontSize) {
            slotChangeFontSize(sizes[i + 1]);
            return ;
        }
    }
}

void
NotationView::slotZoomOut()
{
    std::vector<int> sizes = NoteFontFactory::getScreenSizes(m_fontName);
    for (int i = 1; i < sizes.size(); ++i) {
        if (sizes[i] == m_fontSize) {
            slotChangeFontSize(sizes[i - 1]);
            return ;
        }
    }
}

void
NotationView::slotChangeFont(std::string newName, int newSize)
{
    if (newName == m_fontName && newSize == m_fontSize)
        return ;

    NotePixmapFactory* npf = 0;

    try {
        npf = new NotePixmapFactory(newName, newSize);
    } catch (...) {
        return ;
    }

    bool changedFont = (newName != m_fontName || newSize != m_fontSize);

    std::string oldName = m_fontName;
    m_fontName = newName;
    m_fontSize = newSize;
    setNotePixmapFactory(npf);

    // update the various GUI elements

    std::set<std::string> fs(NoteFontFactory::getFontNames());
    std::vector<std::string> f(fs.begin(), fs.end());
    std::sort(f.begin(), f.end());

    for (unsigned int i = 0; i < f.size(); ++i) {
        bool thisOne = (f[i] == m_fontName);
        if (thisOne)
            m_fontCombo->setCurrentIndex(i);
        /* was toggle */ QAction *action = dynamic_cast<QAction*>
                                (actionCollection()->action("note_font_" + strtoqstr(f[i])));
        NOTATION_DEBUG << "inspecting " << f[i] << (action ? ", have action" : ", no action") << endl;
        if (action)
            action->setChecked(thisOne);
        else {
            std::cerr
            << "WARNING: Expected action \"note_font_" << f[i]
            << "\" to be a QAction, but it isn't (or doesn't exist)"
            << std::endl;
        }
    }

    NOTATION_DEBUG << "about to reinitialise sizes" << endl;

    std::vector<int> sizes = NoteFontFactory::getScreenSizes(m_fontName);
    m_fontSizeCombo->clear();
    QString value;
    for (std::vector<int>::iterator i = sizes.begin(); i != sizes.end(); ++i) {
        value.setNum(*i);
        m_fontSizeCombo->addItem(value);
    }
    value.setNum(m_fontSize);
    m_fontSizeCombo->setItemText(value);

    setupFontSizeMenu(oldName);

    if (!changedFont)
        return ; // might have been called to initialise menus etc

    NOTATION_DEBUG << "about to change font" << endl;

    if (m_pageMode == LinedStaff::MultiPageMode) {

        int pageWidth = getPageWidth();
        int topMargin = 0, leftMargin = 0;
        getPageMargins(leftMargin, topMargin);

        m_hlayout->setPageWidth(pageWidth - leftMargin * 2);
    }

    for (unsigned int i = 0; i < m_staffs.size(); ++i) {
        m_staffs[i]->changeFont(m_fontName, m_fontSize);
    }

    NOTATION_DEBUG << "about to position staffs" << endl;

    positionStaffs();

    bool layoutApplied = applyLayout();
    if (!layoutApplied)
        /* was sorry */ QMessageBox::warning(0, "Couldn't apply layout");
    else {
        for (unsigned int i = 0; i < m_staffs.size(); ++i) {
            m_staffs[i]->markChanged();
        }
    }

    positionPages();
    updateControlRulers(true);
    updateView();
}

void
NotationView::slotFilePrint()
{
    KTmpStatusMsg msg(i18n("Printing..."), this);

    SetWaitCursor waitCursor;
    NotationView printingView(getDocument(), m_segments,
                              (QWidget *)parent(), this);

    if (!printingView.isOK()) {
        NOTATION_DEBUG << "Print : operation cancelled\n";
        return ;
    }

    printingView.print();
}

void
NotationView::slotFilePrintPreview()
{
    KTmpStatusMsg msg(i18n("Previewing..."), this);

    SetWaitCursor waitCursor;
    NotationView printingView(getDocument(), m_segments,
                              (QWidget *)parent(), this);

    if (!printingView.isOK()) {
        NOTATION_DEBUG << "Print preview : operation cancelled\n";
        return ;
    }

    printingView.print(true);
}

std::map<QProcess *, QTemporaryFile *> NotationView::m_lilyTempFileMap;

void NotationView::slotPrintLilyPond()
{
    KTmpStatusMsg msg(i18n("Printing LilyPond file..."), this);
    QTemporaryFile *file = new QTemporaryFile("XXXXXX.ly");
    file->setAutoRemove(true);
    if (!file->open()) {
        CurrentProgressDialog::freeze();
        /* was sorry */ QMessageBox::warning(this, i18n("Failed to open a temporary file for LilyPond export."));
        delete file;
    }
    QString filename = file->fileName();
    file->close(); // we just want the filename
    if (!exportLilyPondFile(filename, true)) {
        return ;
    }

    //setup "rosegarden-lilypondview" QProcess
    QProcess *proc = new QProcess;
    QStringList procArgs;

    procArgs << "--graphical";
    procArgs << "--print";
    procArgs << filename;
    connect(proc, SIGNAL(processExited(QProcess *)),
            this, SLOT(slotLilyPondViewProcessExited(QProcess *)));
    m_lilyTempFileMap[proc] = file;
    proc->start("rosegarden-lilypondview", procArgs);
}

void NotationView::slotPreviewLilyPond()
{
    KTmpStatusMsg msg(i18n("Previewing LilyPond file..."), this);
    QTemporaryFile *file = new QTemporaryFile("XXXXXX.ly");
    file->setAutoRemove(true);
    if (!file->open()) {
        CurrentProgressDialog::freeze();
        /* was sorry */ QMessageBox::warning(this, i18n("Failed to open a temporary file for LilyPond export."));
        delete file;
    }
    QString filename = file->fileName();
    file->close(); // we just want the filename
    if (!exportLilyPondFile(filename, true)) {
        return ;
    }
    //setup "rosegarden-lilypondview" QProcess
    QProcess *proc = new QProcess;
    QStringList procArgs;
    procArgs << "--graphical";
    procArgs << "--pdf";
    procArgs << filename;
    connect(proc, SIGNAL(processExited(QProcess *)),
            this, SLOT(slotLilyPondViewProcessExited(QProcess *)));
    m_lilyTempFileMap[proc] = file;
    proc->start("rosegarden-lilypondview", procArgs);
}

void NotationView::slotLilyPondViewProcessExited(QProcess *p)
{
    delete m_lilyTempFileMap[p];
    m_lilyTempFileMap.erase(p);
    delete p;
}

bool NotationView::exportLilyPondFile(QString file, bool forPreview)
{
    QString caption = "", heading = "";
    if (forPreview) {
        caption = i18n("LilyPond Preview Options");
        heading = i18n("LilyPond preview options");
    }

    LilyPondOptionsDialog dialog(this, m_doc, caption, heading);
    if (dialog.exec() != QDialog::Accepted) {
        return false;
    }

    ProgressDialog progressDlg(i18n("Exporting LilyPond file..."),
                               100,
                               this);

    LilyPondExporter e(this, m_doc, std::string(QFile::encodeName(file)));

    connect(&e, SIGNAL(setValue(int)),
            progressDlg.progressBar(), SLOT(setValue(int)));

    connect(&e, SIGNAL(incrementProgress(int)),
            progressDlg.progressBar(), SLOT(advance(int)));

    if (!e.write()) {
        // CurrentProgressDialog::freeze();
        /* was sorry */ QMessageBox::warning(this, i18n("Export failed.  The file could not be opened for writing."));
        return false;
    }

    return true;
}

void NotationView::slotEditCut()
{
    if (!m_currentEventSelection)
        return ;
    KTmpStatusMsg msg(i18n("Cutting selection to clipboard..."), this);

    addCommandToHistory(new CutCommand(*m_currentEventSelection,
                                       getDocument()->getClipboard()));
}

void NotationView::slotEditDelete()
{
    if (!m_currentEventSelection)
        return ;
    KTmpStatusMsg msg(i18n("Deleting selection..."), this);

    addCommandToHistory(new EraseCommand(*m_currentEventSelection));
}

void NotationView::slotEditCopy()
{
    if (!m_currentEventSelection)
        return ;
    KTmpStatusMsg msg(i18n("Copying selection to clipboard..."), this);

    addCommandToHistory(new CopyCommand(*m_currentEventSelection,
                                        getDocument()->getClipboard()));
}

void NotationView::slotEditCutAndClose()
{
    if (!m_currentEventSelection)
        return ;
    KTmpStatusMsg msg(i18n("Cutting selection to clipboard..."), this);

    addCommandToHistory(new CutAndCloseCommand(*m_currentEventSelection,
                        getDocument()->getClipboard()));
}

static const QString RESTRICTED_PASTE_FAILED_DESCRIPTION = i18n(
                      "The Restricted paste type requires enough empty " \
                      "space (containing only rests) at the paste position " \
                      "to hold all of the events to be pasted.\n" \
                      "Not enough space was found.\n" \
                      "If you want to paste anyway, consider using one of " \
                      "the other paste types from the \"Paste...\" option " \
                      "on the Edit menu.  You can also change the default " \
                      "paste type to something other than Restricted if " \
                      "you wish."
    );

void NotationView::slotEditPaste()
{
    Clipboard * clipboard = getDocument()->getClipboard();

    if (clipboard->isEmpty()) {
        slotStatusHelpMsg(i18n("Clipboard is empty"));
        return ;
    }
    if (!clipboard->isSingleSegment()) {
        slotStatusHelpMsg(i18n("Can't paste multiple Segments into one"));
        return ;
    }

    slotStatusHelpMsg(i18n("Inserting clipboard contents..."));

    LinedStaff *staff = getCurrentLinedStaff();
    Segment &segment = staff->getSegment();

    // Paste at cursor position
    //
    timeT insertionTime = getInsertionTime();
    timeT endTime = insertionTime +
        (clipboard->getSingleSegment()->getEndTime() -
         clipboard->getSingleSegment()->getStartTime());

    QSettings settings;
    settings.beginGroup( NotationViewConfigGroup );

    PasteEventsCommand::PasteType defaultType = (PasteEventsCommand::PasteType)
            settings.value("pastetype",
            PasteEventsCommand::Restricted).toUInt();

    PasteEventsCommand *command = new PasteEventsCommand
        (segment, clipboard, insertionTime, defaultType);

    if (!command->isPossible()) {
        QMessageBox::detailedError
            (this,
             i18n("Couldn't paste at this point."), RESTRICTED_PASTE_FAILED_DESCRIPTION);
    } else {
        addCommandToHistory(command);
        setCurrentSelection(new EventSelection(command->getPastedEvents()));
        slotSetInsertCursorPosition(endTime, true, false);
    }

    settings.endGroup();
}

void NotationView::slotEditGeneralPaste()
{
    Clipboard *clipboard = getDocument()->getClipboard();

    if (clipboard->isEmpty()) {
        slotStatusHelpMsg(i18n("Clipboard is empty"));
        return ;
    }

    slotStatusHelpMsg(i18n("Inserting clipboard contents..."));

    LinedStaff *staff = getCurrentLinedStaff();
    Segment &segment = staff->getSegment();

    QSettings settings;
    settings.beginGroup( NotationViewConfigGroup );

    PasteEventsCommand::PasteType defaultType = (PasteEventsCommand::PasteType)
            settings.value("pastetype", PasteEventsCommand::Restricted).toUInt();

    PasteNotationDialog dialog(this, defaultType);

    if (dialog.exec() == QDialog::Accepted) {

        PasteEventsCommand::PasteType type = dialog.getPasteType();
        if (dialog.setAsDefault()) {
            //###settings.beginGroup( NotationViewConfigGroup );
            settings.setValue("pastetype", type);
        }

        timeT insertionTime = getInsertionTime();
        timeT endTime = insertionTime +
            (clipboard->getSingleSegment()->getEndTime() -
             clipboard->getSingleSegment()->getStartTime());

        PasteEventsCommand *command = new PasteEventsCommand
            (segment, clipboard, insertionTime, type);

        if (!command->isPossible()) {
            QMessageBox::detailedError
                (this,
                 i18n("Couldn't paste at this point."),
                 i18n(RESTRICTED_PASTE_FAILED_DESCRIPTION));
        } else {
            addCommandToHistory(command);
            setCurrentSelection(new EventSelection
                                (segment, insertionTime, endTime));
            slotSetInsertCursorPosition(endTime, true, false);
        }
    }

    settings.endGroup();
}

void
NotationView::slotMoveEventsUpStaff()
{
    LinedStaff *targetStaff = getStaffAbove();
    if (!targetStaff) return;
    if (!m_currentEventSelection) return;
    Segment &targetSegment = targetStaff->getSegment();
    
    MacroCommand *command = new MacroCommand(i18n("Move Events to Staff Above"));

    timeT insertionTime = m_currentEventSelection->getStartTime();

    Clipboard *c = new Clipboard;
    CopyCommand *cc = new CopyCommand(*m_currentEventSelection, c);
    cc->execute();

    command->addCommand(new EraseCommand(*m_currentEventSelection));;

    command->addCommand(new PasteEventsCommand
                        (targetSegment, c,
                         insertionTime,
                         PasteEventsCommand::NoteOverlay));

    addCommandToHistory(command);

    delete c;
}

void
NotationView::slotMoveEventsDownStaff()
{
    LinedStaff *targetStaff = getStaffBelow();
    if (!targetStaff) return;
    if (!m_currentEventSelection) return;
    Segment &targetSegment = targetStaff->getSegment();
    
    MacroCommand *command = new MacroCommand(i18n("Move Events to Staff TicksBelow"));

    timeT insertionTime = m_currentEventSelection->getStartTime();

    Clipboard *c = new Clipboard;
    CopyCommand *cc = new CopyCommand(*m_currentEventSelection, c);
    cc->execute();

    command->addCommand(new EraseCommand(*m_currentEventSelection));;

    command->addCommand(new PasteEventsCommand
                        (targetSegment, c,
                         insertionTime,
                         PasteEventsCommand::NoteOverlay));

    addCommandToHistory(command);

    delete c;
}

void NotationView::slotPreviewSelection()
{
    if (!m_currentEventSelection)
        return ;

    getDocument()->slotSetLoop(m_currentEventSelection->getStartTime(),
                               m_currentEventSelection->getEndTime());
}

void NotationView::slotClearLoop()
{
    getDocument()->slotSetLoop(0, 0);
}

void NotationView::slotClearSelection()
{
    // Actually we don't clear the selection immediately: if we're
    // using some tool other than the select tool, then the first
    // press switches us back to the select tool.

    NotationSelector *selector = dynamic_cast<NotationSelector *>(m_tool);

    if (!selector) {
        slotSelectSelected();
    } else {
        setCurrentSelection(0);
    }
}

void NotationView::slotEditSelectFromStart()
{
    timeT t = getInsertionTime();
    Segment &segment = m_staffs[m_currentStaff]->getSegment();
    setCurrentSelection(new EventSelection(segment,
                                           segment.getStartTime(),
                                           t));
}

void NotationView::slotEditSelectToEnd()
{
    timeT t = getInsertionTime();
    Segment &segment = m_staffs[m_currentStaff]->getSegment();
    setCurrentSelection(new EventSelection(segment,
                                           t,
                                           segment.getEndMarkerTime()));
}

void NotationView::slotEditSelectWholeStaff()
{
    Segment &segment = m_staffs[m_currentStaff]->getSegment();
    setCurrentSelection(new EventSelection(segment,
                                           segment.getStartTime(),
                                           segment.getEndMarkerTime()));
}

void NotationView::slotFilterSelection()
{
    NOTATION_DEBUG << "NotationView::slotFilterSelection" << endl;

    Segment *segment = getCurrentSegment();
    EventSelection *existingSelection = m_currentEventSelection;
    if (!segment || !existingSelection)
        return ;

    EventFilterDialog dialog(this);
    if (dialog.exec() == QDialog::Accepted) {
        NOTATION_DEBUG << "slotFilterSelection- accepted" << endl;

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

void NotationView::slotVelocityUp()
{
    if (!m_currentEventSelection)
        return ;
    KTmpStatusMsg msg(i18n("Raising velocities..."), this);

    addCommandToHistory
    (new ChangeVelocityCommand(10, *m_currentEventSelection));
}

void NotationView::slotVelocityDown()
{
    if (!m_currentEventSelection)
        return ;
    KTmpStatusMsg msg(i18n("Lowering velocities..."), this);

    addCommandToHistory
    (new ChangeVelocityCommand( -10, *m_currentEventSelection));
}

int NotationView::getVelocityFromSelection()
{
    if (!m_currentEventSelection) return 0;

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
        return (totalVelocity / count) + 0.5;
    }
    return 0;
}

void NotationView::slotSetVelocities()
{
    if (!m_currentEventSelection)
        return ;

    EventParameterDialog dialog(this,
                                i18n("Set Event Velocities"),
                                BaseProperties::VELOCITY,
                                getVelocityFromSelection());

    if (dialog.exec() == QDialog::Accepted) {
        KTmpStatusMsg msg(i18n("Setting Velocities..."), this);
        addCommandToHistory(new SelectionPropertyCommand
                            (m_currentEventSelection,
                             BaseProperties::VELOCITY,
                             dialog.getPattern(),
                             dialog.getValue1(),
                             dialog.getValue2()));
    }
}

void NotationView::slotToggleToolsToolBar()
{
    toggleNamedToolBar("Tools Toolbar");
}

void NotationView::slotToggleNotesToolBar()
{
    toggleNamedToolBar("Notes Toolbar");
}

void NotationView::slotToggleRestsToolBar()
{
    toggleNamedToolBar("Rests Toolbar");
}

void NotationView::slotToggleAccidentalsToolBar()
{
    toggleNamedToolBar("Accidentals Toolbar");
}

void NotationView::slotToggleClefsToolBar()
{
    toggleNamedToolBar("Clefs Toolbar");
}

void NotationView::slotToggleMetaToolBar()
{
    toggleNamedToolBar("Meta Toolbar");
}

void NotationView::slotToggleMarksToolBar()
{
    toggleNamedToolBar("Marks Toolbar");
}

void NotationView::slotToggleGroupToolBar()
{
    toggleNamedToolBar("Group Toolbar");
}

void NotationView::slotToggleLayoutToolBar()
{
    toggleNamedToolBar("Layout Toolbar");
}

void NotationView::slotToggleTransportToolBar()
{
    toggleNamedToolBar("Transport Toolbar");
}

void NotationView::toggleNamedToolBar(const QString& toolBarName, bool* force)
{
    KToolBar *namedToolBar = toolBar(toolBarName);

    if (!namedToolBar) {
        NOTATION_DEBUG << "NotationView::toggleNamedToolBar() : toolBar "
                       << toolBarName << " not found" << endl;
        return ;
    }

    if (!force) {

        if (namedToolBar->isVisible())
            namedToolBar->hide();
        else
            namedToolBar->show();
    } else {

        if (*force)
            namedToolBar->show();
        else
            namedToolBar->hide();
    }

    setSettingsDirty();

}

void NotationView::slotGroupSimpleTuplet()
{
    slotGroupTuplet(true);
}

void NotationView::slotGroupGeneralTuplet()
{
    slotGroupTuplet(false);
}

void NotationView::slotGroupTuplet(bool simple)
{
    timeT t = 0;
    timeT unit = 0;
    int tupled = 2;
    int untupled = 3;
    Segment *segment = 0;
    bool hasTimingAlready = false;

    if (m_currentEventSelection) {

        t = m_currentEventSelection->getStartTime();

        timeT duration = m_currentEventSelection->getTotalDuration();
        Note::Type unitType =
            Note::getNearestNote(duration / 3, 0).getNoteType();
        unit = Note(unitType).getDuration();

        if (!simple) {
            TupletDialog dialog(this, unitType, duration);
            if (dialog.exec() != QDialog::Accepted)
                return ;
            unit = Note(dialog.getUnitType()).getDuration();
            tupled = dialog.getTupledCount();
            untupled = dialog.getUntupledCount();
            hasTimingAlready = dialog.hasTimingAlready();
        }

        segment = &m_currentEventSelection->getSegment();

    } else {

        t = getInsertionTime();

        NoteInserter *currentInserter = dynamic_cast<NoteInserter *>
            (m_toolBox->getTool(NoteInserter::ToolName));

        Note::Type unitType;

        if (currentInserter) {
            unitType = currentInserter->getCurrentNote().getNoteType();
        } else {
            unitType = Note::Quaver;
        }

        unit = Note(unitType).getDuration();

        if (!simple) {
            TupletDialog dialog(this, unitType);
            if (dialog.exec() != QDialog::Accepted)
                return ;
            unit = Note(dialog.getUnitType()).getDuration();
            tupled = dialog.getTupledCount();
            untupled = dialog.getUntupledCount();
            hasTimingAlready = dialog.hasTimingAlready();
        }

        segment = &m_staffs[m_currentStaff]->getSegment();
    }

    addCommandToHistory(new TupletCommand
                        (*segment, t, unit, untupled, tupled, hasTimingAlready));

    if (!hasTimingAlready) {
        slotSetInsertCursorPosition(t + (unit * tupled), true, false);
    }
}

void NotationView::slotTransformsNormalizeRests()
{
    if (!m_currentEventSelection)
        return ;
    KTmpStatusMsg msg(i18n("Normalizing rests..."), this);

    addCommandToHistory(new NormalizeRestsCommand
                        (*m_currentEventSelection));
}

void NotationView::slotTransformsCollapseNotes()
{
    if (!m_currentEventSelection)
        return ;
    KTmpStatusMsg msg(i18n("Collapsing notes..."), this);

    addCommandToHistory(new CollapseNotesCommand
                        (*m_currentEventSelection));
}

void NotationView::slotInsertNoteFromAction()
{
    const QObject *s = sender();
    QString name = s->objectName();

    Segment &segment = m_staffs[m_currentStaff]->getSegment();

    NoteInserter *noteInserter = dynamic_cast<NoteInserter *>(m_tool);
    if (!noteInserter) {
        /* was sorry */ QMessageBox::warning(this, i18n("No note duration selected"));
        return ;
    }

    int pitch = 0;
    Accidental accidental =
        Accidentals::NoAccidental;

    timeT time(getInsertionTime());
    Rosegarden::Key key = segment.getKeyAtTime(time);
    Clef clef = segment.getClefAtTime(time);

    try {

        pitch = getPitchFromNoteInsertAction(name, accidental, clef, key);

    } catch (...) {

        /* was sorry */ QMessageBox::warning
            (this, i18n("Unknown note insert action %1", name));
        return ;
    }

    KTmpStatusMsg msg(i18n("Inserting note"), this);

    NOTATION_DEBUG << "Inserting note at pitch " << pitch << endl;

    noteInserter->insertNote(segment, time, pitch, accidental);
}

void NotationView::slotInsertRest()
{
    Segment &segment = m_staffs[m_currentStaff]->getSegment();
    timeT time = getInsertionTime();

    RestInserter *restInserter = dynamic_cast<RestInserter *>(m_tool);

    if (!restInserter) {

        NoteInserter *noteInserter = dynamic_cast<NoteInserter *>(m_tool);
        if (!noteInserter) {
            /* was sorry */ QMessageBox::warning(this, i18n("No note duration selected"));
            return ;
        }

        Note note(noteInserter->getCurrentNote());

        restInserter = dynamic_cast<RestInserter*>
            (m_toolBox->getTool(RestInserter::ToolName));

        restInserter->slotSetNote(note.getNoteType());
        restInserter->slotSetDots(note.getDots());
    }

    restInserter->insertNote(segment, time,
                             0, Accidentals::NoAccidental, true);
}

void NotationView::slotSwitchFromRestToNote()
{
    RestInserter *restInserter = dynamic_cast<RestInserter *>(m_tool);
    if (!restInserter) {
        /* was sorry */ QMessageBox::warning(this, i18n("No rest duration selected"));
        return ;
    }

    Note note(restInserter->getCurrentNote());

    QString actionName = NotationStrings::getReferenceName(note, false);
    actionName = actionName.replace("-", "_");

    KRadioAction *action = dynamic_cast<KRadioAction *>
        (actionCollection()->action(actionName));

    if (!action) {
        std::cerr << "WARNING: Failed to find note action \""
                  << actionName << "\"" << std::endl;
    } else {
        action->activate();
    }

    NoteInserter *noteInserter = dynamic_cast<NoteInserter*>
        (m_toolBox->getTool(NoteInserter::ToolName));

    if (noteInserter) {
        noteInserter->slotSetNote(note.getNoteType());
        noteInserter->slotSetDots(note.getDots());
        setTool(noteInserter);
    }

    setMenuStates();
}

void NotationView::slotSwitchFromNoteToRest()
{
    NoteInserter *noteInserter = dynamic_cast<NoteInserter *>(m_tool);
    if (!noteInserter) {
        /* was sorry */ QMessageBox::warning(this, i18n("No note duration selected"));
        return ;
    }

    Note note(noteInserter->getCurrentNote());

    QString actionName = NotationStrings::getReferenceName(note, true);
    actionName = actionName.replace("-", "_");

    KRadioAction *action = dynamic_cast<KRadioAction *>
        (actionCollection()->action(actionName));

    if (!action) {
        std::cerr << "WARNING: Failed to find rest action \""
                  << actionName << "\"" << std::endl;
    } else {
        action->activate();
    }

    RestInserter *restInserter = dynamic_cast<RestInserter*>
        (m_toolBox->getTool(RestInserter::ToolName));

    if (restInserter) {
        restInserter->slotSetNote(note.getNoteType());
        restInserter->slotSetDots(note.getDots());
        setTool(restInserter);
    }

    setMenuStates();
}

void NotationView::slotToggleDot()
{
    // Test first RestInserter which is a sub-class of NoteInserter.
    RestInserter *restInserter = dynamic_cast<RestInserter *>(m_tool);
    if (restInserter) {
        Note note(restInserter->getCurrentNote());
        if (note.getNoteType() == Note::Shortest ||
            note.getNoteType() == Note::Longest)
            return ;
        restInserter->slotSetDots(note.getDots() ? 0 : 1);
        setTool(restInserter);
    } else {
        NoteInserter *noteInserter = dynamic_cast<NoteInserter *>(m_tool);
        if (noteInserter) {
            Note note(noteInserter->getCurrentNote());
            if (note.getNoteType() == Note::Shortest ||
                note.getNoteType() == Note::Longest)
                return ;
            noteInserter->slotSetDots(note.getDots() ? 0 : 1);
            setTool(noteInserter);
        } else {
            /* was sorry */ QMessageBox::warning(this, i18n("No note or rest duration selected"));
        }
    }

    setMenuStates();
}

void NotationView::slotTransformsQuantize()
{
    if (!m_currentEventSelection)
        return ;

    QuantizeDialog dialog(this, true);

    if (dialog.exec() == QDialog::Accepted) {
        KTmpStatusMsg msg(i18n("Quantizing..."), this);
        addCommandToHistory(new EventQuantizeCommand
                            (*m_currentEventSelection,
                             dialog.getQuantizer()));
    }
}

void NotationView::slotTransformsInterpret()
{
    if (!m_currentEventSelection)
        return ;

    InterpretDialog dialog(this);

    if (dialog.exec() == QDialog::Accepted) {
        KTmpStatusMsg msg(i18n("Interpreting selection..."), this);
        addCommandToHistory(new InterpretCommand
                            (*m_currentEventSelection,
                             getDocument()->getComposition().getNotationQuantizer(),
                             dialog.getInterpretations()));
    }
}

void NotationView::slotSetNoteDurations(Note::Type type, bool notationOnly)
{
    if (!m_currentEventSelection)
        return ;
    KTmpStatusMsg msg(i18n("Setting note durations..."), this);
    addCommandToHistory(new SetNoteTypeCommand(*m_currentEventSelection, type, notationOnly));
}

void NotationView::slotAddDot()
{
    if (!m_currentEventSelection)
        return ;
    KTmpStatusMsg msg(i18n("Adding dot..."), this);
    addCommandToHistory(new AddDotCommand(*m_currentEventSelection, false));
}

void NotationView::slotAddDotNotationOnly()
{
    if (!m_currentEventSelection)
        return ;
    KTmpStatusMsg msg(i18n("Adding dot..."), this);
    addCommandToHistory(new AddDotCommand(*m_currentEventSelection, true));
}

void
NotationView::slotMakeOrnament()
{
    if (!m_currentEventSelection)
        return ;

    EventSelection::eventcontainer &ec =
        m_currentEventSelection->getSegmentEvents();

    int basePitch = -1;
    int baseVelocity = -1;
    NoteStyle *style = NoteStyleFactory::getStyle(NoteStyleFactory::DefaultStyle);

    for (EventSelection::eventcontainer::iterator i =
             ec.begin(); i != ec.end(); ++i) {
        if ((*i)->isa(Note::EventType)) {
            if ((*i)->has(BaseProperties::PITCH)) {
                basePitch = (*i)->get<Int>(BaseProperties::PITCH);
                style = NoteStyleFactory::getStyleForEvent(*i);
                if (baseVelocity != -1) break;
            }
            if ((*i)->has(BaseProperties::VELOCITY)) {
                baseVelocity = (*i)->get<Int>(BaseProperties::VELOCITY);
                if (basePitch != -1) break;
            }
        }
    }

    Staff *staff = getCurrentStaff();
    Segment &segment = staff->getSegment();

    timeT absTime = m_currentEventSelection->getStartTime();
    timeT duration = m_currentEventSelection->getTotalDuration();
    Note note(Note::getNearestNote(duration));

    Track *track =
        segment.getComposition()->getTrackById(segment.getTrack());
    QString name;
    int barNo = segment.getComposition()->getBarNumber(absTime);
    if (track) {
        name = QString(i18n("Ornament track %1 bar %2", track->getPosition() + 1, barNo + 1));
    } else {
        name = QString(i18n("Ornament bar %1", barNo + 1));
    }

    MakeOrnamentDialog dialog(this, name, basePitch);
    if (dialog.exec() != QDialog::Accepted)
        return ;

    name = dialog.getName();
    basePitch = dialog.getBasePitch();

    MacroCommand *command = new MacroCommand(i18n("Make Ornament"));

    command->addCommand(new CutCommand
                        (*m_currentEventSelection,
                         getDocument()->getClipboard()));

    command->addCommand(new PasteToTriggerSegmentCommand
                        (&getDocument()->getComposition(),
                         getDocument()->getClipboard(),
                         name, basePitch));

    command->addCommand(new InsertTriggerNoteCommand
                        (segment, absTime, note, basePitch, baseVelocity,
                         style->getName(),
                         getDocument()->getComposition().getNextTriggerSegmentId(),
                         true,
                         BaseProperties::TRIGGER_SEGMENT_ADJUST_SQUISH,
                         Marks::NoMark)); //!!!

    addCommandToHistory(command);
}

void
NotationView::slotUseOrnament()
{
    // Take an existing note and match an ornament to it.

    if (!m_currentEventSelection)
        return ;

    UseOrnamentDialog dialog(this, &getDocument()->getComposition());
    if (dialog.exec() != QDialog::Accepted)
        return ;

    addCommandToHistory(new SetTriggerCommand(*m_currentEventSelection,
                                              dialog.getId(),
                                              true,
                                              dialog.getRetune(),
                                              dialog.getTimeAdjust(),
                                              dialog.getMark(),
                                              i18n("Use Ornament")));
}

void
NotationView::slotRemoveOrnament()
{
    if (!m_currentEventSelection)
        return ;

    addCommandToHistory(new ClearTriggersCommand(*m_currentEventSelection,
                                                 i18n("Remove Ornaments")));
}

void NotationView::slotEditAddClef()
{
    Staff *staff = getCurrentStaff();
    Segment &segment = staff->getSegment();
    static Clef lastClef;
    Clef clef;
    Rosegarden::Key key;
    timeT insertionTime = getInsertionTime(clef, key);

    ClefDialog dialog(this, m_notePixmapFactory, lastClef);

    if (dialog.exec() == QDialog::Accepted) {

        ClefDialog::ConversionType conversion = dialog.getConversionType();

        bool shouldChangeOctave = (conversion != ClefDialog::NoConversion);
        bool shouldTranspose = (conversion == ClefDialog::Transpose);

        addCommandToHistory
            (new ClefInsertionCommand
             (segment, insertionTime, dialog.getClef(),
              shouldChangeOctave, shouldTranspose));

        lastClef = dialog.getClef();
    }
}

void NotationView::slotEditAddKeySignature()
{
    Staff *staff = getCurrentStaff();
    Segment &segment = staff->getSegment();
    Clef clef;
    Rosegarden::Key key;
    timeT insertionTime = getInsertionTime(clef, key);

    //!!! experimental:
    CompositionTimeSliceAdapter adapter
        (&getDocument()->getComposition(), insertionTime,
         getDocument()->getComposition().getDuration());
    AnalysisHelper helper;
    key = helper.guessKey(adapter);

    KeySignatureDialog dialog
        (this, m_notePixmapFactory, clef, key, true, true,
         i18n("Estimated key signature shown"));

    if (dialog.exec() == QDialog::Accepted &&
        dialog.isValid()) {

        KeySignatureDialog::ConversionType conversion =
            dialog.getConversionType();

        bool transposeKey = dialog.shouldBeTransposed();
        bool applyToAll = dialog.shouldApplyToAll();
	bool ignorePercussion = dialog.shouldIgnorePercussion();

        if (applyToAll) {
            addCommandToHistory
                (new MultiKeyInsertionCommand
                 (getDocument(),
                  insertionTime, dialog.getKey(),
                  conversion == KeySignatureDialog::Convert,
                  conversion == KeySignatureDialog::Transpose,
                  transposeKey,
		  ignorePercussion));
        } else {
            addCommandToHistory
                (new KeyInsertionCommand
                 (segment,
                  insertionTime, dialog.getKey(),
                  conversion == KeySignatureDialog::Convert,
                  conversion == KeySignatureDialog::Transpose,
                  transposeKey,
		  false));
        }
    }
}

void NotationView::slotEditAddSustain(bool down)
{
    Staff *staff = getCurrentStaff();
    Segment &segment = staff->getSegment();
    timeT insertionTime = getInsertionTime();

    Studio *studio = &getDocument()->getStudio();
    Track *track = segment.getComposition()->getTrackById(segment.getTrack());

    if (track) {

        Instrument *instrument = studio->getInstrumentById
            (track->getInstrument());
        if (instrument) {
            MidiDevice *device = dynamic_cast<MidiDevice *>
                (instrument->getDevice());
            if (device) {
                for (ControlList::const_iterator i =
                         device->getControlParameters().begin();
                     i != device->getControlParameters().end(); ++i) {

                    if (i->getType() == Controller::EventType &&
                        (i->getName() == "Sustain" ||
                         strtoqstr(i->getName()) == i18n("Sustain"))) {

                        addCommandToHistory
                            (new SustainInsertionCommand(segment, insertionTime, down,
                                                         i->getControllerValue()));
                        return ;
                    }
                }
            } else if (instrument->getDevice() &&
                       instrument->getDevice()->getType() == Device::SoftSynth) {
                addCommandToHistory
                    (new SustainInsertionCommand(segment, insertionTime, down, 64));
            }
        }
    }

    /* was sorry */ QMessageBox::warning(this, i18n("There is no sustain controller defined for this device.\nPlease ensure the device is configured correctly in the Manage MIDI Devices dialog in the main window."));
}

void NotationView::slotEditAddSustainDown()
{
    slotEditAddSustain(true);
}

void NotationView::slotEditAddSustainUp()
{
    slotEditAddSustain(false);
}

void NotationView::slotEditTranspose()
{
    IntervalDialog intervalDialog(this, true, true);
    int ok = intervalDialog.exec();
    
    int semitones = intervalDialog.getChromaticDistance();
    int steps = intervalDialog.getDiatonicDistance();

    if (!ok || (semitones == 0 && steps == 0)) return;

    // TODO combine commands into one 
    for (int i = 0; i < m_segments.size(); i++)
    {
        addCommandToHistory(new SegmentTransposeCommand(*(m_segments[i]), 
            intervalDialog.getChangeKey(), steps, semitones, 
            intervalDialog.getTransposeSegmentBack()));
    }

    // Fix #1885520 (Update track parameter widget when transpose changed from notation)
    RosegardenGUIApp::self()->getView()->getTrackParameterBox()->slotUpdateControls(-1);

    // And update track headers likewise
    m_headersGroup->slotUpdateAllHeaders(getCanvasLeftX(), 0, true);
}

void NotationView::slotEditSwitchPreset()
{
    PresetHandlerDialog dialog(this, true);
    
    if (dialog.exec() != QDialog::Accepted) return;
    
    if (dialog.getConvertAllSegments()) {
        // get all segments for this track and convert them.
        Composition& comp = getDocument()->getComposition();
        TrackId selectedTrack = getCurrentSegment()->getTrack();

	// satisfy #1885251 the way that seems most reasonble to me at the
	// moment, only changing track parameters when acting on all segments on
	// this track from the notation view 
	//
	//!!! This won't be undoable, and I'm not sure if that's seriously
	// wrong, or just mildly wrong, but I'm betting somebody will tell me
	// about it if this was inappropriate
	Track *track = comp.getTrackById(selectedTrack);
	track->setPresetLabel(dialog.getName());
	track->setClef(dialog.getClef());
	track->setTranspose(dialog.getTranspose());
	track->setLowestPlayable(dialog.getLowRange());
	track->setHighestPlayable(dialog.getHighRange());

        addCommandToHistory(new SegmentSyncCommand(comp.getSegments(), selectedTrack,
                            dialog.getTranspose(), 
                            dialog.getLowRange(), 
                            dialog.getHighRange(),
                            clefIndexToClef(dialog.getClef())));
    } else {
        addCommandToHistory(new SegmentSyncCommand(m_segments, 
                            dialog.getTranspose(), 
                            dialog.getLowRange(), 
                            dialog.getHighRange(),
                            clefIndexToClef(dialog.getClef())));
    }

    m_doc->slotDocumentModified();

    // Fix #1885520 (Update track parameter widget when preset changed from notation)
    RosegardenGUIApp::self()->getView()->getTrackParameterBox()->slotUpdateControls(-1);
}

void NotationView::slotEditElement(NotationStaff *staff,
                                   NotationElement *element, bool advanced)
{
    if (advanced) {

        EventEditDialog dialog(this, *element->event(), true);

        if (dialog.exec() == QDialog::Accepted &&
            dialog.isModified()) {

            EventEditCommand *command = new EventEditCommand
                (staff->getSegment(),
                 element->event(),
                 dialog.getEvent());

            addCommandToHistory(command);
        }

    } else if (element->event()->isa(Clef::EventType)) {

        try {
            ClefDialog dialog(this, m_notePixmapFactory,
                              Clef(*element->event()));

            if (dialog.exec() == QDialog::Accepted) {

                ClefDialog::ConversionType conversion = dialog.getConversionType();
                bool shouldChangeOctave = (conversion != ClefDialog::NoConversion);
                bool shouldTranspose = (conversion == ClefDialog::Transpose);
                addCommandToHistory
                    (new ClefInsertionCommand
                     (staff->getSegment(), element->event()->getAbsoluteTime(),
                      dialog.getClef(), shouldChangeOctave, shouldTranspose));
            }
        } catch (Exception e) {
            std::cerr << e.getMessage() << std::endl;
        }

        return ;

    } else if (element->event()->isa(Rosegarden::Key::EventType)) {

        try {
            Clef clef(staff->getSegment().getClefAtTime
                      (element->event()->getAbsoluteTime()));
            KeySignatureDialog dialog
                (this, m_notePixmapFactory, clef, Rosegarden::Key(*element->event()),
                 false, true);

            if (dialog.exec() == QDialog::Accepted &&
                dialog.isValid()) {

                KeySignatureDialog::ConversionType conversion =
                    dialog.getConversionType();

                addCommandToHistory
                    (new KeyInsertionCommand
                     (staff->getSegment(),
                      element->event()->getAbsoluteTime(), dialog.getKey(),
                      conversion == KeySignatureDialog::Convert,
                      conversion == KeySignatureDialog::Transpose,
                      dialog.shouldBeTransposed(),
		      dialog.shouldIgnorePercussion()));
            }

        } catch (Exception e) {
            std::cerr << e.getMessage() << std::endl;
        }

        return ;

    } else if (element->event()->isa(Text::EventType)) {

        try {
            TextEventDialog dialog
                (this, m_notePixmapFactory, Text(*element->event()));
            if (dialog.exec() == QDialog::Accepted) {
                TextInsertionCommand *command = new TextInsertionCommand
                    (staff->getSegment(),
                     element->event()->getAbsoluteTime(),
                     dialog.getText());
                MacroCommand *macroCommand = new MacroCommand(command->objectName());
                macroCommand->addCommand(new EraseEventCommand(staff->getSegment(),
                                                               element->event(), false));
                macroCommand->addCommand(command);
                addCommandToHistory(macroCommand);
            }
        } catch (Exception e) {
            std::cerr << e.getMessage() << std::endl;
        }

        return ;

    } else if (element->isNote() &&
               element->event()->has(BaseProperties::TRIGGER_SEGMENT_ID)) {

        int id = element->event()->get
            <Int>
            (BaseProperties::TRIGGER_SEGMENT_ID);
        emit editTriggerSegment(id);
        return ;

    } else {

        SimpleEventEditDialog dialog(this, getDocument(), *element->event(), false);

        if (dialog.exec() == QDialog::Accepted &&
            dialog.isModified()) {

            EventEditCommand *command = new EventEditCommand
                (staff->getSegment(),
                 element->event(),
                 dialog.getEvent());

            addCommandToHistory(command);
        }
    }
}

void NotationView::slotBeginLilyPondRepeat()
{}

void NotationView::slotDebugDump()
{
    if (m_currentEventSelection) {
        EventSelection::eventcontainer &ec =
            m_currentEventSelection->getSegmentEvents();
        int n = 0;
        for (EventSelection::eventcontainer::iterator i =
                 ec.begin();
             i != ec.end(); ++i) {
            std::cerr << "\n" << n++ << " [" << (*i) << "]" << std::endl;
            (*i)->dump(std::cerr);
        }
    }
}

void
NotationView::slotSetPointerPosition(timeT time)
{
    slotSetPointerPosition(time, m_playTracking);
}

void
NotationView::slotSetPointerPosition(timeT time, bool scroll)
{
    Composition &comp = getDocument()->getComposition();
    int barNo = comp.getBarNumber(time);

    int minCy = 0;
    double cx = 0;
    bool haveMinCy = false;

    for (unsigned int i = 0; i < m_staffs.size(); ++i) {

        double layoutX = m_hlayout->getXForTimeByEvent(time);
        Segment &seg = m_staffs[i]->getSegment();

        bool good = true;

        if (barNo >= m_hlayout->getLastVisibleBarOnStaff(*m_staffs[i])) {
            if (seg.isRepeating() && time < seg.getRepeatEndTime()) {
                timeT mappedTime =
                    seg.getStartTime() +
                    ((time - seg.getStartTime()) %
                     (seg.getEndMarkerTime() - seg.getStartTime()));
                layoutX = m_hlayout->getXForTimeByEvent(mappedTime);
            } else {
                good = false;
            }
        } else if (barNo < m_hlayout->getFirstVisibleBarOnStaff(*m_staffs[i])) {
            good = false;
        }

        if (!good) {

            m_staffs[i]->hidePointer();

        } else {

            m_staffs[i]->setPointerPosition(layoutX);

            int cy;
            m_staffs[i]->getPointerPosition(cx, cy);

            if (!haveMinCy || cy < minCy) {
                minCy = cy;
                haveMinCy = true;
            }
        }
    }

    if (m_pageMode == LinedStaff::LinearMode) {
        // be careful not to prevent user from scrolling up and down
        haveMinCy = false;
    }

    if (scroll) {
        getCanvasView()->slotScrollHoriz(int(cx));
        if (haveMinCy) {
            getCanvasView()->slotScrollVertToTop(minCy);
        }
    }

    updateView();
}

void
NotationView::slotUpdateRecordingSegment(Segment *segment,
                                         timeT updateFrom)
{
    NOTATION_DEBUG << "NotationView::slotUpdateRecordingSegment: segment " << segment << ", updateFrom " << updateFrom << ", end time " << segment->getEndMarkerTime() << endl;
    if (updateFrom >= segment->getEndMarkerTime())
        return ;
    for (unsigned int i = 0; i < m_staffs.size(); ++i) {
        if (&m_staffs[i]->getSegment() == segment) {
            refreshSegment(segment, 0, 0);
        }
    }
    NOTATION_DEBUG << "NotationView::slotUpdateRecordingSegment: don't have segment " << segment << endl;
}

void
NotationView::slotSetCurrentStaff(double x, int y)
{
    unsigned int staffNo;
    for (staffNo = 0; staffNo < m_staffs.size(); ++staffNo) {
        if (m_staffs[staffNo]->containsCanvasCoords(x, y))
            break;
    }

    if (staffNo < m_staffs.size()) {
        slotSetCurrentStaff(staffNo);
    }
}

void
NotationView::slotSetCurrentStaff(int staffNo)
{
    NOTATION_DEBUG << "NotationView::slotSetCurrentStaff(" << staffNo << ")" << endl;

    if (m_currentStaff != staffNo) {

        m_staffs[m_currentStaff]->setCurrent(false);

        m_currentStaff = staffNo;

        m_staffs[m_currentStaff]->setCurrent(true);

        Segment *segment = &m_staffs[m_currentStaff]->getSegment();

        m_chordNameRuler->setCurrentSegment(segment);
        m_rawNoteRuler->setCurrentSegment(segment);
        m_rawNoteRuler->repaint();
        setControlRulersCurrentSegment();

        updateView();

        slotSetInsertCursorPosition(getInsertionTime(), false, true);

        m_headersGroup->setCurrent(
                                m_staffs[staffNo]->getSegment().getTrack());
    }
}

void
NotationView::slotCurrentStaffUp()
{
    LinedStaff *staff = getStaffAbove();
    if (!staff) return;
    slotSetCurrentStaff(staff->getId());
}

void
NotationView::slotCurrentStaffDown()
{
    LinedStaff *staff = getStaffBelow();
    if (!staff) return;
    slotSetCurrentStaff(staff->getId());
}

void
NotationView::slotCurrentSegmentPrior()
{
    if (m_staffs.size() < 2)
        return ;

    Composition *composition =
        m_staffs[m_currentStaff]->getSegment().getComposition();

    Track *track = composition->
        getTrackById(m_staffs[m_currentStaff]->getSegment().getTrack());
    if (!track)
        return ;

    int lastStaffOnTrack = -1;

    //
    // TODO: Cycle segments through rather in time order?
    //       Cycle only segments in the field of view?
    //
    for (int i = m_staffs.size()-1; i >= 0; --i) {
        if (m_staffs[i]->getSegment().getTrack() == track->getId()) {
	    if (lastStaffOnTrack < 0) {
                lastStaffOnTrack = i;
	    } 
	    if (i < m_currentStaff) {
		slotSetCurrentStaff(i);
		slotEditSelectWholeStaff();
		return ;
	    }
        }
    }
    if (lastStaffOnTrack >= 0) {
	slotSetCurrentStaff(lastStaffOnTrack);
	slotEditSelectWholeStaff();
	return ;
    }
}

void
NotationView::slotCurrentSegmentNext()
{
    if (m_staffs.size() < 2)
        return ;

    Composition *composition =
        m_staffs[m_currentStaff]->getSegment().getComposition();

    Track *track = composition->
        getTrackById(m_staffs[m_currentStaff]->getSegment().getTrack());
    if (!track)
        return ;

    int firstStaffOnTrack = -1;

    //
    // TODO: Cycle segments through rather in time order?
    //       Cycle only segments in the field of view?
    //
    for (unsigned int i = 0; i < m_staffs.size(); ++i) {
        if (m_staffs[i]->getSegment().getTrack() == track->getId()) {
	    if (firstStaffOnTrack < 0) {
                firstStaffOnTrack = i;
	    } 
	    if (i > m_currentStaff) {
		slotSetCurrentStaff(i);
		slotEditSelectWholeStaff();
		return ;
	    }
        }
    }
    if (firstStaffOnTrack >= 0) {
	slotSetCurrentStaff(firstStaffOnTrack);
	slotEditSelectWholeStaff();
	return ;
    }
}

void
NotationView::slotSetInsertCursorPosition(double x, int y, bool scroll,
                                          bool updateNow)
{
    NOTATION_DEBUG << "NotationView::slotSetInsertCursorPosition: x " << x << ", y " << y << ", scroll " << scroll << ", now " << updateNow << endl;

    slotSetCurrentStaff(x, y);

    LinedStaff *staff = getLinedStaff(m_currentStaff);
    Event *clefEvt, *keyEvt;
    NotationElementList::iterator i =
        staff->getElementUnderCanvasCoords(x, y, clefEvt, keyEvt);

    if (i == staff->getViewElementList()->end()) {
        slotSetInsertCursorPosition(staff->getSegment().getEndTime(), scroll,
                                    updateNow);
    } else {
        slotSetInsertCursorPosition((*i)->getViewAbsoluteTime(), scroll,
                                    updateNow);
    }
}

void
NotationView::slotSetInsertCursorPosition(timeT t, bool scroll, bool updateNow)
{
    NOTATION_DEBUG << "NotationView::slotSetInsertCursorPosition: time " << t << ", scroll " << scroll << ", now " << updateNow << endl;

    m_insertionTime = t;
    if (scroll) {
        m_deferredCursorMove = CursorMoveAndMakeVisible;
    } else {
        m_deferredCursorMove = CursorMoveOnly;
    }
    if (updateNow)
        doDeferredCursorMove();
}

void
NotationView::slotSetInsertCursorAndRecentre(timeT t, double cx, int,
                                             bool updateNow)
{
    NOTATION_DEBUG << "NotationView::slotSetInsertCursorAndRecentre: time " << t << ", cx " << cx << ", now " << updateNow << ", contentsx" << getCanvasView()->contentsX() << ", w " << getCanvasView()->visibleWidth() << endl;

    m_insertionTime = t;

    // We only do the scroll bit if cx is in the right two-thirds of
    // the window

    if (cx < (getCanvasView()->contentsX() +
              getCanvasView()->visibleWidth() / 3)) {

        m_deferredCursorMove = CursorMoveOnly;
    } else {
        m_deferredCursorMove = CursorMoveAndScrollToPosition;
        m_deferredCursorScrollToX = cx;
    }

    if (updateNow)
        doDeferredCursorMove();
}

void
NotationView::doDeferredCursorMove()
{
    NOTATION_DEBUG << "NotationView::doDeferredCursorMove: m_deferredCursorMove == " << m_deferredCursorMove << endl;

    if (m_deferredCursorMove == NoCursorMoveNeeded) {
        return ;
    }

    DeferredCursorMoveType type = m_deferredCursorMove;
    m_deferredCursorMove = NoCursorMoveNeeded;

    timeT t = m_insertionTime;

    if (m_staffs.size() == 0)
        return ;
    LinedStaff *staff = getCurrentLinedStaff();
    Segment &segment = staff->getSegment();

    if (t < segment.getStartTime()) {
        t = segment.getStartTime();
    }
    if (t > segment.getEndTime()) {
        t = segment.getEndTime();
    }

    NotationElementList::iterator i =
        staff->getViewElementList()->findNearestTime(t);

    //
    // Up to this point everything goes ok when adding the first note in the 
    // beginning of the composition.
    //
    // However, there is a BUG with rests: not all rests have an associated
    // canvas. The rests which do not have an associated canvas are not
    // recognized by the following code and the _cursor position_ is not
    // updated correctly to be just after the added note when the first note 
    // of a segment have been added in the beginning of the segment.
    //
    // Why the canvas item is missing for the predefined rests ? (hjj)
    //
    while (i != staff->getViewElementList()->end() &&
           !static_cast<NotationElement*>(*i)->getCanvasItem())
        ++i;

    if (i == staff->getViewElementList()->end()) {
        //!!! ???
        if (m_insertionTime >= staff->getSegment().getStartTime()) {
            i = staff->getViewElementList()->begin();
        }
        m_insertionTime = staff->getSegment().getStartTime();
    } else {
        m_insertionTime = static_cast<NotationElement*>(*i)->getViewAbsoluteTime();
    }

    if (i == staff->getViewElementList()->end() ||
        t == segment.getEndTime() ||
        t == segment.getBarStartForTime(t)) {

        staff->setInsertCursorPosition(*m_hlayout, t);

        if (type == CursorMoveAndMakeVisible) {
            double cx;
            int cy;
            staff->getInsertCursorPosition(cx, cy);
            getCanvasView()->slotScrollHoriz(int(cx));
            getCanvasView()->slotScrollVertSmallSteps(cy);
        }

    } else {

        // prefer a note or rest, if there is one, to a non-spacing event
        if (!static_cast<NotationElement*>(*i)->isNote() &&
            !static_cast<NotationElement*>(*i)->isRest()) {
            NotationElementList::iterator j = i;
            while (j != staff->getViewElementList()->end()) {
                if (static_cast<NotationElement*>(*j)->getViewAbsoluteTime() !=
                    static_cast<NotationElement*>(*i)->getViewAbsoluteTime())
                    break;
                if (static_cast<NotationElement*>(*j)->getCanvasItem()) {
                    if (static_cast<NotationElement*>(*j)->isNote() ||
                        static_cast<NotationElement*>(*j)->isRest()) {
                        i = j;
                        break;
                    }
                }
                ++j;
            }
        }

        if (static_cast<NotationElement*>(*i)->getCanvasItem()) {

            staff->setInsertCursorPosition
                (static_cast<NotationElement*>(*i)->getCanvasX() - 2,
                 int(static_cast<NotationElement*>(*i)->getCanvasY()));

            if (type == CursorMoveAndMakeVisible) {
                getCanvasView()->slotScrollHoriz
                    (int(static_cast<NotationElement*>(*i)->getCanvasX()) - 4);
            }
        } else {
            std::cerr << "WARNING: No canvas item for this notation element:";
            (*i)->event()->dump(std::cerr);
        }
    }

    if (type == CursorMoveAndScrollToPosition) {

        // get current canvas x of insert cursor, which might not be
        // what we just set

        double ccx = 0.0;

        NotationElementList::iterator i =
            staff->getViewElementList()->findTime(t);

        if (i == staff->getViewElementList()->end()) {
            if (i == staff->getViewElementList()->begin())
                return ;
            double lx, lwidth;
            --i;
            if (static_cast<NotationElement*>(*i)->getCanvasItem()) {
                ccx = static_cast<NotationElement*>(*i)->getCanvasX();
                static_cast<NotationElement*>(*i)->getLayoutAirspace(lx, lwidth);
            } else {
                std::cerr << "WARNING: No canvas item for this notation element*:";
                (*i)->event()->dump(std::cerr);
            }
            ccx += lwidth;
        } else {
            if (static_cast<NotationElement*>(*i)->getCanvasItem()) {
                ccx = static_cast<NotationElement*>(*i)->getCanvasX();
            } else {
                std::cerr << "WARNING: No canvas item for this notation element*:";
                (*i)->event()->dump(std::cerr);
            }
        }

        QScrollBar* hbar = getCanvasView()->horizontalScrollBar();
        hbar->setValue(int(hbar->value() - (m_deferredCursorScrollToX - ccx)));
    }

    updateView();
}

void
NotationView::slotJumpCursorToPlayback()
{
    slotSetInsertCursorPosition(getDocument()->getComposition().getPosition());
}

void
NotationView::slotJumpPlaybackToCursor()
{
    emit jumpPlaybackTo(getInsertionTime());
}

void
NotationView::slotToggleTracking()
{
    m_playTracking = !m_playTracking;
}

void NotationView::slotNoAccidental()
{
    emit changeAccidental(Accidentals::NoAccidental, false);
}

void NotationView::slotFollowAccidental()
{
    emit changeAccidental(Accidentals::NoAccidental, true);
}

void NotationView::slotSharp()
{
    emit changeAccidental(Accidentals::Sharp, false);
}

void NotationView::slotFlat()
{
    emit changeAccidental(Accidentals::Flat, false);
}

void NotationView::slotNatural()
{
    emit changeAccidental(Accidentals::Natural, false);
}

void NotationView::slotDoubleSharp()
{
    emit changeAccidental(Accidentals::DoubleSharp, false);
}

void NotationView::slotDoubleFlat()
{
    emit changeAccidental(Accidentals::DoubleFlat, false);
}

void NotationView::slotTrebleClef()
{
    m_currentNotePixmap->setPixmap
        (NotePixmapFactory::toQPixmap(NotePixmapFactory::makeToolbarPixmap("clef-treble")));
    setTool(m_toolBox->getTool(ClefInserter::ToolName));

    dynamic_cast<ClefInserter*>(m_tool)->setClef(Clef::Treble);
    setMenuStates();
}

void NotationView::slotAltoClef()
{
    m_currentNotePixmap->setPixmap
        (NotePixmapFactory::toQPixmap(NotePixmapFactory::makeToolbarPixmap("clef-alto")));
    setTool(m_toolBox->getTool(ClefInserter::ToolName));

    dynamic_cast<ClefInserter*>(m_tool)->setClef(Clef::Alto);
    setMenuStates();
}

void NotationView::slotTenorClef()
{
    m_currentNotePixmap->setPixmap
        (NotePixmapFactory::toQPixmap(NotePixmapFactory::makeToolbarPixmap("clef-tenor")));
    setTool(m_toolBox->getTool(ClefInserter::ToolName));

    dynamic_cast<ClefInserter*>(m_tool)->setClef(Clef::Tenor);
    setMenuStates();
}

void NotationView::slotBassClef()
{
    m_currentNotePixmap->setPixmap
        (NotePixmapFactory::toQPixmap(NotePixmapFactory::makeToolbarPixmap("clef-bass")));
    setTool(m_toolBox->getTool(ClefInserter::ToolName));

    dynamic_cast<ClefInserter*>(m_tool)->setClef(Clef::Bass);
    setMenuStates();
}

void NotationView::slotText()
{
    m_currentNotePixmap->setPixmap
        (NotePixmapFactory::toQPixmap(NotePixmapFactory::makeToolbarPixmap("text")));
    setTool(m_toolBox->getTool(TextInserter::ToolName));
    setMenuStates();
}

void NotationView::slotGuitarChord()
{
    m_currentNotePixmap->setPixmap
        (NotePixmapFactory::toQPixmap(NotePixmapFactory::makeToolbarPixmap("guitarchord")));
    setTool(m_toolBox->getTool(GuitarChordInserter::ToolName));
    setMenuStates();
}

void NotationView::slotEraseSelected()
{
    NOTATION_DEBUG << "NotationView::slotEraseSelected()" << endl;
    setTool(m_toolBox->getTool(NotationEraser::ToolName));
    setMenuStates();
}

void NotationView::slotSelectSelected()
{
    NOTATION_DEBUG << "NotationView::slotSelectSelected()" << endl;
    setTool(m_toolBox->getTool(NotationSelector::ToolName));
    setMenuStates();
}

void NotationView::slotLinearMode()
{
    setPageMode(LinedStaff::LinearMode);
}

void NotationView::slotContinuousPageMode()
{
    setPageMode(LinedStaff::ContinuousPageMode);
}

void NotationView::slotMultiPageMode()
{
    setPageMode(LinedStaff::MultiPageMode);
}

void NotationView::slotToggleChordsRuler()
{
    if (m_hlayout->isPageMode())
        return ;
    toggleWidget(m_chordNameRuler, "show_chords_ruler");
}

void NotationView::slotToggleRawNoteRuler()
{
    if (m_hlayout->isPageMode())
        return ;
    toggleWidget(m_rawNoteRuler, "show_raw_note_ruler");
}

void NotationView::slotToggleTempoRuler()
{
    if (m_hlayout->isPageMode())
        return ;
    toggleWidget(m_tempoRuler, "show_tempo_ruler");
}

void NotationView::slotToggleAnnotations()
{
    m_annotationsVisible = !m_annotationsVisible;
    slotUpdateAnnotationsStatus();
    //!!! use refresh mechanism
    refreshSegment(0, 0, 0);
}

void NotationView::slotToggleLilyPondDirectives()
{
    m_lilyPondDirectivesVisible = !m_lilyPondDirectivesVisible;
    slotUpdateLilyPondDirectivesStatus();
    //!!! use refresh mechanism
    refreshSegment(0, 0, 0);
}

void NotationView::slotEditLyrics()
{
    Staff *staff = getCurrentStaff();
    Segment &segment = staff->getSegment();
    int oldVerseCount = 1;
    
    // The loop below is identical with the one in LyricEditDialog::countVerses() 
    // Maybe countVerses() should be moved to a Segment manipulating class ? (hjj)
    for (Segment::iterator i = (&segment)->begin();
         (&segment)->isBeforeEndMarker(i); ++i) {

        if ((*i)->isa(Text::EventType)) {

            std::string textType;
            if ((*i)->get<String>(Text::TextTypePropertyName, textType) &&
                textType == Text::Lyric) {

                long verse = 0;
                (*i)->get<Int>(Text::LyricVersePropertyName, verse);

                if (verse >= oldVerseCount) oldVerseCount = verse + 1;
            }
        }
    }

    LyricEditDialog dialog(this, &segment);

    if (dialog.exec() == QDialog::Accepted) {

        MacroCommand *macro = new MacroCommand
            (SetLyricsCommand::getGlobalName());

        for (int i = 0; i < dialog.getVerseCount(); ++i) {
            SetLyricsCommand *command = new SetLyricsCommand
                (&segment, i, dialog.getLyricData(i));
            macro->addCommand(command);
        }
        for (int i = dialog.getVerseCount(); i < oldVerseCount; ++i) {
	    // (hjj) verse count decreased, delete extra verses.
            SetLyricsCommand *command = new SetLyricsCommand
                (&segment, i, QString(""));
            macro->addCommand(command);
        }

        addCommandToHistory(macro);
    }
}

void NotationView::slotItemPressed(int height, int staffNo,
                                   QMouseEvent* e,
                                   NotationElement* el)
{
    NOTATION_DEBUG << "NotationView::slotItemPressed(height = "
                   << height << ", staffNo = " << staffNo
                   << ")" << endl;

    if (staffNo < 0 && el != 0) {
        // We have an element but no staff -- that's because the
        // element extended outside the staff region.  But we need
        // to handle it properly, so we rather laboriously need to
        // find out which staff it was.
        for (unsigned int i = 0; i < m_staffs.size(); ++i) {
            if (m_staffs[i]->getViewElementList()->findSingle(el) !=
                m_staffs[i]->getViewElementList()->end()) {
                staffNo = m_staffs[i]->getId();
                break;
            }
        }
    }

    ButtonState btnState = e->state();

    if (btnState & ControlButton) { // on ctrl-click, set cursor position

        slotSetInsertCursorPosition(e->x(), (int)e->y());

    } else {

        setActiveItem(0);

        timeT unknownTime = 0;

        if (e->type() == QEvent::MouseButtonDblClick) {
            m_tool->handleMouseDoubleClick(unknownTime, height,
                                           staffNo, e, el);
        } else {
            m_tool->handleMousePress(unknownTime, height,
                                     staffNo, e, el);
        }
    }
}

void NotationView::slotNonNotationItemPressed(QMouseEvent *e, Q3CanvasItem *it)
{
    if (e->type() != QEvent::MouseButtonDblClick)
        return ;

    Staff *staff = getStaffForCanvasCoords(e->x(), e->y());
    if (!staff)
        return ;

    NOTATION_DEBUG << "NotationView::slotNonNotationItemPressed(doubly)" << endl;

    if (dynamic_cast<QCanvasStaffNameSprite *>(it)) {

        std::string name =
            staff->getSegment().getComposition()->
            getTrackById(staff->getSegment().getTrack())->getLabel();

        bool ok = false;
        QRegExpValidator validator(QRegExp(".*"), this); // empty is OK

        QString newText = KLineEditDlg::getText(QString("Change staff name"),
                                                QString("Enter new staff name"),
                                                strtoqstr(name),
                                                &ok,
                                                this,
                                                &validator);

        if (ok) {
            addCommandToHistory(new RenameTrackCommand
                                (staff->getSegment().getComposition(),
                                 staff->getSegment().getTrack(),
                                 qstrtostr(newText)));

            emit staffLabelChanged(staff->getSegment().getTrack(), newText);
        }

    } else if (dynamic_cast<QCanvasTimeSigSprite *>(it)) {

        double layoutX = (dynamic_cast<QCanvasTimeSigSprite *>(it))->getLayoutX();
        emit editTimeSignature(m_hlayout->getTimeForX(layoutX));
    }
}

void NotationView::slotTextItemPressed(QMouseEvent *e, Q3CanvasItem *it)
{
    if (e->type() != QEvent::MouseButtonDblClick)
        return ;

    if (it == m_title) {
        emit editMetadata(strtoqstr(CompositionMetadataKeys::Title.getName()));
    } else if (it == m_subtitle) {
        emit editMetadata(strtoqstr(CompositionMetadataKeys::Subtitle.getName()));
    } else if (it == m_composer) {
        emit editMetadata(strtoqstr(CompositionMetadataKeys::Composer.getName()));
    } else if (it == m_copyright) {
        emit editMetadata(strtoqstr(CompositionMetadataKeys::Copyright.getName()));
    } else {
        return ;
    }

    positionStaffs();
}

void NotationView::slotMouseMoved(QMouseEvent *e)
{
    if (activeItem()) {
        activeItem()->handleMouseMove(e);
        updateView();
    } else {
        int follow = m_tool->handleMouseMove(0, 0,  // unknown time and height
                                             e);

        if (getCanvasView()->isTimeForSmoothScroll()) {

            if (follow & RosegardenCanvasView::FollowHorizontal) {
                getCanvasView()->slotScrollHorizSmallSteps(e->x());
            }

            if (follow & RosegardenCanvasView::FollowVertical) {
                getCanvasView()->slotScrollVertSmallSteps(e->y());
            }

        }
    }
}

void NotationView::slotMouseReleased(QMouseEvent *e)
{
    if (activeItem()) {
        activeItem()->handleMouseRelease(e);
        setActiveItem(0);
        updateView();
    } else
        m_tool->handleMouseRelease(0, 0,  // unknown time and height
                                   e);
}

void
NotationView::slotHoveredOverNoteChanged(const QString &noteName)
{
    m_hoveredOverNoteName->setText(QString(" ") + noteName);
}

void
NotationView::slotHoveredOverAbsoluteTimeChanged(unsigned int time)
{
    timeT t = time;
    RealTime rt =
        getDocument()->getComposition().getElapsedRealTime(t);
    long ms = rt.msec();

    int bar, beat, fraction, remainder;
    getDocument()->getComposition().getMusicalTimeForAbsoluteTime
        (t, bar, beat, fraction, remainder);

    //    QString message;
    //    QString format("%ld (%ld.%03lds)");
    //    format = i18n("Time: %1").arg(format);
    //    message.sprintf(format, t, rt.sec, ms);

    QString message = i18n("Time: %1 (%2.%3s)",
         QString("%1-%2-%3-%4")
             .arg(QString("%1").arg(bar + 1).rightJustify(3, '0'))
             .arg(QString("%1").arg(beat).rightJustify(2, '0'))
             .arg(QString("%1").arg(fraction).rightJustify(2, '0'))
             .arg(QString("%1").arg(remainder).rightJustify(2, '0')),
         rt.sec,
         QString("%1").arg(ms).rightJustify(3, '0'));

    m_hoveredOverAbsoluteTime->setText(message);
}

void
NotationView::slotInsertableNoteEventReceived(int pitch, int velocity, bool noteOn)
{
    //!!! Problematic.  Ideally we wouldn't insert events into windows
    //that weren't actually visible, otherwise all hell could break
    //loose (metaphorically speaking, I should probably add).  I did
    //think of checking isActiveWindow() and returning if the current
    //window wasn't active, but that will prevent anyone from
    //step-recording from e.g. vkeybd, which cannot be used without
    //losing focus (and thus active-ness) from the Rosegarden window.

    //!!! I know -- we'll keep track of which edit view (or main view,
    //or mixer, etc) is active, and we'll only allow insertion into
    //the most recently activated.  How about that?

    /* was toggle */ QAction *action = dynamic_cast<QAction*>
        (actionCollection()->action("toggle_step_by_step"));
    if (!action) {
        NOTATION_DEBUG << "WARNING: No toggle_step_by_step action" << endl;
        return ;
    }
    if (!action->isChecked())
        return ;

    Segment &segment = m_staffs[m_currentStaff]->getSegment();

    NoteInserter *noteInserter = dynamic_cast<NoteInserter *>(m_tool);
    if (!noteInserter) {
        static bool showingError = false;
        if (showingError)
            return ;
        showingError = true;
        /* was sorry */ QMessageBox::warning(this, i18n("Can't insert note: No note duration selected"));
        showingError = false;
        return ;
    }

    if (m_inPaintEvent) {
        NOTATION_DEBUG << "NotationView::slotInsertableNoteEventReceived: in paint event already" << endl;
        if (noteOn) {
            m_pendingInsertableNotes.push_back(std::pair<int, int>(pitch, velocity));
        }
        return ;
    }

    // If the segment is transposed, we want to take that into
    // account.  But the note has already been played back to the user
    // at its untransposed pitch, because that's done by the MIDI THRU
    // code in the sequencer which has no way to know whether a note
    // was intended for step recording.  So rather than adjust the
    // pitch for playback according to the transpose setting, we have
    // to adjust the stored pitch in the opposite direction.

    pitch -= segment.getTranspose();

    //    KTmpStatusMsg msg(i18n("Inserting note"), this);

    // We need to ensure that multiple notes hit at once come out as
    // chords, without imposing the interpretation that overlapping
    // notes are always chords and without getting too involved with
    // the actual absolute times of the notes (this is still step
    // editing, not proper recording).

    // First, if we're in chord mode, there's no problem.

    static int numberOfNotesOn = 0;
    static timeT insertionTime = getInsertionTime();
    static time_t lastInsertionTime = 0;

    if (isInChordMode()) {
        if (!noteOn)
            return ;
        NOTATION_DEBUG << "Inserting note in chord at pitch " << pitch << endl;
        noteInserter->insertNote(segment, getInsertionTime(), pitch,
                                 Accidentals::NoAccidental,
                                 true);

    } else {

        if (!noteOn) {
            numberOfNotesOn--;
        } else if (noteOn) {
            // Rules:
            //
            // * If no other note event has turned up within half a
            //   second, insert this note and advance.
            //
            // * Relatedly, if this note is within half a second of
            //   the previous one, they're chords.  Insert the previous
            //   one, don't advance, and use the same rules for this.
            //
            // * If a note event turns up before that time has elapsed,
            //   we need to wait for the note-off events: if the second
            //   note happened less than half way through the first,
            //   it's a chord.
            //
            // We haven't implemented these yet... For now:
            //
            // Rules (hjj):
            //
            // * The overlapping notes are always included in to a chord.
            //   This is the most convenient for step inserting of chords.
            //
            // * The timer resets the numberOfNotesOn, if noteOff signals were
            //   drop out for some reason (which has not been encountered yet).

            time_t now;
            time (&now);
            double elapsed = difftime(now, lastInsertionTime);
            time (&lastInsertionTime);

            if (numberOfNotesOn <= 0 || elapsed > 10.0 ) {
                numberOfNotesOn = 0;
                insertionTime = getInsertionTime();
            }
            numberOfNotesOn++;

            noteInserter->insertNote(segment, insertionTime, pitch,
                                     Accidentals::NoAccidental,
                                     true);
        }
    }
}

void
NotationView::slotInsertableNoteOnReceived(int pitch, int velocity)
{
    NOTATION_DEBUG << "NotationView::slotInsertableNoteOnReceived: " << pitch << endl;
    slotInsertableNoteEventReceived(pitch, velocity, true);
}

void
NotationView::slotInsertableNoteOffReceived(int pitch, int velocity)
{
    NOTATION_DEBUG << "NotationView::slotInsertableNoteOffReceived: " << pitch << endl;
    slotInsertableNoteEventReceived(pitch, velocity, false);
}

void
NotationView::slotInsertableTimerElapsed()
{}

void
NotationView::slotToggleStepByStep()
{
    /* was toggle */ QAction *action = dynamic_cast<QAction*>
        (actionCollection()->action("toggle_step_by_step"));
    if (!action) {
        NOTATION_DEBUG << "WARNING: No toggle_step_by_step action" << endl;
        return ;
    }
    if (action->isChecked()) { // after toggling, that is
        emit stepByStepTargetRequested(this);
    } else {
        emit stepByStepTargetRequested(0);
    }
}

void
NotationView::slotStepByStepTargetRequested(QObject *obj)
{
    /* was toggle */ QAction *action = dynamic_cast<QAction*>
        (actionCollection()->action("toggle_step_by_step"));
    if (!action) {
        NOTATION_DEBUG << "WARNING: No toggle_step_by_step action" << endl;
        return ;
    }
    action->setChecked(obj == this);
}

void
NotationView::slotCheckRendered(double cx0, double cx1)
{
    //    NOTATION_DEBUG << "slotCheckRendered(" << cx0 << "," << cx1 << ")" << endl;

    bool something = false;

    for (size_t i = 0; i < m_staffs.size(); ++i) {

        LinedStaff *staff = m_staffs[i];

        LinedStaff::LinedStaffCoords cc0 = staff->getLayoutCoordsForCanvasCoords
            (cx0, 0);

        LinedStaff::LinedStaffCoords cc1 = staff->getLayoutCoordsForCanvasCoords
            (cx1, staff->getTotalHeight() + staff->getY());

        timeT t0 = m_hlayout->getTimeForX(cc0.first);
        timeT t1 = m_hlayout->getTimeForX(cc1.first);

        if (dynamic_cast<NotationStaff *>(staff)->checkRendered(t0, t1)) {
            something = true; //!!!
        }
    }

    if (something) {
        emit renderComplete();
        if (m_renderTimer)
            delete m_renderTimer;
        m_renderTimer = new QTimer(this);
        connect(m_renderTimer, SIGNAL(timeout()), SLOT(slotRenderSomething()));
        m_renderTimer->start(0, true);
    }

    if (m_deferredCursorMove != NoCursorMoveNeeded)
        doDeferredCursorMove();
}

void
NotationView::slotRenderSomething()
{
    delete m_renderTimer;
    m_renderTimer = 0;
    static clock_t lastWork = 0;

    clock_t now = clock();
    long elapsed = ((now - lastWork) * 1000 / CLOCKS_PER_SEC);
    if (elapsed < 70) {
        m_renderTimer = new QTimer(this);
        connect(m_renderTimer, SIGNAL(timeout()), SLOT(slotRenderSomething()));
        m_renderTimer->start(0, true);
        return ;
    }
    lastWork = now;

    for (size_t i = 0; i < m_staffs.size(); ++i) {

        if (m_staffs[i]->doRenderWork(m_staffs[i]->getSegment().getStartTime(),
                                      m_staffs[i]->getSegment().getEndTime())) {
            m_renderTimer = new QTimer(this);
            connect(m_renderTimer, SIGNAL(timeout()), SLOT(slotRenderSomething()));
            m_renderTimer->start(0, true);
            return ;
        }
    }

    PixmapArrayGC::deleteAll();
    NOTATION_DEBUG << "NotationView::slotRenderSomething: updating thumbnails" << endl;
    updateThumbnails(true);

    // Update track headers when rendering is done
    // (better late than never)
    m_headersGroup->slotUpdateAllHeaders(getCanvasLeftX(), 0, true);
    m_headersGroupView->setContentsPos(getCanvasView()->contentsX(),
                                           getCanvasView()->contentsY());
}

NotationCanvasView* NotationView::getCanvasView()
{
    return dynamic_cast<NotationCanvasView *>(m_canvasView);
}

void
NotationView::slotVerticalScrollHeadersGroup(int y)
{
    m_headersGroupView->setContentsPos(0, y);
}

void
NotationView::slotShowHeadersGroup()
{
    m_showHeadersGroup = HeadersGroup::ShowAlways;
    showHeadersGroup();

    // Disable menu entry when headers are shown
    m_showHeadersMenuEntry->setEnabled(false);
}

void
NotationView::slotHideHeadersGroup()
{
    m_showHeadersGroup = HeadersGroup::ShowNever;
    hideHeadersGroup();

    // Enable menu entry when headers are hidden
    m_showHeadersMenuEntry->setEnabled(true);
}

void
NotationView::showHeadersGroup()
{
    if (m_headersGroupView && (m_pageMode == LinedStaff::LinearMode)) {
        m_headersGroupView->show();
        m_headersTopFrame->show();
        m_rulerBoxFiller->show();
    }
}

void
NotationView::hideHeadersGroup()
{
    if (m_headersGroupView) {
        m_headersGroupView->hide();
        m_headersTopFrame->hide();
        m_rulerBoxFiller->hide();
    }
}

void
NotationView::slotUpdateHeaders(int x, int y)
{
    m_headersGroup->slotUpdateAllHeaders(x, y);
    m_headersGroupView->setContentsPos(x, y);
}

void
NotationView::slotHeadersWidthChanged(int w)
{
    m_headersTopFrame->setFixedWidth(w);
    m_rulerBoxFiller->setFixedWidth(w);
    m_canvasView->updateLeftWidgetGeometry();
}


int
NotationView::getCanvasVisibleWidth()
{
    if (getCanvasView()) {
        return getCanvasView()->visibleWidth();
    } else {
        return -1;
    }
}

int
NotationView::getHeadersTopFrameMinWidth()
{
    /// TODO : use a real button width got from a real button

    // 2 buttons (2 x 24) + 2 margins (2 x 4) + buttons spacing (4)
    return 4 + 24 + 4 + 24 + 4;
}

}
#include "NotationView.moc"
