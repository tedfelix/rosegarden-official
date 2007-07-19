/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
 
    This program is Copyright 2000-2007
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


#include "NotationView.h"
#include <list>
#include <qlayout.h>
#include "misc/Debug.h"
#include <kapplication.h>

#include "gui/editors/segment/TrackEditor.h"
#include "gui/editors/segment/TrackButtons.h"
#include "base/BaseProperties.h"
#include <klocale.h>
#include <kstddirs.h>
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
#include "commands/notation/AddFingeringMarkCommand.h"
#include "commands/notation/AddIndicationCommand.h"
#include "commands/notation/AddMarkCommand.h"
#include "commands/notation/AddSlashesCommand.h"
#include "commands/notation/AddTextMarkCommand.h"
#include "commands/notation/AutoBeamCommand.h"
#include "commands/notation/BeamCommand.h"
#include "commands/notation/BreakCommand.h"
#include "commands/notation/ChangeSlurPositionCommand.h"
#include "commands/notation/ChangeStemsCommand.h"
#include "commands/notation/ChangeStyleCommand.h"
#include "commands/notation/ClefInsertionCommand.h"
#include "commands/notation/CollapseRestsCommand.h"
#include "commands/notation/DeCounterpointCommand.h"
#include "commands/notation/EraseEventCommand.h"
#include "commands/notation/FixNotationQuantizeCommand.h"
#include "commands/notation/GraceCommand.h"
#include "commands/notation/IncrementDisplacementsCommand.h"
#include "commands/notation/InterpretCommand.h"
#include "commands/notation/KeyInsertionCommand.h"
#include "commands/notation/MakeAccidentalsCautionaryCommand.h"
#include "commands/notation/MakeChordCommand.h"
#include "commands/notation/MakeNotesViableCommand.h"
#include "commands/notation/MultiKeyInsertionCommand.h"
#include "commands/notation/NormalizeRestsCommand.h"
#include "commands/notation/RemoveFingeringMarksCommand.h"
#include "commands/notation/RemoveMarksCommand.h"
#include "commands/notation/RemoveNotationQuantizeCommand.h"
#include "commands/notation/ResetDisplacementsCommand.h"
#include "commands/notation/RespellCommand.h"
#include "commands/notation/RestoreSlursCommand.h"
#include "commands/notation/RestoreStemsCommand.h"
#include "commands/notation/SetVisibilityCommand.h"
#include "commands/notation/SustainInsertionCommand.h"
#include "commands/notation/TextInsertionCommand.h"
#include "commands/notation/TieNotesCommand.h"
#include "commands/notation/TupletCommand.h"
#include "commands/notation/UnGraceCommand.h"
#include "commands/notation/UntieNotesCommand.h"
#include "commands/notation/UnTupletCommand.h"
#include "commands/segment/PasteToTriggerSegmentCommand.h"
#include "commands/segment/SegmentChangeTransposeCommand.h"
#include "commands/segment/RenameTrackCommand.h"
#include "document/RosegardenGUIDoc.h"
#include "document/ConfigGroups.h"
#include "FretboardInserter.h"
#include "gui/application/SetWaitCursor.h"
#include "gui/application/RosegardenGUIView.h"
#include "gui/dialogs/ClefDialog.h"
#include "gui/dialogs/EventEditDialog.h"
#include "gui/dialogs/InterpretDialog.h"
#include "gui/dialogs/IntervalDialog.h"
#include "gui/dialogs/KeySignatureDialog.h"
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
#include "gui/general/EditViewBase.h"
#include "gui/general/EditView.h"
#include "gui/general/GUIPalette.h"
#include "gui/general/LinedStaff.h"
#include "gui/general/LinedStaffManager.h"
#include "gui/general/ProgressReporter.h"
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
#include <kaction.h>
#include <kcombobox.h>
#include <kconfig.h>
#include <kglobal.h>
#include <klineeditdlg.h>
#include <kmessagebox.h>
#include <kprinter.h>
#include <kprogress.h>
#include <kstatusbar.h>
#include <kstdaction.h>
#include <ktoolbar.h>
#include <kxmlguiclient.h>
#include <qbrush.h>
#include <qcanvas.h>
#include <qcursor.h>
#include <qdialog.h>
#include <qevent.h>
#include <qfont.h>
#include <qfontmetrics.h>
#include <qhbox.h>
#include <qiconset.h>
#include <qlabel.h>
#include <qobject.h>
#include <qpaintdevicemetrics.h>
#include <qpainter.h>
#include <qpixmap.h>
#include <qpoint.h>
#include <qprinter.h>
#include <qrect.h>
#include <qregexp.h>
#include <qsize.h>
#include <qstring.h>
#include <qtimer.h>
#include <qwidget.h>
#include <qvalidator.h>
#include <algorithm>


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


class MarkActionData
{
public:
    MarkActionData() :
	title(0),
	actionName(0),
	keycode(0) { }

    MarkActionData(const QString &_title,
		   QString _actionName,
		   int _keycode,
		   Mark _mark) :
	title(_title),
	actionName(_actionName),
	keycode(_keycode),
	mark(_mark) { }

    QString title;
    QString actionName;
    int keycode;
    Mark mark;
};


NotationView::NotationView(RosegardenGUIDoc *doc,
                           std::vector<Segment *> segments,
                           QWidget *parent,
                           bool showProgressive) :
        EditView(doc, segments, 1, parent, "notationview"),
        m_properties(getViewLocalPropertyPrefix()),
        m_selectionCounter(0),
        m_insertModeLabel(0),
        m_annotationsLabel(0),
        m_lilypondDirectivesLabel(0),
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
        m_lilypondDirectivesVisible(false),
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
        m_printSize(8) // set in positionStaffs
{
    initActionDataMaps(); // does something only the 1st time it's called

    m_toolBox = new NotationToolBox(this);

    assert(segments.size() > 0);
    NOTATION_DEBUG << "NotationView ctor" << endl;


    // Initialise the display-related defaults that will be needed
    // by both the actions and the layout toolbar

    m_config->setGroup(NotationViewConfigGroup);

    m_fontName = qstrtostr(m_config->readEntry
                           ("notefont",
                            strtoqstr(NoteFontFactory::getDefaultFontName())));

    try
    {
        (void)NoteFontFactory::getFont
        (m_fontName,
         NoteFontFactory::getDefaultSize(m_fontName));
    } catch (Exception e)
    {
        m_fontName = NoteFontFactory::getDefaultFontName();
    }

    m_fontSize = m_config->readUnsignedNumEntry
                 ((segments.size() > 1 ? "multistaffnotesize" : "singlestaffnotesize"),
                  NoteFontFactory::getDefaultSize(m_fontName));

    int defaultSpacing = m_config->readNumEntry("spacing", 100);
    m_hlayout->setSpacing(defaultSpacing);

    int defaultProportion = m_config->readNumEntry("proportion", 60);
    m_hlayout->setProportion(defaultProportion);

    delete m_notePixmapFactory;
    m_notePixmapFactory = new NotePixmapFactory(m_fontName, m_fontSize);
    m_hlayout->setNotePixmapFactory(m_notePixmapFactory);
    m_vlayout->setNotePixmapFactory(m_notePixmapFactory);

    setupActions();
    //     setupAddControlRulerMenu(); - too early for notation, moved to end of ctor.
    initLayoutToolbar();
    initStatusBar();

    setBackgroundMode(PaletteBase);

    QCanvas *tCanvas = new QCanvas(this);
    tCanvas->resize(width() * 2, height() * 2);

    setCanvasView(new NotationCanvasView(*this, tCanvas, getCentralWidget()));

    updateViewCaption();

    setTopStandardRuler(new StandardRuler(getDocument(),
                                    m_hlayout, m_leftGutter, 25,
                                    false, getCentralWidget()));

    m_topStandardRuler->getLoopRuler()->setBackgroundColor
        (GUIPalette::getColour(GUIPalette::InsertCursorRuler));

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


    setBottomStandardRuler(new StandardRuler(getDocument(), m_hlayout, m_leftGutter, 25,
                                       true, getBottomWidget()));

    for (unsigned int i = 0; i < segments.size(); ++i)
    {
        m_staffs.push_back(new NotationStaff
                           (canvas(), segments[i], 0,  // snap
                            i, this,
                            m_fontName, m_fontSize));
    }

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

    m_config->setGroup(NotationViewConfigGroup);
    int layoutMode = m_config->readNumEntry("layoutmode", 0);

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
    (getCanvasView(), SIGNAL(activeItemPressed(QMouseEvent*, QCanvasItem*)),
     this, SLOT (slotActiveItemPressed(QMouseEvent*, QCanvasItem*)));

    QObject::connect
    (getCanvasView(), SIGNAL(nonNotationItemPressed(QMouseEvent*, QCanvasItem*)),
     this, SLOT (slotNonNotationItemPressed(QMouseEvent*, QCanvasItem*)));

    QObject::connect
    (getCanvasView(), SIGNAL(textItemPressed(QMouseEvent*, QCanvasItem*)),
     this, SLOT (slotTextItemPressed(QMouseEvent*, QCanvasItem*)));

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

    setConfigDialogPageIndex(2);
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
        m_lilypondDirectivesVisible(false),
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
        m_printSize(8) // set in positionStaffs
{
    assert(segments.size() > 0);
    NOTATION_DEBUG << "NotationView print ctor" << endl;


    // Initialise the display-related defaults that will be needed
    // by both the actions and the layout toolbar

    m_config->setGroup(NotationViewConfigGroup);

    if (referenceView)
    {
        m_fontName = referenceView->m_fontName;
    } else
    {
        m_fontName = qstrtostr(m_config->readEntry
                               ("notefont",
                                strtoqstr(NoteFontFactory::getDefaultFontName())));
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
        int defaultSpacing = m_config->readNumEntry("spacing", 100);
        m_hlayout->setSpacing(defaultSpacing);
        int defaultProportion = m_config->readNumEntry("proportion", 60);
        m_hlayout->setProportion(defaultProportion);
    }

    delete m_notePixmapFactory;
    m_notePixmapFactory = new NotePixmapFactory(m_fontName, m_fontSize);
    m_hlayout->setNotePixmapFactory(m_notePixmapFactory);
    m_vlayout->setNotePixmapFactory(m_notePixmapFactory);

    setBackgroundMode(PaletteBase);
    m_config->setGroup(NotationViewConfigGroup);

    QCanvas *tCanvas = new QCanvas(this);
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

    m_config->setGroup(NotationViewConfigGroup);
    m_printSize = m_config->readUnsignedNumEntry("printingnotesize", 5);

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
        m_config->setGroup(NotationViewConfigGroup);
        QFont font = m_config->readFontEntry("textfont", &defaultFont);
        font.setPixelSize(m_fontSize * 5);
        QFontMetrics metrics(font);

        if (metadata.has(CompositionMetadataKeys::Title)) {
            QString title(strtoqstr(metadata.get<String>
                                    (CompositionMetadataKeys::Title)));
            m_title = new QCanvasText(title, font, canvas());
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
            m_subtitle = new QCanvasText(subtitle, font, canvas());
            m_subtitle->setX(m_leftGutter + pageWidth / 2 - metrics.width(subtitle) / 2);
            m_subtitle->setY(20 + titleHeight + metrics.ascent());
            m_subtitle->show();
            titleHeight += metrics.height() * 3 / 2;
        }

        if (metadata.has(CompositionMetadataKeys::Composer)) {
            QString composer(strtoqstr(metadata.get<String>
                                       (CompositionMetadataKeys::Composer)));
            m_composer = new QCanvasText(composer, font, canvas());
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
            m_copyright = new QCanvasText(copyright, font, canvas());
            m_copyright->setX(m_leftGutter + leftMargin);
            m_copyright->setY(20 + pageHeight - topMargin - metrics.descent());
            m_copyright->show();
        }
    }

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
}

void NotationView::positionPages()
{
    if (m_printMode)
        return ;

    QPixmap background;
    QPixmap deskBackground;
    bool haveBackground = false;

    m_config->setGroup(NotationViewConfigGroup);
    if (m_config->readBoolEntry("backgroundtextures", true)) {
        QString pixmapDir =
            KGlobal::dirs()->findResource("appdata", "pixmaps/");
        if (background.load(QString("%1/misc/bg-paper-cream.xpm").
                            arg(pixmapDir))) {
            haveBackground = true;
        }
        // we're happy to ignore errors from this one:
        deskBackground.load(QString("%1/misc/bg-desktop.xpm").arg(pixmapDir));
    }

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
            QCanvasText *text = new QCanvasText(str, pageNumberFont, canvas());
            text->setX(m_leftGutter + pageWidth * page + pageWidth - pageNumberMetrics.width(str) - leftMargin);
            text->setY(y + h - pageNumberMetrics.descent() - topMargin);
            text->setZ( -999);
            text->show();
            m_pageNumbers.push_back(text);

            QCanvasRectangle *rect = new QCanvasRectangle(x, y, w, h, canvas());
            if (haveBackground)
                rect->setBrush(QBrush(Qt::white, background));
            rect->setPen(Qt::black);
            rect->setZ( -1000);
            rect->show();
            m_pages.push_back(rect);
        }

        updateThumbnails(false);
    }

    m_config->setGroup(NotationViewConfigGroup);
}

void NotationView::slotUpdateStaffName()
{
    LinedStaff *staff = getLinedStaff(m_currentStaff);
    staff->drawStaffName();
}

void NotationView::slotSaveOptions()
{
    m_config->setGroup(NotationViewConfigGroup);

    m_config->writeEntry("Show Chord Name Ruler", getToggleAction("show_chords_ruler")->isChecked());
    m_config->writeEntry("Show Raw Note Ruler", getToggleAction("show_raw_note_ruler")->isChecked());
    m_config->writeEntry("Show Tempo Ruler", getToggleAction("show_tempo_ruler")->isChecked());
    m_config->writeEntry("Show Annotations", m_annotationsVisible);
    m_config->writeEntry("Show LilyPond Directives", m_lilypondDirectivesVisible);

    m_config->sync();
}

void NotationView::setOneToolbar(const char *actionName,
                                 const char *toolbarName)
{
    KToggleAction *action = getToggleAction(actionName);
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

    m_config->setGroup(NotationViewConfigGroup);

    bool opt;

    opt = m_config->readBoolEntry("Show Chord Name Ruler", false);
    getToggleAction("show_chords_ruler")->setChecked(opt);
    slotToggleChordsRuler();

    opt = m_config->readBoolEntry("Show Raw Note Ruler", true);
    getToggleAction("show_raw_note_ruler")->setChecked(opt);
    slotToggleRawNoteRuler();

    opt = m_config->readBoolEntry("Show Tempo Ruler", true);
    getToggleAction("show_tempo_ruler")->setChecked(opt);
    slotToggleTempoRuler();

    opt = m_config->readBoolEntry("Show Annotations", true);
    m_annotationsVisible = opt;
    getToggleAction("show_annotations")->setChecked(opt);
    slotUpdateAnnotationsStatus();
    //    slotToggleAnnotations();

    opt = m_config->readBoolEntry("Show LilyPond Directives", true);
    m_lilypondDirectivesVisible = opt;
    getToggleAction("show_lilypond_directives")->setChecked(opt);
    slotUpdateLilyPondDirectivesStatus();
}

void NotationView::setupActions()
{
    KStdAction::print(this, SLOT(slotFilePrint()), actionCollection());
    KStdAction::printPreview(this, SLOT(slotFilePrintPreview()),
                             actionCollection());

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

        KToggleAction *fontAction =
            new KToggleAction
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


    KActionMenu *spacingActionMenu =
        new KActionMenu(i18n("S&pacing"), this, "stretch_actionmenu");

    int defaultSpacing = m_hlayout->getSpacing();
    std::vector<int> spacings = NotationHLayout::getAvailableSpacings();

    for (std::vector<int>::iterator i = spacings.begin();
            i != spacings.end(); ++i) {

        KToggleAction *spacingAction =
            new KToggleAction
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

        KToggleAction *proportionAction =
            new KToggleAction
            (name, 0, this,
             SLOT(slotChangeProportionFromAction()),
             actionCollection(), QString("proportion_%1").arg(*i));

        proportionAction->setExclusiveGroup("proportion");
        proportionAction->setChecked(*i == defaultProportion);
        proportionActionMenu->insert(proportionAction);
    }

    actionCollection()->insert(proportionActionMenu);

    KActionMenu *styleActionMenu =
        new KActionMenu(i18n("Note &Style"), this, "note_style_actionmenu");

    std::vector<NoteStyleName> styles
    (NoteStyleFactory::getAvailableStyleNames());

    for (std::vector<NoteStyleName>::iterator i = styles.begin();
            i != styles.end(); ++i) {

        QString styleQName(strtoqstr(*i));

        KAction *styleAction =
            new KAction
            (styleQName, 0, this, SLOT(slotSetStyleFromAction()),
             actionCollection(), "style_" + styleQName);

        styleActionMenu->insert(styleAction);
    }

    actionCollection()->insert(styleActionMenu);

    KActionMenu *ornamentActionMenu =
        new KActionMenu(i18n("Use Ornament"), this, "ornament_actionmenu");



    new KAction
    (i18n("Insert Rest"), Key_P, this, SLOT(slotInsertRest()),
     actionCollection(), QString("insert_rest"));

    new KAction
    (i18n("Switch from Note to Rest"), Key_T, this,
     SLOT(slotSwitchFromNoteToRest()),
     actionCollection(), QString("switch_from_note_to_rest"));

    new KAction
    (i18n("Switch from Rest to Note"), Key_Y, this,
     SLOT(slotSwitchFromRestToNote()),
     actionCollection(), QString("switch_from_rest_to_note"));


    // setup Notes menu & toolbar
    QIconSet icon;

    for (NoteActionDataMap::Iterator actionDataIter = m_noteActionDataMap->begin();
            actionDataIter != m_noteActionDataMap->end();
            ++actionDataIter) {

        NoteActionData noteActionData = **actionDataIter;

        icon = QIconSet
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

        icon = QIconSet
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

    for (unsigned int i = 0;
            i < sizeof(actionsAccidental) / sizeof(actionsAccidental[0]); ++i) {

        icon = QIconSet(NotePixmapFactory::toQPixmap(NotePixmapFactory::makeToolbarPixmap
                        (actionsAccidental[i][3])));
        noteAction = new KRadioAction(actionsAccidental[i][0], icon, 0, this,
                                      actionsAccidental[i][1],
                                      actionCollection(), actionsAccidental[i][2]);
        noteAction->setExclusiveGroup("accidentals");
    }


    //
    // Clefs
    //

    // Treble
    icon = QIconSet(NotePixmapFactory::toQPixmap(NotePixmapFactory::makeToolbarPixmap("clef-treble")));
    noteAction = new KRadioAction(i18n("&Treble Clef"), icon, 0, this,
                                  SLOT(slotTrebleClef()),
                                  actionCollection(), "treble_clef");
    noteAction->setExclusiveGroup("notes");

    // Alto
    icon = QIconSet(NotePixmapFactory::toQPixmap(NotePixmapFactory::makeToolbarPixmap("clef-alto")));
    noteAction = new KRadioAction(i18n("&Alto Clef"), icon, 0, this,
                                  SLOT(slotAltoClef()),
                                  actionCollection(), "alto_clef");
    noteAction->setExclusiveGroup("notes");

    // Tenor
    icon = QIconSet(NotePixmapFactory::toQPixmap(NotePixmapFactory::makeToolbarPixmap("clef-tenor")));
    noteAction = new KRadioAction(i18n("Te&nor Clef"), icon, 0, this,
                                  SLOT(slotTenorClef()),
                                  actionCollection(), "tenor_clef");
    noteAction->setExclusiveGroup("notes");

    // Bass
    icon = QIconSet(NotePixmapFactory::toQPixmap(NotePixmapFactory::makeToolbarPixmap("clef-bass")));
    noteAction = new KRadioAction(i18n("&Bass Clef"), icon, 0, this,
                                  SLOT(slotBassClef()),
                                  actionCollection(), "bass_clef");
    noteAction->setExclusiveGroup("notes");


    icon = QIconSet(NotePixmapFactory::toQPixmap(NotePixmapFactory::makeToolbarPixmap("text")));
    noteAction = new KRadioAction(i18n("&Text"), icon, Key_F8, this,
                                  SLOT(slotText()),
                                  actionCollection(), "text");
    noteAction->setExclusiveGroup("notes");

    icon = QIconSet(NotePixmapFactory::toQPixmap(NotePixmapFactory::makeToolbarPixmap("fretboard")));
    noteAction = new KRadioAction(i18n("&Fretboard"), icon, Key_F9, this,
                                  SLOT(slotFretboard()),
                                  actionCollection(), "fretboard");
    noteAction->setExclusiveGroup("notes");

    /*    icon = QIconSet(NotePixmapFactory::toQPixmap(NotePixmapFactory::makeToolbarPixmap("lilypond")));
        noteAction = new KRadioAction(i18n("Lil&ypond Directive"), icon, Key_F9, this,
                                      SLOT(slotLilypondDirective()),
                                      actionCollection(), "lilypond_directive");
        noteAction->setExclusiveGroup("notes"); */


    //
    // Edition tools (eraser, selector...)
    //
    noteAction = new KRadioAction(i18n("&Erase"), "eraser", Key_F4,
                                  this, SLOT(slotEraseSelected()),
                                  actionCollection(), "erase");
    noteAction->setExclusiveGroup("notes");

    icon = QIconSet(NotePixmapFactory::toQPixmap(NotePixmapFactory::makeToolbarPixmap("select")));
    noteAction = new KRadioAction(i18n("&Select and Edit"), icon, Key_F2,
                                  this, SLOT(slotSelectSelected()),
                                  actionCollection(), "select");
    noteAction->setExclusiveGroup("notes");

    icon = QIconSet(NotePixmapFactory::toQPixmap(NotePixmapFactory::makeToolbarPixmap("step_by_step")));
    new KToggleAction(i18n("Ste&p Recording"), icon, 0, this,
                      SLOT(slotToggleStepByStep()), actionCollection(),
                      "toggle_step_by_step");


    // Edit menu
    new KAction(i18n("Select from Sta&rt"), 0, this,
                SLOT(slotEditSelectFromStart()), actionCollection(),
                "select_from_start");

    new KAction(i18n("Select to &End"), 0, this,
                SLOT(slotEditSelectToEnd()), actionCollection(),
                "select_to_end");

    new KAction(i18n("Select Whole St&aff"), Key_A + CTRL, this,
                SLOT(slotEditSelectWholeStaff()), actionCollection(),
                "select_whole_staff");

    new KAction(i18n("C&ut and Close"), CTRL + SHIFT + Key_X, this,
                SLOT(slotEditCutAndClose()), actionCollection(),
                "cut_and_close");

    new KAction(i18n("Pa&ste..."), CTRL + SHIFT + Key_V, this,
                SLOT(slotEditGeneralPaste()), actionCollection(),
                "general_paste");

    new KAction(i18n("De&lete"), Key_Delete, this,
                SLOT(slotEditDelete()), actionCollection(),
                "delete");

    //
    // Settings menu
    //
    int layoutMode = m_config->readNumEntry("layoutmode", 0);

    QString pixmapDir = KGlobal::dirs()->findResource("appdata", "pixmaps/");

    QCanvasPixmap pixmap(pixmapDir + "/toolbar/linear-layout.xpm");
    icon = QIconSet(pixmap);
    KRadioAction *linearModeAction = new KRadioAction
                                     (i18n("&Linear Layout"), icon, 0, this, SLOT(slotLinearMode()),
                                      actionCollection(), "linear_mode");
    linearModeAction->setExclusiveGroup("layoutMode");
    if (layoutMode == 0)
        linearModeAction->setChecked(true);

    pixmap.load(pixmapDir + "/toolbar/continuous-page-mode.xpm");
    icon = QIconSet(pixmap);
    KRadioAction *continuousPageModeAction = new KRadioAction
            (i18n("&Continuous Page Layout"), icon, 0, this, SLOT(slotContinuousPageMode()),
             actionCollection(), "continuous_page_mode");
    continuousPageModeAction->setExclusiveGroup("layoutMode");
    if (layoutMode == 1)
        continuousPageModeAction->setChecked(true);

    pixmap.load(pixmapDir + "/toolbar/multi-page-mode.xpm");
    icon = QIconSet(pixmap);
    KRadioAction *multiPageModeAction = new KRadioAction
                                        (i18n("&Multiple Page Layout"), icon, 0, this, SLOT(slotMultiPageMode()),
                                         actionCollection(), "multi_page_mode");
    multiPageModeAction->setExclusiveGroup("layoutMode");
    if (layoutMode == 2)
        multiPageModeAction->setChecked(true);

    new KToggleAction(i18n("Show Ch&ord Name Ruler"), 0, this,
                      SLOT(slotToggleChordsRuler()),
                      actionCollection(), "show_chords_ruler");

    new KToggleAction(i18n("Show Ra&w Note Ruler"), 0, this,
                      SLOT(slotToggleRawNoteRuler()),
                      actionCollection(), "show_raw_note_ruler");

    new KToggleAction(i18n("Show &Tempo Ruler"), 0, this,
                      SLOT(slotToggleTempoRuler()),
                      actionCollection(), "show_tempo_ruler");

    new KToggleAction(i18n("Show &Annotations"), 0, this,
                      SLOT(slotToggleAnnotations()),
                      actionCollection(), "show_annotations");

    new KToggleAction(i18n("Show Lily&Pond Directives"), 0, this,
                      SLOT(slotToggleLilyPondDirectives()),
                      actionCollection(), "show_lilypond_directives");

    new KAction(i18n("Open L&yric Editor"), 0, this, SLOT(slotEditLyrics()),
                actionCollection(), "lyric_editor");

    //
    // Group menu
    //
    icon = QIconSet
           (NotePixmapFactory::toQPixmap(NotePixmapFactory::makeToolbarPixmap
                                         ("group-beam")));

    new KAction(BeamCommand::getGlobalName(), icon, Key_B + CTRL, this,
                SLOT(slotGroupBeam()), actionCollection(), "beam");

    new KAction(AutoBeamCommand::getGlobalName(), 0, this,
                SLOT(slotGroupAutoBeam()), actionCollection(), "auto_beam");

    icon = QIconSet
           (NotePixmapFactory::toQPixmap(NotePixmapFactory::makeToolbarPixmap
                                         ("group-unbeam")));

    new KAction(BreakCommand::getGlobalName(), icon, Key_U + CTRL, this,
                SLOT(slotGroupBreak()), actionCollection(), "break_group");

    icon = QIconSet
           (NotePixmapFactory::toQPixmap(NotePixmapFactory::makeToolbarPixmap
                                         ("group-simple-tuplet")));

    new KAction(TupletCommand::getGlobalName(true), icon, Key_R + CTRL, this,
                SLOT(slotGroupSimpleTuplet()), actionCollection(), "simple_tuplet");

    icon = QIconSet
           (NotePixmapFactory::toQPixmap(NotePixmapFactory::makeToolbarPixmap
                                         ("group-tuplet")));

    new KAction(TupletCommand::getGlobalName(false), icon, Key_T + CTRL, this,
                SLOT(slotGroupGeneralTuplet()), actionCollection(), "tuplet");

    new KAction(UnTupletCommand::getGlobalName(), 0, this,
                SLOT(slotGroupUnTuplet()), actionCollection(), "break_tuplets");

    icon = QIconSet(NotePixmapFactory::toQPixmap(NotePixmapFactory::makeToolbarPixmap("triplet")));
    (new KToggleAction(i18n("Trip&let Insert Mode"), icon, Key_G,
                       this, SLOT(slotUpdateInsertModeStatus()),
                       actionCollection(), "triplet_mode"))->
    setChecked(false);

    icon = QIconSet(NotePixmapFactory::toQPixmap(NotePixmapFactory::makeToolbarPixmap("chord")));
    (new KToggleAction(i18n("C&hord Insert Mode"), icon, Key_H,
                       this, SLOT(slotUpdateInsertModeStatus()),
                       actionCollection(), "chord_mode"))->
    setChecked(false);

    icon = QIconSet
           (NotePixmapFactory::toQPixmap(NotePixmapFactory::makeToolbarPixmap
                                         ("group-grace")));

    new KAction(GraceCommand::getGlobalName(), icon, 0, this,
                SLOT(slotGroupGrace()), actionCollection(), "grace");

    new KAction(UnGraceCommand::getGlobalName(), 0, this,
                SLOT(slotGroupUnGrace()), actionCollection(), "ungrace");

    icon = QIconSet
           (NotePixmapFactory::toQPixmap(NotePixmapFactory::makeToolbarPixmap
                                         ("group-slur")));

    new KAction(AddIndicationCommand::getGlobalName
                (Indication::Slur), icon, Key_ParenRight, this,
                SLOT(slotGroupSlur()), actionCollection(), "slur");

    new KAction(AddIndicationCommand::getGlobalName
                (Indication::PhrasingSlur), 0, Key_ParenRight + CTRL, this,
                SLOT(slotGroupPhrasingSlur()), actionCollection(), "phrasing_slur");

    icon = QIconSet
           (NotePixmapFactory::toQPixmap(NotePixmapFactory::makeToolbarPixmap
                                         ("group-glissando")));

    new KAction(AddIndicationCommand::getGlobalName
                (Indication::Glissando), icon, 0, this,
                SLOT(slotGroupGlissando()), actionCollection(), "glissando");

    icon = QIconSet
           (NotePixmapFactory::toQPixmap(NotePixmapFactory::makeToolbarPixmap
                                         ("group-crescendo")));

    new KAction(AddIndicationCommand::getGlobalName
                (Indication::Crescendo), icon, Key_Less, this,
                SLOT(slotGroupCrescendo()), actionCollection(), "crescendo");

    icon = QIconSet
           (NotePixmapFactory::toQPixmap(NotePixmapFactory::makeToolbarPixmap
                                         ("group-decrescendo")));

    new KAction(AddIndicationCommand::getGlobalName
                (Indication::Decrescendo), icon, Key_Greater, this,
                SLOT(slotGroupDecrescendo()), actionCollection(), "decrescendo");

    new KAction(AddIndicationCommand::getGlobalName
                (Indication::QuindicesimaUp), 0, 0, this,
                SLOT(slotGroupOctave2Up()), actionCollection(), "octave_2up");

    icon = QIconSet
           (NotePixmapFactory::toQPixmap(NotePixmapFactory::makeToolbarPixmap
                                         ("group-ottava")));

    new KAction(AddIndicationCommand::getGlobalName
                (Indication::OttavaUp), icon, 0, this,
                SLOT(slotGroupOctaveUp()), actionCollection(), "octave_up");

    new KAction(AddIndicationCommand::getGlobalName
                (Indication::OttavaDown), 0, 0, this,
                SLOT(slotGroupOctaveDown()), actionCollection(), "octave_down");

    new KAction(AddIndicationCommand::getGlobalName
                (Indication::QuindicesimaDown), 0, 0, this,
                SLOT(slotGroupOctave2Down()), actionCollection(), "octave_2down");

    icon = QIconSet
           (NotePixmapFactory::toQPixmap(NotePixmapFactory::makeToolbarPixmap
                                         ("group-chord")));
    new KAction(MakeChordCommand::getGlobalName(), icon, 0, this,
                SLOT(slotGroupMakeChord()), actionCollection(), "make_chord");

    // setup Transforms menu
    new KAction(NormalizeRestsCommand::getGlobalName(), Key_N + CTRL, this,
                SLOT(slotTransformsNormalizeRests()), actionCollection(),
                "normalize_rests");

    new KAction(CollapseRestsCommand::getGlobalName(), 0, this,
                SLOT(slotTransformsCollapseRests()), actionCollection(),
                "collapse_rests_aggressively");

    new KAction(CollapseNotesCommand::getGlobalName(), Key_Equal + CTRL, this,
                SLOT(slotTransformsCollapseNotes()), actionCollection(),
                "collapse_notes");

    icon = QIconSet
           (NotePixmapFactory::toQPixmap(NotePixmapFactory::makeToolbarPixmap
                                         ("transforms-tie")));

    new KAction(TieNotesCommand::getGlobalName(), icon, Key_AsciiTilde, this,
                SLOT(slotTransformsTieNotes()), actionCollection(),
                "tie_notes");

    new KAction(UntieNotesCommand::getGlobalName(), 0, this,
                SLOT(slotTransformsUntieNotes()), actionCollection(),
                "untie_notes");

    new KAction(MakeNotesViableCommand::getGlobalName(), 0, this,
                SLOT(slotTransformsMakeNotesViable()), actionCollection(),
                "make_notes_viable");

    icon = QIconSet
           (NotePixmapFactory::toQPixmap(NotePixmapFactory::makeToolbarPixmap
                                         ("transforms-decounterpoint")));

    new KAction(DeCounterpointCommand::getGlobalName(), icon, 0, this,
                SLOT(slotTransformsDeCounterpoint()), actionCollection(),
                "de_counterpoint");

    new KAction(ChangeStemsCommand::getGlobalName(true),
                0, Key_PageUp + CTRL, this,
                SLOT(slotTransformsStemsUp()), actionCollection(),
                "stems_up");

    new KAction(ChangeStemsCommand::getGlobalName(false),
                0, Key_PageDown + CTRL, this,
                SLOT(slotTransformsStemsDown()), actionCollection(),
                "stems_down");

    new KAction(RestoreStemsCommand::getGlobalName(), 0, this,
                SLOT(slotTransformsRestoreStems()), actionCollection(),
                "restore_stems");

    new KAction(ChangeSlurPositionCommand::getGlobalName(true),
                0, this,
                SLOT(slotTransformsSlursAbove()), actionCollection(),
                "slurs_above");

    new KAction(ChangeSlurPositionCommand::getGlobalName(false),
                0, this,
                SLOT(slotTransformsSlursBelow()), actionCollection(),
                "slurs_below");

    new KAction(RestoreSlursCommand::getGlobalName(), 0, this,
                SLOT(slotTransformsRestoreSlurs()), actionCollection(),
                "restore_slurs");

    icon = QIconSet
           (NotePixmapFactory::toQPixmap(NotePixmapFactory::makeToolbarPixmap
                                         ("accmenu-doubleflat")));

    new KAction(RespellCommand::getGlobalName
                (RespellCommand::Set, Accidentals::DoubleFlat),
                icon, 0, this,
                SLOT(slotRespellDoubleFlat()), actionCollection(),
                "respell_doubleflat");

    icon = QIconSet
           (NotePixmapFactory::toQPixmap(NotePixmapFactory::makeToolbarPixmap
                                         ("accmenu-flat")));

    new KAction(RespellCommand::getGlobalName
                (RespellCommand::Set, Accidentals::Flat),
                icon, 0, this,
                SLOT(slotRespellFlat()), actionCollection(),
                "respell_flat");

    icon = QIconSet
           (NotePixmapFactory::toQPixmap(NotePixmapFactory::makeToolbarPixmap
                                         ("accmenu-natural")));

    new KAction(RespellCommand::getGlobalName
                (RespellCommand::Set, Accidentals::Natural),
                icon, 0, this,
                SLOT(slotRespellNatural()), actionCollection(),
                "respell_natural");

    icon = QIconSet
           (NotePixmapFactory::toQPixmap(NotePixmapFactory::makeToolbarPixmap
                                         ("accmenu-sharp")));

    new KAction(RespellCommand::getGlobalName
                (RespellCommand::Set, Accidentals::Sharp),
                icon, 0, this,
                SLOT(slotRespellSharp()), actionCollection(),
                "respell_sharp");

    icon = QIconSet
           (NotePixmapFactory::toQPixmap(NotePixmapFactory::makeToolbarPixmap
                                         ("accmenu-doublesharp")));

    new KAction(RespellCommand::getGlobalName
                (RespellCommand::Set, Accidentals::DoubleSharp),
                icon, 0, this,
                SLOT(slotRespellDoubleSharp()), actionCollection(),
                "respell_doublesharp");

    new KAction(RespellCommand::getGlobalName
                (RespellCommand::Up, Accidentals::NoAccidental),
                Key_Up + CTRL + SHIFT, this,
                SLOT(slotRespellUp()), actionCollection(),
                "respell_up");

    new KAction(RespellCommand::getGlobalName
                (RespellCommand::Down, Accidentals::NoAccidental),
                Key_Down + CTRL + SHIFT, this,
                SLOT(slotRespellDown()), actionCollection(),
                "respell_down");

    new KAction(RespellCommand::getGlobalName
                (RespellCommand::Restore, Accidentals::NoAccidental),
                0, this,
                SLOT(slotRespellRestore()), actionCollection(),
                "respell_restore");

    new KAction(MakeAccidentalsCautionaryCommand::getGlobalName(true),
                0, this,
                SLOT(slotShowCautionary()), actionCollection(),
                "show_cautionary");

    new KAction(MakeAccidentalsCautionaryCommand::getGlobalName(false),
                0, this,
                SLOT(slotCancelCautionary()), actionCollection(),
                "cancel_cautionary");

    icon = QIconSet
           (NotePixmapFactory::toQPixmap(NotePixmapFactory::makeToolbarPixmap
                                         ("quantize")));

    new KAction(EventQuantizeCommand::getGlobalName(), icon, Key_Equal, this,
                SLOT(slotTransformsQuantize()), actionCollection(),
                "quantize");

    new KAction(FixNotationQuantizeCommand::getGlobalName(), 0,
                this, SLOT(slotTransformsFixQuantization()), actionCollection(),
                "fix_quantization");

    new KAction(RemoveNotationQuantizeCommand::getGlobalName(), 0,
                this, SLOT(slotTransformsRemoveQuantization()), actionCollection(),
                "remove_quantization");

    new KAction(InterpretCommand::getGlobalName(), 0,
                this, SLOT(slotTransformsInterpret()), actionCollection(),
                "interpret");

    new KAction(i18n("&Dump selected events to stderr"), 0, this,
                SLOT(slotDebugDump()), actionCollection(), "debug_dump");

    for (MarkActionDataMap::Iterator i = m_markActionDataMap->begin();
            i != m_markActionDataMap->end(); ++i) {

        const MarkActionData &markActionData = **i;

        icon = QIconSet(NotePixmapFactory::toQPixmap
                        (NotePixmapFactory::makeMarkMenuPixmap(markActionData.mark)));

        new KAction(markActionData.title,
                    icon,
                    markActionData.keycode,
                    this,
                    SLOT(slotAddMark()),
                    actionCollection(),
                    markActionData.actionName);
    }

    icon = QIconSet
           (NotePixmapFactory::toQPixmap(NotePixmapFactory::makeToolbarPixmap
                                         ("text-mark")));

    new KAction(AddTextMarkCommand::getGlobalName(), icon, 0, this,
                SLOT(slotMarksAddTextMark()), actionCollection(),
                "add_text_mark");

    new KAction(AddFingeringMarkCommand::getGlobalName("0"), 0, Key_0 + ALT, this,
                SLOT(slotMarksAddFingeringMarkFromAction()), actionCollection(),
                "add_fingering_0");

    new KAction(AddFingeringMarkCommand::getGlobalName("1"), 0, Key_1 + ALT, this,
                SLOT(slotMarksAddFingeringMarkFromAction()), actionCollection(),
                "add_fingering_1");

    new KAction(AddFingeringMarkCommand::getGlobalName("2"), 0, Key_2 + ALT, this,
                SLOT(slotMarksAddFingeringMarkFromAction()), actionCollection(),
                "add_fingering_2");

    new KAction(AddFingeringMarkCommand::getGlobalName("3"), 0, Key_3 + ALT, this,
                SLOT(slotMarksAddFingeringMarkFromAction()), actionCollection(),
                "add_fingering_3");

    new KAction(AddFingeringMarkCommand::getGlobalName("4"), 0, Key_4 + ALT, this,
                SLOT(slotMarksAddFingeringMarkFromAction()), actionCollection(),
                "add_fingering_4");

    new KAction(AddFingeringMarkCommand::getGlobalName("5"), 0, Key_5 + ALT, this,
                SLOT(slotMarksAddFingeringMarkFromAction()), actionCollection(),
                "add_fingering_5");

    new KAction(AddFingeringMarkCommand::getGlobalName("+"), 0, Key_9 + ALT, this,
                SLOT(slotMarksAddFingeringMarkFromAction()), actionCollection(),
                "add_fingering_plus");

    new KAction(AddFingeringMarkCommand::getGlobalName(), 0, 0, this,
                SLOT(slotMarksAddFingeringMark()), actionCollection(),
                "add_fingering_mark");

    new KAction(RemoveMarksCommand::getGlobalName(), 0, this,
                SLOT(slotMarksRemoveMarks()), actionCollection(),
                "remove_marks");

    new KAction(RemoveFingeringMarksCommand::getGlobalName(), 0, this,
                SLOT(slotMarksRemoveFingeringMarks()), actionCollection(),
                "remove_fingering_marks");

    new KAction(i18n("Ma&ke Ornament..."), 0, this,
                SLOT(slotMakeOrnament()), actionCollection(),
                "make_ornament");

    new KAction(i18n("Trigger &Ornament..."), 0, this,
                SLOT(slotUseOrnament()), actionCollection(),
                "use_ornament");

    new KAction(i18n("Remove Ornament..."), 0, this,
                SLOT(slotRemoveOrnament()), actionCollection(),
                "remove_ornament");

    static QString slashTitles[] = {
                                       i18n("&None"), "&1", "&2", "&3", "&4", "&5"
                                   };
    for (int i = 0; i <= 5; ++i) {
        new KAction(slashTitles[i], 0, this,
                    SLOT(slotAddSlashes()), actionCollection(),
                    QString("slashes_%1").arg(i));
    }
/*
    new KAction(i18n("Add Fretboard"),
                0,
                this,
                SLOT(slotAddFretboard()),
                actionCollection(),
                "add_fretboard");
*/
    new KAction(ClefInsertionCommand::getGlobalName(), 0, this,
                SLOT(slotEditAddClef()), actionCollection(),
                "add_clef");

    new KAction(KeyInsertionCommand::getGlobalName(), 0, this,
                SLOT(slotEditAddKeySignature()), actionCollection(),
                "add_key_signature");

    new KAction(SustainInsertionCommand::getGlobalName(true), 0, this,
                SLOT(slotEditAddSustainDown()), actionCollection(),
                "add_sustain_down");

    new KAction(SustainInsertionCommand::getGlobalName(false), 0, this,
                SLOT(slotEditAddSustainUp()), actionCollection(),
                "add_sustain_up");

	new KAction(TransposeCommand::getDiatonicGlobalName(false), 0, this,
                SLOT(slotEditTranspose()), actionCollection(),
                "transpose_segment");


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

        icon = QIconSet(NotePixmapFactory::toQPixmap(NotePixmapFactory::makeToolbarPixmap(actionsToolbars[i][3])));

        new KToggleAction(actionsToolbars[i][0], icon, 0,
                          this, actionsToolbars[i][1],
                          actionCollection(), actionsToolbars[i][2]);
    }

    new KAction(i18n("Cursor &Back"), 0, Key_Left, this,
                SLOT(slotStepBackward()), actionCollection(),
                "cursor_back");

    new KAction(i18n("Cursor &Forward"), 0, Key_Right, this,
                SLOT(slotStepForward()), actionCollection(),
                "cursor_forward");

    new KAction(i18n("Cursor Ba&ck Bar"), 0, Key_Left + CTRL, this,
                SLOT(slotJumpBackward()), actionCollection(),
                "cursor_back_bar");

    new KAction(i18n("Cursor For&ward Bar"), 0, Key_Right + CTRL, this,
                SLOT(slotJumpForward()), actionCollection(),
                "cursor_forward_bar");

    new KAction(i18n("Cursor Back and Se&lect"), SHIFT + Key_Left, this,
                SLOT(slotExtendSelectionBackward()), actionCollection(),
                "extend_selection_backward");

    new KAction(i18n("Cursor Forward and &Select"), SHIFT + Key_Right, this,
                SLOT(slotExtendSelectionForward()), actionCollection(),
                "extend_selection_forward");

    new KAction(i18n("Cursor Back Bar and Select"), SHIFT + CTRL + Key_Left, this,
                SLOT(slotExtendSelectionBackwardBar()), actionCollection(),
                "extend_selection_backward_bar");

    new KAction(i18n("Cursor Forward Bar and Select"), SHIFT + CTRL + Key_Right, this,
                SLOT(slotExtendSelectionForwardBar()), actionCollection(),
                "extend_selection_forward_bar");

    /*!!! not here yet
        new KAction(i18n("Move Selection Left"), Key_Minus, this,
    		SLOT(slotMoveSelectionLeft()), actionCollection(),
    		"move_selection_left");
    */

    new KAction(i18n("Cursor to St&art"), 0,
                /* #1025717: conflicting meanings for ctrl+a - dupe with Select All
                  Key_A + CTRL, */ this, 
                SLOT(slotJumpToStart()), actionCollection(),
                "cursor_start");

    new KAction(i18n("Cursor to &End"), 0, Key_E + CTRL, this,
                SLOT(slotJumpToEnd()), actionCollection(),
                "cursor_end");

    new KAction(i18n("Cursor &Up Staff"), 0, Key_Up + SHIFT, this,
                SLOT(slotCurrentStaffUp()), actionCollection(),
                "cursor_up_staff");

    new KAction(i18n("Cursor &Down Staff"), 0, Key_Down + SHIFT, this,
                SLOT(slotCurrentStaffDown()), actionCollection(),
                "cursor_down_staff");

    icon = QIconSet(NotePixmapFactory::toQPixmap(NotePixmapFactory::makeToolbarPixmap
                    ("transport-cursor-to-pointer")));
    new KAction(i18n("Cursor to &Playback Pointer"), icon, 0, this,
                SLOT(slotJumpCursorToPlayback()), actionCollection(),
                "cursor_to_playback_pointer");

    icon = QIconSet(NotePixmapFactory::toQPixmap(NotePixmapFactory::makeToolbarPixmap
                    ("transport-play")));
    new KAction(i18n("&Play"), icon, Key_Enter, this,
                SIGNAL(play()), actionCollection(), "play");

    icon = QIconSet(NotePixmapFactory::toQPixmap(NotePixmapFactory::makeToolbarPixmap
                    ("transport-stop")));
    new KAction(i18n("&Stop"), icon, Key_Insert, this,
                SIGNAL(stop()), actionCollection(), "stop");

    icon = QIconSet(NotePixmapFactory::toQPixmap(NotePixmapFactory::makeToolbarPixmap
                    ("transport-rewind")));
    new KAction(i18n("Re&wind"), icon, Key_End, this,
                SIGNAL(rewindPlayback()), actionCollection(),
                "playback_pointer_back_bar");

    icon = QIconSet(NotePixmapFactory::toQPixmap(NotePixmapFactory::makeToolbarPixmap
                    ("transport-ffwd")));
    new KAction(i18n("&Fast Forward"), icon, Key_PageDown, this,
                SIGNAL(fastForwardPlayback()), actionCollection(),
                "playback_pointer_forward_bar");

    icon = QIconSet(NotePixmapFactory::toQPixmap(NotePixmapFactory::makeToolbarPixmap
                    ("transport-rewind-end")));
    new KAction(i18n("Rewind to &Beginning"), icon, 0, this,
                SIGNAL(rewindPlaybackToBeginning()), actionCollection(),
                "playback_pointer_start");

    icon = QIconSet(NotePixmapFactory::toQPixmap(NotePixmapFactory::makeToolbarPixmap
                    ("transport-ffwd-end")));
    new KAction(i18n("Fast Forward to &End"), icon, 0, this,
                SIGNAL(fastForwardPlaybackToEnd()), actionCollection(),
                "playback_pointer_end");

    icon = QIconSet(NotePixmapFactory::toQPixmap(NotePixmapFactory::makeToolbarPixmap
                    ("transport-pointer-to-cursor")));
    new KAction(i18n("Playback Pointer to &Cursor"), icon, 0, this,
                SLOT(slotJumpPlaybackToCursor()), actionCollection(),
                "playback_pointer_to_cursor");

    icon = QIconSet(NotePixmapFactory::toQPixmap(NotePixmapFactory::makeToolbarPixmap
                    ("transport-solo")));
    new KToggleAction(i18n("&Solo"), icon, 0, this,
                      SLOT(slotToggleSolo()), actionCollection(),
                      "toggle_solo");

    icon = QIconSet(NotePixmapFactory::toQPixmap(NotePixmapFactory::makeToolbarPixmap
                    ("transport-tracking")));
    (new KToggleAction(i18n("Scro&ll to Follow Playback"), icon, Key_Pause, this,
                       SLOT(slotToggleTracking()), actionCollection(),
                       "toggle_tracking"))->setChecked(m_playTracking);

    icon = QIconSet(NotePixmapFactory::toQPixmap(NotePixmapFactory::makeToolbarPixmap
                    ("transport-panic")));
    new KAction(i18n("Panic"), icon, Key_P + CTRL + ALT, this,
                SIGNAL(panic()), actionCollection(), "panic");

    new KAction(i18n("Set Loop to Selection"), Key_Semicolon + CTRL, this,
                SLOT(slotPreviewSelection()), actionCollection(),
                "preview_selection");

    new KAction(i18n("Clear L&oop"), Key_Colon + CTRL, this,
                SLOT(slotClearLoop()), actionCollection(),
                "clear_loop");

    new KAction(i18n("Clear Selection"), Key_Escape, this,
                SLOT(slotClearSelection()), actionCollection(),
                "clear_selection");

    //    QString pixmapDir =
    //	KGlobal::dirs()->findResource("appdata", "pixmaps/");
    //    icon = QIconSet(QCanvasPixmap(pixmapDir + "/toolbar/eventfilter.xpm"));
    new KAction(i18n("&Filter Selection"), "filter", Key_F + CTRL, this,
                SLOT(slotFilterSelection()), actionCollection(),
                "filter_selection");

    new KAction(i18n("Push &Left"), 0, this,
                SLOT(slotFinePositionLeft()), actionCollection(),
                "fine_position_left");

    new KAction(i18n("Push &Right"), 0, this,
                SLOT(slotFinePositionRight()), actionCollection(),
                "fine_position_right");

    new KAction(i18n("Push &Up"), 0, this,
                SLOT(slotFinePositionUp()), actionCollection(),
                "fine_position_up");

    new KAction(i18n("Push &Down"), 0, this,
                SLOT(slotFinePositionDown()), actionCollection(),
                "fine_position_down");

    new KAction(i18n("&Restore Positions"), 0, this,
                SLOT(slotFinePositionRestore()), actionCollection(),
                "fine_position_restore");

    new KAction(i18n("Make &Invisible"), 0, this,
                SLOT(slotMakeInvisible()), actionCollection(),
                "make_invisible");

    new KAction(i18n("Make &Visible"), 0, this,
                SLOT(slotMakeVisible()), actionCollection(),
                "make_visible");

    new KAction(i18n("Toggle Dot"), Key_Period, this,
                SLOT(slotToggleDot()), actionCollection(),
                "toggle_dot");

    new KAction(i18n("Add Dot"), Key_Period + CTRL, this,
                SLOT(slotAddDot()), actionCollection(),
                "add_dot");

    new KAction(i18n("Add Dot"), Key_Period + CTRL + ALT, this,
                SLOT(slotAddDotNotationOnly()), actionCollection(),
                "add_notation_dot");

    createGUI(getRCFileName(), false);
}

bool
NotationView::isInChordMode()
{
    return ((KToggleAction *)actionCollection()->action("chord_mode"))->
           isChecked();
}

bool
NotationView::isInTripletMode()
{
    return ((KToggleAction *)actionCollection()->action("triplet_mode"))->
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

        KToggleAction *sizeAction = dynamic_cast<KToggleAction *>
                                    (actionCollection()->action(actionName));

        if (!sizeAction) {
            sizeAction =
                new KToggleAction(i18n("1 pixel", "%n pixels", sizes[i]),
                                  0, this,
                                  SLOT(slotChangeFontSizeFromAction()),
                                  actionCollection(), actionName);
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
    m_fontCombo = new KComboBox(layoutToolbar);
    m_fontCombo->setEditable(false);

    std::set
        <std::string> fs(NoteFontFactory::getFontNames());
    std::vector<std::string> f(fs.begin(), fs.end());
    std::sort(f.begin(), f.end());

    bool foundFont = false;

    for (std::vector<std::string>::iterator i = f.begin(); i != f.end(); ++i) {

        QString fontQName(strtoqstr(*i));

        m_fontCombo->insertItem(fontQName);
        if (fontQName.lower() == strtoqstr(m_fontName).lower()) {
            m_fontCombo->setCurrentItem(m_fontCombo->count() - 1);
            foundFont = true;
        }
    }

    if (!foundFont) {
        KMessageBox::sorry
        (this, i18n("Unknown font \"%1\", using default").arg
         (strtoqstr(m_fontName)));
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
    m_fontSizeCombo = new KComboBox(layoutToolbar, "font size combo");

    for (std::vector<int>::iterator i = sizes.begin(); i != sizes.end(); ++i) {

        value.setNum(*i);
        m_fontSizeCombo->insertItem(value);
    }
    // set combo's current value to default
    value.setNum(m_fontSize);
    m_fontSizeCombo->setCurrentText(value);

    connect(m_fontSizeCombo, SIGNAL(activated(const QString&)),
            this, SLOT(slotChangeFontSizeFromStringValue(const QString&)));

    new QLabel(i18n("  Spacing:  "), layoutToolbar, "spacing label");

    //
    // spacing combo
    //
    int defaultSpacing = m_hlayout->getSpacing();
    std::vector<int> spacings = NotationHLayout::getAvailableSpacings();

    m_spacingCombo = new KComboBox(layoutToolbar, "spacing combo");
    for (std::vector<int>::iterator i = spacings.begin(); i != spacings.end(); ++i) {

        value.setNum(*i);
        value += "%";
        m_spacingCombo->insertItem(value);
    }
    // set combo's current value to default
    value.setNum(defaultSpacing);
    value += "%";
    m_spacingCombo->setCurrentText(value);

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

    QHBox *hbox = new QHBox(sb);
    m_currentNotePixmap = new QLabel(hbox);
    m_currentNotePixmap->setMinimumWidth(20);
    m_insertModeLabel = new QLabel(hbox);
    m_annotationsLabel = new QLabel(hbox);
    m_lilypondDirectivesLabel = new QLabel(hbox);
    sb->addWidget(hbox);

    sb->insertItem(KTmpStatusMsg::getDefaultMsg(),
                   KTmpStatusMsg::getDefaultId(), 1);
    sb->setItemAlignment(KTmpStatusMsg::getDefaultId(),
                         AlignLeft | AlignVCenter);

    m_selectionCounter = new QLabel(sb);
    sb->addWidget(m_selectionCounter);

    m_progressBar = new ProgressBar(100, true, sb);
    m_progressBar->setMinimumWidth(100);
    sb->addWidget(m_progressBar);
}

QSize NotationView::getViewSize()
{
    return canvas()->size();
}

void NotationView::setViewSize(QSize s)
{
    canvas()->resize(s.width(), s.height());
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
        KMessageBox::sorry(0, "Couldn't apply layout");
    else {
        for (unsigned int i = 0; i < m_staffs.size(); ++i) {
            m_staffs[i]->markChanged();
        }
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

        slotSetOperationNameAndStatus(i18n("Laying out staff %1...").arg(i + 1));
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
    if (m_rawNoteRuler && m_rawNoteRuler->isVisible()) {
        m_rawNoteRuler->update();
    }

    return true;
}

void NotationView::setCurrentSelectedNote(const char *pixmapName,
                                          bool rest, Note::Type n, int dots)
{
    NoteInserter* inserter = 0;

    if (rest)
        inserter = dynamic_cast<NoteInserter*>(m_toolBox->getTool(RestInserter::ToolName));
    else
        inserter = dynamic_cast<NoteInserter*>(m_toolBox->getTool(NoteInserter::ToolName));

    inserter->slotSetNote(n);
    inserter->slotSetDots(dots);

    setTool(inserter);

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
        (i18n("  1 event selected ",
              "  %n events selected ", eventsSelected));
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
                                   const Note &note,
                                   int velocity)
{
    m_staffs[staffNo]->showPreviewNote(layoutX, height, note);
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

LinedStaff *
NotationView::getCurrentLinedStaff()
{
    return getLinedStaff(m_currentStaff);
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
    if (layoutX < 0)
        layoutX = 0;
    Event *clefEvt = 0, *keyEvt = 0;
    (void)staff->getElementUnderLayoutX(layoutX, clefEvt, keyEvt);

    if (clefEvt)
        clef = Clef(*clefEvt);
    else
        clef = Clef();

    if (keyEvt)
        key = Rosegarden::Key(*keyEvt);
    else
        key = Rosegarden::Key();

    return m_insertionTime;
}

LinedStaff*
NotationView::getStaffForCanvasCoords(int x, int y) const
{
    for (unsigned int i = 0; i < m_staffs.size(); ++i) {

        LinedStaff *s = m_staffs[i];

        //	NOTATION_DEBUG << "NotationView::getStaffForCanvasCoords(" << x << "," << y << "): looking at staff " << i << endl;

        if (s->containsCanvasCoords(x, y)) {

            LinedStaff::LinedStaffCoords coords =
                s->getLayoutCoordsForCanvasCoords(x, y);

            //	    NOTATION_DEBUG << "NotationView::getStaffForCanvasCoords(" << x << "," << y << "): layout coords are (" << coords.first << "," << coords.second << ")" << endl;

            int barNo = m_hlayout->getBarForX(coords.first);
            //	    NOTATION_DEBUG << "NotationView::getStaffForCanvasCoords(" << x << "," << y << "): bar number " << barNo << endl;
            // 931067: < instead of <= in conditional:
            if (barNo >= m_hlayout->getFirstVisibleBarOnStaff(*s) &&
                    barNo < m_hlayout->getLastVisibleBarOnStaff(*s)) {
                //		NOTATION_DEBUG << "NotationView::getStaffForCanvasCoords(" << x << "," << y << "): it's within range for staff " << i << m_hlayout->getFirstVisibleBarOnStaff(*s) << "->" << m_hlayout->getLastVisibleBarOnStaff(*s) << ", returning true" << endl;
                return m_staffs[i];
            }
            //	    NOTATION_DEBUG << "NotationView::getStaffForCanvasCoords(" << x << "," << y << "): out of range for this staff " << i << " (" << m_hlayout->getFirstVisibleBarOnStaff(*s) << "->" << m_hlayout->getLastVisibleBarOnStaff(*s) << ")" << endl;
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
        KMessageBox::error(0, "Nothing to print");
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

    QValueList<int> pages = printer.pageList();

    for (QValueList<int>::Iterator pli = pages.begin();
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

        m_config->setGroup(NotationViewConfigGroup);

        NOTATION_DEBUG << "NotationView::print: calling QCanvas::drawArea" << endl;

        {
            Profiler profiler("NotationView::print(QCanvas::drawArea)");

            if (m_config->readBoolEntry("forcedoublebufferprinting", false)) {
                getCanvasView()->canvas()->drawArea(pageRect, &printpainter, true);
            } else {
#if QT_VERSION >= 0x030100
                getCanvasView()->canvas()->drawArea(pageRect, &printpainter, false);
#else

                getCanvasView()->canvas()->drawArea(pageRect, &printpainter, true);
#endif

            }

        }

        NOTATION_DEBUG << "NotationView::print: QCanvas::drawArea done" << endl;

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
    thumbnail.fill(Qt::white);
    QPainter thumbPainter(&thumbnail);

    if (complete) {

        thumbPainter.scale(1.0 / double(thumbScale), 1.0 / double(thumbScale));
        thumbPainter.setPen(Qt::black);
        thumbPainter.setBrush(Qt::white);

        /*
        	QCanvas *canvas = getCanvasView()->canvas();
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

            QCanvas *canvas = getCanvasView()->canvas();
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

        thumbPainter.setPen(Qt::black);

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
	    emit setProgress(progressCount * 100 / progressTotal); \
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
}

void NotationView::slotNoteAction()
{
    const QObject* sigSender = sender();

    NoteActionDataMap::Iterator noteAct =
        m_noteActionDataMap->find(sigSender->name());

    if (noteAct != m_noteActionDataMap->end()) {
        m_lastNoteAction = sigSender->name();
        setCurrentSelectedNote(**noteAct);
        setMenuStates();
    } else {
        std::cerr << "NotationView::slotNoteAction() : couldn't find NoteActionData named '"
        << sigSender->name() << "'\n";
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

void NotationView::slotAddMark()
{
    const QObject *s = sender();
    if (!m_currentEventSelection)
        return ;

    MarkActionDataMap::Iterator i = m_markActionDataMap->find(s->name());

    if (i != m_markActionDataMap->end()) {
        addCommandToHistory(new AddMarkCommand
                            ((**i).mark, *m_currentEventSelection));
    }
}

void NotationView::slotNoteChangeAction()
{
    const QObject* sigSender = sender();

    NoteChangeActionDataMap::Iterator noteAct =
        m_noteChangeActionDataMap->find(sigSender->name());

    if (noteAct != m_noteChangeActionDataMap->end()) {
        slotSetNoteDurations((**noteAct).noteType, (**noteAct).notationOnly);
    } else {
        std::cerr << "NotationView::slotNoteChangeAction() : couldn't find NoteChangeAction named '"
        << sigSender->name() << "'\n";
    }
}

void NotationView::initActionDataMaps()
{
    static bool called = false;
    static int keys[] =
        { Key_0, Key_3, Key_6, Key_8, Key_4, Key_2, Key_1, Key_5 };

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

                titleName = titleName.left(1).upper() +
                            titleName.right(titleName.length() - 1);

                if (rest) {
                    titleName.replace(QRegExp(i18n("note")), i18n("rest"));
                }

                int keycode = keys[type - Note::Shortest];
                if (dots) // keycode += CTRL; -- used below for note change action
                    keycode = 0;
                if (rest) // keycode += SHIFT; -- can't do shift+numbers
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

            titleName = titleName.left(1).upper() +
                        titleName.right(titleName.length() - 1);

            int keycode = keys[type - Note::Shortest];
            keycode += CTRL;
            if (notationOnly)
                keycode += ALT;

            m_noteChangeActionDataMap->insert
                (shortName, new NoteChangeActionData
                 (titleName, shortName, refName, keycode,
                  notationOnly ? true : false, type));
        }
    }

    m_markActionDataMap = new MarkActionDataMap;

    std::vector<Mark> marks = Marks::getStandardMarks();
    for (unsigned int i = 0; i < marks.size(); ++i) {

        Mark mark = marks[i];
        QString markName(strtoqstr(mark));
        QString actionName = QString("add_%1").arg(markName);

        m_markActionDataMap->insert
            (actionName, new MarkActionData
             (AddMarkCommand::getGlobalName(mark),
              actionName, 0, mark));
    }

}

void NotationView::setupProgress(KProgress* bar)
{
    if (bar) {
        NOTATION_DEBUG << "NotationView::setupProgress(bar)\n";

        connect(m_hlayout, SIGNAL(setProgress(int)),
                bar, SLOT(setValue(int)));

        connect(m_hlayout, SIGNAL(incrementProgress(int)),
                bar, SLOT(advance(int)));

        connect(this, SIGNAL(setProgress(int)),
                bar, SLOT(setValue(int)));

        connect(this, SIGNAL(incrementProgress(int)),
                bar, SLOT(advance(int)));

        for (unsigned int i = 0; i < m_staffs.size(); ++i) {
            connect(m_staffs[i], SIGNAL(setProgress(int)),
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
    disconnect(SIGNAL(setProgress(int)));
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
        setupProgress(m_progressBar);
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
        setCaption(i18n("%1 - Segment Track #%2 - Notation")
                   .arg(getDocument()->getTitle())
                   .arg(trackPosition + 1));

    } else if (m_segments.size() == getDocument()->getComposition().getNbSegments()) {

        setCaption(i18n("%1 - All Segments - Notation")
                   .arg(getDocument()->getTitle()));

    } else {

        setCaption(i18n("%1 - Segment - Notation", "%1 - %n Segments - Notation", m_segments.size())
                   .arg(getDocument()->getTitle()));

    }
}

NotationView::NoteActionDataMap* NotationView::m_noteActionDataMap = 0;

NotationView::NoteChangeActionDataMap* NotationView::m_noteChangeActionDataMap = 0;

NotationView::MarkActionDataMap* NotationView::m_markActionDataMap = 0;


/// SLOTS


void
NotationView::slotUpdateInsertModeStatus()
{
    QString message;
    if (isInChordMode()) {
        if (isInTripletMode()) {
            message = i18n(" Triplet Chord");
        } else {
            message = i18n(" Chord");
        }
    } else {
        if (isInTripletMode()) {
            message = i18n(" Triplet");
        } else {
            message = "";
        }
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
                        ((*j)->get
                         <String>
                         (Text::TextTypePropertyName)
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
                         == Text::LilypondDirective)) {
                    m_lilypondDirectivesLabel->setText(i18n("Hidden LilyPond directives"));
                    return ;
                }
            }
        }
    }
    m_lilypondDirectivesLabel->setText("");
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
    QString name = s->name();

    if (name.left(8) == "spacing_") {
        int spacing = name.right(name.length() - 8).toInt();

        if (spacing > 0)
            slotChangeSpacing(spacing);

    } else {
        KMessageBox::sorry
        (this, i18n("Unknown spacing action %1").arg(name));
    }
}

void
NotationView::slotChangeSpacing(int spacing)
{
    if (m_hlayout->getSpacing() == spacing)
        return ;

    m_hlayout->setSpacing(spacing);

    //     m_spacingSlider->setSize(spacing);

    KToggleAction *action = dynamic_cast<KToggleAction *>
                            (actionCollection()->action(QString("spacing_%1").arg(spacing)));
    if (action)
        action->setChecked(true);
    else {
        std::cerr
        << "WARNING: Expected action \"spacing_" << spacing
        << "\" to be a KToggleAction, but it isn't (or doesn't exist)"
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
    QString name = s->name();

    if (name.left(11) == "proportion_") {
        int proportion = name.right(name.length() - 11).toInt();
        slotChangeProportion(proportion);

    } else {
        KMessageBox::sorry
        (this, i18n("Unknown proportion action %1").arg(name));
    }
}

void
NotationView::slotChangeProportion(int proportion)
{
    if (m_hlayout->getProportion() == proportion)
        return ;

    m_hlayout->setProportion(proportion);

    //    m_proportionSlider->setSize(proportion);

    KToggleAction *action = dynamic_cast<KToggleAction *>
                            (actionCollection()->action(QString("proportion_%1").arg(proportion)));
    if (action)
        action->setChecked(true);
    else {
        std::cerr
        << "WARNING: Expected action \"proportion_" << proportion
        << "\" to be a KToggleAction, but it isn't (or doesn't exist)"
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
    QString name = s->name();
    if (name.left(10) == "note_font_") {
        name = name.right(name.length() - 10);
        slotChangeFont(name);
    } else {
        KMessageBox::sorry
        (this, i18n("Unknown font action %1").arg(name));
    }
}

void
NotationView::slotChangeFontSizeFromAction()
{
    const QObject *s = sender();
    QString name = s->name();

    if (name.left(15) == "note_font_size_") {
        name = name.right(name.length() - 15);
        bool ok = false;
        int size = name.toInt(&ok);
        if (ok)
            slotChangeFont(m_fontName, size);
        else {
            KMessageBox::sorry
            (this, i18n("Unknown font size %1").arg(name));
        }
    } else {
        KMessageBox::sorry
        (this, i18n("Unknown font size action %1").arg(name));
    }
}

void
NotationView::slotChangeFont(const QString &newName)
{
    NOTATION_DEBUG << "changeFont: " << newName << endl;
    slotChangeFont(std::string(newName.utf8()));
}

void
NotationView::slotChangeFont(std::string newName)
{
    int newSize = m_fontSize;

    if (!NoteFontFactory::isAvailableInSize(newName, newSize)) {

        int defaultSize = NoteFontFactory::getDefaultSize(newName);
        newSize = m_config->readUnsignedNumEntry
                  ((getStaffCount() > 1 ?
                    "multistaffnotesize" : "singlestaffnotesize"), defaultSize);

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

    std::set
        <std::string> fs(NoteFontFactory::getFontNames());
    std::vector<std::string> f(fs.begin(), fs.end());
    std::sort(f.begin(), f.end());

    for (unsigned int i = 0; i < f.size(); ++i) {
        bool thisOne = (f[i] == m_fontName);
        if (thisOne)
            m_fontCombo->setCurrentItem(i);
        KToggleAction *action = dynamic_cast<KToggleAction *>
                                (actionCollection()->action("note_font_" + strtoqstr(f[i])));
        NOTATION_DEBUG << "inspecting " << f[i] << (action ? ", have action" : ", no action") << endl;
        if (action)
            action->setChecked(thisOne);
        else {
            std::cerr
            << "WARNING: Expected action \"note_font_" << f[i]
            << "\" to be a KToggleAction, but it isn't (or doesn't exist)"
            << std::endl;
        }
    }

    NOTATION_DEBUG << "about to reinitialise sizes" << endl;

    std::vector<int> sizes = NoteFontFactory::getScreenSizes(m_fontName);
    m_fontSizeCombo->clear();
    QString value;
    for (std::vector<int>::iterator i = sizes.begin(); i != sizes.end(); ++i) {
        value.setNum(*i);
        m_fontSizeCombo->insertItem(value);
    }
    value.setNum(m_fontSize);
    m_fontSizeCombo->setCurrentText(value);

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
        KMessageBox::sorry(0, "Couldn't apply layout");
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
                      "The Restricted paste type requires enough empty\n" \
                      "space (containing only rests) at the paste position\n" \
                      "to hold all of the events to be pasted.\n" \
                      "Not enough space was found.\n" \
                      "If you want to paste anyway, consider using one of\n" \
                      "the other paste types from the \"Paste...\" option\n" \
                      "on the Edit menu.  You can also change the default\n" \
                      "paste type to something other than Restricted if\n" \
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

    KConfig *config = kapp->config();
    config->setGroup(NotationViewConfigGroup);
    PasteEventsCommand::PasteType defaultType = (PasteEventsCommand::PasteType)
        config->readUnsignedNumEntry("pastetype",
                                     PasteEventsCommand::Restricted);

    PasteEventsCommand *command = new PasteEventsCommand
        (segment, clipboard, insertionTime, defaultType);

    if (!command->isPossible()) {
        KMessageBox::detailedError
            (this,
             i18n("Couldn't paste at this point."), RESTRICTED_PASTE_FAILED_DESCRIPTION);
    } else {
        addCommandToHistory(command);
        //!!! well, we really just want to select the events
        // we just pasted
        setCurrentSelection(new EventSelection
                            (segment, insertionTime, endTime));
        slotSetInsertCursorPosition(endTime, true, false);
    }
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

    KConfig *config = kapp->config();
    config->setGroup(NotationViewConfigGroup);
    PasteEventsCommand::PasteType defaultType = (PasteEventsCommand::PasteType)
        config->readUnsignedNumEntry("pastetype",
                                     PasteEventsCommand::Restricted);

    PasteNotationDialog dialog(this, defaultType);

    if (dialog.exec() == QDialog::Accepted) {

        PasteEventsCommand::PasteType type = dialog.getPasteType();
        if (dialog.setAsDefault()) {
            config->setGroup(NotationViewConfigGroup);
            config->writeEntry("pastetype", type);
        }

        timeT insertionTime = getInsertionTime();
        timeT endTime = insertionTime +
            (clipboard->getSingleSegment()->getEndTime() -
             clipboard->getSingleSegment()->getStartTime());

        PasteEventsCommand *command = new PasteEventsCommand
            (segment, clipboard, insertionTime, type);

        if (!command->isPossible()) {
            KMessageBox::detailedError
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

void NotationView::slotFinePositionLeft()
{
    if (!m_currentEventSelection)
        return ;
    KTmpStatusMsg msg(i18n("Pushing selection left..."), this);

    // half a note body width
    addCommandToHistory(new IncrementDisplacementsCommand
                        (*m_currentEventSelection, -500, 0));
}

void NotationView::slotFinePositionRight()
{
    if (!m_currentEventSelection)
        return ;
    KTmpStatusMsg msg(i18n("Pushing selection right..."), this);

    // half a note body width
    addCommandToHistory(new IncrementDisplacementsCommand
                        (*m_currentEventSelection, 500, 0));
}

void NotationView::slotFinePositionUp()
{
    if (!m_currentEventSelection)
        return ;
    KTmpStatusMsg msg(i18n("Pushing selection up..."), this);

    // half line height
    addCommandToHistory(new IncrementDisplacementsCommand
                        (*m_currentEventSelection, 0, -500));
}

void NotationView::slotFinePositionDown()
{
    if (!m_currentEventSelection)
        return ;
    KTmpStatusMsg msg(i18n("Pushing selection down..."), this);

    // half line height
    addCommandToHistory(new IncrementDisplacementsCommand
                        (*m_currentEventSelection, 0, 500));
}

void NotationView::slotFinePositionRestore()
{
    if (!m_currentEventSelection)
        return ;
    KTmpStatusMsg msg(i18n("Restoring computed positions..."), this);

    addCommandToHistory(new ResetDisplacementsCommand(*m_currentEventSelection));
}

void NotationView::slotMakeVisible()
{
    if (!m_currentEventSelection)
        return ;
    KTmpStatusMsg msg(i18n("Making visible..."), this);

    addCommandToHistory(new SetVisibilityCommand(*m_currentEventSelection, true));
}

void NotationView::slotMakeInvisible()
{
    if (!m_currentEventSelection)
        return ;
    KTmpStatusMsg msg(i18n("Making invisible..."), this);

    addCommandToHistory(new SetVisibilityCommand(*m_currentEventSelection, false));
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

void NotationView::slotGroupBeam()
{
    if (!m_currentEventSelection)
        return ;
    KTmpStatusMsg msg(i18n("Beaming group..."), this);

    addCommandToHistory(new BeamCommand
                        (*m_currentEventSelection));
}

void NotationView::slotGroupAutoBeam()
{
    if (!m_currentEventSelection)
        return ;
    KTmpStatusMsg msg(i18n("Auto-beaming selection..."), this);

    addCommandToHistory(new AutoBeamCommand
                        (*m_currentEventSelection));
}

void NotationView::slotGroupBreak()
{
    if (!m_currentEventSelection)
        return ;
    KTmpStatusMsg msg(i18n("Breaking groups..."), this);

    addCommandToHistory(new BreakCommand
                        (*m_currentEventSelection));
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

void NotationView::slotGroupUnTuplet()
{
    if (!m_currentEventSelection)
        return ;
    KTmpStatusMsg msg(i18n("Untupleting..."), this);

    addCommandToHistory(new UnTupletCommand
                        (*m_currentEventSelection));
}

void NotationView::slotGroupGrace()
{
    if (!m_currentEventSelection)
        return ;
    KTmpStatusMsg msg(i18n("Making grace notes..."), this);

    addCommandToHistory(new GraceCommand(*m_currentEventSelection));
}

void NotationView::slotGroupUnGrace()
{
    if (!m_currentEventSelection)
        return ;
    KTmpStatusMsg msg(i18n("Making non-grace notes..."), this);

    addCommandToHistory(new UnGraceCommand(*m_currentEventSelection));
}

void NotationView::slotGroupSlur()
{
    KTmpStatusMsg msg(i18n("Adding slur..."), this);
    slotAddIndication(Indication::Slur, i18n("slur"));
}

void NotationView::slotGroupPhrasingSlur()
{
    KTmpStatusMsg msg(i18n("Adding phrasing slur..."), this);
    slotAddIndication(Indication::PhrasingSlur, i18n("phrasing slur"));
}

void NotationView::slotGroupGlissando()
{
    KTmpStatusMsg msg(i18n("Adding glissando..."), this);
    slotAddIndication(Indication::Glissando, i18n("glissando"));
}

void NotationView::slotGroupCrescendo()
{
    KTmpStatusMsg msg(i18n("Adding crescendo..."), this);
    slotAddIndication(Indication::Crescendo, i18n("dynamic"));
}

void NotationView::slotGroupDecrescendo()
{
    KTmpStatusMsg msg(i18n("Adding decrescendo..."), this);
    slotAddIndication(Indication::Decrescendo, i18n("dynamic"));
}

void NotationView::slotGroupOctave2Up()
{
    KTmpStatusMsg msg(i18n("Adding octave..."), this);
    slotAddIndication(Indication::QuindicesimaUp, i18n("ottava"));
}

void NotationView::slotGroupOctaveUp()
{
    KTmpStatusMsg msg(i18n("Adding octave..."), this);
    slotAddIndication(Indication::OttavaUp, i18n("ottava"));
}

void NotationView::slotGroupOctaveDown()
{
    KTmpStatusMsg msg(i18n("Adding octave..."), this);
    slotAddIndication(Indication::OttavaDown, i18n("ottava"));
}

void NotationView::slotGroupOctave2Down()
{
    KTmpStatusMsg msg(i18n("Adding octave..."), this);
    slotAddIndication(Indication::QuindicesimaDown, i18n("ottava"));
}

void NotationView::slotAddIndication(std::string type, QString desc)
{
    if (!m_currentEventSelection)
        return ;

    AddIndicationCommand *command =
        new AddIndicationCommand(type, *m_currentEventSelection);

    if (command->canExecute()) {
        addCommandToHistory(command);
        setSingleSelectedEvent(m_currentEventSelection->getSegment(),
                               command->getLastInsertedEvent());
    } else {
        KMessageBox::sorry(this, i18n("Can't add overlapping %1 indications").arg(desc)); // TODO PLURAL - how many 'indications' ?
        delete command;
    }
}

void NotationView::slotGroupMakeChord()
{
    if (!m_currentEventSelection)
        return ;
    KTmpStatusMsg msg(i18n("Making chord..."), this);

    MakeChordCommand *command =
        new MakeChordCommand(*m_currentEventSelection);

    addCommandToHistory(command);
}

void NotationView::slotTransformsNormalizeRests()
{
    if (!m_currentEventSelection)
        return ;
    KTmpStatusMsg msg(i18n("Normalizing rests..."), this);

    addCommandToHistory(new NormalizeRestsCommand
                        (*m_currentEventSelection));
}

void NotationView::slotTransformsCollapseRests()
{
    if (!m_currentEventSelection)
        return ;
    KTmpStatusMsg msg(i18n("Collapsing rests..."), this);

    addCommandToHistory(new CollapseRestsCommand
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

void NotationView::slotTransformsTieNotes()
{
    if (!m_currentEventSelection)
        return ;
    KTmpStatusMsg msg(i18n("Tying notes..."), this);

    addCommandToHistory(new TieNotesCommand
                        (*m_currentEventSelection));
}

void NotationView::slotTransformsUntieNotes()
{
    if (!m_currentEventSelection)
        return ;
    KTmpStatusMsg msg(i18n("Untying notes..."), this);

    addCommandToHistory(new UntieNotesCommand
                        (*m_currentEventSelection));
}

void NotationView::slotTransformsMakeNotesViable()
{
    if (!m_currentEventSelection)
        return ;
    KTmpStatusMsg msg(i18n("Making notes viable..."), this);

    addCommandToHistory(new MakeNotesViableCommand
                        (*m_currentEventSelection));
}

void NotationView::slotTransformsDeCounterpoint()
{
    if (!m_currentEventSelection)
        return ;
    KTmpStatusMsg msg(i18n("Removing counterpoint..."), this);

    addCommandToHistory(new DeCounterpointCommand
                        (*m_currentEventSelection));
}

void NotationView::slotTransformsStemsUp()
{
    if (!m_currentEventSelection)
        return ;
    KTmpStatusMsg msg(i18n("Pointing stems up..."), this);

    addCommandToHistory(new ChangeStemsCommand
                        (true, *m_currentEventSelection));
}

void NotationView::slotTransformsStemsDown()
{
    if (!m_currentEventSelection)
        return ;
    KTmpStatusMsg msg(i18n("Pointing stems down..."), this);

    addCommandToHistory(new ChangeStemsCommand
                        (false, *m_currentEventSelection));

}

void NotationView::slotTransformsRestoreStems()
{
    if (!m_currentEventSelection)
        return ;
    KTmpStatusMsg msg(i18n("Restoring computed stem directions..."), this);

    addCommandToHistory(new RestoreStemsCommand
                        (*m_currentEventSelection));
}

void NotationView::slotTransformsSlursAbove()
{
    if (!m_currentEventSelection)
        return ;
    KTmpStatusMsg msg(i18n("Positioning slurs..."), this);

    addCommandToHistory(new ChangeSlurPositionCommand
                        (true, *m_currentEventSelection));
}

void NotationView::slotTransformsSlursBelow()
{
    if (!m_currentEventSelection)
        return ;
    KTmpStatusMsg msg(i18n("Positioning slurs..."), this);

    addCommandToHistory(new ChangeSlurPositionCommand
                        (false, *m_currentEventSelection));

}

void NotationView::slotTransformsRestoreSlurs()
{
    if (!m_currentEventSelection)
        return ;
    KTmpStatusMsg msg(i18n("Restoring slur positions..."), this);

    addCommandToHistory(new RestoreSlursCommand
                        (*m_currentEventSelection));
}

void NotationView::slotTransformsFixQuantization()
{
    if (!m_currentEventSelection)
        return ;
    KTmpStatusMsg msg(i18n("Fixing notation quantization..."), this);

    addCommandToHistory(new FixNotationQuantizeCommand
                        (*m_currentEventSelection));
}

void NotationView::slotTransformsRemoveQuantization()
{
    if (!m_currentEventSelection)
        return ;
    KTmpStatusMsg msg(i18n("Removing notation quantization..."), this);

    addCommandToHistory(new RemoveNotationQuantizeCommand
                        (*m_currentEventSelection));
}

void NotationView::slotSetStyleFromAction()
{
    const QObject *s = sender();
    QString name = s->name();

    if (!m_currentEventSelection)
        return ;

    if (name.left(6) == "style_") {
        name = name.right(name.length() - 6);

        KTmpStatusMsg msg(i18n("Changing to %1 style...").arg(name),
                          this);

        addCommandToHistory(new ChangeStyleCommand
                            (NoteStyleName(qstrtostr(name)),
                             *m_currentEventSelection));
    } else {
        KMessageBox::sorry
            (this, i18n("Unknown style action %1").arg(name));
    }
}

void NotationView::slotInsertNoteFromAction()
{
    const QObject *s = sender();
    QString name = s->name();

    Segment &segment = m_staffs[m_currentStaff]->getSegment();

    NoteInserter *noteInserter = dynamic_cast<NoteInserter *>(m_tool);
    if (!noteInserter) {
        KMessageBox::sorry(this, i18n("No note duration selected"));
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

        KMessageBox::sorry
            (this, i18n("Unknown note insert action %1").arg(name));
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
            KMessageBox::sorry(this, i18n("No note duration selected"));
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
        KMessageBox::sorry(this, i18n("No rest duration selected"));
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
        KMessageBox::sorry(this, i18n("No note duration selected"));
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
    NoteInserter *noteInserter = dynamic_cast<NoteInserter *>(m_tool);
    if (noteInserter) {
        Note note(noteInserter->getCurrentNote());
        if (note.getNoteType() == Note::Shortest ||
            note.getNoteType() == Note::Longest)
            return ;
        noteInserter->slotSetDots(note.getDots() ? 0 : 1);
        setTool(noteInserter);
    } else {
        RestInserter *restInserter = dynamic_cast<RestInserter *>(m_tool);
        if (restInserter) {
            Note note(restInserter->getCurrentNote());
            if (note.getNoteType() == Note::Shortest ||
                note.getNoteType() == Note::Longest)
                return ;
            restInserter->slotSetDots(note.getDots() ? 0 : 1);
            setTool(restInserter);
        } else {
            KMessageBox::sorry(this, i18n("No note or rest duration selected"));
        }
    }

    setMenuStates();
}

void NotationView::slotRespellDoubleFlat()
{
    if (!m_currentEventSelection)
        return ;
    KTmpStatusMsg msg(i18n("Forcing accidentals..."), this);

    addCommandToHistory(new RespellCommand(RespellCommand::Set,
                                           Accidentals::DoubleFlat,
                                           *m_currentEventSelection));
}

void NotationView::slotRespellFlat()
{
    if (!m_currentEventSelection)
        return ;
    KTmpStatusMsg msg(i18n("Forcing accidentals..."), this);

    addCommandToHistory(new RespellCommand(RespellCommand::Set,
                                           Accidentals::Flat,
                                           *m_currentEventSelection));
}

void NotationView::slotRespellNatural()
{
    if (!m_currentEventSelection)
        return ;
    KTmpStatusMsg msg(i18n("Forcing accidentals..."), this);

    addCommandToHistory(new RespellCommand(RespellCommand::Set,
                                           Accidentals::Natural,
                                           *m_currentEventSelection));
}

void NotationView::slotRespellSharp()
{
    if (!m_currentEventSelection)
        return ;
    KTmpStatusMsg msg(i18n("Forcing accidentals..."), this);

    addCommandToHistory(new RespellCommand(RespellCommand::Set,
                                           Accidentals::Sharp,
                                           *m_currentEventSelection));
}

void NotationView::slotRespellDoubleSharp()
{
    if (!m_currentEventSelection)
        return ;
    KTmpStatusMsg msg(i18n("Forcing accidentals..."), this);

    addCommandToHistory(new RespellCommand(RespellCommand::Set,
                                           Accidentals::DoubleSharp,
                                           *m_currentEventSelection));
}

void NotationView::slotRespellUp()
{
    if (!m_currentEventSelection)
        return ;
    KTmpStatusMsg msg(i18n("Forcing accidentals..."), this);

    addCommandToHistory(new RespellCommand(RespellCommand::Up,
                                           Accidentals::NoAccidental,
                                           *m_currentEventSelection));
}

void NotationView::slotRespellDown()
{
    if (!m_currentEventSelection)
        return ;
    KTmpStatusMsg msg(i18n("Forcing accidentals..."), this);

    addCommandToHistory(new RespellCommand(RespellCommand::Down,
                                           Accidentals::NoAccidental,
                                           *m_currentEventSelection));
}

void NotationView::slotRespellRestore()
{
    if (!m_currentEventSelection)
        return ;
    KTmpStatusMsg msg(i18n("Restoring accidentals..."), this);

    addCommandToHistory(new RespellCommand(RespellCommand::Restore,
                                           Accidentals::NoAccidental,
                                           *m_currentEventSelection));
}

void NotationView::slotShowCautionary()
{
    if (!m_currentEventSelection)
        return ;
    KTmpStatusMsg msg(i18n("Showing cautionary accidentals..."), this);

    addCommandToHistory(new MakeAccidentalsCautionaryCommand
                        (true, *m_currentEventSelection));
}

void NotationView::slotCancelCautionary()
{
    if (!m_currentEventSelection)
        return ;
    KTmpStatusMsg msg(i18n("Cancelling cautionary accidentals..."), this);

    addCommandToHistory(new MakeAccidentalsCautionaryCommand
                        (false, *m_currentEventSelection));
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

void NotationView::slotAddSlashes()
{
    const QObject *s = sender();
    if (!m_currentEventSelection)
        return ;

    QString name = s->name();
    int slashes = name.right(1).toInt();

    addCommandToHistory(new AddSlashesCommand
                        (slashes, *m_currentEventSelection));
}

void NotationView::slotMarksAddTextMark()
{
    if (m_currentEventSelection) {
        bool pressedOK = false;

        QString txt = KLineEditDlg::getText(i18n("Text: "), "", &pressedOK, this);

        if (pressedOK) {
            addCommandToHistory(new AddTextMarkCommand
                                (qstrtostr(txt), *m_currentEventSelection));
        }
    }
}

void NotationView::slotMarksAddFingeringMark()
{
    if (m_currentEventSelection) {
        bool pressedOK = false;

        QString txt = KLineEditDlg::getText(i18n("Fingering: "), "", &pressedOK, this);

        if (pressedOK) {
            addCommandToHistory(new AddFingeringMarkCommand
                                (qstrtostr(txt), *m_currentEventSelection));
        }
    }
}

void NotationView::slotMarksAddFingeringMarkFromAction()
{
    const QObject *s = sender();
    QString name = s->name();

    if (name.left(14) == "add_fingering_") {

        QString fingering = name.right(name.length() - 14);

        if (fingering == "plus")
            fingering = "+";

        if (m_currentEventSelection) {
            addCommandToHistory(new AddFingeringMarkCommand
                                (qstrtostr(fingering), *m_currentEventSelection));
        }
    }
}

void NotationView::slotMarksRemoveMarks()
{
    if (m_currentEventSelection)
        addCommandToHistory(new RemoveMarksCommand
                            (*m_currentEventSelection));
}

void NotationView::slotMarksRemoveFingeringMarks()
{
    if (m_currentEventSelection)
        addCommandToHistory(new RemoveFingeringMarksCommand
                            (*m_currentEventSelection));
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
                basePitch = (*i)->get
                    <Int>
                    (BaseProperties::PITCH);
                style = NoteStyleFactory::getStyleForEvent(*i);
                if (baseVelocity != -1)
                    break;
            }
            if ((*i)->has(BaseProperties::VELOCITY)) {
                baseVelocity = (*i)->get
                    <Int>
                    (BaseProperties::VELOCITY);
                if (basePitch != -1)
                    break;
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
        name = QString(i18n("Ornament track %1 bar %2").arg(track->getPosition() + 1).arg(barNo + 1));
    } else {
        name = QString(i18n("Ornament bar %1").arg(barNo + 1));
    }

    MakeOrnamentDialog dialog(this, name, basePitch);
    if (dialog.exec() != QDialog::Accepted)
        return ;

    name = dialog.getName();
    basePitch = dialog.getBasePitch();

    KMacroCommand *command = new KMacroCommand(i18n("Make Ornament"));

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

        if (applyToAll) {
            addCommandToHistory
                (new MultiKeyInsertionCommand
                 (getDocument()->getComposition(),
                  insertionTime, dialog.getKey(),
                  conversion == KeySignatureDialog::Convert,
                  conversion == KeySignatureDialog::Transpose,
                  transposeKey));
        } else {
            addCommandToHistory
                (new KeyInsertionCommand
                 (segment,
                  insertionTime, dialog.getKey(),
                  conversion == KeySignatureDialog::Convert,
                  conversion == KeySignatureDialog::Transpose,
                  transposeKey));
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

    KMessageBox::sorry(this, i18n("There is no sustain controller defined for this device.\nPlease ensure the device is configured correctly in the Manage MIDI Devices dialog in the main window."));
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
    
    Segment &segment = m_staffs[m_currentStaff]->getSegment();
	EventSelection wholeSegment(segment, segment.getStartTime(), segment.getEndMarkerTime());
    
    //TODO who cleans this up?
    KMacroCommand *macro = new KMacroCommand("Transpose Segment by Interval");
    
    // Key insertion can do transposition, but a C4 to D becomes a D4, while
	//  a C4 to G becomes a G3. Because we let the user specify an explicit number
	//  of octaves to move the notes up/down, we add the keys without transposing
	//  and handle the transposition ourselves:
	if (intervalDialog.getChangeKey())
	{
		Rosegarden::Key key = segment.getKeyAtTime(segment.getStartTime());
		Rosegarden::Key newKey = key.transpose(semitones, steps);
		
		macro->addCommand
			(new KeyInsertionCommand
			 (segment,
			  segment.getStartTime(),
			  newKey,
			  false,
			  false,
			  true));
		
		EventSelection::eventcontainer::iterator i;
		std::list<KeyInsertionCommand*> commands;
		
		for (i = wholeSegment.getSegmentEvents().begin();
            i != wholeSegment.getSegmentEvents().end(); ++i) {
        		// transpose key
				if ((*i)->isa(Rosegarden::Key::EventType)) {
        			macro->addCommand
						(new KeyInsertionCommand
						 (segment,
			  			 (*i)->getAbsoluteTime(),
			  			 (Rosegarden::Key (**i)).transpose(semitones, steps),
			 			 false,
			 			 false,
					 	 true));
        		}
        		
        }
	}
	
	macro->addCommand(new TransposeCommand
		(semitones, steps, wholeSegment));
	
	if (intervalDialog.getTransposeSegmentBack())
	{
		// Transpose segment in opposite direction
		int newTranspose = segment.getTranspose() - semitones;
		macro->addCommand(new SegmentChangeTransposeCommand(newTranspose, &segment));
	}
	
	addCommandToHistory(macro);
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
                      dialog.shouldBeTransposed()));
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
                KMacroCommand *macroCommand = new KMacroCommand(command->name());
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

void NotationView::slotBeginLilypondRepeat()
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
            //	    refreshSegment(segment, updateFrom, segment->getEndMarkerTime());
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

        slotSetInsertCursorPosition(getInsertionTime(), false, false);
    }
}

void
NotationView::slotCurrentStaffUp()
{
    if (m_staffs.size() < 2)
        return ;

    Composition *composition =
        m_staffs[m_currentStaff]->getSegment().getComposition();

    Track *track = composition->
        getTrackById(m_staffs[m_currentStaff]->getSegment().getTrack());
    if (!track)
        return ;

    int position = track->getPosition();
    Track *newTrack = 0;

    while ((newTrack = composition->getTrackByPosition(--position))) {
        for (unsigned int i = 0; i < m_staffs.size(); ++i) {
            if (m_staffs[i]->getSegment().getTrack() == newTrack->getId()) {
                slotSetCurrentStaff(i);
                return ;
            }
        }
    }
}

void
NotationView::slotCurrentStaffDown()
{
    if (m_staffs.size() < 2)
        return ;

    Composition *composition =
        m_staffs[m_currentStaff]->getSegment().getComposition();

    Track *track = composition->
        getTrackById(m_staffs[m_currentStaff]->getSegment().getTrack());
    if (!track)
        return ;

    int position = track->getPosition();
    Track *newTrack = 0;

    while ((newTrack = composition->getTrackByPosition(++position))) {
        for (unsigned int i = 0; i < m_staffs.size(); ++i) {
            if (m_staffs[i]->getSegment().getTrack() == newTrack->getId()) {
                slotSetCurrentStaff(i);
                return ;
            }
        }
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

void NotationView::slotFretboard()
{
    m_currentNotePixmap->setPixmap
        (NotePixmapFactory::toQPixmap(NotePixmapFactory::makeToolbarPixmap("text")));
    setTool(m_toolBox->getTool(FretboardInserter::ToolName));
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
    m_lilypondDirectivesVisible = !m_lilypondDirectivesVisible;
    slotUpdateLilyPondDirectivesStatus();
    //!!! use refresh mechanism
    refreshSegment(0, 0, 0);
}

void NotationView::slotEditLyrics()
{
    Staff *staff = getCurrentStaff();
    Segment &segment = staff->getSegment();

    LyricEditDialog dialog(this, &segment);

    if (dialog.exec() == QDialog::Accepted) {

        KMacroCommand *macro = new KMacroCommand
            (SetLyricsCommand::getGlobalName());

        for (int i = 0; i < dialog.getVerseCount(); ++i) {
            SetLyricsCommand *command = new SetLyricsCommand
                (&segment, i, dialog.getLyricData(i));
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

void NotationView::slotNonNotationItemPressed(QMouseEvent *e, QCanvasItem *it)
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

void NotationView::slotTextItemPressed(QMouseEvent *e, QCanvasItem *it)
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

    QString message = i18n("Time: %1 (%2.%3s)")
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

    KToggleAction *action = dynamic_cast<KToggleAction *>
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
        KMessageBox::sorry(this, i18n("Can't insert note: No note duration selected"));
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
    KToggleAction *action = dynamic_cast<KToggleAction *>
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
    KToggleAction *action = dynamic_cast<KToggleAction *>
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
}

NotationCanvasView* NotationView::getCanvasView()
{
    return dynamic_cast<NotationCanvasView *>(m_canvasView);
}

}
#include "NotationView.moc"
