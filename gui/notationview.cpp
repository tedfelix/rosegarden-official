// -*- c-basic-offset: 4 -*-

/*
    Rosegarden-4 v0.2
    A sequencer and musical notation editor.

    This program is Copyright 2000-2002
        Guillaume Laurent   <glaurent@telegraph-road.org>,
        Chris Cannam        <cannam@all-day-breakfast.com>,
        Richard Bown        <bownie@bownie.com>

    The moral right of the authors to claim authorship of this work
    has been asserted.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/


#include <qstring.h>
#include <qregexp.h>
#include <qpaintdevicemetrics.h>

#include <kmessagebox.h>
#include <kstatusbar.h>
#include <klocale.h>
#include <kconfig.h>
#include <kaction.h>
#include <kstdaction.h>
#include <kapp.h>
#include <kprinter.h>

#include "notationview.h"

#include "NotationTypes.h"
#include "Quantizer.h"
#include "Selection.h"
#include "Progress.h"
#include "BaseProperties.h"

#include "Profiler.h"

#include "notepixmapfactory.h"
#include "notestyle.h"
#include "notationtool.h"
#include "barbuttons.h"
#include "loopruler.h"
#include "rosedebug.h"
#include "editcommands.h"
#include "notationcommands.h"
#include "segmentcommands.h"
#include "widgets.h"
#include "chordnameruler.h"
#include "temporuler.h"
#include "studiocontrol.h"

#include "qcanvassimplesprite.h"
#include "ktmpstatusmsg.h"

using Rosegarden::Event;
using Rosegarden::Note;
using Rosegarden::Segment;
using Rosegarden::EventSelection;
using Rosegarden::Quantizer;
using Rosegarden::timeT;
using Rosegarden::Mark;
using namespace Rosegarden::Marks;


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
    : title(i18n(_title)),
      actionName(_actionName),
      pixmapName(_pixmapName),
      keycode(_keycode),
      rest(_rest),
      noteType(_noteType),
      dots(_dots)
{
}


class MarkActionData
{
public:
    MarkActionData() :
	title(0),
	actionName(0),
	pixmapName(0),
	keycode(0) { }

    MarkActionData(const QString &_title,
		   QString _actionName,
		   QString _pixmapName,
		   int _keycode,
		   Mark _mark) :
	title(_title),
	actionName(_actionName),
	pixmapName(_pixmapName),
	keycode(_keycode),
	mark(_mark) { }

    QString title;
    QString actionName;
    QString pixmapName;
    int keycode;
    Mark mark;
};




//////////////////////////////////////////////////////////////////////

NotationView::NotationView(RosegardenGUIDoc *doc,
                           std::vector<Segment *> segments,
                           QWidget *parent,
			   bool showProgressive) :
    EditView(doc, segments, 1, parent, "notationview"),
    m_properties(getViewLocalPropertyPrefix()),
    m_currentEventSelection(0),
    m_legatoQuantizer(new Quantizer(Quantizer::RawEventData,
				    getViewLocalPropertyPrefix() + "Q",
				    Quantizer::LegatoQuantize)),
    m_selectionCounter(0),
    m_currentNotePixmap(0),
    m_hoveredOverNoteName(0),
    m_hoveredOverAbsoluteTime(0),
    m_lastFinishingStaff(-1),
    m_insertionTime(0),
    m_fontName(NotePixmapFactory::getDefaultFont()),
    m_fontSize(NotePixmapFactory::getDefaultSize(m_fontName)),
    m_notePixmapFactory(new NotePixmapFactory(m_fontName, m_fontSize)),
    m_hlayout(&doc->getComposition(), m_notePixmapFactory,
	      m_legatoQuantizer, m_properties),
    m_vlayout(&doc->getComposition(),
	      m_legatoQuantizer, m_properties),
    m_topBarButtons(0),
    m_bottomBarButtons(0),
    m_chordNameRuler(0),
    m_tempoRuler(0),
    m_chordNamesVisible(false),
    m_temposVisible(false),
    m_annotationsVisible(false),
//!!!    m_tupletMode(false),
    m_selectDefaultNote(0),
    m_fontCombo(0),
    m_fontSizeSlider(0),
    m_spacingSlider(0),
    m_smoothingSlider(0),
    m_fontSizeActionMenu(0),
    m_progress(0),
    m_inhibitRefresh(true),
    m_documentDestroyed(false),
    m_ok(false)
{
    initActionDataMaps(); // does something only the 1st time it's called
    
    m_toolBox = new NotationToolBox(this);

    assert(segments.size() > 0);
    NOTATION_DEBUG << "NotationView ctor" << endl;


    // Initialise the display-related defaults that will be needed
    // by both the actions and the font toolbar

    m_config->setGroup("Notation Options");

    m_fontName = qstrtostr(m_config->readEntry
			   ("notefont",
			    strtoqstr(NotePixmapFactory::getDefaultFont())));

    m_fontSize = m_config->readUnsignedNumEntry
	((segments.size() > 1 ? "multistaffnotesize" : "singlestaffnotesize"),
	 NotePixmapFactory::getDefaultSize(m_fontName));

    int defaultSpacing = m_config->readNumEntry("spacing", 100);
    m_hlayout.setSpacing(defaultSpacing);

    Note::Type defaultSmoothingType = 
	m_config->readNumEntry("smoothing", Note::Shortest);
    timeT defaultSmoothing = Note(defaultSmoothingType).getDuration();
    for (int type = Note::Shortest; type <= Note::Longest; ++type) {
	m_legatoDurations.push_back((int)(Note(type).getDuration()));
    }
    Rosegarden::Quantizer q(Rosegarden::Quantizer::RawEventData,
			    getViewLocalPropertyPrefix() + "Q",
			    Rosegarden::Quantizer::LegatoQuantize,
			    defaultSmoothing);
    *m_legatoQuantizer = q;
    

    setupActions();
    initFontToolbar();
    initStatusBar();
    
    setBackgroundMode(PaletteBase);

    QCanvas *tCanvas = new QCanvas(this);
    tCanvas->resize(width() * 2, height() * 2);
    
    setCanvasView(new NotationCanvasView(*this, m_horizontalScrollBar,
                                         tCanvas, getCentralFrame()));

    if (segments.size() == 1) {
        setCaption(QString("%1 - Segment Track #%2")
                   .arg(doc->getTitle())
                   .arg(segments[0]->getTrack()));
    } else if (segments.size() == doc->getComposition().getNbSegments()) {
        setCaption(QString("%1 - All Segments")
                   .arg(doc->getTitle()));
    } else {
        setCaption(QString("%1 - %2-Segment Partial View")
                   .arg(doc->getTitle())
                   .arg(segments.size()));
    }

    m_topBarButtons = new BarButtons(&m_hlayout, 20.0, 25,
                                     false, getCentralFrame());
    setTopBarButtons(m_topBarButtons);

    m_topBarButtons->getLoopRuler()->setBackgroundColor
	(RosegardenGUIColours::InsertCursorRuler);

    m_chordNameRuler = new ChordNameRuler
	(&m_hlayout, 0, 20.0, 20, getCentralFrame());
    addRuler(m_chordNameRuler);
    if (showProgressive) m_chordNameRuler->show();
    m_chordNamesVisible = true;

    m_tempoRuler = new TempoRuler
	(&m_hlayout, &doc->getComposition(),
	 20.0, 20, false, getCentralFrame());
    addRuler(m_tempoRuler);
    m_tempoRuler->hide();
    m_temposVisible = false;

    m_bottomBarButtons = new BarButtons(&m_hlayout, 20.0, 25,
                                        true, getCentralFrame());
    setBottomBarButtons(m_bottomBarButtons);

    for (unsigned int i = 0; i < segments.size(); ++i) {
        m_staffs.push_back(new NotationStaff(canvas(), segments[i], 0, // snap
                                             i, this, false, getPageWidth(),
                                             m_fontName, m_fontSize));
    }

    if (showProgressive) {
	show();
	RosegardenProgressDialog *progressDlg = new RosegardenProgressDialog
	    (i18n("Starting..."), i18n("Cancel"), 100, this,
	     i18n("Notation progress"), true);
	progressDlg->setAutoClose(false);
	m_progress = progressDlg;
	for (unsigned int i = 0; i < m_staffs.size(); ++i) {
	    m_staffs[i]->setProgressReporter(m_progress);
	}
	progressDlg->hide();
    }

    m_chordNameRuler->setComposition(&doc->getComposition());

    positionStaffs();
    m_currentStaff = 0;
    m_staffs[0]->setCurrent(true);

    m_hlayout.setPageMode(false);
    m_hlayout.setPageWidth(getPageWidth());

    try {
	bool layoutApplied = applyLayout();
	if (!layoutApplied) {
	    KMessageBox::sorry(0, i18n("Couldn't apply score layout"));
	} else {
	    for (unsigned int i = 0; i < m_staffs.size(); ++i) {
		
		m_staffs[i]->renderAllElements();
		m_staffs[i]->positionAllElements();
		m_staffs[i]->getSegment().getRefreshStatus
		    (m_segmentsRefreshStatusIds[i]).setNeedsRefresh(false);
		
		canvas()->update();
	    }
	}
	m_ok = true;
    } catch (std::string s) {
	if (s != "Action cancelled") {
	    throw;
	}
	// when cancelled, m_ok is false -- checked by calling method
    }

    if (showProgressive) {
	m_progress->done();
	m_progress = 0;
	for (unsigned int i = 0; i < m_staffs.size(); ++i) {
	    m_staffs[i]->setProgressReporter(0);
	}
    }

    //
    // Connect signals
    //

    QObject::connect
	(m_topBarButtons->getLoopRuler(),
	 SIGNAL(setPointerPosition(Rosegarden::timeT)),
	 this, SLOT(slotSetInsertCursorPosition(Rosegarden::timeT)));

    m_bottomBarButtons->connectRulerToDocPointer(doc);

    QObject::connect
        (getCanvasView(), SIGNAL(itemPressed(int, int, QMouseEvent*, NotationElement*)),
         this,            SLOT  (slotItemPressed(int, int, QMouseEvent*, NotationElement*)));

    QObject::connect
        (getCanvasView(), SIGNAL(activeItemPressed(QMouseEvent*, QCanvasItem*)),
         this,         SLOT  (slotActiveItemPressed(QMouseEvent*, QCanvasItem*)));

    QObject::connect
        (getCanvasView(), SIGNAL(mouseMoved(QMouseEvent*)),
         this,         SLOT  (slotMouseMoved(QMouseEvent*)));

    QObject::connect
        (getCanvasView(), SIGNAL(mouseReleased(QMouseEvent*)),
         this,         SLOT  (slotMouseReleased(QMouseEvent*)));

    QObject::connect
        (getCanvasView(), SIGNAL(hoveredOverNoteChanged(const QString&)),
         this,         SLOT  (slotHoveredOverNoteChanged(const QString&)));

    QObject::connect
        (getCanvasView(), SIGNAL(hoveredOverAbsoluteTimeChanged(unsigned int)),
         this,         SLOT  (slotHoveredOverAbsoluteTimeChanged(unsigned int)));

    QObject::connect
	(doc, SIGNAL(pointerPositionChanged(Rosegarden::timeT)),
	 this, SLOT(slotSetPointerPosition(Rosegarden::timeT)));

    QObject::connect
	(doc, SIGNAL(destroyed()), this, SLOT(slotDocumentDestroyed()));

#ifdef RGKDE3
    stateChanged("have_selection", KXMLGUIClient::StateReverse);
    stateChanged("have_notes_in_selection", KXMLGUIClient::StateReverse);
    stateChanged("have_rests_in_selection", KXMLGUIClient::StateReverse);
    stateChanged("have_multiple_staffs",
		 (m_staffs.size() > 1 ? KXMLGUIClient::StateNoReverse :
		                        KXMLGUIClient::StateReverse));
    stateChanged("rest_insert_tool_current", KXMLGUIClient::StateReverse);
    slotTestClipboard();
#endif

    if (getSegmentsOnlyRests()) {
        m_selectDefaultNote->activate();
#ifdef RGKDE3
	stateChanged("note_insert_tool_current", 
		     KXMLGUIClient::StateNoReverse);
#endif
    } else {
        actionCollection()->action("select")->activate();
#ifdef RGKDE3
	stateChanged("note_insert_tool_current",
		     KXMLGUIClient::StateReverse);
#endif
    }

    slotSetInsertCursorPosition(0);
    slotSetPointerPosition(doc->getComposition().getPosition());
    setCurrentSelection(0, false);
    m_chordNameRuler->repaint();
    m_inhibitRefresh = false;

    setConfigDialogPageIndex(1);

    // All toolbars should be created before this is called
    setAutoSaveSettings("NotationView", true);

    readOptions();
}

//
// Notation Print mode
//
NotationView::NotationView(RosegardenGUIDoc *doc,
                           std::vector<Segment *> segments,
                           KPrinter *printer)
    : EditView(doc, segments, 1, 0, "printview"),
    m_properties(getViewLocalPropertyPrefix()),
    m_currentEventSelection(0),
    m_legatoQuantizer(new Quantizer(Quantizer::RawEventData,
				    getViewLocalPropertyPrefix() + "Q",
				    Quantizer::LegatoQuantize)),
    m_selectionCounter(0),
    m_currentNotePixmap(0),
    m_hoveredOverNoteName(0),
    m_hoveredOverAbsoluteTime(0),
    m_lastFinishingStaff(-1),
    m_insertionTime(0),
    m_fontName(NotePixmapFactory::getDefaultFont()),
    m_fontSize(NotePixmapFactory::getDefaultSize(m_fontName)),
    m_notePixmapFactory(new NotePixmapFactory(m_fontName, m_fontSize)),
    m_hlayout(&doc->getComposition(), m_notePixmapFactory,
	      m_legatoQuantizer, m_properties),
    m_vlayout(&doc->getComposition(),
	      m_legatoQuantizer, m_properties),
    m_topBarButtons(0),
    m_bottomBarButtons(0),
    m_chordNameRuler(0),
    m_tempoRuler(0),
    m_chordNamesVisible(false),
    m_temposVisible(false),
    m_annotationsVisible(false),
//!!!    m_tupletMode(false),
    m_selectDefaultNote(0),
    m_fontCombo(0),
    m_fontSizeSlider(0),
    m_spacingSlider(0),
    m_smoothingSlider(0),
    m_fontSizeActionMenu(0),
    m_progress(0),
    m_inhibitRefresh(true),
    m_documentDestroyed(false),
    m_ok(false)
{
    assert(segments.size() > 0);
    NOTATION_DEBUG << "NotationView print ctor" << endl;


    // Initialise the display-related defaults that will be needed
    // by both the actions and the font toolbar

    m_config->setGroup("Notation Options");

    m_fontName = qstrtostr(m_config->readEntry
			   ("notefont",
			    strtoqstr(NotePixmapFactory::getDefaultFont())));

    m_fontSize = m_config->readUnsignedNumEntry
	((segments.size() > 1 ? "multistaffnotesize" : "singlestaffnotesize"),
	 NotePixmapFactory::getDefaultSize(m_fontName));

    int defaultSpacing = m_config->readNumEntry("spacing", 100);
    m_hlayout.setSpacing(defaultSpacing);

    Note::Type defaultSmoothingType = 
	m_config->readNumEntry("smoothing", Note::Shortest);
    timeT defaultSmoothing = Note(defaultSmoothingType).getDuration();
    for (int type = Note::Shortest; type <= Note::Longest; ++type) {
	m_legatoDurations.push_back((int)(Note(type).getDuration()));
    }
    Rosegarden::Quantizer q(Rosegarden::Quantizer::RawEventData,
			    getViewLocalPropertyPrefix() + "Q",
			    Rosegarden::Quantizer::LegatoQuantize,
			    defaultSmoothing);
    *m_legatoQuantizer = q;
    
    setBackgroundMode(PaletteBase);

    QPaintDeviceMetrics pdm(printer);

    QCanvas *tCanvas = new QCanvas(this);
    RG_DEBUG << "Print area size : "
             << pdm.width() << ", " << pdm.height()
             << " - printer resolution : " << printer->resolution() << "\n";

    tCanvas->resize(pdm.width(), pdm.height());
    
    setCanvasView(new NotationCanvasView(*this, m_horizontalScrollBar,
                                         tCanvas, getCentralFrame()));

    for (unsigned int i = 0; i < segments.size(); ++i) {
        m_staffs.push_back(new NotationStaff(canvas(), segments[i], 0, // snap
                                             i, this, false, getPageWidth(),
                                             m_fontName, m_fontSize));
    }

    positionStaffs();
    m_currentStaff = 0;
    m_staffs[0]->setCurrent(true);

    setPageMode(true);

    try {
	bool layoutApplied = applyLayout();
	if (!layoutApplied) {
	    KMessageBox::sorry(0, i18n("Couldn't apply score layout"));
	} else {
	    for (unsigned int i = 0; i < m_staffs.size(); ++i) {
		
		m_staffs[i]->renderAllElements();
		m_staffs[i]->positionAllElements();
		m_staffs[i]->getSegment().getRefreshStatus
		    (m_segmentsRefreshStatusIds[i]).setNeedsRefresh(false);
		
		canvas()->update();
	    }
	}
	m_ok = true;
    } catch (std::string s) {
	if (s != "Action cancelled") {
	    throw;
	}
	// when cancelled, m_ok is false -- checked by calling method
    }
}


NotationView::~NotationView()
{
    NOTATION_DEBUG << "-> ~NotationView()\n";

    slotSaveOptions();

    if (m_documentDestroyed) return;

    delete m_currentEventSelection;
    m_currentEventSelection = 0;

    for (unsigned int i = 0; i < m_staffs.size(); ++i) {
	for (Segment::iterator j = m_staffs[i]->getSegment().begin();
	     j != m_staffs[i]->getSegment().end(); ++j) {
	    removeViewLocalProperties(*j);
	}
	delete m_staffs[i]; // this will erase all "notes" canvas items
    }

    // Delete remaining canvas items.
    QCanvasItemList allItems = canvas()->allItems();
    QCanvasItemList::Iterator it;

    for (it = allItems.begin(); it != allItems.end(); ++it) delete *it;
    // delete canvas();

    Rosegarden::Profiles::getInstance()->dump();

    NOTATION_DEBUG << "<- ~NotationView()\n";
}
    
void
NotationView::removeViewLocalProperties(Rosegarden::Event *e)
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
    Rosegarden::TrackId minTrack = 0, maxTrack = 0;
    bool haveMinTrack = false;
    typedef std::map<Rosegarden::TrackId, int> TrackIntMap;
    TrackIntMap trackHeights;
    TrackIntMap trackCoords;

    for (unsigned int i = 0; i < m_staffs.size(); ++i) {

	int height = m_staffs[i]->getHeightOfRow();
	Rosegarden::TrackId track = m_staffs[i]->getSegment().getTrack();

	TrackIntMap::iterator hi = trackHeights.find(track);
	if (hi == trackHeights.end()) {
	    trackHeights.insert(TrackIntMap::value_type(track, height));
	} else if (height > hi->second) {
	    hi->second = height;
	}
	
	if (track < minTrack || !haveMinTrack) {
	    minTrack = track;
	    haveMinTrack = true;
	}
	if (track > maxTrack) {
	    maxTrack = track;
	}
    }

    int accumulatedHeight = 0;
    for (Rosegarden::TrackId i = minTrack; i <= maxTrack; ++i) {
	TrackIntMap::iterator hi = trackHeights.find(i);
	if (hi != trackHeights.end()) {
	    trackCoords[i] = accumulatedHeight;
	    accumulatedHeight += hi->second;
	}
    }

    for (unsigned int i = 0; i < m_staffs.size(); ++i) {
	Rosegarden::TrackId track = m_staffs[i]->getSegment().getTrack();
        m_staffs[i]->setRowSpacing(accumulatedHeight + trackHeights[track] / 7);
        if (track < maxTrack) {
            m_staffs[i]->setConnectingLineLength(trackHeights[track]);
        }
        
        m_staffs[i]->setX(20);
	m_staffs[i]->setY(trackCoords[track]);
    }
}    

void NotationView::slotSaveOptions()
{
    m_config->setGroup("Notation Options");

    m_config->writeEntry("Show Chord Name Ruler", m_chordNamesVisible);
    m_config->writeEntry("Show Tempo Ruler", m_temposVisible);
    m_config->writeEntry("Show Annotations", m_annotationsVisible);
}

void NotationView::readOptions()
{
    EditView::readOptions();

    getToggleAction("show_notes_toolbar")      ->setChecked(!toolBar("notesToolBar")      ->isHidden());
    getToggleAction("show_rests_toolbar")      ->setChecked(!toolBar("restsToolBar")      ->isHidden());
    getToggleAction("show_clefs_toolbar")      ->setChecked(!toolBar("clefsToolBar")      ->isHidden());
    getToggleAction("show_font_toolbar")       ->setChecked(!toolBar("fontToolBar")       ->isHidden());
    getToggleAction("show_transport_toolbar")  ->setChecked(!toolBar("transportToolBar")  ->isHidden());
    getToggleAction("show_accidentals_toolbar")->setChecked(!toolBar("accidentalsToolBar")->isHidden());

    m_config->setGroup("Notation Options");

    bool opt;

    opt = m_config->readBoolEntry("Show Chord Name Ruler", true);
    if (opt) m_chordNameRuler->show();
    else m_chordNameRuler->hide();
    m_chordNamesVisible = opt;
    getToggleAction("label_chords")->setChecked(opt);

    opt = m_config->readBoolEntry("Show Tempo Ruler", false);
    if (opt) m_tempoRuler->show();
    else m_tempoRuler->hide();
    m_temposVisible = opt;
    getToggleAction("display_tempo_changes")->setChecked(opt);

    opt = m_config->readBoolEntry("Show Annotations", true);
    m_annotationsVisible = opt;
    getToggleAction("show_annotations")->setChecked(opt);

}

void NotationView::setupActions()
{
    EditView::setupActions("notation.rc");

    KRadioAction* noteAction = 0;
    
    // View menu stuff
    
    KActionMenu *fontActionMenu =
	new KActionMenu(i18n("Note &Font"), this, "note_font_actionmenu");

    std::set<std::string> fs(NotePixmapFactory::getAvailableFontNames());
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
	new KActionMenu(i18n("Spa&cing"), this, "stretch_actionmenu");

    int defaultSpacing = m_hlayout.getSpacing();
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

    
    KActionMenu *smoothingActionMenu =
	new KActionMenu(i18n("S&moothing"), this, "smoothing_actionmenu");

    timeT defaultSmoothing = m_legatoQuantizer->getUnit();
    NotePixmapFactory npf;

    for (std::vector<int>::iterator i = m_legatoDurations.begin();
	 i != m_legatoDurations.end(); ++i) {

	QPixmap pmap = 0;
	QString label = 0;

	Note nearestNote = Note::getNearestNote(*i);
	if (nearestNote.getDuration() == *i) {
	    std::string noteName = nearestNote.getReferenceName(); 
	    noteName = "menu-" + noteName;
	    pmap = npf.makeToolbarPixmap(strtoqstr(noteName));
	    label = strtoqstr(nearestNote.getEnglishName());
	} else {
	    label = QString("%1").arg(*i);
	}
	
	KToggleAction *smoothingAction =
	    new KToggleAction
	    (label, QIconSet(pmap), 0, this,
	     SLOT(slotChangeLegatoFromAction()),
	     actionCollection(), QString("smoothing_%1").arg(*i));

	smoothingAction->setExclusiveGroup("smoothing");
	smoothingAction->setChecked(*i == defaultSmoothing);
	smoothingActionMenu->insert(smoothingAction);
    }

    actionCollection()->insert(smoothingActionMenu);



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


    const char *notePitchNames[] = {
	"Do", "Re", "Mi", "Fa", "So", "La", "Ti"
    };
    const Key notePitchKeys[3][7] = {
	{
	    Key_A, Key_S, Key_D, Key_F, Key_J, Key_K, Key_L,
	},
	{
	    Key_Q, Key_W, Key_E, Key_R, Key_U, Key_I, Key_O,
	},
	{
	    Key_Z, Key_X, Key_C, Key_V, Key_B, Key_N, Key_M,
	},
    };

    KActionMenu *insertNoteActionMenu =
	new KActionMenu(i18n("&Insert Note"), this, "insert_note_actionmenu");

    for (int octave = 0; octave <= 2; ++octave) {

	KActionMenu *menu = insertNoteActionMenu;
	if (octave == 1) {
	    menu = new KActionMenu(i18n("&Upper Octave"), this,
				   "insert_note_actionmenu_upper_octave");
	    insertNoteActionMenu->insert(new KActionSeparator(this));
	    insertNoteActionMenu->insert(menu);
	} else if (octave == 2) {
	    menu = new KActionMenu(i18n("&Lower Octave"), this,
				   "insert_note_actionmenu_lower_octave");
	    insertNoteActionMenu->insert(menu);
	}

	for (unsigned int i = 0; i < 7; ++i) {

	    KAction *insertNoteAction = 0;

	    QString octaveSuffix;
	    if (octave == 1) octaveSuffix = "_high";
	    else if (octave == 2) octaveSuffix = "_low";

	    // do and fa lack a flat

	    if (i != 0 && i != 3) {
      
		insertNoteAction =
		    new KAction
		    (i18n(QString(notePitchNames[i]) + " flat"),
		     CTRL + SHIFT + notePitchKeys[octave][i],
		     this, SLOT(slotInsertNoteFromAction()), actionCollection(),
		     QString("insert_%1_flat%2").arg(i).arg(octaveSuffix));

		menu->insert(insertNoteAction);
	    }

	    insertNoteAction =
		new KAction
		(i18n(QString(notePitchNames[i])),
		 notePitchKeys[octave][i],
		 this, SLOT(slotInsertNoteFromAction()), actionCollection(),
		 QString("insert_%1%2").arg(i).arg(octaveSuffix));

	    menu->insert(insertNoteAction);

	    // and mi and ti lack a sharp

	    if (i != 2 && i != 6) {

		insertNoteAction =
		    new KAction
		    (i18n(QString(notePitchNames[i]) + " sharp"),
		     SHIFT + notePitchKeys[octave][i],
		     this, SLOT(slotInsertNoteFromAction()), actionCollection(),
		     QString("insert_%1_sharp%2").arg(i).arg(octaveSuffix));

		menu->insert(insertNoteAction);
	    }

	    if (i < 6) menu->insert(new KActionSeparator(this));
	}
    }

    actionCollection()->insert(insertNoteActionMenu);


/*!!!    
    KRadioAction *insertChordMode = new KRadioAction
	(i18n("&Chord Insert Mode"), Key_G, this, SLOT(slotInsertChordMode()),
	 actionCollection(), "insert_chord_mode");
    insertChordMode->setExclusiveGroup("insertMode");

    KRadioAction *insertMelodyMode = new KRadioAction
	(i18n("&Melody Insert Mode"), Key_H, this, SLOT(slotInsertMelodyMode()),
	 actionCollection(), "insert_melody_mode");
    insertMelodyMode->setExclusiveGroup("insertMode");
    insertMelodyMode->setChecked(true);
    m_insertChordMode = false;
*/

    (new KToggleAction(i18n("C&hord Insert Mode"), Key_H, this, 0,
		      actionCollection(), "chord_mode"))->
	setChecked(false);


    // setup Notes menu & toolbar
    QIconSet icon;
 
    for (NoteActionDataMap::Iterator actionDataIter = m_noteActionDataMap->begin();
	 actionDataIter != m_noteActionDataMap->end();
	 ++actionDataIter) {

        NoteActionData noteActionData = *actionDataIter;
        
        icon = QIconSet
	    (m_toolbarNotePixmapFactory.makeToolbarPixmap
	     (noteActionData.pixmapName));
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
    
    //
    // Accidentals
    //
    static const char* actionsAccidental[][4] = 
        {
            { "No accidental",  "1slotNoAccidental()",  "no_accidental",           "accidental-none" },
            { "Sharp",          "1slotSharp()",         "sharp_accidental",        "accidental-sharp" },
            { "Flat",           "1slotFlat()",          "flat_accidental",         "accidental-flat" },
            { "Natural",        "1slotNatural()",       "natural_accidental",      "accidental-natural" },
            { "Double sharp",   "1slotDoubleSharp()",   "double_sharp_accidental", "accidental-doublesharp" },
            { "Double flat",    "1slotDoubleFlat()",    "double_flat_accidental",  "accidental-doubleflat" }
        };

    for (unsigned int i = 0;
	 i < sizeof(actionsAccidental)/sizeof(actionsAccidental[0]); ++i) {

        icon = QIconSet(m_toolbarNotePixmapFactory.makeToolbarPixmap
                        (actionsAccidental[i][3]));
        noteAction = new KRadioAction(i18n(actionsAccidental[i][0]), icon, 0, this,
                                      actionsAccidental[i][1],
                                      actionCollection(), actionsAccidental[i][2]);
        noteAction->setExclusiveGroup("accidentals");
    }
    

    //
    // Clefs
    //

    // Treble
    icon = QIconSet(m_toolbarNotePixmapFactory.makeToolbarPixmap("clef-treble"));
    noteAction = new KRadioAction(i18n("&Treble Clef"), icon, 0, this,
                                  SLOT(slotTrebleClef()),
                                  actionCollection(), "treble_clef");
    noteAction->setExclusiveGroup("notes");

    // Tenor
    icon = QIconSet(m_toolbarNotePixmapFactory.makeToolbarPixmap("clef-tenor"));
    noteAction = new KRadioAction(i18n("Te&nor Clef"), icon, 0, this,
                                  SLOT(slotTenorClef()),
                                  actionCollection(), "tenor_clef");
    noteAction->setExclusiveGroup("notes");

    // Alto
    icon = QIconSet(m_toolbarNotePixmapFactory.makeToolbarPixmap("clef-alto"));
    noteAction = new KRadioAction(i18n("&Alto Clef"), icon, 0, this,
                                  SLOT(slotAltoClef()),
                                  actionCollection(), "alto_clef");
    noteAction->setExclusiveGroup("notes");

    // Bass
    icon = QIconSet(m_toolbarNotePixmapFactory.makeToolbarPixmap("clef-bass"));
    noteAction = new KRadioAction(i18n("&Bass Clef"), icon, 0, this,
                                  SLOT(slotBassClef()),
                                  actionCollection(), "bass_clef");
    noteAction->setExclusiveGroup("notes");


    icon = QIconSet(m_toolbarNotePixmapFactory.makeToolbarPixmap("text"));
    noteAction = new KRadioAction(i18n("&Text"), icon, 0, this,
                                  SLOT(slotText()),
                                  actionCollection(), "text");
    noteAction->setExclusiveGroup("notes");


    //
    // Edition tools (eraser, selector...)
    //
    noteAction = new KRadioAction(i18n("&Erase"), "eraser", 0,
                                  this, SLOT(slotEraseSelected()),
                                  actionCollection(), "erase");
    noteAction->setExclusiveGroup("notes");

    icon = QIconSet(m_toolbarNotePixmapFactory.makeToolbarPixmap("select"));
    noteAction = new KRadioAction(i18n("&Select"), icon, 0,
                                  this, SLOT(slotSelectSelected()),
                                  actionCollection(), "select");
    noteAction->setExclusiveGroup("notes");
    

    // File menu
    KStdAction::close (this, SLOT(slotCloseWindow()),      actionCollection());

    // Edit menu
    new KAction(i18n("Select from Sta&rt"), 0, this,
		SLOT(slotEditSelectFromStart()), actionCollection(),
		"select_from_start");

    new KAction(i18n("Select to &End"), 0, this,
		SLOT(slotEditSelectToEnd()), actionCollection(),
		"select_to_end");

    new KAction(i18n("Select Whole St&aff"), 0, this,
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

    KStdAction::cut     (this, SLOT(slotEditCut()),        actionCollection());
    KStdAction::copy    (this, SLOT(slotEditCopy()),       actionCollection());
    KStdAction::paste   (this, SLOT(slotEditPaste()),      actionCollection());

    // View menu
    KRadioAction *linearModeAction = new KRadioAction
        (i18n("&Linear Layout"), 0, this, SLOT(slotLinearMode()),
         actionCollection(), "linear_mode");
    linearModeAction->setExclusiveGroup("layoutMode");
    linearModeAction->setChecked(true);

    KRadioAction *pageModeAction = new KRadioAction
        (i18n("&Page Layout"), 0, this, SLOT(slotPageMode()),
         actionCollection(), "page_mode");
    pageModeAction->setExclusiveGroup("layoutMode");

    (new KToggleAction
     (i18n("Show &Chord Name Ruler"), 0, this, SLOT(slotLabelChords()),
      actionCollection(), "label_chords"))->setChecked(true);

    (new KToggleAction
     (i18n("Show &Annotations"), 0, this, SLOT(slotShowAnnotations()),
      actionCollection(), "show_annotations"))->setChecked(true);

    new KToggleAction
	(i18n("Show &Tempo Ruler"), 0, this, SLOT(slotShowTempos()),
	 actionCollection(), "display_tempo_changes");

    new KAction(i18n("Open L&yric Editor"), 0, this, SLOT(slotEditLyrics()),
		actionCollection(), "lyric_editor");

    // setup Group menu
    new KAction(i18n(GroupMenuBeamCommand::getGlobalName()), 0, this,
                SLOT(slotGroupBeam()), actionCollection(), "beam");

    new KAction(i18n(GroupMenuAutoBeamCommand::getGlobalName()), 0, this,
                SLOT(slotGroupAutoBeam()), actionCollection(), "auto_beam");

    new KAction(i18n(GroupMenuBreakCommand::getGlobalName()), 0, this,
                SLOT(slotGroupBreak()), actionCollection(), "break_group");

    new KAction(i18n(GroupMenuTupletCommand::getGlobalName(true)), 0, this,
		SLOT(slotGroupSimpleTuplet()), actionCollection(), "simple_tuplet");

    new KAction(i18n(GroupMenuTupletCommand::getGlobalName(false)), 0, this,
		SLOT(slotGroupGeneralTuplet()), actionCollection(), "tuplet");

    (new KToggleAction(i18n("Tri&plet Insert Mode"), Key_G, this,
		      0, /*!!! SLOT(slotToggleTriplet()),*/ actionCollection(), "triplet_mode"))->
	setChecked(false);

    new KAction(i18n(GroupMenuGraceCommand::getGlobalName()), 0, this,
		SLOT(slotGroupGrace()), actionCollection(), "grace");

    new KAction(i18n(GroupMenuUnGraceCommand::getGlobalName()), 0, this,
		SLOT(slotGroupUnGrace()), actionCollection(), "ungrace");

    new KAction(i18n(GroupMenuAddIndicationCommand::getGlobalName
                (Rosegarden::Indication::Slur)), 0, this,
                SLOT(slotGroupSlur()), actionCollection(), "slur");

    new KAction(i18n(GroupMenuAddIndicationCommand::getGlobalName
                (Rosegarden::Indication::Crescendo)), 0, this,
                SLOT(slotGroupCrescendo()), actionCollection(), "crescendo");

    new KAction(i18n(GroupMenuAddIndicationCommand::getGlobalName
                (Rosegarden::Indication::Decrescendo)), 0, this,
                SLOT(slotGroupDecrescendo()), actionCollection(), "decrescendo");

    // setup Transforms menu
    new KAction(i18n(TransformsMenuNormalizeRestsCommand::getGlobalName()), 0, this,
                SLOT(slotTransformsNormalizeRests()), actionCollection(),
                "normalize_rests");

    new KAction(i18n(TransformsMenuCollapseRestsCommand::getGlobalName()), 0, this,
                SLOT(slotTransformsCollapseRests()), actionCollection(),
                "collapse_rests_aggressively");

    new KAction(i18n(TransformsMenuCollapseNotesCommand::getGlobalName()), 0, this,
                SLOT(slotTransformsCollapseNotes()), actionCollection(),
                "collapse_notes");

    new KAction(i18n(TransformsMenuTieNotesCommand::getGlobalName()), 0, this,
                SLOT(slotTransformsTieNotes()), actionCollection(),
                "tie_notes");

    new KAction(i18n(TransformsMenuUntieNotesCommand::getGlobalName()), 0, this,
                SLOT(slotTransformsUntieNotes()), actionCollection(),
                "untie_notes");

    new KAction(i18n(TransformsMenuChangeStemsCommand::getGlobalName(true)), 0,Key_Up + CTRL, this,
                SLOT(slotTransformsStemsUp()), actionCollection(),
                "stems_up");

    new KAction(i18n(TransformsMenuChangeStemsCommand::getGlobalName(false)), 0, Key_Down + CTRL, this,
                SLOT(slotTransformsStemsDown()), actionCollection(),
                "stems_down");

    new KAction(i18n(TransformsMenuRestoreStemsCommand::getGlobalName()), 0, this,
                SLOT(slotTransformsRestoreStems()), actionCollection(),
                "restore_stems");

    new KAction(i18n(TransformsMenuTransposeCommand::getGlobalName(1)), 0,
		Key_Up, this,
                SLOT(slotTransformsTransposeUp()), actionCollection(),
                "transpose_up");

    new KAction(i18n(TransformsMenuTransposeCommand::getGlobalName(12)), 0,
		Key_Up + CTRL, this,
                SLOT(slotTransformsTransposeUpOctave()), actionCollection(),
                "transpose_up_octave");

    new KAction(i18n(TransformsMenuTransposeCommand::getGlobalName(-1)), 0,
		Key_Down, this,
                SLOT(slotTransformsTransposeDown()), actionCollection(),
                "transpose_down");

    new KAction(i18n(TransformsMenuTransposeCommand::getGlobalName(-12)), 0,
		Key_Down + CTRL, this,
                SLOT(slotTransformsTransposeDownOctave()), actionCollection(),
                "transpose_down_octave");

    new KAction(i18n(TransformsMenuTransposeCommand::getGlobalName(0)), 0, this,
                SLOT(slotTransformsTranspose()), actionCollection(),
                "general_transpose");

    new KAction(i18n(EventQuantizeCommand::getGlobalName()), 0, this,
                SLOT(slotTransformsQuantize()), actionCollection(),
                "quantize");

    new KAction(i18n(TransformsMenuFixSmoothingCommand::getGlobalName()), 0,
		this, SLOT(slotTransformsFixSmoothing()), actionCollection(),
                "fix_smoothing");

    new KAction(i18n("&Dump selected events to stderr"), 0, this,
		SLOT(slotDebugDump()), actionCollection(), "debug_dump");

    for (MarkActionDataMap::Iterator i = m_markActionDataMap->begin();
	 i != m_markActionDataMap->end(); ++i) {

        const MarkActionData &markActionData = *i;
        
        icon = QIconSet
	    (m_toolbarNotePixmapFactory.makeToolbarPixmap
	     (markActionData.pixmapName));

	new KAction(i18n(markActionData.title),
		    icon,
		    markActionData.keycode,
		    this,
		    SLOT(slotAddMark()),
		    actionCollection(),
		    markActionData.actionName);
    }

    new KAction(i18n(MarksMenuAddTextMarkCommand::getGlobalName()), 0, this,
                SLOT(slotMarksAddTextMark()), actionCollection(),
                "add_text_mark");

    new KAction(i18n(MarksMenuRemoveMarksCommand::getGlobalName()), 0, this,
                SLOT(slotMarksRemoveMarks()), actionCollection(),
                "remove_marks");

    static const char *slashTitles[] = {
	"&None", "&1", "&2", "&3", "&4", "&5"
    };
    for (int i = 0; i <= 5; ++i) {
	new KAction(i18n(slashTitles[i]), 0, this,
		    SLOT(slotAddSlashes()), actionCollection(),
		    QString("slashes_%1").arg(i));
    }

    new KAction(i18n(ClefInsertionCommand::getGlobalName()), 0, this,
                SLOT(slotEditAddClef()), actionCollection(),
                "add_clef");

    new KAction(i18n(AddTempoChangeCommand::getGlobalName()), 0, this,
                SLOT(slotEditAddTempo()), actionCollection(),
                "add_tempo");

    new KAction(i18n(AddTimeSignatureCommand::getGlobalName()), 0, this,
                SLOT(slotEditAddTimeSignature()), actionCollection(),
                "add_time_signature");

    new KAction(i18n(KeyInsertionCommand::getGlobalName()), 0, this,
                SLOT(slotEditAddKeySignature()), actionCollection(),
                "add_key_signature");

    // setup Settings menu
    static const char* actionsToolbars[][4] = 
        {
            { "Show &Notes Toolbar",  "1slotToggleNotesToolBar()",  "show_notes_toolbar",                    "palette-notes" },
            { "Show &Rests Toolbar",  "1slotToggleRestsToolBar()",  "show_rests_toolbar",                    "palette-rests" },
            { "Show &Accidentals Toolbar",   "1slotToggleAccidentalsToolBar()",  "show_accidentals_toolbar", "palette-accidentals" },
            { "Show Cle&fs Toolbar",         "1slotToggleClefsToolBar()",        "show_clefs_toolbar",       "palette-clefs" },
            { "Show &Layout Toolbar",         "1slotToggleFontToolBar()",        "show_font_toolbar",       "palette-font" },
            { "Show Trans&port Toolbar",      "1slotToggleTransportToolBar()",        "show_transport_toolbar",       "palette-transport" }
        };

    for (unsigned int i = 0;
	 i < sizeof(actionsToolbars)/sizeof(actionsToolbars[0]); ++i) {

        icon = QIconSet(m_toolbarNotePixmapFactory.makeToolbarPixmap(actionsToolbars[i][3]));

        KToggleAction* toolbarAction = new KToggleAction
            (i18n(actionsToolbars[i][0]), icon, 0,
             this, actionsToolbars[i][1],
             actionCollection(), actionsToolbars[i][2]);

        if (i == 3)
            toolbarAction->setChecked(false);
        else
            toolbarAction->setChecked(true);
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

    new KAction(i18n("Cursor to St&art"), 0, Key_A + CTRL, this,
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

    icon = QIconSet(m_toolbarNotePixmapFactory.makeToolbarPixmap
		    ("transport-cursor-to-pointer"));
    new KAction(i18n("Cursor to &Playback Pointer"), icon, 0, this,
		SLOT(slotJumpCursorToPlayback()), actionCollection(),
		"cursor_to_playback_pointer");

    icon = QIconSet(m_toolbarNotePixmapFactory.makeToolbarPixmap
		    ("transport-play"));
    new KAction(i18n("&Play"), icon, Key_Enter, this,
		SIGNAL(play()), actionCollection(), "play");

    icon = QIconSet(m_toolbarNotePixmapFactory.makeToolbarPixmap
		    ("transport-stop"));
    new KAction(i18n("&Stop"), icon, Key_Insert, this,
		SIGNAL(stop()), actionCollection(), "stop");

    icon = QIconSet(m_toolbarNotePixmapFactory.makeToolbarPixmap
		    ("transport-rewind"));
    new KAction(i18n("Re&wind"), icon, Key_End, this,
		SIGNAL(rewindPlayback()), actionCollection(),
		"playback_pointer_back_bar");

    icon = QIconSet(m_toolbarNotePixmapFactory.makeToolbarPixmap
		    ("transport-ffwd"));
    new KAction(i18n("&Fast Forward"), icon, Key_PageDown, this,
		SIGNAL(fastForwardPlayback()), actionCollection(),
		"playback_pointer_forward_bar");

    icon = QIconSet(m_toolbarNotePixmapFactory.makeToolbarPixmap
		    ("transport-rewind-end"));
    new KAction(i18n("Rewind to &Beginning"), icon, 0, this,
		SIGNAL(rewindPlaybackToBeginning()), actionCollection(),
		"playback_pointer_start");

    icon = QIconSet(m_toolbarNotePixmapFactory.makeToolbarPixmap
		    ("transport-ffwd-end"));
    new KAction(i18n("Fast Forward to &End"), icon, 0, this,
		SIGNAL(fastForwardPlaybackToEnd()), actionCollection(),
		"playback_pointer_end");

    icon = QIconSet(m_toolbarNotePixmapFactory.makeToolbarPixmap
		    ("transport-pointer-to-cursor"));
    new KAction(i18n("Playback Pointer to &Cursor"), icon, 0, this,
		SLOT(slotJumpPlaybackToCursor()), actionCollection(),
		"playback_pointer_to_cursor");

    new KAction(i18n("Clear Selection"), Key_Escape, this,
		SLOT(slotClearSelection()), actionCollection(),
		"clear_selection");

    createGUI(getRCFileName());

    // transport toolbar is hidden by default
    //
    toolBar("transportToolBar")->hide();
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
	
	std::vector<int> sizes = NotePixmapFactory::getAvailableSizes
	    (oldFontName);
	
	for (unsigned int i = 0; i < sizes.size(); ++i) {
	    KAction *action =
		actionCollection()->action
		(QString("note_font_size_%1").arg(sizes[i]));
	    m_fontSizeActionMenu->remove(action);
	    delete action;
	}
    }

    std::vector<int> sizes = NotePixmapFactory::getAvailableSizes(m_fontName);

    for (unsigned int i = 0; i < sizes.size(); ++i) {

	KToggleAction *sizeAction = 
	    new KToggleAction
	    (i18n("%1 pixels").arg(sizes[i]), 0, this,
	     SLOT(slotChangeFontSizeFromAction()),
	     actionCollection(), QString("note_font_size_%1").arg(sizes[i]));

	sizeAction->setChecked(sizes[i] == m_fontSize);
	m_fontSizeActionMenu->insert(sizeAction);
    }
}


NotationStaff *
NotationView::getStaff(const Segment &segment)
{
    for (unsigned int i = 0; i < m_staffs.size(); ++i) {
        if (&(m_staffs[i]->getSegment()) == &segment) return m_staffs[i];
    }
    return 0;
}

void NotationView::initFontToolbar()
{
    KToolBar *fontToolbar = toolBar("fontToolBar");
    
    if (!fontToolbar) {
        NOTATION_DEBUG
            << "NotationView::initFontToolbar() : font toolbar not found\n";
        return;
    }

    new QLabel(i18n("  Font:  "), fontToolbar);

    m_fontCombo = new QComboBox(fontToolbar);
    m_fontCombo->setEditable(false);

    std::set<std::string> fs(NotePixmapFactory::getAvailableFontNames());
    std::vector<std::string> f(fs.begin(), fs.end());
    std::sort(f.begin(), f.end());

    bool foundFont = false;

    for (std::vector<std::string>::iterator i = f.begin(); i != f.end(); ++i) {

	QString fontQName(strtoqstr(*i));

        m_fontCombo->insertItem(fontQName);
        if (*i == m_fontName) {
            m_fontCombo->setCurrentItem(m_fontCombo->count() - 1);
	    foundFont = true;
        }
    }

    if (!foundFont) {
	KMessageBox::sorry
	    (this, QString(i18n("Unknown font \"%1\", using default")).arg
	     (strtoqstr(m_fontName)));
	m_fontName = NotePixmapFactory::getDefaultFont();
    }
    
    connect(m_fontCombo, SIGNAL(activated(const QString &)),
            this,        SLOT(slotChangeFont(const QString &)));

    new QLabel(i18n("  Size:  "), fontToolbar);

    std::vector<int> sizes = NotePixmapFactory::getAvailableSizes(m_fontName);
    m_fontSizeSlider = new ZoomSlider<int>
        (sizes, m_fontSize, QSlider::Horizontal, fontToolbar);
    connect(m_fontSizeSlider, SIGNAL(valueChanged(int)),
            this, SLOT(slotChangeFontSizeFromIndex(int)));

    new QLabel(i18n("  Spacing:  "), fontToolbar);

    int defaultSpacing = m_hlayout.getSpacing();
    std::vector<int> spacings = NotationHLayout::getAvailableSpacings();
    m_spacingSlider = new ZoomSlider<int>
        (spacings, defaultSpacing, QSlider::Horizontal, fontToolbar);
    connect(m_spacingSlider, SIGNAL(valueChanged(int)),
            this, SLOT(slotChangeSpacingFromIndex(int)));

    new QLabel(i18n("  Smoothing:  "), fontToolbar);

    timeT defaultSmoothing = m_legatoQuantizer->getUnit();
    m_smoothingSlider = new ZoomSlider<int>
        (m_legatoDurations, defaultSmoothing,
	 QSlider::Horizontal, fontToolbar);
    connect(m_smoothingSlider, SIGNAL(valueChanged(int)),
            this, SLOT(slotChangeLegatoFromIndex(int)));
}

void NotationView::initStatusBar()
{
    KStatusBar* sb = statusBar();

    m_currentNotePixmap       = new QLabel(sb);
    m_hoveredOverNoteName     = new QLabel(sb);
    m_hoveredOverAbsoluteTime = new QLabel(sb);

    m_currentNotePixmap->setMinimumWidth(20);
    m_hoveredOverNoteName->setMinimumWidth(32);
    m_hoveredOverAbsoluteTime->setMinimumWidth(160);

    sb->addWidget(m_hoveredOverAbsoluteTime);
    sb->addWidget(m_hoveredOverNoteName);
    sb->addWidget(m_currentNotePixmap);

    sb->insertItem(KTmpStatusMsg::getDefaultMsg(),
                   KTmpStatusMsg::getDefaultId(), 1);
    sb->setItemAlignment(KTmpStatusMsg::getDefaultId(), 
                         AlignLeft | AlignVCenter);

    m_selectionCounter = new QLabel(sb);
    sb->addWidget(m_selectionCounter);

    m_progressBar = new RosegardenProgressBar(100, true, sb);
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
NotationView::setPageMode(bool pageMode)
{
    if (pageMode) {
	if (m_topBarButtons) m_topBarButtons->hide();
	if (m_bottomBarButtons) m_bottomBarButtons->hide();
	if (m_chordNameRuler) m_chordNameRuler->hide();
	if (m_tempoRuler) m_tempoRuler->hide();
    } else {
	if (m_topBarButtons) m_topBarButtons->show();
	if (m_bottomBarButtons) m_bottomBarButtons->show();
	if (m_chordNameRuler && m_chordNamesVisible) m_chordNameRuler->show();
	if (m_tempoRuler && m_temposVisible) m_tempoRuler->show();
    }

    int pageWidth = getCanvasView()->canvas()->width();

    m_hlayout.setPageMode(pageMode);
    m_hlayout.setPageWidth(pageWidth);

    RG_DEBUG << "Page mode : page width = " << pageWidth << endl;
    
    for (unsigned int i = 0; i < m_staffs.size(); ++i) {
        m_staffs[i]->setPageMode(pageMode);
        m_staffs[i]->setPageWidth(pageWidth);
    }

    bool layoutApplied = applyLayout();
    if (!layoutApplied) KMessageBox::sorry(0, "Couldn't apply layout");
    else {
        for (unsigned int i = 0; i < m_staffs.size(); ++i) {
//            m_staffs[i]->renderAllElements();
            m_staffs[i]->positionAllElements();
        }
    }

    updateView();
}   

int
NotationView::getPageWidth()
{
    if (m_hlayout.isPageMode() && getCanvasView() && getCanvasView()->canvas())
        return getCanvasView()->canvas()->width();

    return width() - 50;
}


void
NotationView::paintEvent(QPaintEvent *e)
{
    if (m_hlayout.isPageMode()) {
	int diff = int(getPageWidth() - m_hlayout.getPageWidth());
	if (diff > -10 && diff < 10) {
	    m_hlayout.setPageWidth(getPageWidth());
	    refreshSegment(0, 0, 0);
	}
    }
    EditView::paintEvent(e);
}


bool NotationView::applyLayout(int staffNo, timeT startTime, timeT endTime)
{
    if (m_progress) {
	m_progress->setOperationName(qstrtostr(i18n("Laying out score...")));
	m_progress->processEvents();
	m_hlayout.setProgressReporter(m_progress);
	m_hlayout.setStaffCount(m_staffs.size());
    }

    START_TIMING;
    unsigned int i;

    for (i = 0; i < m_staffs.size(); ++i) {

        if (staffNo >= 0 && (int)i != staffNo) continue;

	if (m_progress) {
	    m_progress->setOperationName
		(qstrtostr(i18n("Laying out staff %1...").arg(i + 1)));
	}

        m_hlayout.resetStaff(*m_staffs[i], startTime, endTime);
        m_vlayout.resetStaff(*m_staffs[i], startTime, endTime);
        m_hlayout.scanStaff(*m_staffs[i], startTime, endTime);
        m_vlayout.scanStaff(*m_staffs[i], startTime, endTime);
    }

    if (m_progress) {
	m_progress->setOperationName(qstrtostr(i18n("Reconciling staffs...")));
    }

    m_hlayout.finishLayout(startTime, endTime);
    m_vlayout.finishLayout(startTime, endTime);
    m_hlayout.setProgressReporter(0);

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
    if (m_topBarButtons) {
	m_topBarButtons->update();
    }
    if (m_bottomBarButtons) {
	m_bottomBarButtons->update();
    }

    PRINT_ELAPSED("NotationView::applyLayout");
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
        (m_toolbarNotePixmapFactory.makeToolbarPixmap(pixmapName));

    emit changeCurrentNote(rest, n);
}

void NotationView::setCurrentSelectedNote(NoteActionData noteAction)
{
    setCurrentSelectedNote(noteAction.pixmapName,
                           noteAction.rest,
                           noteAction.noteType,
                           noteAction.dots);
}


void NotationView::setCurrentSelection(EventSelection* s, bool preview)
{
    if (!m_currentEventSelection && !s) return;
    bool usingProgressBar = false;

    if (!m_progress) {
	m_progress = m_progressBar;
	usingProgressBar = true;
	for (unsigned int i = 0; i < m_staffs.size(); ++i) {
	    m_staffs[i]->setProgressReporter(m_progress);
	}
    }

    EventSelection *oldSelection = m_currentEventSelection;
    m_currentEventSelection = s;

    // positionElements is overkill here, but we hope it's not too
    // much overkill (if that's not a contradiction)

    timeT startA, endA, startB, endB;

    if (oldSelection) {
	startA = oldSelection->getStartTime();
	endA   = oldSelection->getEndTime();
	startB = s ? s->getStartTime() : startA;
	endB   = s ? s->getEndTime()   : endA;
    } else {
	// we know they can't both be null -- first thing we tested above
	startA = startB = s->getStartTime();
	endA   = endB   = s->getEndTime();
    }

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
		&& oldSelection->contains(*i)) continue;
		
	    foundNewEvent = true;

	    long pitch;
	    if (!(*i)->get<Rosegarden::Int>(Rosegarden::BaseProperties::PITCH,
					    pitch)) continue;

	    playNote(s->getSegment(), pitch);
	}

	if (!foundNewEvent) {
	    if (oldSelection &&
		oldSelection->getSegment() == s->getSegment() &&
		oldSelection->getSegmentEvents().size() ==
		s->getSegmentEvents().size()) updateRequired = false;
	}
    }

    if (updateRequired) {

	if ((endA >= startB && endB >= startA) &&
	    (!s || !oldSelection ||
	     oldSelection->getSegment() == s->getSegment())) {
	    
	    // the regions overlap: use their union and just do one reposition
	    Segment &segment(s ? s->getSegment() : oldSelection->getSegment());
	    getStaff(segment)->positionElements(std::min(startA, startB),
						std::max(endA, endB));
	    
	} else {
	    // do two repositions, one for each -- here we know neither is null
	    getStaff(oldSelection->getSegment())->positionElements(startA,
								   endA);
	    getStaff(s->getSegment())->positionElements(startB, endB);
	}
    }

    delete oldSelection;

    if (usingProgressBar) {
	m_progress->done();
	m_progress = 0;
	for (unsigned int i = 0; i < m_staffs.size(); ++i) {
	    m_staffs[i]->setProgressReporter(0);
	}
    }

    int eventsSelected = 0;
    if (s) eventsSelected = s->getSegmentEvents().size();
    if (s) {
	m_selectionCounter->setText
	    (i18n("  %1 event%2 selected ").
	     arg(eventsSelected).arg(eventsSelected == 1 ? "" : "s"));
    } else {
	m_selectionCounter->setText(i18n("  No selection "));
    }
    m_selectionCounter->update();

    //!!! dup with refreshSegment, move to other fn somewhere

#ifdef RGKDE3
    // Clear states first, then enter only those ones that apply
    // (so as to avoid ever clearing one after entering another, in
    // case the two overlap at all)
    stateChanged("have_selection", KXMLGUIClient::StateReverse);
    stateChanged("have_notes_in_selection", KXMLGUIClient::StateReverse);
    stateChanged("have_rests_in_selection", KXMLGUIClient::StateReverse);

    if (s) {

	NOTATION_DEBUG << "NotationView::setCurrentSelection: Have selection; it's " << s << " covering range from " << s->getStartTime() << " to " << s->getEndTime() << " (" << s->getSegmentEvents().size() << " events)" << endl;

	stateChanged("have_selection", KXMLGUIClient::StateNoReverse);
	if (s->contains(Rosegarden::Note::EventType)) {
	    stateChanged("have_notes_in_selection",
			 KXMLGUIClient::StateNoReverse);
	}
	if (s->contains(Rosegarden::Note::EventRestType)) {
	    stateChanged("have_rests_in_selection",
			 KXMLGUIClient::StateNoReverse);
	}
    }
#else

    NOTATION_DEBUG << "Not using KDE3, not setting selection-related states"
		   << endl;
#endif

    updateView();
}

void NotationView::setSingleSelectedEvent(int staffNo, Event *event)
{
    setSingleSelectedEvent(getStaff(staffNo)->getSegment(), event);
}

void NotationView::setSingleSelectedEvent(Segment &segment, Event *event)
{
    EventSelection *selection = new EventSelection(segment);
    selection->addEvent(event);
    setCurrentSelection(selection);
}

void NotationView::playNote(Rosegarden::Segment &s, int pitch)
{
    Rosegarden::Composition &comp = m_document->getComposition();
    Rosegarden::Studio &studio = m_document->getStudio();
    Rosegarden::Track *track = comp.getTrackByIndex(s.getTrack());

    Rosegarden::Instrument *ins =
        studio.getInstrumentById(track->getInstrument());

    // check for null instrument
    //
    if (ins == 0) return;

    // Send out note of half second duration
    //
    Rosegarden::MappedEvent *mE = 
        new Rosegarden::MappedEvent(ins->getId(),
                                    Rosegarden::MappedEvent::MidiNoteOneShot,
                                    pitch,
                                    Rosegarden::MidiMaxValue,
                                    Rosegarden::RealTime(0,0),
                                    Rosegarden::RealTime(0, 500000),
                                    Rosegarden::RealTime(0, 0));

    Rosegarden::StudioControl::sendMappedEvent(mE);
}

void NotationView::showPreviewNote(int staffNo, double layoutX,
				   int pitch, int height,
				   const Rosegarden::Note &note)
{ 
    m_staffs[staffNo]->showPreviewNote(layoutX, height, note);
    playNote(m_staffs[staffNo]->getSegment(), pitch);
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
}


NotationCanvasView* NotationView::getCanvasView()
{
    return dynamic_cast<NotationCanvasView *>(m_canvasView);
}


timeT
NotationView::getInsertionTime()
{
    return m_insertionTime;
}


timeT
NotationView::getInsertionTime(Event *&clefEvt,
			       Event *&keyEvt)
{
    // This fuss is solely to recover the clef and key: we already
    // set m_insertionTime to the right value when we first placed
    // the insert cursor.  We could get clef and key directly from
    // the segment but the staff has a more efficient lookup

    NotationStaff *staff = m_staffs[m_currentStaff];
    double layoutX = staff->getLayoutXOfInsertCursor();
    if (layoutX < 0) layoutX = 0;
    (void)staff->getElementUnderLayoutX(layoutX, clefEvt, keyEvt);

    return m_insertionTime;
}


//////////////////////////////////////////////////////////////////////
//                    Slots
//////////////////////////////////////////////////////////////////////



NotationStaff *
NotationView::getStaffForCanvasY(int y) const
{
    for (unsigned int i = 0; i < m_staffs.size(); ++i) {
        if (m_staffs[i]->containsCanvasY(y)) return m_staffs[i];
    }
    return 0;
}

void NotationView::updateView()
{
    canvas()->update();
}

void NotationView::print(KPrinter* printer)
{
    if (m_staffs.size() == 0) {
        KMessageBox::error(0, "Nothing to print");
        return;
    }
    

    QPaintDeviceMetrics pdm(printer);

    unsigned int pageHeight = pdm.height(),
        staffRowSpacing = m_staffs[0]->getRowSpacing();

    RG_DEBUG << "Page height : " << pageHeight << endl;

    if (staffRowSpacing > pageHeight) {
        KMessageBox::error(0, "Too many staffs - Don't know how to print that yet");
        return;
    }

    unsigned int nbStaffRowsPerPage = pageHeight / staffRowSpacing;
    int printSliceHeight = staffRowSpacing * nbStaffRowsPerPage;

    NotationStaff* lastStaff = m_staffs[getStaffCount() - 1];

    int printY = 0,
        fullHeight = lastStaff->getTotalHeight() + lastStaff->getY();
    
    QPainter printpainter(printer);

    QWMatrix translator;
    
    RG_DEBUG << "Printing total height " << fullHeight
             << ", nbStaffRowsPerPage = " << nbStaffRowsPerPage
             << ", printSliceHeight = " << printSliceHeight
             << endl;

    unsigned int pageNum = 1;

    unsigned int canvasWidth = getCanvasView()->canvas()->width(),
        canvasHeight =  getCanvasView()->canvas()->height();

    // Stuff text items for debugging
//     for (int y = 0; y < canvasHeight; y += 10) {
//         QCanvasText* t = new QCanvasText(QString("y = %1").arg(y),
//                                          getCanvasView()->canvas());
//         t->setY(y);
//         t->show();
//     }

//     getCanvasView()->canvas()->update();
    // end of stuffing

    while (printY < fullHeight) {

        RG_DEBUG << "Printing from " << printY << " to "
                 << printY + printSliceHeight
                 << endl;

        getCanvasView()->canvas()->drawArea(QRect(0, printY,
                                                  canvasWidth, printSliceHeight),
                                            &printpainter);

        printpainter.translate(0, -printSliceHeight);

        printY += printSliceHeight;
        ++pageNum;

        if (printY < fullHeight) printer->newPage();
    }

}

void NotationView::refreshSegment(Segment *segment,
				  timeT startTime, timeT endTime)
{
    START_TIMING;
    Rosegarden::Profiler foo("NotationView::refreshSegment()");

    if (m_inhibitRefresh) return;

    bool usingProgressBar = false;

    if (!m_progress) {
	m_progress = m_progressBar;
	usingProgressBar = true;
	for (unsigned int i = 0; i < m_staffs.size(); ++i) {
	    m_staffs[i]->setProgressReporter(m_progress);
	}
    }

    emit usedSelection();

    if (segment) {
        NotationStaff *staff = getStaff(*segment);
        if (staff) applyLayout(staff->getId(), startTime, endTime);
    } else {
        applyLayout(-1, startTime, endTime);
    }

    for (unsigned int i = 0; i < m_staffs.size(); ++i) {

        Segment *ssegment = &m_staffs[i]->getSegment();
        bool thisStaff = (ssegment == segment || segment == 0);

        NotationElementList *notes = m_staffs[i]->getViewElementList();
        NotationElementList::iterator starti = notes->begin();
        NotationElementList::iterator endi = notes->end();

        timeT barStartTime = ssegment->getStartTime(),
	      barEndTime   = ssegment->getEndTime();

        if (startTime != endTime) {
	    barStartTime = ssegment->getBarStartForTime(startTime);
	    barEndTime = ssegment->getBarEndForTime(endTime);
	    starti = notes->findTime(barStartTime);
	    endi = notes->findTime(barEndTime);
	}

        NOTATION_DEBUG << "NotationView::refreshSegment: "
                             << "start = " << startTime << ", end = " << endTime << ", barStart = " << barStartTime << ", barEnd = " << barEndTime << endl;

        if (thisStaff) {
            m_staffs[i]->renderElements(starti, endi);
        }
        m_staffs[i]->positionElements(barStartTime, barEndTime);
    }

    PRINT_ELAPSED("NotationView::refreshSegment (without update/GC)");

    PixmapArrayGC::deleteAll();
    if (usingProgressBar) {
	m_progress->done();
	m_progress = 0;
	for (unsigned int i = 0; i < m_staffs.size(); ++i) {
	    m_staffs[i]->setProgressReporter(0);
	}
    }	

    Event::dumpStats(cerr);
    doDeferredCursorMove();
//!!!    slotSetInsertCursorPosition(m_insertionTime, false);
    slotSetPointerPosition(m_document->getComposition().getPosition(), false);


    //!!! dup with setCurrentSelection, move to other fn somewhere

    if (m_currentEventSelection &&
	m_currentEventSelection->getSegmentEvents().size() == 0) {
	delete m_currentEventSelection;
	m_currentEventSelection = 0;
	//!!!??? was that the right thing to do?
    }

#ifdef RGKDE3
    // Clear states first, then enter only those ones that apply
    // (so as to avoid ever clearing one after entering another, in
    // case the two overlap at all)
    stateChanged("have_selection", KXMLGUIClient::StateReverse);
    stateChanged("have_notes_in_selection", KXMLGUIClient::StateReverse);
    stateChanged("have_rests_in_selection", KXMLGUIClient::StateReverse);

    if (m_currentEventSelection) {

	NOTATION_DEBUG << "NotationView::refreshSegment: Have selection; it's " << m_currentEventSelection << " covering range from " << m_currentEventSelection->getStartTime() << " to " << m_currentEventSelection->getEndTime() << " (" << m_currentEventSelection->getSegmentEvents().size() << " events)" << endl;

	stateChanged("have_selection", KXMLGUIClient::StateNoReverse);
	if (m_currentEventSelection->contains
	    (Rosegarden::Note::EventType)) {
	    stateChanged("have_notes_in_selection",
			 KXMLGUIClient::StateNoReverse);
	}
	if (m_currentEventSelection->contains
	    (Rosegarden::Note::EventRestType)) {
	    stateChanged("have_rests_in_selection",
			 KXMLGUIClient::StateNoReverse);
	}
    }
#else

    NOTATION_DEBUG << "Not using KDE3, not setting selection-related states"
		   << endl;
#endif


    PRINT_ELAPSED("NotationView::refreshSegment (including update/GC)");
}



#define UPDATE_PROGRESS(n) \
	progressCount += (n); \
	if (m_progress && (progressTotal > 0)) { \
	    m_progress->setCompleted(progressCount * 100 / progressTotal); \
	    m_progress->processEvents(); \
	}

void NotationView::readjustCanvasSize()
{
    START_TIMING;

    double maxWidth = 0.0;
    int maxHeight = 0;

    if (m_progress) {
	m_progress->setOperationName
	    (qstrtostr(i18n("Sizing and allocating canvas...")));
	m_progress->processEvents();
    }

    int progressTotal = m_staffs.size() + 2;
    int progressCount = 0;

    for (unsigned int i = 0; i < m_staffs.size(); ++i) {

        NotationStaff &staff = *m_staffs[i];

        staff.sizeStaff(m_hlayout);
	UPDATE_PROGRESS(1);

        if (staff.getTotalWidth() + staff.getX() > maxWidth) {
            maxWidth = staff.getTotalWidth() + staff.getX() + 1;
        }

        if (staff.getTotalHeight() + staff.getY() > maxHeight) {
            maxHeight = staff.getTotalHeight() + staff.getY() + 1;
        }

	PRINT_ELAPSED("NotationView::readjustCanvasSize checkpoint");
    }

    // now get the EditView to do the biz
    readjustViewSize(QSize(int(maxWidth), maxHeight));
    UPDATE_PROGRESS(2);

    PRINT_ELAPSED("NotationView::readjustCanvasSize total");
}


// Slots: These are here because they use the note action data map
// or mark action data map


void NotationView::slotNoteAction()
{
    const QObject* sigSender = sender();

    NoteActionDataMap::Iterator noteAct =
	m_noteActionDataMap->find(sigSender->name());
    
    if (noteAct != m_noteActionDataMap->end()) {
        setCurrentSelectedNote(*noteAct);
#ifdef RGKDE3
	if ((*noteAct).rest) {
	    stateChanged("note_insert_tool_current", StateReverse);
	    stateChanged("rest_insert_tool_current", StateNoReverse);
	} else {
	    stateChanged("note_insert_tool_current", StateNoReverse);
	    stateChanged("rest_insert_tool_current", StateReverse);
	}
#endif
    } else {
        NOTATION_DEBUG << "NotationView::slotNoteAction() : couldn't find NoteActionData named '"
                             << sigSender->name() << "'\n";
    }
}
    

void NotationView::slotAddMark()
{
    const QObject *s = sender();
    if (!m_currentEventSelection) return;

    MarkActionDataMap::Iterator i = m_markActionDataMap->find(s->name());

    if (i != m_markActionDataMap->end()) {
	addCommandToHistory(new MarksMenuAddMarkCommand
			    ((*i).mark, *m_currentEventSelection));
    }
}



void NotationView::initActionDataMaps()
{
    static bool called = false;
    static int keys[] =
    { Key_0, Key_3, Key_6, Key_8, Key_4, Key_2, Key_1, Key_5 };
    
    if (called) return;
    called = true;

    m_noteActionDataMap = new NoteActionDataMap;

    for (int rest = 0; rest < 2; ++rest) {
	for (int dots = 0; dots < 2; ++dots) {
	    for (int type = Note::Longest; type >= Note::Shortest; --type) {
		if (dots && (type == Note::Longest)) continue;

		Note note(type, dots);

		QString refName(strtoqstr(note.getReferenceName(rest)));
		QString shortName(strtoqstr(note.getShortName()));
		QString titleName(strtoqstr(note.getAmericanName()));
		titleName = titleName.left(1).upper() +
		            titleName.right(titleName.length()-1);

		shortName.replace(QRegExp(" "), "_");

		if (rest) {
		    shortName += "_rest";
		    titleName.replace(QRegExp("note"), "rest");
		}

		int keycode = keys[type - Note::Shortest];
		if (dots) keycode += CTRL;
		if (rest) // keycode += SHIFT; -- can't do shift+numbers
		    keycode = 0;

		m_noteActionDataMap->insert
		    (shortName, NoteActionData
		     (titleName, shortName, refName, keycode,
		      rest > 0, type, dots));
	    }
	}
    }

    m_markActionDataMap = new MarkActionDataMap;

    std::vector<Mark> marks = Rosegarden::Marks::getStandardMarks();
    for (unsigned int i = 0; i < marks.size(); ++i) {

	Mark mark = marks[i];
	QString markName(strtoqstr(mark));
	QString actionName = QString("add_%1").arg(markName);

	m_markActionDataMap->insert
	    (actionName, MarkActionData
	     (MarksMenuAddMarkCommand::getGlobalName(mark),
	      actionName, markName, 0, mark));
    }
	     
}

    
NotationView::NoteActionDataMap* NotationView::m_noteActionDataMap = 0;
NotationView::MarkActionDataMap* NotationView::m_markActionDataMap = 0;

