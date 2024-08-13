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

#define RG_MODULE_STRING "[TempoView]"
#define RG_NO_DEBUG_PRINT

#include "TempoView.h"

#include "TempoListItem.h"

#include "misc/Debug.h"
#include "base/Composition.h"
#include "base/RealTime.h"
#include "commands/segment/AddTimeSignatureAndNormalizeCommand.h"
#include "commands/segment/AddTimeSignatureCommand.h"
#include "commands/segment/RemoveTempoChangeCommand.h"
#include "commands/segment/RemoveTimeSignatureCommand.h"
#include "document/RosegardenDocument.h"
#include "document/CommandHistory.h"
#include "misc/ConfigGroups.h"
#include "gui/dialogs/TimeSignatureDialog.h"
#include "gui/dialogs/AboutDialog.h"
#include "gui/general/EditTempoController.h"
#include "misc/PreferenceInt.h"
#include "misc/PreferenceBool.h"

#include <QAction>
#include <QSettings>
#include <QTreeWidget>
#include <QGroupBox>
#include <QCheckBox>
#include <QDialog>
#include <QString>
#include <QStringList>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QStatusBar>
#include <QList>
#include <QDesktopServices>
#include <QSettings>
#include <QHeaderView>
#include <QScrollBar>


namespace
{

    Rosegarden::PreferenceInt a_timeMode(
            Rosegarden::TempoViewConfigGroup, "timemode", 0);

    Rosegarden::PreferenceBool a_tempoFilter(
            Rosegarden::TempoViewConfigGroup,
            "tempofilter",
            true);
    Rosegarden::PreferenceBool a_timeSignatureFilter(
            Rosegarden::TempoViewConfigGroup,
            "timesignaturefilter",
            true);

}


namespace Rosegarden
{


TempoView::TempoView(timeT openTime)
{

    updateWindowTitle();

    setStatusBar(new QStatusBar(this));

    // Connect for changes so we can update the list.
    connect(RosegardenDocument::currentDocument,
                &RosegardenDocument::documentModified,
            this, &TempoView::slotDocumentModified);

    initMenu();

    // Create frame and layout.
    m_frame = new QFrame(this);
    m_frame->setMinimumSize(500, 300);
    m_frame->setMaximumSize(2200, 1400);
    m_mainLayout = new QHBoxLayout(m_frame);
    m_frame->setLayout(m_mainLayout);
    setCentralWidget(m_frame);

    // Filter Group Box
    m_filterGroup = new QGroupBox(tr("Filter"), m_frame);
    m_mainLayout->addWidget(m_filterGroup);
    QVBoxLayout *filterGroupLayout = new QVBoxLayout;
    m_filterGroup->setLayout(filterGroupLayout);

    // Tempo
    m_tempoCheckBox = new QCheckBox(tr("Tempo"), m_filterGroup);
    m_tempoCheckBox->setChecked(a_tempoFilter.get());
    connect(m_tempoCheckBox, &QCheckBox::clicked,
            this, &TempoView::slotFilterClicked);
    filterGroupLayout->addWidget(m_tempoCheckBox);

    // Time Signature
    m_timeSigCheckBox = new QCheckBox(tr("Time Signature"), m_filterGroup);
    m_timeSigCheckBox->setChecked(a_timeSignatureFilter.get());
    connect(m_timeSigCheckBox, &QCheckBox::clicked,
            this, &TempoView::slotFilterClicked);
    filterGroupLayout->addWidget(m_timeSigCheckBox);

    // Fill the rest of the empty space to keep the widgets together.
    filterGroupLayout->addStretch(1);

    // Tempo/Time Signature List
    m_list = new QTreeWidget(m_frame);
    m_mainLayout->addWidget(m_list);
    m_list->setAllColumnsShowFocus(true);
    m_list->setSelectionMode(QAbstractItemView::ExtendedSelection);
    QStringList headers;
    // ??? The extra space at the end of each of these is probably an
    //     attempt at getting the columns to be wider.  This does not
    //     work.  Leaving this here to avoid creating work for the
    //     translators.  For now.
    headers << tr("Time  ") <<
               tr("Type  ") <<
               tr("Value  ") <<
               tr("Properties  ");
    m_list->setColumnCount(headers.size());
    m_list->setHeaderLabels(headers);
    // Make sure columns have a reasonable amount of space.
    m_list->setColumnWidth(0, 133);
    m_list->setColumnWidth(1, 125);
    connect(m_list, &QTreeWidget::itemDoubleClicked,
            this, &TempoView::slotPopupEditor);

    // Update the list.
    updateList();
    makeInitialSelection(openTime);

    // Restore window geometry and header state.
    QSettings settings;
    settings.beginGroup(WindowGeometryConfigGroup);
    restoreGeometry(settings.value("Tempo_View_Geometry").toByteArray());
    //restoreState(settings.value("Tempo_View_State").toByteArray());
    m_list->header()->restoreState(settings.value("Tempo_View_Header_State").toByteArray());
    settings.endGroup();

    m_doc->getComposition().addObserver(this);

}

TempoView::~TempoView()
{
    // ??? This is not getting called if we close rg with this window
    //     up.  We also get a "use after free" from Composition when it
    //     is dumping its extant observers.

    // Save state for next time.
    a_tempoFilter.set(m_tempoCheckBox->checkState() != Qt::Unchecked);
    a_timeSignatureFilter.set(m_timeSigCheckBox->checkState() != Qt::Unchecked);

    // Save window geometry and header state.
    QSettings settings;
    settings.beginGroup(WindowGeometryConfigGroup);
    settings.setValue("Tempo_View_Geometry", saveGeometry());
    //settings.setValue("Tempo_View_State", saveState());
    settings.setValue("Tempo_View_Header_State", m_list->header()->saveState());
    settings.endGroup();

    // We use m_doc instead of RosegardenDocument::currentDocument to
    // make sure that we disconnect from the old document when the
    // documents are switching.
    if (m_doc  &&  !isCompositionDeleted())
        m_doc->getComposition().removeObserver(this);
}

void
TempoView::closeEvent(QCloseEvent *e)
{
    // Let RosegardenMainWindow know we are going down.
    emit closing();

    EditViewBase::closeEvent(e);
}

void
TempoView::tempoChanged(const Composition * /*comp*/)
{
    updateList();
}

void
TempoView::timeSignatureChanged(const Composition * /*comp*/)
{
    updateList();
}

bool
TempoView::updateList()
{
    // Preserve Selection.

    // We use a key instead of an index because indexes change.  Keys
    // do not.  This guarantees that we recreate the original selection
    // as closely as possible.
    struct Key {
        timeT midiTicks{0};
        TempoListItem::Type itemType{TempoListItem::TimeSignature};

        bool operator<(const Key &rhs) const
        {
            if (midiTicks < rhs.midiTicks)
                return true;
            if (midiTicks > rhs.midiTicks)
                return false;
            return (itemType < rhs.itemType);
        }
    };
    std::set<Key> selectionSet;

    // For each item in the list...
    for (int itemIndex = 0;
         itemIndex < m_list->topLevelItemCount();
         ++itemIndex) {
        TempoListItem *item =
                dynamic_cast<TempoListItem *>(m_list->topLevelItem(itemIndex));
        if (!item)
            continue;

        // Not selected?  Try the next...
        if (!item->isSelected())
            continue;

        // Create key.
        Key key;
        key.midiTicks = item->getTime();
        key.itemType = item->getType();

        // Add to set.
        selectionSet.insert(key);
    }

    // Preserve the "current item".

    bool haveCurrentItem{false};
    Key currentItemKey;

    // Scope to avoid accidentally reusing currentItem after it is gone.
    {
        // The "current item" is always selected and has the focus outline.
        TempoListItem *currentItem =
                dynamic_cast<TempoListItem *>(m_list->currentItem());
        haveCurrentItem = currentItem;
        if (haveCurrentItem) {
            // Make a key so we can re-select it if it still exists.
            currentItemKey.midiTicks = currentItem->getTime();
            currentItemKey.itemType = currentItem->getType();
        }
    }

    // Preserve scroll position.
    int scrollPos{0};
    if (m_list->verticalScrollBar())
        scrollPos = m_list->verticalScrollBar()->value();

    // Recreate list.

    m_list->clear();

    Composition *comp = &RosegardenDocument::currentDocument->getComposition();

    // Time Signatures
    if (m_timeSigCheckBox->isChecked()) {

        for (int timeSignatureIndex = 0;
             timeSignatureIndex < comp->getTimeSignatureCount();
             ++timeSignatureIndex) {

            std::pair<timeT, Rosegarden::TimeSignature> sig =
                    comp->getTimeSignatureChange(timeSignatureIndex);

            QString properties;
            if (sig.second.isHidden()) {
                if (sig.second.isCommon())
                    properties = tr("Common, hidden");
                else
                    properties = tr("Hidden");
            } else {
                if (sig.second.isCommon())
                    properties = tr("Common");
            }

            QString timeString = comp->makeTimeString(
                    sig.first,
                    static_cast<Composition::TimeMode>(a_timeMode.get()));

            QStringList labels;
            labels << timeString <<
                      tr("Time Signature   ") <<
                      QString("%1/%2   ").
                              arg(sig.second.getNumerator()).
                              arg(sig.second.getDenominator()) <<
                      properties;

            // Create a new TempoListItem and add to the list.
            new TempoListItem(
                    m_list,  // treeWidget
                    comp,  // composition
                    TempoListItem::TimeSignature,  // type
                    sig.first,  // time
                    timeSignatureIndex,  // index
                    labels);
        }
    }

    // Tempos
    if (m_tempoCheckBox->isChecked()) {

        for (int tempoIndex = 0;
             tempoIndex < comp->getTempoChangeCount();
             ++tempoIndex) {

            const std::pair<timeT, tempoT> tempoPair =
                    comp->getTempoChange(tempoIndex);
            const timeT time = tempoPair.first;
            const tempoT &tempo = tempoPair.second;

            QString desc;

            const float qpm = comp->getTempoQpm(tempo);
            const int qpmUnits = int(qpm + 0.001);
            const int qpmTenths = int((qpm - qpmUnits) * 10 + 0.001);
            const int qpmHundredths =
                    int((qpm - qpmUnits - qpmTenths / 10.0) * 100 + 0.001);

            const Rosegarden::TimeSignature sig =
                    comp->getTimeSignatureAt(time);

            if (sig.getBeatDuration() ==
                    Note(Note::Crotchet).getDuration()) {
                desc = tr("%1.%2%3")
                       .arg(qpmUnits).arg(qpmTenths).arg(qpmHundredths);
            } else {
                const float bpm = (qpm *
                             Note(Note::Crotchet).getDuration()) /
                            sig.getBeatDuration();
                const int bpmUnits = int(bpm + 0.001);
                const int bpmTenths = int((bpm - bpmUnits) * 10 + 0.001);
                const int bpmHundredths = int((bpm - bpmUnits - bpmTenths / 10.0) * 100 + 0.001);

                desc = tr("%1.%2%3 qpm (%4.%5%6 bpm)   ")
                       .arg(qpmUnits).arg(qpmTenths).arg(qpmHundredths)
                       .arg(bpmUnits).arg(bpmTenths).arg(bpmHundredths);
            }

            const QString timeString = comp->makeTimeString(
                    time, static_cast<Composition::TimeMode>(a_timeMode.get()));

            QStringList labels;
            labels << timeString << tr("Tempo   ") << desc;

            // Create a new TempoListItem and add to the list.
            new TempoListItem(
                    m_list,  // treeWidget
                    comp,  // composition
                    TempoListItem::Tempo,  // type
                    time,
                    tempoIndex,
                    labels);
        }
    }

    // Restore Current Item.

    // This has to be done prior to restoring the selection or else it
    // will clear the selection.
    // ??? We could do this in the loops above and avoid another loop.

    if (haveCurrentItem) {
        // For each item in the list...
        for (int itemIndex = 0;
             itemIndex < m_list->topLevelItemCount();
             ++itemIndex) {
            TempoListItem *item =
                    dynamic_cast<TempoListItem *>(m_list->topLevelItem(itemIndex));
            if (!item)
                continue;

            // Found it?  Make it current and bail.
            if (currentItemKey.midiTicks == item->getTime()  &&
                currentItemKey.itemType == item->getType()) {
                m_list->setCurrentItem(item);
                break;
            }
        }
    }

    // Restore Selection.

    bool haveSelection{false};

    // For each item in the list...
    for (int itemIndex = 0;
         itemIndex < m_list->topLevelItemCount();
         ++itemIndex) {
        TempoListItem *item =
                dynamic_cast<TempoListItem *>(m_list->topLevelItem(itemIndex));
        if (!item)
            continue;

        // Create key.
        Key key;
        key.midiTicks = item->getTime();
        key.itemType = item->getType();

        // Not selected?  Try the next.
        if (selectionSet.find(key) == selectionSet.end())
            continue;

        item->setSelected(true);
        haveSelection = true;
    }

    if (haveSelection)
        enterActionState("have_selection");
    else
        leaveActionState("have_selection");

    // Restore scroll position.
    if (scrollPos  &&  m_list->verticalScrollBar())
        m_list->verticalScrollBar()->setValue(scrollPos);

    // ??? We never return false.  Make this function return void.
    return true;
}

void
TempoView::makeInitialSelection(timeT time)
{
    // Start with nothing selected.
    // updateList() is called before this routine and it selects
    // the first item.  This clears it.
    // ??? I think this is no longer the case.  Remove this.
    slotClearSelection();

    // Select an item around the given time.

    // Note that this is complicated by the fact that Time Signatures
    // appear before Tempos.

    TempoListItem *goodItem = nullptr;
    int goodItemNo = 0;

    // For each item...
    for (int itemIndex = 0;
         itemIndex < m_list->topLevelItemCount();
         ++itemIndex) {
        TempoListItem *item =
                dynamic_cast<TempoListItem *>(m_list->topLevelItem(itemIndex));
        if (!item)
            continue;

        // Past the time we are looking for?  We're done.
        if (item->getTime() > time)
            break;

        // Keep track of the last item we examined.
        goodItem = item;
        goodItemNo = itemIndex;
    }

    if (goodItem) {
        m_listSelection.push_back(goodItemNo);
        goodItem->setSelected(true);
        m_list->scrollToItem(goodItem);
    }
}

Segment *
TempoView::getCurrentSegment()
{
    // TempoView does not deal in Segments.
    return nullptr;
}

void
TempoView::slotEditDelete()
{
    // ??? Select All then Delete leaves one item behind.  Why?  Also
    //     trying to delete that last one can cause a crash.

    QList<QTreeWidgetItem *> selection = m_list->selectedItems();

    // Nothing selected?  Nothing to delete.  Bail.
    if (selection.empty())
        return;

    RG_DEBUG << "slotEditDelete() - deleting " << selection.count() << " items";

    // Create a map of each selected item to sort them in index order.
    typedef std::map<int /*index*/, TempoListItem *> ItemMap;
    ItemMap itemMap;

    for (QTreeWidgetItem *twi : selection) {
        TempoListItem *tempoListItem = dynamic_cast<TempoListItem *>(twi);
        if (!tempoListItem)
            continue;

        itemMap[tempoListItem->getIndex()] = tempoListItem;
    }

    // Nothing to delete?  Bail.
    if (itemMap.empty())
        return;

    // We want the Remove commands to be in reverse order, because
    // removing one item by index will affect the indices of
    // subsequent items.  So we'll stack them onto here and then pull
    // them off again.
    std::vector<Command *> commands;

    // For each selected item in index order.
    // ??? Why not just read the map backwards with reverse iterators?
    //     rbegin() and rend().
    //     Then the commands vector is not needed.  We can assemble the
    //     macro command in this loop and be done with it.
    for (ItemMap::iterator iter = itemMap.begin();
         iter != itemMap.end();
         ++iter) {
        TempoListItem *item = iter->second;

        // Add the appropriate command to the "commands" list.

        if (item->getType() == TempoListItem::TimeSignature) {
            commands.push_back(new RemoveTimeSignatureCommand
                               (item->getComposition(),
                                item->getIndex()));
        } else {  // Tempo
            commands.push_back(new RemoveTempoChangeCommand
                               (item->getComposition(),
                                item->getIndex()));
        }
    }

    // No commands to run?  Bail.
    if (commands.empty())
        return;

    MacroCommand *macroCommand = new MacroCommand
                             (tr("Delete Tempo or Time Signature"));

    // For each command in reverse order which also happens to be
    // reverse index order, add the remove command to the macro.
    for (int i = commands.size() - 1; i >= 0; --i) {
        macroCommand->addCommand(commands[i]);
    }
    CommandHistory::getInstance()->addCommand(macroCommand);

    // No need to call updateList().  The CompositionObserver handlers
    // will be notified of the changes.
}

void
TempoView::slotAddTempoChange()
{
    timeT insertTime = 0;

    QList<QTreeWidgetItem *> selection = m_list->selectedItems();

    // If something is selected, use the time of that item.
    if (!selection.empty()) {
        TempoListItem *item =
            dynamic_cast<TempoListItem *>(selection.first());
        if (item)
            insertTime = item->getTime();
    }

    // Launch the TempoDialog.
    EditTempoController::self()->editTempo(
            this,  // parent
            insertTime,  // atTime
            true);  // timeEditable
}

void
TempoView::slotAddTimeSignatureChange()
{
    timeT insertTime = 0;

    QList<QTreeWidgetItem*> selection = m_list->selectedItems();

    // If something is selected, use the time of that item.
    if (!selection.empty()) {
        TempoListItem *item =
            dynamic_cast<TempoListItem *>(selection.first());
        if (item)
            insertTime = item->getTime();
    }

    Composition &composition =
            RosegardenDocument::currentDocument->getComposition();
    Rosegarden::TimeSignature sig = composition.getTimeSignatureAt(insertTime);

    TimeSignatureDialog dialog(this, &composition, insertTime, sig, true);

    if (dialog.exec() == QDialog::Accepted) {

        insertTime = dialog.getTime();

        if (dialog.shouldNormalizeRests()) {
            CommandHistory::getInstance()->addCommand(
                    new AddTimeSignatureAndNormalizeCommand(
                            &composition,
                            insertTime,
                            dialog.getTimeSignature()));
        } else {
            CommandHistory::getInstance()->addCommand(
                    new AddTimeSignatureCommand(
                            &composition,
                            insertTime,
                            dialog.getTimeSignature()));
        }
    }
}

void
TempoView::slotEditItem()
{
    QList<QTreeWidgetItem *> selection = m_list->selectedItems();
    if (selection.empty())
        return;

    // Edit the first one selected.
    TempoListItem *item = dynamic_cast<TempoListItem *>(selection.first());
    if (item)
        slotPopupEditor(item);
}

void
TempoView::slotSelectAll()
{
    m_listSelection.clear();

    for (int i = 0; m_list->topLevelItem(i); ++i) {
        m_listSelection.push_back(i);
        m_list->topLevelItem(i)->setSelected(true);
    }
}

void
TempoView::slotClearSelection()
{
    m_listSelection.clear();

    for (int i = 0; m_list->topLevelItem(i); ++i) {
        m_list->topLevelItem(i)->setSelected(false);
    }
}

void
TempoView::initMenu()
{
    setupBaseActions();

    createAction("insert_tempo", SLOT(slotAddTempoChange()));
    createAction("insert_timesig", SLOT(slotAddTimeSignatureChange()));
    createAction("delete", SLOT(slotEditDelete()));
    createAction("edit", SLOT(slotEditItem()));
    createAction("select_all", SLOT(slotSelectAll()));
    createAction("clear_selection", SLOT(slotClearSelection()));
    createAction("tempo_help", SLOT(slotHelpRequested()));
    createAction("help_about_app", SLOT(slotHelpAbout()));

    QAction *a;
    a = createAction("time_musical", SLOT(slotViewMusicalTimes()));
    a->setCheckable(true);
    if (a_timeMode.get() == (int)Composition::TimeMode::MusicalTime)
        a->setChecked(true);

    a = createAction("time_real", SLOT(slotViewRealTimes()));
    a->setCheckable(true);
    if (a_timeMode.get() == (int)Composition::TimeMode::RealTime)
        a->setChecked(true);

    a = createAction("time_raw", SLOT(slotViewRawTimes()));
    a->setCheckable(true);
    if (a_timeMode.get() == (int)Composition::TimeMode::RawTime)
        a->setChecked(true);

    createMenusAndToolbars("tempoview.rc");
}

void
TempoView::slotFilterClicked(bool)
{
    updateList();
}

void
TempoView::slotViewMusicalTimes()
{
    findAction("time_musical")->setChecked(true);
    findAction("time_real")->setChecked(false);
    findAction("time_raw")->setChecked(false);

    a_timeMode.set((int)Composition::TimeMode::MusicalTime);

    updateList();
}

void
TempoView::slotViewRealTimes()
{
    findAction("time_musical")->setChecked(false);
    findAction("time_real")->setChecked(true);
    findAction("time_raw")->setChecked(false);

    a_timeMode.set((int)Composition::TimeMode::RealTime);

    updateList();
}

void
TempoView::slotViewRawTimes()
{
    findAction("time_musical")->setChecked(false);
    findAction("time_real")->setChecked(false);
    findAction("time_raw")->setChecked(true);

    a_timeMode.set((int)Composition::TimeMode::RawTime);

    updateList();
}

void
TempoView::slotPopupEditor(QTreeWidgetItem *twi, int /*column*/)
{
    TempoListItem *item = dynamic_cast<TempoListItem *>(twi);
    if (!item)
        return;

    timeT time = item->getTime();

    switch (item->getType())
    {

    case TempoListItem::Tempo:
        // Launch the TempoDialog.
        EditTempoController::self()->editTempo(
                this, time, true /* timeEditable */);
        break;

    case TempoListItem::TimeSignature:
        {
            Composition &composition =
                    RosegardenDocument::currentDocument->getComposition();
            Rosegarden::TimeSignature sig = composition.getTimeSignatureAt(time);

            TimeSignatureDialog dialog(this, &composition, time, sig, true);

            if (dialog.exec() == QDialog::Accepted) {

                time = dialog.getTime();

                if (dialog.shouldNormalizeRests()) {
                    CommandHistory::getInstance()->addCommand(
                            new AddTimeSignatureAndNormalizeCommand(
                                    &composition,
                                    time,
                                    dialog.getTimeSignature()));
                } else {
                    CommandHistory::getInstance()->addCommand(
                            new AddTimeSignatureCommand(
                                    &composition,
                                    time,
                                    dialog.getTimeSignature()));
                }
            }

            break;
        }

    default:
        break;
    }
}

void
TempoView::updateWindowTitle()
{
    setWindowTitle(tr("%1 - Tempo and Time Signature Editor").
            arg(RosegardenDocument::currentDocument->getTitle()));
}

void
TempoView::slotHelpRequested()
{
    // TRANSLATORS: if the manual is translated into your language, you can
    // change the two-letter language code in this URL to point to your language
    // version, eg. "http://rosegardenmusic.com/wiki/doc:tempoView-es" for the
    // Spanish version. If your language doesn't yet have a translation, feel
    // free to create one.
    QString helpURL = tr("http://rosegardenmusic.com/wiki/doc:tempoView-en");
    QDesktopServices::openUrl(QUrl(helpURL));
}

void
TempoView::slotHelpAbout()
{
    new AboutDialog(this);
}

void
TempoView::slotDocumentModified(bool /*modified*/)
{
    // Update the name in the window title in case we just did a Save As.
    updateWindowTitle();

    updateList();
}


}
