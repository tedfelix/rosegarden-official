// -*- c-basic-offset: 4 -*-

/*
    Rosegarden-4
    A sequencer and musical notation editor.

    This program is Copyright 2000-2004
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


#include <kconfig.h>
#include <kstdaction.h>
#include <kaction.h>
#include <kstddirs.h>
#include <kmessagebox.h>

#include <qvbox.h>
#include <qlayout.h>
#include <qtooltip.h>
#include <qlineedit.h>
#include <qspinbox.h>
#include <qlabel.h>
#include <qaccel.h>
#include <qiconset.h>

#include "triggermanager.h"
#include "rosegardenguidoc.h"
#include "rosedebug.h"
#include "rosestrings.h"
#include "editcommands.h"
#include "segmentcommands.h"
#include "widgets.h"
#include "midipitchlabel.h"

#include "CompositionTimeSliceAdapter.h"
#include "BaseProperties.h"
#include "Clipboard.h"
#include "TriggerSegment.h"

#include <set>

using Rosegarden::Composition;
using Rosegarden::CompositionTimeSliceAdapter;
using Rosegarden::TriggerSegmentId;
using Rosegarden::timeT;
using Rosegarden::Int;


class TriggerManagerItem : public QListViewItem
{
public:
    TriggerManagerItem(QListView * parent, QString label1, 
		      QString label2 = QString::null, 
		      QString label3 = QString::null,
		      QString label4 = QString::null, 
		      QString label5 = QString::null, 
		      QString label6 = QString::null, 
		      QString label7 = QString::null, 
		      QString label8 = QString::null):
        QListViewItem(parent, label1, label2, label3, label4,
                      label5, label6, label7, label8) { ; }

    virtual int compare(QListViewItem * i, int col, bool ascending) const;

    void setRawDuration(timeT raw) { m_rawDuration = raw; }
    timeT getRawDuration() const { return m_rawDuration; }

    void setId(TriggerSegmentId id) { m_id = id; }
    TriggerSegmentId getId() const { return m_id; }

    void setUsage(int usage) { m_usage = usage; }
    int getUsage() const { return m_usage; }

    void setPitch(int pitch) { m_pitch = pitch; }
    int getPitch() const { return m_pitch; }

protected:
    timeT m_rawDuration;
    TriggerSegmentId m_id;
    int m_usage;
    int m_pitch;
};

int
TriggerManagerItem::compare(QListViewItem * i, int col, bool ascending) const
{
    TriggerManagerItem *ei = 
        dynamic_cast<TriggerManagerItem *>(i);

    if (!ei) return QListViewItem::compare(i, col, ascending);

    // col 0 -> index -- numeric compare
    // col 1 -> ID -- numeric compare
    // col 2 -> label -- default string compare
    // col 3 -> duration -- raw duration compare
    // col 4 -> base pitch -- pitch compare
    // col 5 -> base velocity -- numeric compare
    // col 6 -> usage count -- numeric compare
    //
    if (col == 2) {  
        return QListViewItem::compare(i, col, ascending);
    } else if (col == 3) {
        if (m_rawDuration < ei->getRawDuration()) return -1;
        else if (ei->getRawDuration() < m_rawDuration) return 1;
        else return 0;
    } else if (col == 4) {
        if (m_pitch < ei->getPitch()) return -1;
        else if (ei->getPitch() < m_pitch) return 1;
        else return 0;
    } else {
        return key(col, ascending).toInt() - i->key(col, ascending).toInt();
    }
}


// -------- TriggerSegmentManager --------
//
//

TriggerSegmentManager::TriggerSegmentManager(QWidget *parent,
					     RosegardenGUIDoc *doc):
    KMainWindow(parent, "triggereditordialog"),
    m_doc(doc),
    m_modified(false)
{
    QVBox* mainFrame = new QVBox(this);
    setCentralWidget(mainFrame);

    setCaption(i18n("Manage Triggered Segments"));

    m_listView = new KListView(mainFrame);
    m_listView->addColumn("Index");
    m_listView->addColumn(i18n("ID"));
    m_listView->addColumn(i18n("Label"));
    m_listView->addColumn(i18n("Duration"));
    m_listView->addColumn(i18n("Base pitch"));
    m_listView->addColumn(i18n("Base velocity"));
    m_listView->addColumn(i18n("Triggers"));

    // Align centrally
    for (int i = 0; i < 2; ++i)
        m_listView->setColumnAlignment(i, Qt::AlignHCenter);

    QFrame* btnBox = new QFrame(mainFrame);

    btnBox->setSizePolicy(
            QSizePolicy(QSizePolicy::Minimum, QSizePolicy::Fixed));

    QHBoxLayout* layout = new QHBoxLayout(btnBox, 4, 10);

    m_addButton = new QPushButton(i18n("Add"), btnBox);
    m_deleteButton = new QPushButton(i18n("Delete"), btnBox);
    m_deleteAllButton = new QPushButton(i18n("Delete All"), btnBox);

    m_closeButton = new QPushButton(i18n("Close"), btnBox);

    QToolTip::add(m_addButton,
                  i18n("Add a Triggered Segment"));

    QToolTip::add(m_deleteButton,
                  i18n("Delete a Triggered Segment"));

    QToolTip::add(m_deleteAllButton,
                  i18n("Delete All Triggered Segments"));

    QToolTip::add(m_closeButton,
                  i18n("Close the Triggered Segment Manager"));

    layout->addStretch(10);
    layout->addWidget(m_addButton);
    layout->addWidget(m_deleteButton);
    layout->addWidget(m_deleteAllButton);
    layout->addSpacing(30);

    layout->addWidget(m_closeButton);
    layout->addSpacing(5);

    connect(m_addButton, SIGNAL(released()),
            SLOT(slotAdd()));

    connect(m_deleteButton, SIGNAL(released()),
            SLOT(slotDelete()));

    connect(m_closeButton, SIGNAL(released()),
            SLOT(slotClose()));

    connect(m_deleteAllButton, SIGNAL(released()),
            SLOT(slotDeleteAll()));

    setupActions();

    m_doc->getCommandHistory()->attachView(actionCollection());
    connect(m_doc->getCommandHistory(), SIGNAL(commandExecuted()),
            this, SLOT(slotUpdate()));

    connect(m_listView, SIGNAL(doubleClicked(QListViewItem *)),
            SLOT(slotEdit(QListViewItem *)));

    connect(m_listView, SIGNAL(pressed(QListViewItem *)),
            this, SLOT(slotItemClicked(QListViewItem *)));

    // Highlight all columns - enable extended selection mode
    //
    m_listView->setAllColumnsShowFocus(true);
    m_listView->setSelectionMode(QListView::Extended);
    m_listView->setItemsRenameable(true);

    initDialog();

    setAutoSaveSettings(TriggerManagerConfigGroup, true);

    m_accelerators = new QAccel(this);
}

TriggerSegmentManager::~TriggerSegmentManager()
{
    RG_DEBUG << "TriggerSegmentManager::~TriggerSegmentManager" << endl;

    m_listView->saveLayout(kapp->config(), TriggerManagerConfigGroup);

    if (m_doc)
        m_doc->getCommandHistory()->detachView(actionCollection());
}

void 
TriggerSegmentManager::initDialog()
{
    RG_DEBUG << "TriggerSegmentManager::initDialog" << endl;
    slotUpdate();
}

void
TriggerSegmentManager::slotUpdate()
{
    RG_DEBUG << "TriggerSegmentManager::slotUpdate" << endl;

    TriggerManagerItem *item;

    m_listView->clear();

    Composition &comp = m_doc->getComposition();

    const Composition::triggersegmentcontainer &triggers =
	comp.getTriggerSegments();

    Composition::triggersegmentcontainerconstiterator it;

    kapp->config()->setGroup(TriggerManagerConfigGroup);
    int timeMode = kapp->config()->readNumEntry("timemode", 0);

    int i = 0;

    for (it = triggers.begin(); it != triggers.end(); ++it) {

	// duration is as of first usage, or 0

	int uses = 0;
	timeT first = 0;
	std::set<int> tracks;

	CompositionTimeSliceAdapter tsa(&m_doc->getComposition());
	for (CompositionTimeSliceAdapter::iterator ci = tsa.begin();
	     ci != tsa.end(); ++ci) {
	    if ((*ci)->has(Rosegarden::BaseProperties::TRIGGER_SEGMENT_ID) &&
		(*ci)->get<Int>(Rosegarden::BaseProperties::TRIGGER_SEGMENT_ID) == (long)(*it)->getId()) {
		++uses;
		if (tracks.empty()) {
		    first = (*ci)->getAbsoluteTime();
		}
		tracks.insert(ci.getTrack());
	    }
	}

	timeT duration =
	    (*it)->getSegment()->getEndMarkerTime() -
	    (*it)->getSegment()->getStartTime();

        QString timeString = makeDurationString
	    (first, duration, timeMode);

	QString label = strtoqstr((*it)->getSegment()->getLabel());
	if (label == "") label = i18n("<no label>");

	QString used;
	if (tracks.size() == 1)
	    used = i18n("%1 on %2 track").arg(uses).arg(tracks.size());
	else
	    used = i18n("%1 on %2 tracks").arg(uses).arg(tracks.size());

	QString pitch = QString("%1 (%2)")
	    .arg(Rosegarden::MidiPitchLabel((*it)->getBasePitch()).getQString())
	    .arg((*it)->getBasePitch());

	QString velocity = QString("%1").arg((*it)->getBaseVelocity());

        item = new TriggerManagerItem
	    (m_listView, QString("%1").arg(i+1), QString("%1").arg((*it)->getId()),
	     label, timeString, pitch, velocity, used);

        item->setRawDuration(duration);
	item->setId((*it)->getId());
	item->setUsage(uses);
	item->setPitch((*it)->getBasePitch());

        m_listView->insertItem(item);
	++i;
    }

    if (m_listView->childCount() == 0)
    {
        QListViewItem *item = 
            new TriggerManagerItem(m_listView, i18n("<none>"));
        m_listView->insertItem(item);

        m_listView->setSelectionMode(QListView::NoSelection);
    }
    else
    {
        m_listView->setSelectionMode(QListView::Extended);
    }
}

void 
TriggerSegmentManager::slotDeleteAll()
{
    if (KMessageBox::warningContinueCancel(this, i18n("This will remove all triggered segments from the whole composition.  Are you sure?")) != KMessageBox::Continue) return;

    RG_DEBUG << "TriggerSegmentManager::slotDeleteAll" << endl;
    KMacroCommand *command = new KMacroCommand(i18n("Remove all triggered segments"));

    QListViewItem *it = m_listView->firstChild();

    do {

	TriggerManagerItem *item = 
	    dynamic_cast<TriggerManagerItem*>(it);

	if (!item) continue;

        DeleteTriggerSegmentCommand *c = 
            new DeleteTriggerSegmentCommand(m_doc,
					    item->getId());
        command->addCommand(c);

    } while((it = it->nextSibling()));

    addCommandToHistory(command);
}

void
TriggerSegmentManager::slotAdd()
{
    TimeDialog dialog(this, i18n("Trigger Segment Duration"),
		      &m_doc->getComposition(),
		      0, 3840);

    if (dialog.exec() == QDialog::Accepted) {
        addCommandToHistory(new AddTriggerSegmentCommand
			    (m_doc, dialog.getTime(), 64));
    }
}

void
TriggerSegmentManager::slotDelete()
{
    RG_DEBUG << "TriggerSegmentManager::slotDelete" << endl;

    TriggerManagerItem *item = 
        dynamic_cast<TriggerManagerItem*>(m_listView->currentItem());

    if (!item) return;

    if (item->getUsage() > 0) {
	if (KMessageBox::warningContinueCancel(this, i18n("This triggered segment is used %1 times in the current composition.  Are you sure you want to remove it?").arg(item->getUsage())) != KMessageBox::Continue) return;
    }

    DeleteTriggerSegmentCommand *command =
        new DeleteTriggerSegmentCommand(m_doc, item->getId());

    addCommandToHistory(command);
}

void
TriggerSegmentManager::slotPasteAsNew()
{
    Rosegarden::Clipboard *clipboard = m_doc->getClipboard();

    if (clipboard->isEmpty()) {
        KMessageBox::information(this, i18n("Clipboard is empty"));
        return;
    }

    addCommandToHistory(new PasteToTriggerSegmentCommand
			(&m_doc->getComposition(),
			 clipboard,
			 "",
			 -1));
}

void
TriggerSegmentManager::slotClose()
{
    RG_DEBUG << "TriggerSegmentManager::slotClose" << endl;

    if (m_doc) m_doc->getCommandHistory()->detachView(actionCollection());
    m_doc = 0;

    close();
}

void
TriggerSegmentManager::setupActions()
{
    KAction* close = KStdAction::close(this,
                                       SLOT(slotClose()),
                                       actionCollection());

    m_closeButton->setText(close->text());
    connect(m_closeButton, SIGNAL(released()), this, SLOT(slotClose()));

    QString pixmapDir = KGlobal::dirs()->findResource("appdata", "pixmaps/");

    // some adjustments
    new KToolBarPopupAction(i18n("Und&o"),
                            "undo",
                            KStdAccel::key(KStdAccel::Undo),
                            actionCollection(),
                            KStdAction::stdName(KStdAction::Undo));

    new KToolBarPopupAction(i18n("Re&do"),
                            "redo",
                            KStdAccel::key(KStdAccel::Redo),
                            actionCollection(),
                            KStdAction::stdName(KStdAction::Redo));

    new KAction(i18n("Pa&ste as New Triggered Segment"), CTRL + SHIFT + Key_V, this,
		SLOT(slotPasteAsNew()), actionCollection(),
		"paste_to_trigger_segment");

    kapp->config()->setGroup(TriggerManagerConfigGroup);
    int timeMode = kapp->config()->readNumEntry("timemode", 0);

    KRadioAction *action;

    QIconSet icon(QCanvasPixmap(pixmapDir + "/toolbar/time-musical.xpm"));

    action = new KRadioAction(i18n("&Musical Times"), icon, 0, this,
                              SLOT(slotMusicalTime()),
                              actionCollection(), "time_musical");
    action->setExclusiveGroup("timeMode");
    if (timeMode == 0) action->setChecked(true);

    icon = QIconSet(QCanvasPixmap(pixmapDir + "/toolbar/time-real.xpm"));

    action = new KRadioAction(i18n("&Real Times"), icon, 0, this,
                              SLOT(slotRealTime()),
                              actionCollection(), "time_real");
    action->setExclusiveGroup("timeMode");
    if (timeMode == 1) action->setChecked(true);

    icon = QIconSet(QCanvasPixmap(pixmapDir + "/toolbar/time-raw.xpm"));

    action = new KRadioAction(i18n("Ra&w Times"), icon, 0, this,
                              SLOT(slotRawTime()),
                              actionCollection(), "time_raw");
    action->setExclusiveGroup("timeMode");
    if (timeMode == 2) action->setChecked(true);

    createGUI("triggermanager.rc");
}

void 
TriggerSegmentManager::addCommandToHistory(KCommand *command)
{
    getCommandHistory()->addCommand(command);
    setModified(false);
}

MultiViewCommandHistory* 
TriggerSegmentManager::getCommandHistory()
{
    return m_doc->getCommandHistory();
}


void
TriggerSegmentManager::setModified(bool modified)
{
    RG_DEBUG << "TriggerSegmentManager::setModified(" << modified << ")" << endl;

    m_modified = modified;
}

void
TriggerSegmentManager::checkModified()
{
    RG_DEBUG << "TriggerSegmentManager::checkModified(" << m_modified << ")" 
             << endl;

}

void
TriggerSegmentManager::slotEdit(QListViewItem *i)
{
    RG_DEBUG << "TriggerSegmentManager::slotEdit" << endl;

    TriggerManagerItem *item = 
        dynamic_cast<TriggerManagerItem*>(i);

    if (!item) return;

    TriggerSegmentId id = item->getId();

    RG_DEBUG << "id is " << id << endl;

    emit editTriggerSegment(id);
}

void
TriggerSegmentManager::closeEvent(QCloseEvent *e)
{
    emit closing();
    KMainWindow::closeEvent(e);
}

// Reset the document
//
void
TriggerSegmentManager::setDocument(RosegardenGUIDoc *doc)
{
    // reset our pointers
    m_doc = doc;
    m_modified = false;

    slotUpdate();
}

void
TriggerSegmentManager::slotItemClicked(QListViewItem *item)
{
    RG_DEBUG << "TriggerSegmentManager::slotItemClicked" << endl;
}


QString
TriggerSegmentManager::makeDurationString(Rosegarden::timeT time,
					  Rosegarden::timeT duration, int timeMode)
{
    //!!! duplication with EventView::makeDurationString -- merge somewhere?

    switch (timeMode) {

    case 0: // musical time
    {
	int bar, beat, fraction, remainder;
	m_doc->getComposition().getMusicalTimeForDuration
	    (time, duration, bar, beat, fraction, remainder);
	return QString("%1%2%3-%4%5-%6%7-%8%9   ")
	    .arg(bar/100)
	    .arg((bar%100)/10)
	    .arg(bar%10)
	    .arg(beat/10)
	    .arg(beat%10)
	    .arg(fraction/10)
	    .arg(fraction%10)
	    .arg(remainder/10)
	    .arg(remainder%10);
    }

    case 1: // real time
    {
	Rosegarden::RealTime rt =
	    m_doc->getComposition().getRealTimeDifference
	    (time, time + duration);
	return QString("%1  ").arg(rt.toString().c_str());
    }

    default:
	return QString("%1  ").arg(duration);
    }
}

void
TriggerSegmentManager::slotMusicalTime()
{
    kapp->config()->setGroup(TriggerManagerConfigGroup);
    kapp->config()->writeEntry("timemode", 0);
    slotUpdate();
}

void
TriggerSegmentManager::slotRealTime()
{
    kapp->config()->setGroup(TriggerManagerConfigGroup);
    kapp->config()->writeEntry("timemode", 1);
    slotUpdate();
}

void
TriggerSegmentManager::slotRawTime()
{
    kapp->config()->setGroup(TriggerManagerConfigGroup);
    kapp->config()->writeEntry("timemode", 2);
    slotUpdate();
}



const char* const TriggerSegmentManager::TriggerManagerConfigGroup = "Trigger Editor";
