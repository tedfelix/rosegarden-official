
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
#include "NotationTypes.h"
#include "notationproperties.h"

#include "staff.h"
#include "notepixmapfactory.h"
#include "qcanvaslinegroupable.h"
#include "qcanvassimplesprite.h"
#include "quantizer.h"
#include "resource.h"

#include "rosedebug.h"

using Rosegarden::Event;
using Rosegarden::Int;
using Rosegarden::Bool;
using Rosegarden::String;
using Rosegarden::NoAccidental;
using Rosegarden::Note;
using Rosegarden::Track;
using Rosegarden::Clef;
using Rosegarden::Key;
using Rosegarden::Accidental;
using Rosegarden::TimeSignature;

class RestSplitter
{
public:
    /**
     * restDuration : duration of rest being replaced
     * replacedBit  : duration of note replacing the rest
     */
    RestSplitter(Event::timeT restDuration, Event::timeT replacedBit);
    
    Event::timeT nextBit();

protected:
    Event::timeT m_restDuration;
    Event::timeT m_remainDuration;
    Event::timeT m_replacedBit;
    Event::timeT m_currentBit;
    
    static Event::timeT m_baseRestDuration;
};

RestSplitter::RestSplitter(Event::timeT restDuration,
                           Event::timeT replacedBit)
    : m_restDuration(restDuration),
      m_remainDuration(0),
      m_replacedBit(replacedBit),
      m_currentBit(m_baseRestDuration)
{
    kdDebug(KDEBUG_AREA) << "RestSplitter::RestSplitter() : restDuration = "
                         << restDuration << " - replacedBit = "
                         << replacedBit << endl;

    m_remainDuration = restDuration - replacedBit;

    while (m_currentBit > m_remainDuration) m_currentBit /= 2;

}

    
Event::timeT
RestSplitter::nextBit()
{
    if (m_remainDuration == 0) return 0;

    if (m_remainDuration >= m_currentBit) {

        m_remainDuration -= m_currentBit;

    } else {

        if (m_currentBit == 6)
            return 0;

        m_currentBit /= 2;

        m_remainDuration -= m_currentBit;
    }
    
    return m_currentBit;
}

Event::timeT
RestSplitter::m_baseRestDuration = 384; // whole note rest

//////////////////////////////////////////////////////////////////////

NotationView::NotationView(RosegardenGUIDoc* doc,
                           unsigned int trackNb,
                           QWidget *parent,
                           int resolution) :
    KMainWindow(parent),
    m_config(kapp->config()),
    m_document(doc),
    m_currentNotePixmap(0),
    m_canvasView(new NotationCanvasView(new QCanvas(width() * 2,
                                                    height() * 2),
                                        this)),
    m_mainStaff(new Staff(canvas(), resolution)),
    m_currentStaff(m_mainStaff),
    m_notePixmapFactory(m_currentStaff->getNotePixmapFactory()),
    m_toolbarNotePixmapFactory(9),
    m_viewElementsManager(0),
    m_notationElements(0),
    m_hlayout(0),
    m_vlayout(0),
    m_currentSelectedNoteIsRest(false),
    m_currentSelectedNoteType(Note::QuarterNote),
    m_currentSelectedNoteDotted(false)
{

    kdDebug(KDEBUG_AREA) << "NotationView ctor" << endl;

    setupActions();
    initStatusBar();
    
    setBackgroundMode(PaletteBase);

    setCentralWidget(m_canvasView);

    QObject::connect(m_canvasView, SIGNAL(noteInserted(int, const QPoint&)),
                     this,         SLOT  (insertNote  (int, const QPoint&)));

    QObject::connect(m_canvasView, SIGNAL(hoveredOverNoteChange(const QString&)),
                     this,         SLOT  (hoveredOverNoteChanged(const QString&)));

//     QObject::connect(this,         SIGNAL(changeCurrentNote(Note::Type)),
//                      m_canvasView, SLOT(currentNoteChanged(Note::Type)));

    readOptions();

    if (!doc) {
        kdDebug(KDEBUG_AREA) << "NotationView ctor : getDocument() returned 0" << endl;
        KMessageBox::sorry(0, "No document");
        throw -1;
    }

    setCaption(QString("%1 - Track #%2")
               .arg(trackNb)
               .arg(doc->getTitle()));

    Track* t = doc->getTrack(trackNb);

    if (!t) {
        kdDebug(KDEBUG_AREA) << "NotationView ctor : getTrack("
                             << trackNb
                             << ") returned 0" << endl;
        KMessageBox::sorry(0, QString("No track %1").arg(trackNb));
            
        throw -1;
    }
        
    Track &allEvents = *t;

    m_viewElementsManager = new ViewElementsManager(allEvents);

    m_notationElements = m_viewElementsManager->notationElementList(allEvents.begin(),
                                                                    allEvents.end());

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

    slotQuarter();
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

void
NotationView::saveOptions()
{	
    m_config->setGroup("Notation Options");
    m_config->writeEntry("Geometry", size());
    m_config->writeEntry("Show Toolbar", toolBar()->isVisible());
    m_config->writeEntry("Show Statusbar",statusBar()->isVisible());
    m_config->writeEntry("ToolBarPos", (int) toolBar()->barPos());
}

void
NotationView::readOptions()
{
    m_config->setGroup("Notation Options");
	
    QSize size(m_config->readSizeEntry("Geometry"));

    if(!size.isEmpty()) {
        resize(size);
    }
}

void
NotationView::setupActions()
{
    KRadioAction* noteAction = 0;
    
    // setup Notes menu & toolbar

    // Breve
    QIconSet icon(m_toolbarNotePixmapFactory.makeNotePixmap
                  (Note::Breve, false, NoAccidental, true, true, true));
    noteAction = new KRadioAction(i18n("Breve"), icon, 0, this,
                                  SLOT(slotBreve()),
                                  actionCollection(), "breve" );
    noteAction->setExclusiveGroup("notes");
    
    // Whole
    icon = QIconSet(m_toolbarNotePixmapFactory.makeNotePixmap
                    (Note::WholeNote, false, NoAccidental, true, true, true));
    noteAction = new KRadioAction(i18n("Whole"), icon, 0, this,
                                  SLOT(slotWhole()),
                                  actionCollection(), "whole_note" );
    noteAction->setExclusiveGroup("notes");
    
    // Half
    icon = QIconSet(m_toolbarNotePixmapFactory.makeNotePixmap
                    (Note::HalfNote, false, NoAccidental, true, true, true));
    noteAction = new KRadioAction(i18n("Half"), icon, 0, this,
                                  SLOT(slotHalf()),
                                  actionCollection(), "half" );
    noteAction->setExclusiveGroup("notes");

    // Quarter
    icon = QIconSet(m_toolbarNotePixmapFactory.makeNotePixmap
                    (Note::QuarterNote, false, NoAccidental,
                     true, true, true));
    noteAction = new KRadioAction(i18n("Quarter"), icon, 0, this,
                                  SLOT(slotQuarter()),
                                  actionCollection(), "quarter" );
    noteAction->setExclusiveGroup("notes");

    // 8th
    icon = QIconSet(m_toolbarNotePixmapFactory.makeNotePixmap
                    (Note::EighthNote, false, NoAccidental,
                     true, true, true));
    noteAction = new KRadioAction(i18n("8th"), icon, 0, this,
                                  SLOT(slot8th()),
                                  actionCollection(), "8th" );
    noteAction->setExclusiveGroup("notes");

    // 16th
    icon = QIconSet(m_toolbarNotePixmapFactory.makeNotePixmap
                    (Note::SixteenthNote, false, NoAccidental,
                     true, true, true));
    noteAction = new KRadioAction(i18n("16th"), icon, 0, this,
                                  SLOT(slot16th()),
                                  actionCollection(), "16th" );
    noteAction->setExclusiveGroup("notes");

    // 32nd
    icon = QIconSet(m_toolbarNotePixmapFactory.makeNotePixmap
                    (Note::ThirtySecondNote, false, NoAccidental,
                     true, true, true));
    noteAction = new KRadioAction(i18n("32nd"), icon, 0, this,
                                  SLOT(slot32nd()),
                                  actionCollection(), "32nd" );
    noteAction->setExclusiveGroup("notes");

    // 64th
    icon = QIconSet(m_toolbarNotePixmapFactory.makeNotePixmap
                    (Note::SixtyFourthNote, false, NoAccidental,
                     true, true, true));
    noteAction = new KRadioAction(i18n("64th"), icon, 0, this,
                                  SLOT(slot64th()),
                                  actionCollection(), "64th" );
    noteAction->setExclusiveGroup("notes");

    //!!! surely we should be doing all this in a loop, making the
    //buttons all call back on the same method when pressed but
    //passing in different data to the method to indicate which button
    //was pressed?  no idea how to do that with this qt lark though

    // Breve
    icon = QIconSet(m_toolbarNotePixmapFactory.makeRestPixmap
                    (Note::Breve, false));
    noteAction = new KRadioAction(i18n("Breve Rest"), icon, 0, this,
                                  SLOT(slotRBreve()),
                                  actionCollection(), "breve_rest" );
    noteAction->setExclusiveGroup("notes");
    
    // Whole
    icon = QIconSet(m_toolbarNotePixmapFactory.makeRestPixmap
                    (Note::WholeNote, false));
    noteAction = new KRadioAction(i18n("Whole Rest"), icon, 0, this,
                                  SLOT(slotRWhole()),
                                  actionCollection(), "whole_note_rest" );
    noteAction->setExclusiveGroup("notes");
    
    // Half
    icon = QIconSet(m_toolbarNotePixmapFactory.makeRestPixmap
                    (Note::HalfNote, false));
    noteAction = new KRadioAction(i18n("Half Rest"), icon, 0, this,
                                  SLOT(slotRHalf()),
                                  actionCollection(), "half_rest" );
    noteAction->setExclusiveGroup("notes");

    // Quarter
    icon = QIconSet(m_toolbarNotePixmapFactory.makeRestPixmap
                    (Note::QuarterNote, false));
    noteAction = new KRadioAction(i18n("Quarter Rest"), icon, 0, this,
                                  SLOT(slotRQuarter()),
                                  actionCollection(), "quarter_rest" );
    noteAction->setExclusiveGroup("notes");

    // 8th
    icon = QIconSet(m_toolbarNotePixmapFactory.makeRestPixmap
                    (Note::EighthNote, false));
    noteAction = new KRadioAction(i18n("8th Rest"), icon, 0, this,
                                  SLOT(slotR8th()),
                                  actionCollection(), "8th_rest" );
    noteAction->setExclusiveGroup("notes");

    // 16th
    icon = QIconSet(m_toolbarNotePixmapFactory.makeRestPixmap
                    (Note::SixteenthNote, false));
    noteAction = new KRadioAction(i18n("16th Rest"), icon, 0, this,
                                  SLOT(slotR16th()),
                                  actionCollection(), "16th_rest" );
    noteAction->setExclusiveGroup("notes");

    // 32nd
    icon = QIconSet(m_toolbarNotePixmapFactory.makeRestPixmap
                    (Note::ThirtySecondNote, false));
    noteAction = new KRadioAction(i18n("32nd Rest"), icon, 0, this,
                                  SLOT(slotR32nd()),
                                  actionCollection(), "32nd_rest" );
    noteAction->setExclusiveGroup("notes");

    // 64th
    icon = QIconSet(m_toolbarNotePixmapFactory.makeRestPixmap
                    (Note::SixtyFourthNote, false));
    noteAction = new KRadioAction(i18n("64th Rest"), icon, 0, this,
                                  SLOT(slotR64th()),
                                  actionCollection(), "64th_rest" );
    noteAction->setExclusiveGroup("notes");
    

    // setup edit menu
    KStdAction::undo    (this, SLOT(slotEditUndo()),       actionCollection());
    KStdAction::redo    (this, SLOT(slotEditRedo()),       actionCollection());
    KStdAction::cut     (this, SLOT(slotEditCut()),        actionCollection());
    KStdAction::copy    (this, SLOT(slotEditCopy()),       actionCollection());
    KStdAction::paste   (this, SLOT(slotEditPaste()),      actionCollection());

    // setup Settings menu
    KStdAction::showToolbar
        (this, SLOT(slotToggleToolBar()), actionCollection());
    KStdAction::showStatusbar
        (this, SLOT(slotToggleStatusBar()), actionCollection());

    KStdAction::saveOptions
        (this, SLOT(save_options()), actionCollection());
    KStdAction::preferences
        (this, SLOT(customize()), actionCollection());

    KStdAction::keyBindings 
        (this, SLOT(editKeys()), actionCollection());
    KStdAction::configureToolbars
        (this, SLOT(editToolbars()), actionCollection());

    createGUI("notation.rc");
}


void NotationView::initStatusBar()
{
    KStatusBar* sb = statusBar();

    sb->insertItem(i18n(IDS_STATUS_DEFAULT), ID_STATUS_MSG);
    m_currentNotePixmap = new QLabel(sb);
    m_hoveredOverNoteName = new QLabel(sb);
    
    sb->addWidget(m_currentNotePixmap);
    sb->addWidget(m_hoveredOverNoteName);
}


bool
NotationView::showElements(NotationElementList::iterator from,
                           NotationElementList::iterator to)
{
    return showElements(from, to, 0, 0);
}

bool
NotationView::showElements(NotationElementList::iterator from,
                           NotationElementList::iterator to,
                           QCanvasItem *item)
{
    return showElements(from, to, item->x(), item->y());
}

bool
NotationView::showElements(NotationElementList::iterator from,
                           NotationElementList::iterator to,
                           double dxoffset, double dyoffset)
{
    kdDebug(KDEBUG_AREA) << "NotationView::showElements()" << endl;

    if (from == to) return true;

//    static ChordPixmapFactory npf(*m_mainStaff);
    // let's revert to this for now
    NotePixmapFactory &npf(m_notePixmapFactory);
    Clef currentClef; // default is okay to start with

    for (NotationElementList::iterator it = from; it != to; ++it) {

        //
        // process event
        //
        try {

            QCanvasSimpleSprite *sprite = 0;

            if ((*it)->isNote()) {

                Note::Type note = (*it)->event()->get<Int>(P_NOTE_TYPE);
                int dots = (*it)->event()->get<Int>(P_NOTE_DOTS);

                Accidental accidental = NoAccidental;

                long acc;
                if ((*it)->event()->get<Int>(P_DISPLAY_ACCIDENTAL, acc)) {
                    accidental = Accidental(acc);
                }

                bool up = true;
                (void)((*it)->event()->get<Bool>(P_STALK_UP, up));

                bool tail = true;
                (void)((*it)->event()->get<Bool>(P_DRAW_TAIL, tail));

//		kdDebug(KDEBUG_AREA) << "NotationView::showElements(): found a note of type " << note << " with accidental " << accidental << endl;
                
                bool beamed = false;
                (void)((*it)->event()->get<Bool>(P_BEAMED, beamed));

		if (beamed) {

		    int stemLength = npf.getNoteBodyHeight();

		    if ((*it)->event()->get<Bool>(P_BEAM_PRIMARY_NOTE)) {

			int myY = (*it)->event()->get<Int>(P_BEAM_MY_Y);
//			int nextY = (*it)->event()->get<Int>(P_BEAM_NEXT_Y);
//			int dx = (*it)->event()->get<Int>(P_BEAM_SECTION_WIDTH);

//                        kdDebug(KDEBUG_AREA) << "NotationView::showElements(): should be drawing a beam here... event is " << *(*it)->event() << endl;

			stemLength = myY - (int)(*it)->getLayoutY();
			if (stemLength < 0) stemLength = -stemLength;

			int nextTailCount =
			    (*it)->event()->get<Int>(P_BEAM_NEXT_TAIL_COUNT);
			int width =
			    (*it)->event()->get<Int>(P_BEAM_SECTION_WIDTH);
			int gradient =
			    (*it)->event()->get<Int>(P_BEAM_GRADIENT);

                        bool thisPartialTails(false), nextPartialTails(false);
                        (void)(*it)->event()->get<Bool>
                            (P_BEAM_THIS_PART_TAILS, thisPartialTails);
                        (void)(*it)->event()->get<Bool>
                            (P_BEAM_NEXT_PART_TAILS, nextPartialTails);

			QCanvasPixmap notePixmap
			    (npf.makeBeamedNotePixmap
			     (note, dots, accidental, up, stemLength,
			      nextTailCount, thisPartialTails, nextPartialTails,
                              width, (double)gradient / 100.0));
			sprite = new QCanvasSimpleSprite(&notePixmap, canvas());

		    } else {

			QCanvasPixmap notePixmap
			    (npf.makeNotePixmap
			     (note, dots, accidental, tail, up));
			sprite = new QCanvasSimpleSprite(&notePixmap, canvas());
		    }

		
		} else {

		    QCanvasPixmap notePixmap
			(npf.makeNotePixmap(note, dots, accidental, tail, up));

		    sprite = new QCanvasSimpleSprite(&notePixmap, canvas());
		}

            } else if ((*it)->isRest()) {

                Note::Type note = (*it)->event()->get<Int>(P_NOTE_TYPE);
                int dots = (*it)->event()->get<Int>(P_NOTE_DOTS);

                QCanvasPixmap notePixmap(npf.makeRestPixmap(note, dots));
                sprite = new QCanvasSimpleSprite(&notePixmap, canvas());

            } else if ((*it)->event()->isa(Clef::EventType)) {

		currentClef = Clef(*(*it)->event());
                QCanvasPixmap clefPixmap(npf.makeClefPixmap(currentClef));
                sprite = new QCanvasSimpleSprite(&clefPixmap, canvas());

            } else if ((*it)->event()->isa(Rosegarden::Key::EventType)) {

                QCanvasPixmap keyPixmap
                    (npf.makeKeyPixmap
                     (Rosegarden::Key((*it)->event()->get<String>
			  (Rosegarden::Key::KeyPropertyName)),
                      currentClef));
                sprite = new QCanvasSimpleSprite(&keyPixmap, canvas());

            } else if ((*it)->event()->isa(TimeSignature::EventType)) {

                QCanvasPixmap timeSigPixmap
                    (npf.makeTimeSigPixmap(TimeSignature(*(*it)->event())));
                sprite = new QCanvasSimpleSprite(&timeSigPixmap, canvas());

            } else {
                    
                kdDebug(KDEBUG_AREA) << "NotationElement of unrecognised type "
                                     << (*it)->event()->getType()
                                     << endl;
                QCanvasPixmap unknownPixmap(npf.makeUnknownPixmap());
                sprite = new QCanvasSimpleSprite(&unknownPixmap, canvas());
            }
                
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

bool
NotationView::showBars(NotationElementList::iterator from,
                       NotationElementList::iterator to)
{
    if (from == to) return true;

    const NotationHLayout::BarPositions& barPos(m_hlayout->getBarPositions());
    NotationElementList::iterator lastElement = to;
    --lastElement;

//     kdDebug(KDEBUG_AREA) << "NotationView::showBars() : from->x = " <<(*from)->x()
//                          << " - lastElement->x = " << (*lastElement)->x() << endl
//                          << "lastElement : " << *(*lastElement) << endl;
    
    m_currentStaff->deleteBars((*from)->getEffectiveX());
        
    for (NotationHLayout::BarPositions::const_iterator it = barPos.begin();
        it != barPos.end(); ++it) {

        kdDebug(KDEBUG_AREA) << "Adding bar at pos " << it->x << endl;
        m_currentStaff->insertBar(it->x, it->correct);
    }
    
    return true;
}


bool
NotationView::applyLayout()
{
    bool rcp = applyHorizontalPreparse();
    bool rcv = applyVerticalLayout();
    bool rch = applyHorizontalLayout();

    kdDebug(KDEBUG_AREA) << "NotationView::applyLayout() : done" << endl;

    return rcp && rch && rcv;
}


bool
NotationView::applyHorizontalPreparse()
{
    if (!m_hlayout) {
        KMessageBox::error(0, "No Horizontal Layout engine");
        return false;
    }

    m_hlayout->reset();
    m_hlayout->preparse(m_notationElements->begin(), m_notationElements->end());

    kdDebug(KDEBUG_AREA) << "NotationView::applyHorizontalPreparse() : done" << endl;

    return m_hlayout->status() == 0;
}


bool
NotationView::applyHorizontalLayout()
{
    if (!m_hlayout) {
        KMessageBox::error(0, "No Horizontal Layout engine");
        return false;
    }

    m_hlayout->layout();

    kdDebug(KDEBUG_AREA) << "NotationView::applyHorizontalLayout() : done" << endl;

    return m_hlayout->status() == 0;
}


bool 
NotationView::applyVerticalLayout()
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


void
NotationView::setCurrentSelectedNote(bool rest, Note::Type n)
{
    m_currentSelectedNoteIsRest = rest;
    m_currentSelectedNoteType = n;

    if (!rest) {
        m_currentNotePixmap->setPixmap
            (m_toolbarNotePixmapFactory.makeNotePixmap
             (n, m_currentSelectedNoteDotted ? 1 : 0, NoAccidental, true, true, true));
    } else {
        m_currentNotePixmap->setPixmap
            (m_toolbarNotePixmapFactory.makeRestPixmap
             (n, m_currentSelectedNoteDotted ? 1 : 0));
    }

    emit changeCurrentNote(rest, n);
}


//////////////////////////////////////////////////////////////////////
//                    Slots
//////////////////////////////////////////////////////////////////////

void
NotationView::slotEditUndo()
{
    slotStatusMsg(i18n("Undo..."));

    slotStatusMsg(i18n(IDS_STATUS_DEFAULT));
}

void
NotationView::slotEditRedo()
{
    slotStatusMsg(i18n("Redo..."));

    slotStatusMsg(i18n(IDS_STATUS_DEFAULT));
}

void
NotationView::slotEditCut()
{
    slotStatusMsg(i18n("Cutting selection..."));

    slotStatusMsg(i18n(IDS_STATUS_DEFAULT));
}

void
NotationView::slotEditCopy()
{
    slotStatusMsg(i18n("Copying selection to clipboard..."));

    slotStatusMsg(i18n(IDS_STATUS_DEFAULT));
}

void
NotationView::slotEditPaste()
{
    slotStatusMsg(i18n("Inserting clipboard contents..."));

    slotStatusMsg(i18n(IDS_STATUS_DEFAULT));
}

void
NotationView::slotToggleToolBar()
{
    slotStatusMsg(i18n("Toggle the toolbar..."));

    if (toolBar()->isVisible())
        toolBar()->hide();
    else
        toolBar()->show();

    slotStatusMsg(i18n(IDS_STATUS_DEFAULT));
}

void
NotationView::slotToggleStatusBar()
{
    slotStatusMsg(i18n("Toggle the statusbar..."));

    if (statusBar()->isVisible())
        statusBar()->hide();
    else
        statusBar()->show();

    slotStatusMsg(i18n(IDS_STATUS_DEFAULT));
}


void
NotationView::slotStatusMsg(const QString &text)
{
    ///////////////////////////////////////////////////////////////////
    // change status message permanently
    statusBar()->clear();
    statusBar()->changeItem(text, ID_STATUS_MSG);
}


void
NotationView::slotStatusHelpMsg(const QString &text)
{
    ///////////////////////////////////////////////////////////////////
    // change status message of whole statusbar temporary (text, msec)
    statusBar()->message(text, 2000);
}

//////////////////////////////////////////////////////////////////////

void
NotationView::slotBreve()
{
    kdDebug(KDEBUG_AREA) << "NotationView::slotBreve()\n";
    setCurrentSelectedNote(false, Note::Breve);
}

void
NotationView::slotWhole()
{
    kdDebug(KDEBUG_AREA) << "NotationView::slotWhole()\n";
    setCurrentSelectedNote(false, Note::WholeNote);
}

void
NotationView::slotHalf()
{
    kdDebug(KDEBUG_AREA) << "NotationView::slotHalf()\n";
    setCurrentSelectedNote(false, Note::HalfNote);
}

void
NotationView::slotQuarter()
{
    kdDebug(KDEBUG_AREA) << "NotationView::slotQuarter()\n";
    setCurrentSelectedNote(false, Note::QuarterNote);
}

void
NotationView::slot8th()
{
    kdDebug(KDEBUG_AREA) << "NotationView::slot8th()\n";
    setCurrentSelectedNote(false, Note::EighthNote);
}

void
NotationView::slot16th()
{
    kdDebug(KDEBUG_AREA) << "NotationView::slot16th()\n";
    setCurrentSelectedNote(false, Note::SixteenthNote);
}

void
NotationView::slot32nd()
{
    kdDebug(KDEBUG_AREA) << "NotationView::slot32nd()\n";
    setCurrentSelectedNote(false, Note::ThirtySecondNote);
}

void
NotationView::slot64th()
{
    kdDebug(KDEBUG_AREA) << "NotationView::slot64th()\n";
    setCurrentSelectedNote(false, Note::SixtyFourthNote);
}

void
NotationView::slotRBreve()
{
    kdDebug(KDEBUG_AREA) << "NotationView::slotRBreve()\n";
    setCurrentSelectedNote(true, Note::Breve);
}

void
NotationView::slotRWhole()
{
    kdDebug(KDEBUG_AREA) << "NotationView::slotRWhole()\n";
    setCurrentSelectedNote(true, Note::WholeNote);
}

void
NotationView::slotRHalf()
{
    kdDebug(KDEBUG_AREA) << "NotationView::slotRHalf()\n";
    setCurrentSelectedNote(true, Note::HalfNote);
}

void
NotationView::slotRQuarter()
{
    kdDebug(KDEBUG_AREA) << "NotationView::slotRQuarter()\n";
    setCurrentSelectedNote(true, Note::QuarterNote);
}

void
NotationView::slotR8th()
{
    kdDebug(KDEBUG_AREA) << "NotationView::slotR8th()\n";
    setCurrentSelectedNote(true, Note::EighthNote);
}

void
NotationView::slotR16th()
{
    kdDebug(KDEBUG_AREA) << "NotationView::slotR16th()\n";
    setCurrentSelectedNote(true, Note::SixteenthNote);
}

void
NotationView::slotR32nd()
{
    kdDebug(KDEBUG_AREA) << "NotationView::slotR32nd()\n";
    setCurrentSelectedNote(true, Note::ThirtySecondNote);
}

void
NotationView::slotR64th()
{
    kdDebug(KDEBUG_AREA) << "NotationView::slotR64th()\n";
    setCurrentSelectedNote(true, Note::SixtyFourthNote);
}

void
NotationView::insertNote(int height, const QPoint &eventPos)
{
    Rosegarden::Key key;
    Clef clef;
    NotationElementList::iterator closestNote =
        findClosestNote(eventPos.x(), clef, key);

    kdDebug(KDEBUG_AREA) << "NotationView::insertNote called; height is "
                         << height << endl;

    //!!! still need to take accidental into account
    int pitch = Rosegarden::NotationDisplayPitch(height, NoAccidental).
        getPerformancePitch(clef, key);

    kdDebug(KDEBUG_AREA) << "NotationView::insertNote called; pitch is "
                         << pitch << endl;

    if (closestNote == m_notationElements->end()) {
        return;
    }

    // create new event
    //
    Event *insertedEvent = new Event;

    if (m_currentSelectedNoteIsRest) {
        insertedEvent->setType("rest");
    } else {
        insertedEvent->setType(Note::EventType);
        insertedEvent->set<Int>("pitch", pitch);
    }

    NotationElement *newNotationElement =
        new NotationElement(insertedEvent);
    newNotationElement->setNote(Note(m_currentSelectedNoteType,
                                     m_currentSelectedNoteDotted ? 1 : 0));

    newNotationElement->event()->setMaybe<String>("Name", "INSERTED_NOTE");

    NotationElementList::iterator redoLayoutStart = closestNote;
    
    if ((*closestNote)->isRest()) {

        // replace rest (or part of it) with note
        //

        kdDebug(KDEBUG_AREA) << "NotationHLayout::insertNote : replacing rest with note"
                             << endl;

        if (!replaceRestWithNote(closestNote, newNotationElement))
            return;
        else
            --redoLayoutStart;
            
    } else { // it's a note : chord it

        //!!! This is wrong when inserting rests, of course -- sort it out

        // if closest note has the same pitch as the one we're
        // inserting, bail out
        if ( (*closestNote)->event()->get<Int>("pitch") == pitch ) {
            kdDebug(KDEBUG_AREA) << "NotationHLayout::insertNote : note is of same pitch - no insert\n";
            delete insertedEvent;
            delete newNotationElement;
            return;
        }

        kdDebug(KDEBUG_AREA) << "NotationHLayout::insertNote : insert over note - absoluteTime = "
                             << (*closestNote)->getAbsoluteTime()
                             << endl;

        /*
         * Alter inserted note's duration if needed
         */
        if ((*closestNote)->event()->getDuration() < insertedEvent->getDuration()) {
            // new note is being chorded with notes which are shorter
            // shorten its duration to same one as other notes
            newNotationElement->setNote((*closestNote)->getNote());

        } else if ((*closestNote)->event()->getDuration() > insertedEvent->getDuration()) {

            // new note is being chorded with notes which are longer
            // for the moment, do the same as in other case
            newNotationElement->setNote((*closestNote)->getNote());
            
        }

        newNotationElement->setAbsoluteTime((*closestNote)->getAbsoluteTime());
        // m_notationElements->insert(newNotationElement);

	//!!! should do this if we're between two events in the same
	//group, as well
	long groupNo;
	if ((*closestNote)->event()->get<Int>(P_GROUP_NO, groupNo)) {
	    newNotationElement->event()->set<Int>(P_GROUP_NO, groupNo);
	    newNotationElement->event()->set<String>
		(P_GROUP_TYPE,
		 (*closestNote)->event()->get<String>(P_GROUP_TYPE));
	}

        kdDebug(KDEBUG_AREA) << "new event is: " << (*newNotationElement) << endl;

        m_viewElementsManager->insert(newNotationElement);
            
    }

//     kdDebug(KDEBUG_AREA) << "NotationView::insertNote() : Elements before relayout : "
//                          << endl << *m_notationElements << endl;

    applyLayout(); // TODO : be more subtle than this

//     kdDebug(KDEBUG_AREA) << "NotationView::insertNote() : Elements after relayout : "
//                          << endl << *m_notationElements << endl;

    // (*m_hlayout)(notationElement);

    // TODO : m_currentStaff should be updated by the mouse click 

    if (redoLayoutStart != m_notationElements->begin())
        showElements(--redoLayoutStart,
                     m_notationElements->end(),
                     m_currentStaff);
    else
        showElements(m_notationElements->begin(),
                     m_notationElements->end(),
                     m_currentStaff);

    showBars(m_notationElements->begin(),
             m_notationElements->end());
}


NotationElementList::iterator
NotationView::findClosestNote(double eventX, Clef &clef, Rosegarden::Key &key)
{
    static const unsigned int proximityThreshold = 10; // in pixels

    double minDist = 10e9,
        prevDist = 10e9;

    NotationElementList::iterator it, res;

    // TODO: this is grossly inefficient
    //
    for (it = m_notationElements->begin();
         it != m_notationElements->end(); ++it) {

        if (!(*it)->isNote() && !(*it)->isRest()) {
            if ((*it)->event()->isa(Clef::EventType)) {
                kdDebug(KDEBUG_AREA) << "NotationView::findClosestNote() : found clef: type is "
                                     << (*it)->event()->get<String>(Clef::ClefPropertyName) << endl;
                clef = Clef(*(*it)->event());
            } else if ((*it)->event()->isa(Rosegarden::Key::EventType)) {
                kdDebug(KDEBUG_AREA) << "NotationView::findClosestNote() : found key: type is "
                                     << (*it)->event()->get<String>(Rosegarden::Key::KeyPropertyName) << endl;
                key = Rosegarden::Key(*(*it)->event());
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

bool
NotationView::replaceRestWithNote(NotationElementList::iterator rest,
                                  NotationElement *newNote)
{
    // if the new note's duration is longer than the rest it's
    // supposed to replace, set it to the rest's duration
    //
    if ((*rest)->event()->getDuration() < newNote->event()->getDuration()) {

        newNote->setNote(Note::getNearestNote((*rest)->event()->getDuration()));
    }

    bool newNoteIsSameDurationAsRest = (*rest)->event()->getDuration() == newNote->event()->getDuration();

    // set new note absolute time to the one of the rest it's replacing
    //
    newNote->setAbsoluteTime((*rest)->event()->getAbsoluteTime());

    if (!newNoteIsSameDurationAsRest) { // we need to insert shorter rests
        
        RestSplitter splitter((*rest)->event()->getDuration(),
                              newNote->event()->getDuration());

        Event::timeT restAbsoluteTime = newNote->event()->getAbsoluteTime() +
            newNote->event()->getDuration();
    
        while(Event::timeT bit = splitter.nextBit()) {
            kdDebug(KDEBUG_AREA) << "Inserting rest of duration " << bit
                                 << " at time " << restAbsoluteTime << endl;

            Event *newRest = new Event;
            newRest->setType("rest");
            newRest->setDuration(bit);
            newRest->setAbsoluteTime(restAbsoluteTime);
            newRest->setMaybe<String>("Name", "INSERTED_REST");
            NotationElement *newNotationRest = new NotationElement(newRest);
            restAbsoluteTime += bit;
            // m_notationElements->insert(newNotationRest);
            m_viewElementsManager->insert(newNotationRest);
        }
    }
    
//     m_notationElements->insert(newNote);
//     m_notationElements->erase(rest);

    m_viewElementsManager->insert(newNote);
    m_viewElementsManager->erase(rest);

    return true;
}

void
NotationView::hoveredOverNoteChanged(const QString &noteName)
{
    m_hoveredOverNoteName->setText(noteName);
}



//////////////////////////////////////////////////////////////////////


void
NotationView::perfTest()
{
    // perf test - add many many notes
    clock_t st, et;
    struct tms spare;
    st = times(&spare);


    std::cout << "Adding 1000 notes" << std::endl;
    setUpdatesEnabled(false);

    QCanvasPixmapArray *notePixmap = new QCanvasPixmapArray("pixmaps/note-bodyfilled.xpm");

    for(unsigned int x = 0; x < 1000; ++x) {
        for(unsigned int y = 0; y < 100; ++y) {


            QCanvasSprite *clef = new QCanvasSprite(notePixmap, canvas());

            clef->move(x * 10, y * 10);
        }
    }
    setUpdatesEnabled(true);

    std::cout << "Done adding 1000 notes" << std::endl;
    et = times(&spare);

    std::cout << (et-st)*10 << "ms" << std::endl;
}


#ifdef NOT_DEFINED

void
NotationView::test()
{
    //     QCanvasEllipse *t = new QCanvasEllipse(10, 10, canvas());
    //     t->setX(50);
    //     t->setY(50);

    //     QBrush brush(blue);
    //     t->setBrush(brush);

    Staff *staff = new Staff(canvas());
    staff->move(20, 15);
		
    staff = new Staff(canvas());
    staff->move(20, 130);
		
    // Add some notes

    QCanvasPixmapArray *notePixmap = new QCanvasPixmapArray("pixmaps/note-bodyfilled.xpm");

    for(unsigned int i = 0; i <= 8 /*! 17 */; ++i) {
        QCanvasSprite *note = new QCanvasSprite(notePixmap, canvas());
        note->move(20,14);
        note->moveBy(40 + i * 20, staff->yCoordOfHeight/*! pitchYOffset*/(i));
    }

    ChordPixmapFactory npf(*staff);

    for(unsigned int j = 0; j < 100; ++j) {

        for(unsigned int i = 0; i < 7; ++i) {

            QPixmap note(npf.makeNotePixmap((Note::Type)i, false,
                                            NoAccidental,
                                            true, true));

            QCanvasSimpleSprite *noteSprite = new QCanvasSimpleSprite(&note,
                                                                      canvas());

            noteSprite->move(50 + (i+j) * 20 , 100);

        }
    }
    
#if 0

    for(unsigned int i = 0; i < 7; ++i) {


        QPixmap note(npf.makeNotePixmap(i, false, true, false));

        QCanvasSprite *noteSprite = new QCanvasSimpleSpriteSprite(&note,
                                                                  canvas());

        noteSprite->move(50 + i * 20, 150);

    }
#endif

    ChordPitches pitches;
    pitches.push_back(6); // something wrong here - pitches aren't in the right order
    pitches.push_back(4);
    pitches.push_back(0);

    Accidentals accidentals;
    accidentals.push_back(NoAccidental);
    accidentals.push_back(Sharp);
    accidentals.push_back(NoAccidental);

    QPixmap chord(npf.makeChordPixmap(pitches, accidentals, 6, true, true, false));

    QCanvasSprite *chordSprite = new QCanvasSimpleSprite(&chord, canvas());

    chordSprite->move(50, 50);
   
}
#endif
