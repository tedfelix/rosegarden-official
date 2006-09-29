/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
 
    This program is Copyright 2000-2006
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


#include "TriggerSegmentManager.h"
#include <qlayout.h>
#include <kapplication.h>

#include "base/BaseProperties.h"
#include <klocale.h>
#include <kstddirs.h>
#include "misc/Debug.h"
#include "misc/Strings.h"
#include "base/Clipboard.h"
#include "base/Composition.h"
#include "base/CompositionTimeSliceAdapter.h"
#include "base/RealTime.h"
#include "base/Segment.h"
#include "base/TriggerSegment.h"
#include "commands/segment/AddTriggerSegmentCommand.h"
#include "commands/segment/DeleteTriggerSegmentCommand.h"
#include "commands/segment/PasteToTriggerSegmentCommand.h"
#include "document/MultiViewCommandHistory.h"
#include "document/RosegardenGUIDoc.h"
#include "gui/dialogs/TimeDialog.h"
#include "gui/general/MidiPitchLabel.h"
#include <kaction.h>
#include <kcommand.h>
#include <kglobal.h>
#include <klistview.h>
#include <kmainwindow.h>
#include <kmessagebox.h>
#include <kstdaccel.h>
#include <kstdaction.h>
#include <qaccel.h>
#include <qdialog.h>
#include <qframe.h>
#include <qiconset.h>
#include <qlistview.h>
#include <qpushbutton.h>
#include <qsizepolicy.h>
#include <qstring.h>
#include <qtooltip.h>
#include <qvbox.h>
#include <qwidget.h>


namespace Rosegarden
{

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

    QToolTip::add
        (m_addButton,
                i18n("Add a Triggered Segment"));

    QToolTip::add
        (m_deleteButton,
                i18n("Delete a Triggered Segment"));

    QToolTip::add
        (m_deleteAllButton,
                i18n("Delete All Triggered Segments"));

    QToolTip::add
        (m_closeButton,
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
        std::set
            <int> tracks;

        CompositionTimeSliceAdapter tsa(&m_doc->getComposition());
        for (CompositionTimeSliceAdapter::iterator ci = tsa.begin();
                ci != tsa.end(); ++ci) {
            if ((*ci)->has(BaseProperties::TRIGGER_SEGMENT_ID) &&
                    (*ci)->get
                    <Int>(BaseProperties::TRIGGER_SEGMENT_ID) == (long)(*it)->getId()) {
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
        if (label == "")
            label = i18n("<no label>");

        QString used = i18n("%1 on 1 track",
                            "%1 on %n tracks",
                            tracks.size()).arg(uses);

        QString pitch = QString("%1 (%2)")
                        .arg(MidiPitchLabel((*it)->getBasePitch()).getQString())
                        .arg((*it)->getBasePitch());

        QString velocity = QString("%1").arg((*it)->getBaseVelocity());

        item = new TriggerManagerItem
               (m_listView, QString("%1").arg(i + 1), QString("%1").arg((*it)->getId()),
                label, timeString, pitch, velocity, used);

        item->setRawDuration(duration);
        item->setId((*it)->getId());
        item->setUsage(uses);
        item->setPitch((*it)->getBasePitch());

        m_listView->insertItem(item);
        ++i;
    }

    if (m_listView->childCount() == 0) {
        QListViewItem *item =
            new TriggerManagerItem(m_listView, i18n("<none>"));
        m_listView->insertItem(item);

        m_listView->setSelectionMode(QListView::NoSelection);
    } else {
        m_listView->setSelectionMode(QListView::Extended);
    }
}

void
TriggerSegmentManager::slotDeleteAll()
{
    if (KMessageBox::warningContinueCancel(this, i18n("This will remove all triggered segments from the whole composition.  Are you sure?")) != KMessageBox::Continue)
        return ;

    RG_DEBUG << "TriggerSegmentManager::slotDeleteAll" << endl;
    KMacroCommand *command = new KMacroCommand(i18n("Remove all triggered segments"));

    QListViewItem *it = m_listView->firstChild();

    do {

        TriggerManagerItem *item =
            dynamic_cast<TriggerManagerItem*>(it);

        if (!item)
            continue;

        DeleteTriggerSegmentCommand *c =
            new DeleteTriggerSegmentCommand(m_doc,
                                            item->getId());
        command->addCommand(c);

    } while ((it = it->nextSibling()));

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

    if (!item)
        return ;

    if (item->getUsage() > 0) {
        if (KMessageBox::warningContinueCancel(this, i18n("This triggered segment is used 1 time in the current composition.  Are you sure you want to remove it?",
                                               "This triggered segment is used %n times in the current composition.  Are you sure you want to remove it?", item->getUsage())) != KMessageBox::Continue)
            return ;
    }

    DeleteTriggerSegmentCommand *command =
        new DeleteTriggerSegmentCommand(m_doc, item->getId());

    addCommandToHistory(command);
}

void
TriggerSegmentManager::slotPasteAsNew()
{
    Clipboard *clipboard = m_doc->getClipboard();

    if (clipboard->isEmpty()) {
        KMessageBox::information(this, i18n("Clipboard is empty"));
        return ;
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

    if (m_doc)
        m_doc->getCommandHistory()->detachView(actionCollection());
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

    QCanvasPixmap pixmap(pixmapDir + "/toolbar/time-musical.png");
    QIconSet icon(pixmap);

    action = new KRadioAction(i18n("&Musical Times"), icon, 0, this,
                              SLOT(slotMusicalTime()),
                              actionCollection(), "time_musical");
    action->setExclusiveGroup("timeMode");
    if (timeMode == 0)
        action->setChecked(true);

    pixmap.load(pixmapDir + "/toolbar/time-real.png");
    icon = QIconSet(pixmap);

    action = new KRadioAction(i18n("&Real Times"), icon, 0, this,
                              SLOT(slotRealTime()),
                              actionCollection(), "time_real");
    action->setExclusiveGroup("timeMode");
    if (timeMode == 1)
        action->setChecked(true);

    pixmap.load(pixmapDir + "/toolbar/time-raw.png");
    icon = QIconSet(pixmap);

    action = new KRadioAction(i18n("Ra&w Times"), icon, 0, this,
                              SLOT(slotRawTime()),
                              actionCollection(), "time_raw");
    action->setExclusiveGroup("timeMode");
    if (timeMode == 2)
        action->setChecked(true);

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

    if (!item)
        return ;

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
TriggerSegmentManager::makeDurationString(timeT time,
        timeT duration, int timeMode)
{
    //!!! duplication with EventView::makeDurationString -- merge somewhere?

    switch (timeMode) {

    case 0:  // musical time
        {
            int bar, beat, fraction, remainder;
            m_doc->getComposition().getMusicalTimeForDuration
            (time, duration, bar, beat, fraction, remainder);
            return QString("%1%2%3-%4%5-%6%7-%8%9   ")
                   .arg(bar / 100)
                   .arg((bar % 100) / 10)
                   .arg(bar % 10)
                   .arg(beat / 10)
                   .arg(beat % 10)
                   .arg(fraction / 10)
                   .arg(fraction % 10)
                   .arg(remainder / 10)
                   .arg(remainder % 10);
        }

    case 1:  // real time
        {
            RealTime rt =
                m_doc->getComposition().getRealTimeDifference
                (time, time + duration);
            //	return QString("%1  ").arg(rt.toString().c_str());
            return QString("%1  ").arg(rt.toText().c_str());
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

}
#include "TriggerSegmentManager.moc"
