
/*
    Rosegarden-4 v0.1
    A sequencer and musical notation editor.

    This program is Copyright 2000-2001
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

#include <sys/times.h>

#include <qcanvas.h>

#include <kmessagebox.h>
#include <kmenubar.h>
#include <klocale.h>
#include <kconfig.h>
#include <kaction.h>
#include <kstdaction.h>
#include <kapp.h>

#include "rosegardenguidoc.h"
#include "notationview.h"
#include "notationelement.h"
#include "notationproperties.h"

#include "staff.h"
#include "notepixmapfactory.h"
#include "qcanvaslinegroupable.h"
#include "qcanvassimplesprite.h"
#include "resource.h"

#include "rosedebug.h"

#include "NotationTypes.h"
#include "TrackNotationHelper.h"
#include "Quantizer.h"

using Rosegarden::Event;
using Rosegarden::Int;
using Rosegarden::Bool;
using Rosegarden::String;
using Rosegarden::NoAccidental;
using Rosegarden::Note;
using Rosegarden::Track;
using Rosegarden::TrackNotationHelper;
using Rosegarden::Clef;
using Rosegarden::Key;
using Rosegarden::Accidental;
using Rosegarden::TimeSignature;
using Rosegarden::Quantizer;
using Rosegarden::timeT;



//////////////////////////////////////////////////////////////////////

NotationView::NotationView(RosegardenGUIDoc* doc,
                           Track* t,
                           QWidget *parent,
                           int resolution) :
    KMainWindow(parent),
    m_config(kapp->config()),
    m_document(doc),
    m_currentNotePixmap(0),
    m_hoveredOverNoteName(0),
    m_hoveredOverAbsoluteTime(0),
    m_canvasView(new NotationCanvasView(new QCanvas(width() * 2,
                                                    height() * 2),
                                        this)),
    m_mainStaff(new Staff(canvas(), resolution)),
    m_currentStaff(m_mainStaff),
    m_notePixmapFactory(m_currentStaff->getNotePixmapFactory()),
    m_toolbarNotePixmapFactory(5),
    m_viewElementsManager(0),
    m_notationElements(0),
    m_hlayout(0),
    m_vlayout(0),
    m_tool(0),
    m_selectDefaultNote(0)
{

    kdDebug(KDEBUG_AREA) << "NotationView ctor" << endl;

    setupActions();
    initStatusBar();
    
    setBackgroundMode(PaletteBase);

    setCentralWidget(m_canvasView);

    QObject::connect(m_canvasView, SIGNAL(itemClicked(int, const QPoint&, NotationElement*)),
                     this,         SLOT  (itemClicked(int, const QPoint&, NotationElement*)));

    QObject::connect(m_canvasView, SIGNAL(hoveredOverNoteChange(const QString&)),
                     this,         SLOT  (hoveredOverNoteChanged(const QString&)));

    QObject::connect(m_canvasView, SIGNAL(hoveredOverAbsoluteTimeChange(unsigned int)),
                     this,         SLOT  (hoveredOverAbsoluteTimeChange(unsigned int)));

    //     QObject::connect(this,         SIGNAL(changeCurrentNote(Note::Type)),
    //                      m_canvasView, SLOT(currentNoteChanged(Note::Type)));

    readOptions();

    setCaption(QString("%1 - Track Instrument #%2")
               .arg(doc->getTitle())
               .arg(t->getInstrument()));

    Track &allEvents = *t;

    m_viewElementsManager = new ViewElementsManager(allEvents);

    m_notationElements = m_viewElementsManager->getNotationElementList
        (allEvents.begin(), allEvents.end());

    m_mainStaff->move(20, 15);
    m_mainStaff->show();

    m_vlayout = new NotationVLayout(*m_mainStaff, *m_notationElements);
    m_hlayout = new NotationHLayout(*m_mainStaff, *m_notationElements);

    if (applyLayout()) {

        // Show all elements in the staff
        //             kdDebug(KDEBUG_AREA) << "Elements after layout : "
        //                                  << *m_notationElements << endl;
        showElements(m_notationElements->begin(), m_notationElements->end(), m_mainStaff);
        showBars(m_notationElements->begin(), m_notationElements->end());

    } else {
        KMessageBox::sorry(0, "Couldn't apply layout");
    }

    m_selectDefaultNote->activate();
}

NotationView::~NotationView()
{
    kdDebug(KDEBUG_AREA) << "-> ~NotationView()\n";

    saveOptions();

    delete m_hlayout;
    delete m_vlayout;
    delete m_viewElementsManager;
    delete m_notationElements; // this will erase all "notes" canvas items

    // Delete remaining canvas items.
    QCanvasItemList allItems = canvas()->allItems();
    QCanvasItemList::Iterator it;

    for(it = allItems.begin(); it != allItems.end(); ++it)
        delete *it;

    delete canvas();

    kdDebug(KDEBUG_AREA) << "<- ~NotationView()\n";
}

void NotationView::saveOptions()
{	
    m_config->setGroup("Notation Options");
    m_config->writeEntry("Geometry", size());
    m_config->writeEntry("Show Toolbar", toolBar()->isVisible());
    m_config->writeEntry("Show Statusbar",statusBar()->isVisible());
    m_config->writeEntry("ToolBarPos", (int) toolBar()->barPos());
}

void NotationView::readOptions()
{
    m_config->setGroup("Notation Options");
	
    QSize size(m_config->readSizeEntry("Geometry"));

    if(!size.isEmpty()) {
        resize(size);
    }
}

void NotationView::setupActions()
{   
    KRadioAction* noteAction = 0;
    
    // setup Notes menu & toolbar
    QIconSet icon;
 
    //
    // Notes
    //
    static const char* actionsNote[][3] = 
        {   // i18n,     slotName,         action name
            { "Breve",   "1slotBreve()",   "breve" },
            { "Whole",   "1slotWhole()",   "whole_note" },
            { "Half",    "1slotHalf()",    "half" },
            { "Quarter", "1slotQuarter()", "quarter" },
            { "8th",     "1slot8th()",     "8th" },
            { "16th",    "1slot16th()",    "16th" },
            { "32th",    "1slot32nd()",    "32th" },
            { "64th",    "1slot64th()",    "64th" }
        };
   
    for (unsigned int i = 0, noteType = Note::Longest;
         i < 8; ++i, --noteType) {

        icon = QIconSet(m_toolbarNotePixmapFactory.makeNotePixmap(noteType,
                                                                  0,
                                                                  NoAccidental,
                                                                  false, true, true, true));
        noteAction = new KRadioAction(i18n(actionsNote[i][0]), icon, 0, this,
                                      actionsNote[i][1],
                                      actionCollection(), actionsNote[i][2]);
        noteAction->setExclusiveGroup("notes");

        if (i == 3)
            m_selectDefaultNote = noteAction; // quarter is the default selected note

    }

    //
    // Dotted Notes
    //
    static const char* actionsDottedNote[][3] = 
        {
            { "Dotted Breve",   "1slotDottedBreve()",   "dotted_breve" },
            { "Dotted Whole",   "1slotDottedWhole()",   "dotted_whole_note" },
            { "Dotted Half",    "1slotDottedHalf()",    "dotted_half" },
            { "Dotted Quarter", "1slotDottedQuarter()", "dotted_quarter" },
            { "Dotted 8th",     "1slotDotted8th()",     "dotted_8th" },
            { "Dotted 16th",    "1slotDotted16th()",    "dotted_16th" },
            { "Dotted 32th",    "1slotDotted32nd()",    "dotted_32th" },
            { "Dotted 64th",    "1slotDotted64th()",    "dotted_64th" }
        };

    for (unsigned int i = 0, noteType = Note::Longest;
         i < 8; ++i, --noteType) {

        icon = QIconSet(m_toolbarNotePixmapFactory.makeNotePixmap(noteType,
                                                                  1,
                                                                  NoAccidental,
                                                                  false, true, true, true));
        noteAction = new KRadioAction(i18n(actionsDottedNote[i][0]), icon, 0, this,
                                      actionsDottedNote[i][1],
                                      actionCollection(), actionsDottedNote[i][2]);
        noteAction->setExclusiveGroup("notes");

    }

    //
    // Rests
    //
    static const char* actionsRest[][3] = 
        {
            { "Breve Rest",   "1slotRBreve()",   "breve_rest" },
            { "Whole Rest",   "1slotRWhole()",   "whole_note_rest" },
            { "Half Rest",    "1slotRHalf()",    "half_rest" },
            { "Quarter Rest", "1slotRQuarter()", "quarter_rest" },
            { "8th Rest",     "1slotR8th()",     "8th_rest" },
            { "16th Rest",    "1slotR16th()",    "16th_rest" },
            { "32th Rest",    "1slotR32nd()",    "32th_rest" },
            { "64th Rest",    "1slotR64th()",    "64th_rest" }
        };

    for (unsigned int i = 0, noteType = Note::Longest;
         i < 8; ++i, --noteType) {

        icon = QIconSet(m_toolbarNotePixmapFactory.makeRestPixmap(Note(noteType, false)));
        noteAction = new KRadioAction(i18n(actionsRest[i][0]), icon, 0, this,
                                      actionsRest[i][1],
                                      actionCollection(), actionsRest[i][2]);
        noteAction->setExclusiveGroup("notes");

    }

    //
    // Dotted Rests
    //
    static const char* actionsDottedRest[][3] = 
        {
            { "Dotted Breve Rest",   "1slotDottedRBreve()",   "dotted_breve_rest" },
            { "Dotted Whole Rest",   "1slotDottedRWhole()",   "dotted_whole_note_rest" },
            { "Dotted Half Rest",    "1slotDottedRHalf()",    "dotted_half_rest" },
            { "Dotted Quarter Rest", "1slotDottedRQuarter()", "dotted_quarter_rest" },
            { "Dotted 8th Rest",     "1slotDottedR8th()",     "dotted_8th_rest" },
            { "Dotted 16th Rest",    "1slotDottedR16th()",    "dotted_16th_rest" },
            { "Dotted 32th Rest",    "1slotDottedR32nd()",    "dotted_32th_rest" },
            { "Dotted 64th Rest",    "1slotDottedR64th()",    "dotted_64th_rest" }
        };

    for (unsigned int i = 0, noteType = Note::Longest;
         i < 8 && noteType > 0; ++i, --noteType) {

        icon = QIconSet(m_toolbarNotePixmapFactory.makeRestPixmap(Note(noteType, 1)));
        noteAction = new KRadioAction(i18n(actionsDottedRest[i][0]), icon, 0, this,
                                      actionsDottedRest[i][1],
                                      actionCollection(), actionsDottedRest[i][2]);
        noteAction->setExclusiveGroup("notes");

    }

    //
    // Accidentals
    //
    static const char* actionsAccidental[][3] = 
        {
            { "No accidental",  "1slotNoAccidental()",  "no_accidental" },
            { "Sharp",          "1slotSharp()",         "sharp_accidental" },
            { "Flat",           "1slotFlat()",          "flat_accidental" },
            { "Natural",        "1slotNatural()",       "natural_accidental" },
            { "Double sharp",   "1slotDoubleSharp()",   "double_sharp_accidental" },
            { "Double flat",    "1slotDoubleFlat()",    "double_flat_accidental" }
        };

    for (unsigned int i = 0, accidental = NoAccidental;
         i < 6; ++i, ++accidental) {

        icon = QIconSet(m_toolbarNotePixmapFactory.makeNotePixmap(Note::Crotchet,
                                                                  0,
                                                                  Accidental(accidental),
                                                                  false, true, true, true));
        noteAction = new KRadioAction(i18n(actionsAccidental[i][0]), icon, 0, this,
                                      actionsAccidental[i][1],
                                      actionCollection(), actionsAccidental[i][2]);
        noteAction->setExclusiveGroup("accidentals");
    }
    

    //
    // Clefs
    //

    // Treble
    icon = QIconSet(m_toolbarNotePixmapFactory.makeClefPixmap(Clef(Clef::Treble)));
    noteAction = new KRadioAction(i18n("Treble Clef"), icon, 0, this,
                                  SLOT(slotTrebleClef()),
                                  actionCollection(), "treble_clef");
    noteAction->setExclusiveGroup("notes");

    // Tenor
    icon = QIconSet(m_toolbarNotePixmapFactory.makeClefPixmap(Clef(Clef::Tenor)));
    noteAction = new KRadioAction(i18n("Tenor Clef"), icon, 0, this,
                                  SLOT(slotTenorClef()),
                                  actionCollection(), "tenor_clef");
    noteAction->setExclusiveGroup("notes");

    // Alto
    icon = QIconSet(m_toolbarNotePixmapFactory.makeClefPixmap(Clef(Clef::Alto)));
    noteAction = new KRadioAction(i18n("Alto Clef"), icon, 0, this,
                                  SLOT(slotAltoClef()),
                                  actionCollection(), "alto_clef");
    noteAction->setExclusiveGroup("notes");

    // Bass
    icon = QIconSet(m_toolbarNotePixmapFactory.makeClefPixmap(Clef(Clef::Bass)));
    noteAction = new KRadioAction(i18n("Bass Clef"), icon, 0, this,
                                  SLOT(slotBassClef()),
                                  actionCollection(), "bass_clef");
    noteAction->setExclusiveGroup("notes");

    //
    // Edition tools (at the moment, there's only an eraser)
    //
    noteAction = new KRadioAction(i18n("Erase"), "eraser", 0,
                                  this, SLOT(slotEraseSelected()),
                                  actionCollection(), "erase");
    noteAction->setExclusiveGroup("notes");
    

    // File menu
    KStdAction::close (this, SLOT(fileClose()),         actionCollection());

   // setup edit menu
    KStdAction::undo    (this, SLOT(slotEditUndo()),       actionCollection());
    KStdAction::redo    (this, SLOT(slotEditRedo()),       actionCollection());
    KStdAction::cut     (this, SLOT(slotEditCut()),        actionCollection());
    KStdAction::copy    (this, SLOT(slotEditCopy()),       actionCollection());
    KStdAction::paste   (this, SLOT(slotEditPaste()),      actionCollection());

    // setup Settings menu
    KStdAction::showToolbar(this, SLOT(slotToggleToolBar()), actionCollection());

    static const char* actionsToolbars[][3] = 
        {
            { "Show Notes Toolbar",  "1slotToggleNotesToolBar()",  "show_notes_toolbar" },
            { "Show Rests Toolbar",  "1slotToggleRestsToolBar()",  "show_rests_toolbar" },
            { "Show Accidentals Toolbar",   "1slotToggleAccidentalsToolBar()",  "show_accidentals_toolbar" },
            { "Show Clefs Toolbar",         "1slotToggleClefsToolBar()",        "show_clefs_toolbar" }
        };

    for (unsigned int i = 0; i < 4; ++i) {

        KToggleAction* toolbarAction = new KToggleAction(i18n(actionsToolbars[i][0]), 0,
                                                         this, actionsToolbars[i][1],
                                                         actionCollection(), actionsToolbars[i][2]);

        toolbarAction->setChecked(true);
    }
    
    KStdAction::showStatusbar(this, SLOT(slotToggleStatusBar()), actionCollection());

    KStdAction::saveOptions(this, SLOT(save_options()), actionCollection());
    KStdAction::preferences(this, SLOT(customize()), actionCollection());

    KStdAction::keyBindings(this, SLOT(editKeys()), actionCollection());
    KStdAction::configureToolbars(this, SLOT(editToolbars()), actionCollection());

    createGUI("notation.rc");
}


void NotationView::initStatusBar()
{
    KStatusBar* sb = statusBar();

    sb->insertItem(i18n(IDS_STATUS_DEFAULT), ID_STATUS_MSG);
    m_currentNotePixmap       = new QLabel(sb);
    m_hoveredOverNoteName     = new QLabel(sb);
    m_hoveredOverAbsoluteTime = new QLabel(sb);
    
    sb->addWidget(m_currentNotePixmap);
    sb->addWidget(m_hoveredOverNoteName);
    sb->addWidget(m_hoveredOverAbsoluteTime);
}


bool NotationView::showElements(NotationElementList::iterator from,
                                NotationElementList::iterator to)
{
    return showElements(from, to, 0, 0);
}

bool NotationView::showElements(NotationElementList::iterator from,
                                NotationElementList::iterator to,
                                QCanvasItem *item)
{
    return showElements(from, to, item->x(), item->y());
}

bool NotationView::showElements(NotationElementList::iterator from,
                                NotationElementList::iterator to,
                                double dxoffset, double dyoffset)
{
    kdDebug(KDEBUG_AREA) << "NotationView::showElements()" << endl;

    if (from == to) return true;

    NotePixmapFactory &npf(m_notePixmapFactory);
    Clef currentClef; // default is okay to start with

    for (NotationElementList::iterator it = from; it != to; ++it) {

        //
        // process event
        //
        try {

            QCanvasSimpleSprite *sprite = 0;

            if ((*it)->isNote()) {

                sprite = makeNoteSprite(it);

            } else if ((*it)->isRest()) {

                Note::Type note = (*it)->event()->get<Int>(Rosegarden::Note::NoteType);
                int dots = (*it)->event()->get<Int>(Rosegarden::Note::NoteDots);

                QCanvasPixmap notePixmap(npf.makeRestPixmap(Note(note, dots)));
                sprite = new QCanvasNotationSprite(*(*it), &notePixmap, canvas());

            } else if ((*it)->event()->isa(Clef::EventType)) {

		currentClef = Clef(*(*it)->event());
                QCanvasPixmap clefPixmap(npf.makeClefPixmap(currentClef));
                sprite = new QCanvasNotationSprite(*(*it), &clefPixmap, canvas());

            } else if ((*it)->event()->isa(Rosegarden::Key::EventType)) {

                QCanvasPixmap keyPixmap
                    (npf.makeKeyPixmap
                     (Rosegarden::Key((*it)->event()->get<String>
                                      (Rosegarden::Key::KeyPropertyName)),
                      currentClef));
                sprite = new QCanvasNotationSprite(*(*it), &keyPixmap, canvas());

            } else if ((*it)->event()->isa(TimeSignature::EventType)) {

                QCanvasPixmap timeSigPixmap
                    (npf.makeTimeSigPixmap(TimeSignature(*(*it)->event())));
                sprite = new QCanvasNotationSprite(*(*it), &timeSigPixmap, canvas());

            } else {
                    
                kdDebug(KDEBUG_AREA) << "NotationElement of unrecognised type "
                                     << (*it)->event()->getType()
                                     << endl;
                QCanvasPixmap unknownPixmap(npf.makeUnknownPixmap());
                sprite = new QCanvasNotationSprite(*(*it), &unknownPixmap, canvas());
            }

            //
            // Show the sprite
            //
            if (sprite) {

                (*it)->setCanvasItem(sprite, dxoffset, dyoffset);
                sprite->show();

            }
            
        } catch (...) {
            kdDebug(KDEBUG_AREA) << "Event lacks the proper properties: "
				 << (*(*it)->event())
                                 << endl;
        }
    }

    kdDebug(KDEBUG_AREA) << "NotationView::showElements() exiting" << endl;

    return true;
}


QCanvasSimpleSprite *NotationView::makeNoteSprite(NotationElementList::iterator it)
{
    NotePixmapFactory &npf(m_notePixmapFactory);

    Note::Type note = (*it)->event()->get<Int>(Rosegarden::Note::NoteType);
    int dots = (*it)->event()->get<Int>(Rosegarden::Note::NoteDots);

    Accidental accidental = NoAccidental;

    long acc;
    if ((*it)->event()->get<Int>(Properties::DISPLAY_ACCIDENTAL, acc)) {
        accidental = Accidental(acc);
    }

    bool up = true;
    (void)((*it)->event()->get<Bool>(Properties::STEM_UP, up));

    bool tail = true;
    (void)((*it)->event()->get<Bool>(Properties::DRAW_TAIL, tail));

    bool beamed = false;
    (void)((*it)->event()->get<Bool>(Properties::BEAMED, beamed));

    bool shifted = false;
    (void)((*it)->event()->get<Bool>(Properties::NOTE_HEAD_SHIFTED, shifted));

    long stemLength = npf.getNoteBodyHeight();
    (void)((*it)->event()->get<Int>
           (Properties::UNBEAMED_STEM_LENGTH, stemLength));

    kdDebug(KDEBUG_AREA) << "Got stem length of "
                         << stemLength << endl;

    if (beamed) {

        if ((*it)->event()->get<Bool>(Properties::BEAM_PRIMARY_NOTE)) {

            int myY = (*it)->event()->get<Int>(Properties::BEAM_MY_Y);

            stemLength = myY - (int)(*it)->getLayoutY();
            if (stemLength < 0) stemLength = -stemLength;

            int nextTailCount =
                (*it)->event()->get<Int>(Properties::BEAM_NEXT_TAIL_COUNT);
            int width =
                (*it)->event()->get<Int>(Properties::BEAM_SECTION_WIDTH);
            int gradient =
                (*it)->event()->get<Int>(Properties::BEAM_GRADIENT);

            bool thisPartialTails(false), nextPartialTails(false);
            (void)(*it)->event()->get<Bool>
                (Properties::BEAM_THIS_PART_TAILS, thisPartialTails);
            (void)(*it)->event()->get<Bool>
                (Properties::BEAM_NEXT_PART_TAILS, nextPartialTails);

            QCanvasPixmap notePixmap
                (npf.makeBeamedNotePixmap
                 (note, dots, accidental, shifted, up, stemLength,
                  nextTailCount, thisPartialTails, nextPartialTails,
                  width, (double)gradient / 100.0));
            return new QCanvasNotationSprite(*(*it), &notePixmap, canvas());

        } else {

            QCanvasPixmap notePixmap
                (npf.makeNotePixmap
                 (note, dots, accidental, shifted, tail, up, false,
                  stemLength));
            return new QCanvasNotationSprite(*(*it), &notePixmap, canvas());
        }

		
    } else {

        QCanvasPixmap notePixmap
            (npf.makeNotePixmap(note, dots, accidental,
                                shifted, tail, up, false, stemLength));

        return new QCanvasNotationSprite(*(*it), &notePixmap, canvas());
    }
}


bool NotationView::showBars(NotationElementList::iterator from,
                            NotationElementList::iterator to)
{
    if (from == to) return true;

    const NotationHLayout::BarDataList &barData(m_hlayout->getBarData());
    const Track::BarPositionList &barPositions(getTrack().getBarPositions());

    NotationElementList::iterator lastElement = to;
    --lastElement;

    //     kdDebug(KDEBUG_AREA) << "NotationView::showBars() : from->x = " <<(*from)->x()
    //                          << " - lastElement->x = " << (*lastElement)->x() << endl
    //                          << "lastElement : " << *(*lastElement) << endl;
    
    m_currentStaff->deleteBars(int((*from)->getEffectiveX()));

    for (NotationHLayout::BarDataList::const_iterator it = barData.begin();
         it != barData.end(); ++it) {

	if (it->barNo < 0 || it->barNo >= (int)barPositions.size()) {
	    kdDebug(KDEBUG_AREA) << "ERROR: Synchronisation problem: barNo "
				 << it->barNo << " is out of legal range (0,"
				 << barPositions.size()-1 << ")" << endl;
	} else {
	    kdDebug(KDEBUG_AREA) << "Adding bar number " << it->barNo
				 << " at pos " << it->x << endl;
	    m_currentStaff->insertBar(it->x, barPositions[it->barNo].correct);
	}
    }
    
    return true;
}


bool NotationView::applyLayout()
{
    bool rcp = applyHorizontalPreparse();
    bool rcv = applyVerticalLayout();
    bool rch = applyHorizontalLayout();

    readjustCanvasWidth();

    kdDebug(KDEBUG_AREA) << "NotationView::applyLayout() : done" << endl;

    return rcp && rch && rcv;
}


bool NotationView::applyHorizontalPreparse()
{
    if (!m_hlayout) {
        KMessageBox::error(0, "No Horizontal Layout engine");
        return false;
    }

    Track &t(getTrack());
    t.calculateBarPositions();
    const Track::BarPositionList &bpl(t.getBarPositions());

    m_hlayout->reset();
    m_hlayout->preparse(bpl, 0, bpl.size() - 1);

    kdDebug(KDEBUG_AREA) << "NotationView::applyHorizontalPreparse() : done" << endl;

    return m_hlayout->status() == 0;
}


bool NotationView::applyHorizontalLayout()
{
    if (!m_hlayout) {
        KMessageBox::error(0, "No Horizontal Layout engine");
        return false;
    }

    m_hlayout->layout();

    kdDebug(KDEBUG_AREA) << "NotationView::applyHorizontalLayout() : done" << endl;

    return m_hlayout->status() == 0;
}


bool NotationView::applyVerticalLayout()
{
    if (!m_vlayout) {
        KMessageBox::error(0, "No Vertical Layout engine");
        return false;
    }

    m_vlayout->reset();
    m_vlayout->layout(m_notationElements->begin(), m_notationElements->end());
    
    kdDebug(KDEBUG_AREA) << "NotationView::applyVerticalLayout() : done" << endl;

    return m_vlayout->status() == 0;
}


void NotationView::setCurrentSelectedNote(bool rest, Note::Type n, int dots)
{
    if (rest)
        setTool(new RestInserter(n, dots, *this));
    else
        setTool(new NoteInserter(n, dots, *this));

    if (!rest) {
        m_currentNotePixmap->setPixmap
            (m_toolbarNotePixmapFactory.makeNotePixmap(n, dots, NoAccidental,
                                                       false, true, true, true));
    } else {
        m_currentNotePixmap->setPixmap
            (m_toolbarNotePixmapFactory.makeRestPixmap(Note(n, dots)));
    }

    emit changeCurrentNote(rest, n);
}

void NotationView::setTool(NotationTool* tool)
{
    delete m_tool;
    m_tool = tool;
}


//////////////////////////////////////////////////////////////////////
//                    Slots
//////////////////////////////////////////////////////////////////////

void NotationView::slotEditUndo()
{
    slotStatusMsg(i18n("Undo..."));

    slotStatusMsg(i18n(IDS_STATUS_DEFAULT));
}

void NotationView::slotEditRedo()
{
    slotStatusMsg(i18n("Redo..."));

    slotStatusMsg(i18n(IDS_STATUS_DEFAULT));
}

void NotationView::slotEditCut()
{
    slotStatusMsg(i18n("Cutting selection..."));

    slotStatusMsg(i18n(IDS_STATUS_DEFAULT));
}

void NotationView::slotEditCopy()
{
    slotStatusMsg(i18n("Copying selection to clipboard..."));

    slotStatusMsg(i18n(IDS_STATUS_DEFAULT));
}

void NotationView::slotEditPaste()
{
    slotStatusMsg(i18n("Inserting clipboard contents..."));

    slotStatusMsg(i18n(IDS_STATUS_DEFAULT));
}

void NotationView::slotToggleToolBar()
{
    slotStatusMsg(i18n("Toggle the toolbar..."));

    if (toolBar()->isVisible())
        toolBar()->hide();
    else
        toolBar()->show();

    slotStatusMsg(i18n(IDS_STATUS_DEFAULT));
}

void NotationView::slotToggleNotesToolBar()
{
    toggleNamedToolBar("notesToolBar");
}

void NotationView::slotToggleRestsToolBar()
{
    toggleNamedToolBar("restsToolBar");
}

void NotationView::slotToggleAccidentalsToolBar()
{
    toggleNamedToolBar("accidentalsToolBar");
}

void NotationView::slotToggleClefsToolBar()
{
    toggleNamedToolBar("clefsToolBar");
}

void NotationView::toggleNamedToolBar(const QString& toolBarName)
{
    KToolBar *namedToolBar = toolBar(toolBarName);
    if (!namedToolBar) {
        kdDebug(KDEBUG_AREA) << "NotationView::toggleNamedToolBar() : toolBar "
                             << toolBarName << " not found" << endl;
        return;
    }
    
    if (namedToolBar->isVisible())
        namedToolBar->hide();
    else
        namedToolBar->show();
}

void NotationView::slotToggleStatusBar()
{
    slotStatusMsg(i18n("Toggle the statusbar..."));

    if (statusBar()->isVisible())
        statusBar()->hide();
    else
        statusBar()->show();

    slotStatusMsg(i18n(IDS_STATUS_DEFAULT));
}


void NotationView::slotStatusMsg(const QString &text)
{
    ///////////////////////////////////////////////////////////////////
    // change status message permanently
    statusBar()->clear();
    statusBar()->changeItem(text, ID_STATUS_MSG);
}


void NotationView::slotStatusHelpMsg(const QString &text)
{
    ///////////////////////////////////////////////////////////////////
    // change status message of whole statusbar temporary (text, msec)
    statusBar()->message(text, 2000);
}

//////////////////////////////////////////////////////////////////////

//----------------------------------------
// Notes
//----------------------------------------

void NotationView::slotBreve()
{
    kdDebug(KDEBUG_AREA) << "NotationView::slotBreve()\n";
    setCurrentSelectedNote(false, Note::Breve);
}

void NotationView::slotWhole()
{
    kdDebug(KDEBUG_AREA) << "NotationView::slotWhole()\n";
    setCurrentSelectedNote(false, Note::WholeNote);
}

void NotationView::slotHalf()
{
    kdDebug(KDEBUG_AREA) << "NotationView::slotHalf()\n";
    setCurrentSelectedNote(false, Note::HalfNote);
}

void NotationView::slotQuarter()
{
    kdDebug(KDEBUG_AREA) << "NotationView::slotQuarter()\n";
    setCurrentSelectedNote(false, Note::QuarterNote);
}

void NotationView::slot8th()
{
    kdDebug(KDEBUG_AREA) << "NotationView::slot8th()\n";
    setCurrentSelectedNote(false, Note::EighthNote);
}

void NotationView::slot16th()
{
    kdDebug(KDEBUG_AREA) << "NotationView::slot16th()\n";
    setCurrentSelectedNote(false, Note::SixteenthNote);
}

void NotationView::slot32nd()
{
    kdDebug(KDEBUG_AREA) << "NotationView::slot32nd()\n";
    setCurrentSelectedNote(false, Note::ThirtySecondNote);
}

void NotationView::slot64th()
{
    kdDebug(KDEBUG_AREA) << "NotationView::slot64th()\n";
    setCurrentSelectedNote(false, Note::SixtyFourthNote);
}

void NotationView::slotDottedBreve()
{
    kdDebug(KDEBUG_AREA) << "NotationView::slotDottedBreve()\n";
    setCurrentSelectedNote(false, Note::Breve, 1);
}

void NotationView::slotDottedWhole()
{
    kdDebug(KDEBUG_AREA) << "NotationView::slotDottedWhole()\n";
    setCurrentSelectedNote(false, Note::WholeNote, 1);
}

void NotationView::slotDottedHalf()
{
    kdDebug(KDEBUG_AREA) << "NotationView::slotDottedHalf()\n";
    setCurrentSelectedNote(false, Note::HalfNote, 1);
}

void NotationView::slotDottedQuarter()
{
    kdDebug(KDEBUG_AREA) << "NotationView::slotDottedQuarter()\n";
    setCurrentSelectedNote(false, Note::QuarterNote, 1);
}

void NotationView::slotDotted8th()
{
    kdDebug(KDEBUG_AREA) << "NotationView::slotDotted8th()\n";
    setCurrentSelectedNote(false, Note::EighthNote, 1);
}

void NotationView::slotDotted16th()
{
    kdDebug(KDEBUG_AREA) << "NotationView::slotDotted16th()\n";
    setCurrentSelectedNote(false, Note::SixteenthNote, 1);
}

void NotationView::slotDotted32nd()
{
    kdDebug(KDEBUG_AREA) << "NotationView::slotDotted32nd()\n";
    setCurrentSelectedNote(false, Note::ThirtySecondNote, 1);
}

void NotationView::slotDotted64th()
{
    kdDebug(KDEBUG_AREA) << "NotationView::slotDotted64th()\n";
    setCurrentSelectedNote(false, Note::SixtyFourthNote, 1);
}

//----------------------------------------
// Rests
//----------------------------------------

void NotationView::slotRBreve()
{
    kdDebug(KDEBUG_AREA) << "NotationView::slotRBreve()\n";
    setCurrentSelectedNote(true, Note::Breve);
}

void NotationView::slotRWhole()
{
    kdDebug(KDEBUG_AREA) << "NotationView::slotRWhole()\n";
    setCurrentSelectedNote(true, Note::WholeNote);
}

void NotationView::slotRHalf()
{
    kdDebug(KDEBUG_AREA) << "NotationView::slotRHalf()\n";
    setCurrentSelectedNote(true, Note::HalfNote);
}

void NotationView::slotRQuarter()
{
    kdDebug(KDEBUG_AREA) << "NotationView::slotRQuarter()\n";
    setCurrentSelectedNote(true, Note::QuarterNote);
}

void NotationView::slotR8th()
{
    kdDebug(KDEBUG_AREA) << "NotationView::slotR8th()\n";
    setCurrentSelectedNote(true, Note::EighthNote);
}

void NotationView::slotR16th()
{
    kdDebug(KDEBUG_AREA) << "NotationView::slotR16th()\n";
    setCurrentSelectedNote(true, Note::SixteenthNote);
}

void NotationView::slotR32nd()
{
    kdDebug(KDEBUG_AREA) << "NotationView::slotR32nd()\n";
    setCurrentSelectedNote(true, Note::ThirtySecondNote);
}

void NotationView::slotR64th()
{
    kdDebug(KDEBUG_AREA) << "NotationView::slotR64th()\n";
    setCurrentSelectedNote(true, Note::SixtyFourthNote);
}

void NotationView::slotDottedRBreve()
{
    kdDebug(KDEBUG_AREA) << "NotationView::slotDottedRBreve()\n";
    setCurrentSelectedNote(true, Note::Breve, 1);
}

void NotationView::slotDottedRWhole()
{
    kdDebug(KDEBUG_AREA) << "NotationView::slotDottedRWhole()\n";
    setCurrentSelectedNote(true, Note::WholeNote, 1);
}

void NotationView::slotDottedRHalf()
{
    kdDebug(KDEBUG_AREA) << "NotationView::slotDottedRHalf()\n";
    setCurrentSelectedNote(true, Note::HalfNote, 1);
}

void NotationView::slotDottedRQuarter()
{
    kdDebug(KDEBUG_AREA) << "NotationView::slotDottedRQuarter()\n";
    setCurrentSelectedNote(true, Note::QuarterNote, 1);
}

void NotationView::slotDottedR8th()
{
    kdDebug(KDEBUG_AREA) << "NotationView::slotDottedR8th()\n";
    setCurrentSelectedNote(true, Note::EighthNote, 1);
}

void NotationView::slotDottedR16th()
{
    kdDebug(KDEBUG_AREA) << "NotationView::slotDottedR16th()\n";
    setCurrentSelectedNote(true, Note::SixteenthNote, 1);
}

void NotationView::slotDottedR32nd()
{
    kdDebug(KDEBUG_AREA) << "NotationView::slotDottedR32nd()\n";
    setCurrentSelectedNote(true, Note::ThirtySecondNote, 1);
}

void NotationView::slotDottedR64th()
{
    kdDebug(KDEBUG_AREA) << "NotationView::slotDottedR64th()\n";
    setCurrentSelectedNote(true, Note::SixtyFourthNote, 1);
}

//----------------------------------------
// Accidentals
//----------------------------------------
void NotationView::slotNoAccidental()
{
    NoteInserter::setAccidental(Rosegarden::NoAccidental);
}

void NotationView::slotSharp()
{
    NoteInserter::setAccidental(Rosegarden::Sharp);
}

void NotationView::slotFlat()
{
    NoteInserter::setAccidental(Rosegarden::Flat);
}

void NotationView::slotNatural()
{
    NoteInserter::setAccidental(Rosegarden::Natural);
}

void NotationView::slotDoubleSharp()
{
    NoteInserter::setAccidental(Rosegarden::DoubleSharp);
}

void NotationView::slotDoubleFlat()
{
    NoteInserter::setAccidental(Rosegarden::DoubleFlat);
}


//----------------------------------------
// Clefs
//----------------------------------------
void NotationView::slotTrebleClef()
{
    setTool(new ClefInserter(Clef::Treble, *this));
}

void NotationView::slotTenorClef()
{
    setTool(new ClefInserter(Clef::Tenor, *this));
}

void NotationView::slotAltoClef()
{
    setTool(new ClefInserter(Clef::Alto, *this));
}

void NotationView::slotBassClef()
{
    setTool(new ClefInserter(Clef::Bass, *this));
}


//----------------------------------------
// Time sigs.
//----------------------------------------

//----------------------------------------
// Edition Tools
//----------------------------------------

void NotationView::slotEraseSelected()
{
    kdDebug(KDEBUG_AREA) << "NotationView::slotEraseSelected()\n";
    setTool(new NotationEraser(*this));
}

//----------------------------------------------------------------------

void NotationView::itemClicked(int height, const QPoint &eventPos,
                               NotationElement* el)
{
    m_tool->handleClick(height, eventPos, el);
}

NotationElementList::iterator
NotationView::findClosestNote(double eventX, Event *&timeSignature,
			      Event *&clef, Event *&key,
                               unsigned int proximityThreshold)
{
    double minDist = 10e9,
        prevDist = 10e9;

    NotationElementList::iterator it, res;

    // TODO: this is grossly inefficient
    //
    for (it = m_notationElements->begin();
         it != m_notationElements->end(); ++it) 
{
        if (!(*it)->isNote() && !(*it)->isRest()) {
            if ((*it)->event()->isa(Clef::EventType)) {
                kdDebug(KDEBUG_AREA) << "NotationView::findClosestNote() : found clef: type is "
                                     << (*it)->event()->get<String>(Clef::ClefPropertyName) << endl;
		//          clef = Clef(*(*it)->event());
		clef = (*it)->event();
            } else if ((*it)->event()->isa(TimeSignature::EventType)) {
	      kdDebug(KDEBUG_AREA) << "NotationView::findClosestNote() : found time sig " << endl;
	      //          tsig = TimeSignature(*(*it)->event());
	        timeSignature = (*it)->event();
            } else if ((*it)->event()->isa(Rosegarden::Key::EventType)) {
                kdDebug(KDEBUG_AREA) << "NotationView::findClosestNote() : found key: type is "
                                     << (*it)->event()->get<String>(Rosegarden::Key::KeyPropertyName) << endl;
		//          key = Rosegarden::Key(*(*it)->event());
		key = (*it)->event();
            }
            continue;
        }

        double dist;
        
        if ( (*it)->getEffectiveX() >= eventX )
            dist = (*it)->getEffectiveX() - eventX;
        else
            dist = eventX - (*it)->getEffectiveX();

        if (dist < minDist) {
            kdDebug(KDEBUG_AREA) << "NotationView::findClosestNote() : minDist was "
                                 << minDist << " now = " << dist << endl;
            minDist = dist;
            res = it;
        }
        
        // not sure about this
        if (dist > prevDist) break; // we can stop right now

        prevDist = dist;
    }

    kdDebug(KDEBUG_AREA) << "NotationView::findClosestNote(" << eventX << ") : found "
                         << *(*res) << endl;

    if (minDist > proximityThreshold) {
        kdDebug(KDEBUG_AREA) << "NotationView::findClosestNote() : element is too far away : "
                             << minDist << endl;
        return m_notationElements->end();
    }
        
    return res;
}


void NotationView::redoLayout(NotationElementList::iterator from)
{
    applyLayout(); // TODO : be more subtle than this

    //     kdDebug(KDEBUG_AREA) << "NotationView::insertNote() : Elements after relayout : "
    //                          << endl << *m_notationElements << endl;

    // (*m_hlayout)(notationElement);

    // TODO : m_currentStaff should be updated by the mouse click 

    if (from != m_notationElements->begin())
        showElements(
                     //!!! sadly we can't just redo from where we are, because
                     //things like beamed notes earlier in the same group may
                     //need to be redrawn with different beam angles.  We may
                     //be able to get away with redrawing from the start of the
                     //group or bar, but for now let's just do this:
                     m_notationElements->begin()  /*--redoLayoutStart */,
                     m_notationElements->end(),
                     m_currentStaff);
    else
        showElements(m_notationElements->begin(),
                     m_notationElements->end(),
                     m_currentStaff);

    showBars(m_notationElements->begin(),
             m_notationElements->end());
}

void NotationView::readjustCanvasWidth()
{
    double totalWidth = m_hlayout->getTotalWidth();

    kdDebug(KDEBUG_AREA) << "NotationView::readjustCanvasWidth() : totalWidth = "
                         << totalWidth << endl;

    if (canvas()->width() < totalWidth) {

        kdDebug(KDEBUG_AREA) << "NotationView::readjustCanvasWidth() to "
                             << totalWidth << endl;

        canvas()->resize(int(totalWidth) + 50, canvas()->height());

        m_mainStaff->setLinesLength(canvas()->width() - 20);
    }
    

}

void
NotationView::hoveredOverNoteChanged(const QString &noteName)
{
    m_hoveredOverNoteName->setText(noteName);
}

void
NotationView::hoveredOverAbsoluteTimeChange(unsigned int time)
{
    m_hoveredOverAbsoluteTime->setText(QString("Time : %1").arg(time));
}

//////////////////////////////////////////////////////////////////////
//               Notation Tools
//////////////////////////////////////////////////////////////////////
NotationTool::NotationTool(NotationView& view)
    : m_parentView(view)
{
}

NotationTool::~NotationTool()
{
}

//------------------------------

NoteInserter::NoteInserter(Rosegarden::Note::Type type,
                           unsigned int dots,
                           NotationView& view)
    : NotationTool(view),
      m_noteType(type),
      m_noteDots(dots)
{
}
    
void    
NoteInserter::handleClick(int height, const QPoint &eventPos,
                          NotationElement*)
{
    //        Rosegarden::Key key;
    //        Clef clef;
    //	TimeSignature tsig;
    Event *tsig = 0, *clef = 0, *key = 0;
    NotationElementList::iterator closestNote =
        m_parentView.findClosestNote(eventPos.x(), tsig, clef, key);

    if (closestNote == m_parentView.getNotationElements()->end()) {
        return;
    }


    kdDebug(KDEBUG_AREA) << "NoteInserter::handleClick() : accidental = "
                         << m_accidental << endl;

    int pitch = Rosegarden::NotationDisplayPitch(height, m_accidental).
        getPerformancePitch(clef ? Clef(*clef) : Clef::DefaultClef,
                            key ? Rosegarden::Key(*key) :
                            Rosegarden::Key::DefaultKey);

    // We are going to modify the document so mark it as such
    //
    m_parentView.getDocument()->setModified();

    Note note(m_noteType, m_noteDots);
    TrackNotationHelper nt(m_parentView.getTrack());

    doInsert(nt, (*closestNote)->getAbsoluteTime(), note, pitch,
             m_accidental);

    // TODO: be less silly
    m_parentView.redoLayout(m_parentView.getNotationElements()->begin());

}

void NoteInserter::doInsert(TrackNotationHelper& nt,
                            Rosegarden::timeT absTime,
                            const Note& note, int pitch,
                            Accidental accidental)
{
    nt.insertNote(absTime, note, pitch, accidental);
}

void NoteInserter::setAccidental(Rosegarden::Accidental accidental)
{
    m_accidental = accidental;
}

Rosegarden::Accidental NoteInserter::m_accidental = NoAccidental;

//------------------------------

RestInserter::RestInserter(Rosegarden::Note::Type type,
                           unsigned int dots, NotationView& view)
    : NoteInserter(type, dots, view)
{
}

void RestInserter::doInsert(TrackNotationHelper& nt,
                            Rosegarden::timeT absTime,
                            const Note& note, int,
                            Accidental)
{
    nt.insertRest(absTime, note);
}

//------------------------------

ClefInserter::ClefInserter(std::string clefType, NotationView& view)
    : NotationTool(view),
      m_clef(clefType)
{
}
    
void ClefInserter::handleClick(int height, const QPoint &eventPos,
                               NotationElement*)
{
    Event *tsig = 0, *clef = 0, *key = 0;
    NotationElementList::iterator closestNote =
        m_parentView.findClosestNote(eventPos.x(), tsig, clef, key,
                                     100);

    if (closestNote == m_parentView.getNotationElements()->end()) {
        return;
    }

     Rosegarden::timeT absTime = (*closestNote)->getAbsoluteTime();
//     double closestNoteX = (*closestNote)->getEffectiveX();
    
    TrackNotationHelper nt(m_parentView.getTrack());
    nt.insertClef(absTime, m_clef);

    // TODO: be less silly
    m_parentView.redoLayout(m_parentView.getNotationElements()->begin());

}


//------------------------------

NotationEraser::NotationEraser(NotationView& view)
    : NotationTool(view)
{
}

void NotationEraser::handleClick(int, const QPoint&,
                                 NotationElement* element)
{
    bool needLayout = false;
    TrackNotationHelper nt(m_parentView.getTrack());

    if (element->isNote()) {
        
	nt.deleteNote(element->event());
	needLayout = true;

    } else if (element->isRest()) {

	nt.deleteRest(element->event());
	needLayout = true;

    } else {
        // we don't know what it is
        KMessageBox::sorry(0, "Not Implemented Yet");

    }
    
    if (needLayout) // TODO : be more subtle
        m_parentView.redoLayout(m_parentView.getNotationElements()->begin());
}
