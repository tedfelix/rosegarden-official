/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2024 the Rosegarden development team.

    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#define RG_MODULE_STRING "[TriggerSegmentManager]"

#include "TriggerSegmentManager.h"
#include "TriggerManagerItem.h"

#include "base/BaseProperties.h"
#include "misc/Debug.h"
#include "misc/Strings.h"
#include "base/Clipboard.h"
#include "base/Composition.h"
#include "base/CompositionTimeSliceAdapter.h"
#include "base/Pitch.h"
#include "base/RealTime.h"
#include "base/Segment.h"
#include "base/TriggerSegment.h"
#include "commands/segment/AddTriggerSegmentCommand.h"
#include "commands/segment/DeleteTriggerSegmentCommand.h"
#include "commands/segment/PasteToTriggerSegmentCommand.h"
#include "document/CommandHistory.h"
#include "document/RosegardenDocument.h"
#include "misc/ConfigGroups.h"
#include "gui/dialogs/TimeDialog.h"
#include "gui/dialogs/AboutDialog.h"
#include "document/Command.h"

#include <QLayout>
#include <QApplication>
#include <QAction>
#include <QTreeWidget>
#include <QMessageBox>
#include <QSettings>
#include <QDialog>
#include <QFrame>
#include <QIcon>
#include <QTreeWidget>
#include <QTreeWidgetItem>
#include <QPushButton>
#include <QSizePolicy>
#include <QString>
#include <QToolTip>
#include <QWidget>
#include <QVBoxLayout>
#include <QDialogButtonBox>
#include <QDesktopServices>


namespace Rosegarden
{

TriggerSegmentManager::TriggerSegmentManager(QWidget *parent,
        RosegardenDocument *doc):
        QMainWindow(parent),    //, "triggereditordialog"),
        m_doc(doc),
        m_modified(false)
{

    this->setObjectName( "triggereditordialog" );
    setAttribute(Qt::WA_DeleteOnClose);

    QWidget *mainFrame = new QWidget(this);
    QVBoxLayout *mainFrameLayout = new QVBoxLayout;
    mainFrame->setLayout(mainFrameLayout);
    setCentralWidget(mainFrame);

    setWindowTitle(tr("Manage Triggered Segments"));

    m_listView = new QTreeWidget( mainFrame );
    mainFrameLayout->addWidget(m_listView);

    QStringList sl;
    sl         << "Index"
            << tr("ID")
            << tr("Label")
            << tr("Duration")
            << tr("Base pitch")
            << tr("Base velocity")
            << tr("Triggers");

    m_listView->setColumnCount( 7 );
    m_listView->setHeaderLabels( sl );

    // Rewrite all the old gobbityblather with a QDialogButtonBox instead, for
    // consistency and cleanliness
    QDialogButtonBox *btnBox = new QDialogButtonBox(QDialogButtonBox::Close);
    mainFrameLayout->addWidget(btnBox);


    m_addButton = btnBox->addButton(tr("Add"), QDialogButtonBox::ActionRole);
    m_addButton->setToolTip(tr("Add a Triggered Segment"));

    m_deleteButton = btnBox->addButton(tr("Delete"), QDialogButtonBox::ActionRole);
    m_deleteButton->setToolTip(tr("Delete a Triggered Segment"));

    m_deleteAllButton = btnBox->addButton(tr("Delete All"), QDialogButtonBox::ActionRole);
    m_deleteAllButton->setToolTip(tr("Delete All Triggered Segments"));


    // Whether accepted or rejected, we always just want to call slotClose()
    connect(btnBox, &QDialogButtonBox::accepted, this, &TriggerSegmentManager::slotClose);
    connect(btnBox, &QDialogButtonBox::rejected, this, &TriggerSegmentManager::slotClose);

    connect(m_addButton, &QAbstractButton::released,
            this, &TriggerSegmentManager::slotAdd);

    connect(m_deleteButton, &QAbstractButton::released,
            this, &TriggerSegmentManager::slotDelete);

    connect(m_deleteAllButton, &QAbstractButton::released,
            this, &TriggerSegmentManager::slotDeleteAll);

    setupActions();

//     CommandHistory::getInstance()->attachView(actionCollection());    //&&&

    connect(CommandHistory::getInstance(), &CommandHistory::commandExecuted,
            this, &TriggerSegmentManager::slotUpdate);

    connect(m_listView, &QTreeWidget::itemDoubleClicked,
            this, &TriggerSegmentManager::slotEdit);

    connect(m_listView, &QTreeWidget::itemPressed,
            this, &TriggerSegmentManager::slotItemClicked);

    // Highlight all columns - enable extended selection mode
    //
    m_listView->setAllColumnsShowFocus(true);
//     m_listView->setSelectionMode(QTreeWidget::Extended);
    m_listView->setSelectionMode( QAbstractItemView::ExtendedSelection );
//     m_listView->setSelectionBehavior( QAbstractItemView::SelectRows );

//     m_listView->setItemsRenameable(true);    //&&&

    initDialog();

//     setAutoSaveSettings(TriggerManagerConfigGroup, true);    //&&&
}

TriggerSegmentManager::~TriggerSegmentManager()
{
    RG_DEBUG << "TriggerSegmentManager::~TriggerSegmentManager";

//     m_listView->saveLayout(TriggerManagerConfigGroup);    //&&&

//     if (m_doc)
//         CommandHistory::getInstance()->detachView(actionCollection());
}

void
TriggerSegmentManager::initDialog()
{
    RG_DEBUG << "TriggerSegmentManager::initDialog";
    slotUpdate();
}

void
TriggerSegmentManager::slotUpdate()
{
    RG_DEBUG << "TriggerSegmentManager::slotUpdate";

    m_listView->clear();

    Composition &comp = m_doc->getComposition();

    const Composition::TriggerSegmentSet &triggers =
        comp.getTriggerSegments();

    QSettings settings;
    settings.beginGroup(TriggerManagerConfigGroup);
    const Composition::TimeMode timeMode =
            static_cast<Composition::TimeMode>(settings.value("timemode", 0).toInt());

    int i = 0;

    for (Composition::TriggerSegmentSet::const_iterator it = triggers.begin();
         it != triggers.end();
         ++it) {

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

        QString timeString = comp.makeDurationString(first, duration, timeMode);

        QString label = strtoqstr((*it)->getSegment()->getLabel());
        if (label == "")
            label = tr("<no label>");

        QString used = tr("%1 on %n track(s)", "",
                            tracks.size()).arg(uses);

        QString pitch = QString("%1 (%2)")
                        .arg(Pitch::toStringOctave((*it)->getBasePitch()))
                        .arg((*it)->getBasePitch());

        QString velocity = QString("%1").arg((*it)->getBaseVelocity());

        TriggerManagerItem *item =
            new TriggerManagerItem
            (m_listView,
             QStringList()
             << QString("%1").arg(i + 1)
             << QString("%1").arg((*it)->getId())
             << label << timeString << pitch << velocity << used );

        item->setRawDuration(duration);
        item->setId((*it)->getId());
        item->setUsage(uses);
        item->setPitch((*it)->getBasePitch());

        m_listView->addTopLevelItem(item);
        ++i;
    }

    if (m_listView->topLevelItemCount() == 0) {
        QTreeWidgetItem *item =
            new TriggerManagerItem(m_listView, QStringList() << tr("<none>") );
        m_listView->addTopLevelItem(item);

//         m_listView->setSelectionMode(QTreeWidget::NoSelection);
        m_listView->setSelectionMode( QAbstractItemView::NoSelection );
    } else {
//         m_listView->setSelectionMode(QTreeWidget::Extended);
        m_listView->setSelectionMode( QAbstractItemView::ExtendedSelection );

    }

    settings.endGroup();
}

void
TriggerSegmentManager::slotDeleteAll()
{
    if (QMessageBox::warning(this, tr("Rosegarden"), tr("This will remove all triggered segments from the whole composition.  Are you sure?"), QMessageBox::Yes|QMessageBox::Cancel, QMessageBox::Cancel ) != QMessageBox::Yes )
        return ;

    RG_DEBUG << "TriggerSegmentManager::slotDeleteAll";
    MacroCommand *command = new MacroCommand(tr("Remove all triggered segments"));

//     QTreeWidgetItem *it = m_listView->firstChild();
    QTreeWidgetItem *it = m_listView->topLevelItem(0);

    do {

        TriggerManagerItem *item =
            dynamic_cast<TriggerManagerItem*>(it);

        if (!item)
            continue;

        DeleteTriggerSegmentCommand *c =
            new DeleteTriggerSegmentCommand(m_doc,
                                            item->getId());
        command->addCommand(c);

    } while ( (it = m_listView->itemBelow( it )) );

    addCommandToHistory(command);
}

void
TriggerSegmentManager::slotAdd()
{
    TimeDialog dialog(this, tr("Trigger Segment Duration"),
                      &m_doc->getComposition(),
                      0, 3840, 0, false);

    if (dialog.exec() == QDialog::Accepted) {
        addCommandToHistory(new AddTriggerSegmentCommand
                            (m_doc, dialog.getTime(), 64));
    }
}

void
TriggerSegmentManager::slotDelete()
{
    RG_DEBUG << "slotDelete()";

    // ??? If a triggered segment is deleted, it still appears to play just
    //     fine.  It is never deleted.  Very odd.  I wonder if at some point
    //     it will cause a crash.  Across a save/load it does indeed stop
    //     working.  The sequencer must have some sort of memory of it.

    const TriggerManagerItem *item =
            dynamic_cast<TriggerManagerItem *>(m_listView->currentItem());

    if (!item)
        return;

    if (item->getUsage() > 0) {
        if (QMessageBox::warning(
                this,
                tr("Rosegarden"),
                tr("This triggered segment is used %n time(s) in the current composition.  Are you sure you want to remove it?", "", item->getUsage()),
                QMessageBox::Yes | QMessageBox::Cancel,
                QMessageBox::Cancel) != QMessageBox::Yes)
            return;
    }

    addCommandToHistory(new DeleteTriggerSegmentCommand(m_doc, item->getId()));
}

void
TriggerSegmentManager::slotPasteAsNew()
{
    Clipboard *clipboard = Clipboard::mainClipboard();

    if (clipboard->isEmpty()) {
        QMessageBox::information(this, tr("Rosegarden"), tr("Clipboard is empty"));
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
    RG_DEBUG << "TriggerSegmentManager::slotClose";

//     if (m_doc)
//         CommandHistory::getInstance()->detachView(actionCollection());    //&&&
    m_doc = nullptr;

    close();
}

void
TriggerSegmentManager::setupActions()
{
    createAction("paste_to_trigger_segment", SLOT(slotPasteAsNew()));

    QSettings settings;
    settings.beginGroup( TriggerManagerConfigGroup );

    int timeMode = settings.value("timemode", 0).toInt() ;

    QAction *a;
    a = createAction("time_musical", SLOT(slotMusicalTime()));
    a->setCheckable(true);
    if (timeMode == 0)  a->setChecked(true);

    a = createAction("time_real", SLOT(slotRealTime()));
    a->setCheckable(true);
    if (timeMode == 1)  a->setChecked(true);

    a = createAction("time_raw", SLOT(slotRawTime()));
    a->setCheckable(true);
    if (timeMode == 2)  a->setChecked(true);

    createAction("trigger_help", SLOT(slotHelpRequested()));
    createAction("help_about_app", SLOT(slotHelpAbout()));

    createMenusAndToolbars("triggermanager.rc"); //@@@ JAS orig. 0

    settings.endGroup();
}

void
TriggerSegmentManager::addCommandToHistory(Command *command)
{
    CommandHistory::getInstance()->addCommand(command);
    setModified(false);
}

void
TriggerSegmentManager::setModified(bool modified)
{
    RG_DEBUG << "TriggerSegmentManager::setModified(" << modified << ")";

    m_modified = modified;
}

/* unused
void
TriggerSegmentManager::checkModified()
{
    RG_DEBUG << "TriggerSegmentManager::checkModified(" << m_modified << ")";

}
*/

void
TriggerSegmentManager::slotEdit(QTreeWidgetItem *i)
{
    RG_DEBUG << "TriggerSegmentManager::slotEdit";

    TriggerManagerItem *item =
        dynamic_cast<TriggerManagerItem*>(i);

    if (!item)
        return ;

    TriggerSegmentId id = item->getId();

    RG_DEBUG << "id is " << id;

    // Launch the Event List editor.
    // ??? Why do we only allow the event list editor?  The user should also be
    //     allowed to launch the notation and matrix editors.  In fact, those
    //     would probably be preferred.  We should use the "Double-click opens
    //     segment in" editor preference ("doubleclickclient") and offer
    //     context menu items for launching any of the three editors as
    //     desired.
    //
    //     The event list editor used to have code to prevent launching the
    //     notation and matrix editors on triggered segments.  That has been
    //     removed.
    emit editTriggerSegment(id);
}

void
TriggerSegmentManager::closeEvent(QCloseEvent */* e */)
{
    emit closing();
    close();
//     KMainWindow::closeEvent(e);
}

void
TriggerSegmentManager::setDocument(RosegardenDocument *doc)
{
    // reset our pointers
    m_doc = doc;
    m_modified = false;

    slotUpdate();
}

void
TriggerSegmentManager::slotItemClicked(QTreeWidgetItem */* item */)
{
    RG_DEBUG << "TriggerSegmentManager::slotItemClicked";
}

void
TriggerSegmentManager::slotMusicalTime()
{
    QSettings settings;
    settings.beginGroup( TriggerManagerConfigGroup );

    settings.setValue("timemode", 0);
    findAction("time_musical")->setChecked(true);
    findAction("time_real")->setChecked(false);
    findAction("time_raw")->setChecked(false);
    slotUpdate();

    settings.endGroup();
}

void
TriggerSegmentManager::slotRealTime()
{
    QSettings settings;
    settings.beginGroup( TriggerManagerConfigGroup );

    settings.setValue("timemode", 1);
    findAction("time_musical")->setChecked(false);
    findAction("time_real")->setChecked(true);
    findAction("time_raw")->setChecked(false);
    slotUpdate();

    settings.endGroup();
}

void
TriggerSegmentManager::slotRawTime()
{
    QSettings settings;
    settings.beginGroup( TriggerManagerConfigGroup );

    settings.setValue("timemode", 2);
    findAction("time_musical")->setChecked(false);
    findAction("time_real")->setChecked(false);
    findAction("time_raw")->setChecked(true);
    slotUpdate();

    settings.endGroup();
}


void
TriggerSegmentManager::slotHelpRequested()
{
    // TRANSLATORS: if the manual is translated into your language, you can
    // change the two-letter language code in this URL to point to your language
    // version, eg. "http://rosegardenmusic.com/wiki/doc:triggerSegmentManager-es" for the
    // Spanish version. If your language doesn't yet have a translation, feel
    // free to create one.
    QString helpURL = tr("http://rosegardenmusic.com/wiki/doc:triggerSegmentManager-en");
    QDesktopServices::openUrl(QUrl(helpURL));
}

void
TriggerSegmentManager::slotHelpAbout()
{
    new AboutDialog(this);
}
}
