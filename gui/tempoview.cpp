// -*- c-basic-offset: 4 -*-

/*
    Rosegarden-4
    A sequencer and musical notation editor.

    This program is Copyright 2000-2005
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

#include <klocale.h>
#include <kconfig.h>
#include <kstatusbar.h>
#include <ktoolbar.h>
#include <ktmpstatusmsg.h>
#include <kstdaction.h> 
#include <kaction.h>
#include <kglobal.h>
#include <kstddirs.h>

#include <qvbox.h>
#include <qlayout.h>
#include <qpushbutton.h>
#include <qlabel.h>
#include <qcheckbox.h>
#include <qbuttongroup.h>
#include <qiconset.h>

#include <klistview.h>

#include "editviewbase.h"
#include "tempoview.h"
#include "rosegardenguidoc.h"
#include "rosestrings.h"
#include "dialogs.h"
#include "segmentcommands.h"
#include "rosedebug.h"

#include "Composition.h"


class TempoListItem : public KListViewItem
{
public:
    enum Type { TimeSignature, Tempo };

    TempoListItem(Rosegarden::Composition *composition,
		  Type type,
		  Rosegarden::timeT time,
		  int index,
		  KListView *parent,
		  QString label1,
		  QString label2,
		  QString label3,
		  QString label4 = QString::null) :
	KListViewItem(parent, label1, label2, label3, label4),
	m_composition(composition),
	m_type(type),
	m_time(time),
	m_index(index) { }

    Rosegarden::Composition *getComposition() { return m_composition; }
    Type getType() const { return m_type; }
    Rosegarden::timeT getTime() const { return m_time; }
    int getIndex() const { return m_index; }

    virtual int compare(QListViewItem *i, int col, bool ascending) const;

protected:
    Rosegarden::Composition *m_composition;
    Type m_type;
    Rosegarden::timeT m_time;
    int m_index;
};


int
TempoListItem::compare(QListViewItem *i, int col, bool ascending) const
{
    TempoListItem *ti = dynamic_cast<TempoListItem *>(i);
    if (!ti) return QListViewItem::compare(i, col, ascending);

    if (col == 0) { // time
	if (m_time == ti->m_time) {
	    return int(m_type) - int(ti->m_type);
	} else {
	    return int(m_time - ti->m_time);
	}
    } else if (col == 1) { // type
	if (m_type == ti->m_type) {
	    return int(m_time - ti->m_time);
	} else {
	    return int(m_type) - int(ti->m_type);
	}
    } else {
	return key(col, ascending).compare(i->key(col, ascending));
    }
}

int
TempoView::m_lastSetFilter = -1;

TempoView::TempoView(RosegardenGUIDoc *doc, QWidget *parent, Rosegarden::timeT openTime):
    EditViewBase(doc, std::vector<Rosegarden::Segment *>(), 2, parent, "tempoview"),
    m_filter(Tempo | TimeSignature),
    m_ignoreUpdates(true)
{
    if (m_lastSetFilter < 0) m_lastSetFilter = m_filter;
    else m_filter = m_lastSetFilter;

    initStatusBar();
    setupActions();

    // define some note filtering buttons in a group
    //
    m_filterGroup =
        new QButtonGroup(1, Horizontal, i18n("Filter"), getCentralWidget());

    m_tempoCheckBox = new QCheckBox(i18n("Tempo"), m_filterGroup);
    m_timeSigCheckBox = new QCheckBox(i18n("Time Signature"), m_filterGroup);
    m_grid->addWidget(m_filterGroup, 2, 0);

    // Connect up
    //
    connect(m_filterGroup, SIGNAL(released(int)),
            SLOT(slotModifyFilter(int)));

    m_list = new KListView(getCentralWidget());
    m_list->setItemsRenameable(true);

    m_grid->addWidget(m_list, 2, 1);

    updateViewCaption();

    doc->getComposition().addObserver(this);

    // Connect double clicker
    //
    connect(m_list, SIGNAL(doubleClicked(QListViewItem*)),
            SLOT(slotPopupEditor(QListViewItem*)));

    m_list->setAllColumnsShowFocus(true);
    m_list->setSelectionMode(QListView::Extended);

    m_list->addColumn(i18n("Time  "));
    m_list->addColumn(i18n("Type  "));
    m_list->addColumn(i18n("Value  "));
    m_list->addColumn(i18n("Properties  "));

    for(int col = 0; col < m_list->columns(); ++col)
   	m_list->setRenameable(col, true);

    readOptions();
    setButtonsToFilter();
    applyLayout();

    makeInitialSelection(openTime);

    m_ignoreUpdates = false;
    setOutOfCtor();
}

TempoView::~TempoView()
{
    if (!getDocument()->isBeingDestroyed()) {
	getDocument()->getComposition().removeObserver(this);
    }
}

void
TempoView::tempoChanged(const Rosegarden::Composition *comp)
{
    if (m_ignoreUpdates) return;
    if (comp == &getDocument()->getComposition()) {
	applyLayout();
    }
}

void
TempoView::timeSignatureChanged(const Rosegarden::Composition *comp)
{
    if (m_ignoreUpdates) return;
    if (comp == &getDocument()->getComposition()) {
	applyLayout();
    }
}

bool
TempoView::applyLayout(int /*staffNo*/)
{
    // If no selection has already been set then we copy what's
    // already set and try to replicate this after the rebuild
    // of the view.  This code borrowed from EventView.
    //
    if (m_listSelection.size() == 0)
    {
        QPtrList<QListViewItem> selection = m_list->selectedItems();

        if (selection.count())
        {
            QPtrListIterator<QListViewItem> it(selection);
            QListViewItem *listItem;

            while((listItem = it.current()) != 0)
            {
                m_listSelection.push_back(m_list->itemIndex(*it));
                ++it;
            }
        }
    }

    // Ok, recreate list
    //
    m_list->clear();

    Rosegarden::Composition *comp = &getDocument()->getComposition();

    m_config->setGroup(ConfigGroup);
    int timeMode = m_config->readNumEntry("timemode", 0);

    if (m_filter & TimeSignature) {
	for (int i = 0; i < comp->getTimeSignatureCount(); ++i) {

	    std::pair<Rosegarden::timeT, Rosegarden::TimeSignature> sig =
		comp->getTimeSignatureChange(i);

	    QString properties;
	    if (sig.second.isHidden()) {
		if (sig.second.isCommon()) properties = i18n("Common, hidden");
		else properties = i18n("Hidden");
	    } else {
		if (sig.second.isCommon()) properties = i18n("Common");
	    }

	    QString timeString = makeTimeString(sig.first, timeMode);

	    new TempoListItem(comp, TempoListItem::TimeSignature, 
			      sig.first, i, m_list, timeString,
			      i18n("Time Signature   "),
			      QString("%1/%2   ").arg(sig.second.getNumerator()).
			      arg(sig.second.getDenominator()),
			      properties);
	}
    }

    if (m_filter & Tempo) {
	for (int i = 0; i < comp->getTempoChangeCount(); ++i) {

	    std::pair<Rosegarden::timeT, long> tempo = comp->getRawTempoChange(i);
	
	    QString desc;

	    float qpm = float(tempo.second) / 60.0;
	    int qpmUnits = int(qpm);
	    int qpmTenths = int((qpm - qpmUnits) * 10);
	    int qpmHundredths = int((qpm - qpmUnits - qpmTenths/10.0) * 100);

	    Rosegarden::TimeSignature sig = comp->getTimeSignatureAt(tempo.first);
	    if (sig.getBeatDuration() ==
		Rosegarden::Note(Rosegarden::Note::Crotchet).getDuration()) {
		desc = i18n("%1.%2%3").
		    arg(qpmUnits).arg(qpmTenths).arg(qpmHundredths);
	    } else {
		float bpm = (float(tempo.second) *
			     Rosegarden::Note(Rosegarden::Note::Crotchet).getDuration()) /
		    (sig.getBeatDuration() * 60.0);
		int bpmUnits = int(bpm);
		int bpmTenths = int((bpm - bpmUnits) * 10);
		int bpmHundredths = int((bpm - bpmUnits - bpmTenths/10.0) * 100);

		desc = i18n("%1.%2%3 qpm (%4.%5%6 bpm)   ").
		    arg(qpmUnits).arg(qpmTenths).arg(qpmHundredths).
		    arg(bpmUnits).arg(bpmTenths).arg(bpmHundredths);
	    }

	    QString timeString = makeTimeString(tempo.first, timeMode);

	    new TempoListItem(comp, TempoListItem::Tempo,
			      tempo.first, i, m_list, timeString,
			      i18n("Tempo   "),
			      desc);
	}
    }

    if (m_list->childCount() == 0)
    {
	new QListViewItem(m_list,
                          i18n("<nothing at this filter level>"));
        m_list->setSelectionMode(QListView::NoSelection);
	stateChanged("have_selection", KXMLGUIClient::StateReverse);
    }
    else
    {
        m_list->setSelectionMode(QListView::Extended);

        // If no selection then select the first event
        if (m_listSelection.size() == 0)
            m_listSelection.push_back(0);
	stateChanged("have_selection", KXMLGUIClient::StateNoReverse);
    }

    // Set a selection from a range of indexes
    //
    std::vector<int>::iterator sIt = m_listSelection.begin();
    int index = 0;

    for (; sIt != m_listSelection.end(); ++sIt)
    {
        index = *sIt;

        while (index > 0 && !m_list->itemAtIndex(index))
                index--;

        m_list->setSelected(m_list->itemAtIndex(index), true);
        m_list->setCurrentItem(m_list->itemAtIndex(index));

        // ensure visible
        m_list->ensureItemVisible(m_list->itemAtIndex(index));
    }

    m_listSelection.clear();

    return true;
}

void
TempoView::makeInitialSelection(Rosegarden::timeT time)
{
    m_listSelection.clear();

    TempoListItem *goodItem = 0;
    int goodItemNo = 0;

    for (int i = 0; m_list->itemAtIndex(i); ++i) {

	TempoListItem *item = dynamic_cast<TempoListItem *>
	    (m_list->itemAtIndex(i));

	m_list->setSelected(item, false);

	if (item) {
	    if (item->getTime() > time) break;
	    goodItem = item;
	    goodItemNo = i;
	}
    }

    if (goodItem) {
	m_listSelection.push_back(goodItemNo);
	m_list->setSelected(goodItem, true);
	m_list->ensureItemVisible(goodItem);
    }
}

Rosegarden::Segment *
TempoView::getCurrentSegment()
{
    if (m_segments.empty()) return 0;
    else return *m_segments.begin();
}

QString
TempoView::makeTimeString(Rosegarden::timeT time, int timeMode)
{
    switch (timeMode) {

    case 0: // musical time
    {
	int bar, beat, fraction, remainder;
	getDocument()->getComposition().getMusicalTimeForAbsoluteTime
	    (time, bar, beat, fraction, remainder);
	++bar;
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
	    getDocument()->getComposition().getElapsedRealTime(time);
	return QString("%1   ").arg(rt.toString().c_str());
    }

    default:
	return QString("%1   ").arg(time);
    }
}

void
TempoView::refreshSegment(Rosegarden::Segment * /*segment*/,
                          Rosegarden::timeT /*startTime*/,
                          Rosegarden::timeT /*endTime*/)
{
    RG_DEBUG << "TempoView::refreshSegment" << endl;
    applyLayout(0);
}

void
TempoView::updateView()
{
    m_list->update();
}

void
TempoView::slotEditCut()
{
    // not implemented yet -- can't use traditional clipboard (which
    // only holds events from segments, or segments)
}

void
TempoView::slotEditCopy()
{
    // likewise
}

void
TempoView::slotEditPaste()
{
    // likewise
}

void
TempoView::slotEditDelete()
{
    QPtrList<QListViewItem> selection = m_list->selectedItems();
    if (selection.count() == 0) return;

    RG_DEBUG << "TempoView::slotEditDelete - deleting "
             << selection.count() << " items" << endl;

    QPtrListIterator<QListViewItem> it(selection);
    QListViewItem *listItem;
    TempoListItem *item;
    int itemIndex = -1;

    m_ignoreUpdates = true;
    bool haveSomething = false;

    // We want the Remove commands to be in reverse order, because
    // removing one item by index will affect the indices of
    // subsequent items.  So we'll stack them onto here and then pull
    // them off again.
    std::vector<KCommand *> commands;

    while ((listItem = it.current()) != 0)
    {
        item = dynamic_cast<TempoListItem*>((*it));

        if (itemIndex == -1) itemIndex = m_list->itemIndex(*it);

        if (item)
        {
	    if (item->getType() == TempoListItem::TimeSignature) {
		commands.push_back(new RemoveTimeSignatureCommand
				   (item->getComposition(),
				    item->getIndex()));
		haveSomething = true;
	    } else {
		commands.push_back(new RemoveTempoChangeCommand
				   (item->getComposition(),
				    item->getIndex()));
		haveSomething = true;
	    }		
        }
        ++it;
    }

    if (haveSomething) {
	KMacroCommand *command = new KMacroCommand
	    (i18n("Delete Tempo or Time Signature"));
	for (std::vector<KCommand *>::iterator i = commands.end();
	     i != commands.begin();) {
	    command->addCommand(*--i);
	}
	addCommandToHistory(command);
    }

    applyLayout();
    m_ignoreUpdates = false;
}

void
TempoView::slotEditInsertTempo()
{
    Rosegarden::timeT insertTime = 0;
    QPtrList<QListViewItem> selection = m_list->selectedItems();

    if (selection.count() > 0) {
        TempoListItem *item =
            dynamic_cast<TempoListItem*>(selection.getFirst());
        if (item) insertTime = item->getTime();
    }

    TempoDialog dialog(this, getDocument(), true);
    dialog.setTempoPosition(insertTime);

    connect(&dialog, 
	    SIGNAL(changeTempo(Rosegarden::timeT,
                               double, TempoDialog::TempoDialogAction)),
	    this,
            SIGNAL(changeTempo(Rosegarden::timeT,
			       double, TempoDialog::TempoDialogAction)));

    dialog.exec();
}

void
TempoView::slotEditInsertTimeSignature()
{
    Rosegarden::timeT insertTime = 0;
    QPtrList<QListViewItem> selection = m_list->selectedItems();

    if (selection.count() > 0) {
        TempoListItem *item =
            dynamic_cast<TempoListItem*>(selection.getFirst());
        if (item) insertTime = item->getTime();
    }

    Rosegarden::Composition &composition(m_doc->getComposition());
    Rosegarden::TimeSignature sig = composition.getTimeSignatureAt(insertTime);

    TimeSignatureDialog dialog(this, &composition, insertTime, sig, true);

    if (dialog.exec() == QDialog::Accepted) {

	insertTime = dialog.getTime();

        if (dialog.shouldNormalizeRests()) {
	    addCommandToHistory
                (new AddTimeSignatureAndNormalizeCommand
                 (&composition, insertTime, dialog.getTimeSignature()));
        } else { 
	    addCommandToHistory
                (new AddTimeSignatureCommand
                 (&composition, insertTime, dialog.getTimeSignature()));
        }
    }
}

void
TempoView::slotEdit()
{
    RG_DEBUG << "TempoView::slotEdit" << endl;

    QPtrList<QListViewItem> selection = m_list->selectedItems();

    if (selection.count() > 0)
    {
        TempoListItem *item =
            dynamic_cast<TempoListItem*>(selection.getFirst());
        if (item) slotPopupEditor(item);
    }
}

void
TempoView::slotSelectAll()
{
    m_listSelection.clear();
    for (int i = 0; m_list->itemAtIndex(i); ++i) {
	m_listSelection.push_back(i);
	m_list->setSelected(m_list->itemAtIndex(i), true);
    }
}

void
TempoView::slotClearSelection()
{
    m_listSelection.clear();
    for (int i = 0; m_list->itemAtIndex(i); ++i) {
	m_list->setSelected(m_list->itemAtIndex(i), false);
    }
}


void
TempoView::setupActions()
{
    EditViewBase::setupActions("tempoview.rc", false);
    
    QString pixmapDir = KGlobal::dirs()->findResource("appdata", "pixmaps/");
    QIconSet icon(QPixmap(pixmapDir + "/toolbar/event-insert-tempo.xpm"));
    
    new KAction(AddTempoChangeCommand::getGlobalName(), icon, Key_I, this,
                SLOT(slotEditInsertTempo()), actionCollection(),
                "insert_tempo");

    QCanvasPixmap pixmap(pixmapDir + "/toolbar/event-insert-timesig.xpm");
    icon = QIconSet(pixmap);
    
    new KAction(AddTimeSignatureCommand::getGlobalName(), icon, Key_G, this,
                SLOT(slotEditInsertTimeSignature()), actionCollection(),
                "insert_timesig");

    pixmap.load(pixmapDir + "/toolbar/event-delete.xpm");
    icon = QIconSet(pixmap);
    
    new KAction(i18n("&Delete"), icon, Key_Delete, this,
                SLOT(slotEditDelete()), actionCollection(),
                "delete");
    
    pixmap.load(pixmapDir + "/toolbar/event-edit.xpm");
    icon = QIconSet(pixmap);

    new KAction(i18n("&Edit Item"), icon, Key_E, this,
                SLOT(slotEdit()), actionCollection(),
                "edit");
    
    new KAction(i18n("Select &All"), 0, this,
                SLOT(slotSelectAll()), actionCollection(),
                "select_all");

    new KAction(i18n("Clear Selection"), Key_Escape, this,
		SLOT(slotClearSelection()), actionCollection(),
		"clear_selection");

    m_config->setGroup(ConfigGroup);
    int timeMode = m_config->readNumEntry("timemode", 0);

    KRadioAction *action;

    pixmap.load(pixmapDir + "/toolbar/time-musical.xpm");
    icon = QIconSet(pixmap);

    action = new KRadioAction(i18n("&Musical Times"), icon, 0, this,
			      SLOT(slotMusicalTime()),
			      actionCollection(), "time_musical");
    action->setExclusiveGroup("timeMode");
    if (timeMode == 0) action->setChecked(true);

    pixmap.load(pixmapDir + "/toolbar/time-real.xpm");
    icon = QIconSet(pixmap);

    action = new KRadioAction(i18n("&Real Times"), icon, 0, this,
			      SLOT(slotRealTime()),
			      actionCollection(), "time_real");
    action->setExclusiveGroup("timeMode");
    if (timeMode == 1) action->setChecked(true);

    pixmap.load(pixmapDir + "/toolbar/time-raw.xpm");
    icon = QIconSet(pixmap);

    action = new KRadioAction(i18n("Ra&w Times"), icon, 0, this,
			      SLOT(slotRawTime()),
			      actionCollection(), "time_raw");
    action->setExclusiveGroup("timeMode");
    if (timeMode == 2) action->setChecked(true);

    createGUI(getRCFileName());
}

void
TempoView::initStatusBar()
{
    KStatusBar* sb = statusBar();

    sb->insertItem(KTmpStatusMsg::getDefaultMsg(),
                   KTmpStatusMsg::getDefaultId(), 1);
    sb->setItemAlignment(KTmpStatusMsg::getDefaultId(),
                         AlignLeft | AlignVCenter);
}

QSize
TempoView::getViewSize()
{
    return m_list->size();
}

void
TempoView::setViewSize(QSize s)
{
    m_list->setFixedSize(s);
}

void
TempoView::readOptions()
{
    m_config->setGroup(ConfigGroup);
    EditViewBase::readOptions();
    m_filter = m_config->readNumEntry("filter", m_filter);
    m_list->restoreLayout(m_config, LayoutConfigGroupName);
}

void
TempoView::slotSaveOptions()
{
    m_config->setGroup(ConfigGroup);
    m_config->writeEntry("filter", m_filter);
    m_list->saveLayout(m_config, LayoutConfigGroupName);
}

void 
TempoView::slotModifyFilter(int button)
{
    QCheckBox *checkBox = dynamic_cast<QCheckBox*>(m_filterGroup->find(button));

    if (checkBox == 0) return;

    if (checkBox->isChecked())
    {
        switch (button)
        {
            case 0:
                m_filter |= Tempo;
                break;

            case 1:
                m_filter |= TimeSignature;
                break;

            default:
                break;
        }

    }
    else
    {
        switch (button)
        {
            case 0:
                m_filter ^= Tempo;
                break;

            case 1:
                m_filter ^= TimeSignature;
                break;

            default:
                break;
        }
    }

    m_lastSetFilter = m_filter;

    applyLayout(0);
}


void
TempoView::setButtonsToFilter()
{
    if (m_filter & Tempo)
        m_tempoCheckBox->setChecked(true);
    else
        m_tempoCheckBox->setChecked(false);

    if (m_filter & TimeSignature)
        m_timeSigCheckBox->setChecked(true);
    else
        m_timeSigCheckBox->setChecked(false);
}

void
TempoView::slotMusicalTime()
{
    m_config->setGroup(ConfigGroup);
    m_config->writeEntry("timemode", 0);
    applyLayout();
}

void
TempoView::slotRealTime()
{
    m_config->setGroup(ConfigGroup);
    m_config->writeEntry("timemode", 1);
    applyLayout();
}

void
TempoView::slotRawTime()
{
    m_config->setGroup(ConfigGroup);
    m_config->writeEntry("timemode", 2);
    applyLayout();
}

void
TempoView::slotPopupEditor(QListViewItem *qitem)
{
    TempoListItem *item = dynamic_cast<TempoListItem *>(qitem);
    if (!item) return;

    Rosegarden::timeT time = item->getTime();

    switch (item->getType()) {
	
    case TempoListItem::Tempo:
    {
	TempoDialog dialog(this, getDocument(), true);
	dialog.setTempoPosition(time);

	connect(&dialog, 
		SIGNAL(changeTempo(Rosegarden::timeT,
				   double, TempoDialog::TempoDialogAction)),
		this,
		SIGNAL(changeTempo(Rosegarden::timeT,
				   double, TempoDialog::TempoDialogAction)));
	
	dialog.exec();
	break;
    }
    
    case TempoListItem::TimeSignature:
    {
	Rosegarden::Composition &composition(getDocument()->getComposition());
	Rosegarden::TimeSignature sig = composition.getTimeSignatureAt(time);
	
	TimeSignatureDialog dialog(this, &composition, time, sig, true);
	
	if (dialog.exec() == QDialog::Accepted) {
	    
	    time = dialog.getTime();
	    
	    if (dialog.shouldNormalizeRests()) {
		addCommandToHistory
		    (new AddTimeSignatureAndNormalizeCommand
		     (&composition, time, dialog.getTimeSignature()));
	    } else { 
		addCommandToHistory
		    (new AddTimeSignatureCommand
		     (&composition, time, dialog.getTimeSignature()));
	    }
	}
    }
    
    default:
	break;
    }
}


void
TempoView::updateViewCaption()
{
    setCaption(i18n("%1 - Tempo and Time Signature Editor")
	       .arg(getDocument()->getTitle()));
}



const char* const TempoView::LayoutConfigGroupName = "TempoView Layout";
const char* const TempoView::ConfigGroup = "TempoView Options";

