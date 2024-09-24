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

#define RG_MODULE_STRING "[TempoView2]"
//#define RG_NO_DEBUG_PRINT

#include "TempoView2.h"

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
#include <QTableWidget>
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


#define broken 0

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

    constexpr int TimeRole = Qt::UserRole;
    constexpr int TypeRole = Qt::UserRole + 1;

}


namespace Rosegarden
{


TempoView2::TempoView2(timeT openTime)
{

    updateWindowTitle();

    setStatusBar(new QStatusBar(this));

    // Connect for changes so we can update the list.
    connect(RosegardenDocument::currentDocument,
                &RosegardenDocument::documentModified,
            this, &TempoView2::slotDocumentModified);

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
            this, &TempoView2::slotFilterClicked);
    filterGroupLayout->addWidget(m_tempoCheckBox);

    // Time Signature
    m_timeSigCheckBox = new QCheckBox(tr("Time Signature"), m_filterGroup);
    m_timeSigCheckBox->setChecked(a_timeSignatureFilter.get());
    connect(m_timeSigCheckBox, &QCheckBox::clicked,
            this, &TempoView2::slotFilterClicked);
    filterGroupLayout->addWidget(m_timeSigCheckBox);

    // Fill the rest of the empty space to keep the widgets together.
    filterGroupLayout->addStretch(1);

    // Tempo/Time Signature List
    m_tableWidget = new QTableWidget(m_frame);
    m_mainLayout->addWidget(m_tableWidget);
    //m_tableWidget->setAllColumnsShowFocus(true);
    // ??? Need to disable double-click editing of each field!
    m_tableWidget->setSelectionMode(QAbstractItemView::ExtendedSelection);
    m_tableWidget->setSelectionBehavior(QAbstractItemView::SelectRows);
    // Hide the vertical header
    m_tableWidget->verticalHeader()->hide();
    QStringList headers;
    // ??? The extra space at the end of each of these is probably an
    //     attempt at getting the columns to be wider.  This does not
    //     work.  Leaving the space here to avoid creating work for the
    //     translators.  For now.
    headers << tr("Time  ") <<
               tr("Type  ") <<
               tr("Value  ") <<
               tr("Properties  ");
    m_tableWidget->setColumnCount(headers.size());
    m_tableWidget->setHorizontalHeaderLabels(headers);
    // Make sure columns have a reasonable amount of space.
    m_tableWidget->setColumnWidth(0, 133);
    m_tableWidget->setColumnWidth(1, 125);
#if broken  // ???
    connect(m_tableWidget, &QTableWidget::itemDoubleClicked,
            this, &TempoView2::slotPopupEditor);
#endif

    // Update the list.
    updateList();
    makeInitialSelection(openTime);

    // Restore window geometry and header state.
    QSettings settings;
    settings.beginGroup(WindowGeometryConfigGroup);
    restoreGeometry(settings.value("Tempo_View2_Geometry").toByteArray());
    //restoreState(settings.value("Tempo_View2_State").toByteArray());
    m_tableWidget->horizontalHeader()->restoreState(settings.value("Tempo_View2_Header_State").toByteArray());
    settings.endGroup();

    m_doc->getComposition().addObserver(this);

}

TempoView2::~TempoView2()
{
    // Save state for next time.
    a_tempoFilter.set(m_tempoCheckBox->checkState() != Qt::Unchecked);
    a_timeSignatureFilter.set(m_timeSigCheckBox->checkState() != Qt::Unchecked);

    // Save window geometry and header state.
    QSettings settings;
    settings.beginGroup(WindowGeometryConfigGroup);
    settings.setValue("Tempo_View2_Geometry", saveGeometry());
    //settings.setValue("Tempo_View2_State", saveState());
    settings.setValue("Tempo_View2_Header_State", m_tableWidget->horizontalHeader()->saveState());
    settings.endGroup();

    // We use m_doc instead of RosegardenDocument::currentDocument to
    // make sure that we disconnect from the old document when the
    // documents are switching.
    if (m_doc  &&  !isCompositionDeleted())
        m_doc->getComposition().removeObserver(this);
}

void
TempoView2::closeEvent(QCloseEvent *e)
{
    // Let RosegardenMainWindow know we are going down.
    emit closing();

    EditViewBase::closeEvent(e);
}

void
TempoView2::tempoChanged(const Composition * /*comp*/)
{
    updateList();
}

void
TempoView2::timeSignatureChanged(const Composition * /*comp*/)
{
    updateList();
}

void
TempoView2::updateList()
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
         itemIndex < m_tableWidget->rowCount();
         ++itemIndex) {
        TempoListItem *item =
                dynamic_cast<TempoListItem *>(m_tableWidget->item(itemIndex, 0));
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

    bool haveCurrentItem;
    Key currentItemKey;
    // It's possible for the current item to not be selected.  Keep
    // track of that so we can restore it exactly.
    bool currentItemSelected;

    // Scope to avoid accidentally reusing currentItem after it is gone.
    {
        // The "current item" has the focus outline which is only
        // visible if it happens to be selected.
        TempoListItem *currentItem =
                dynamic_cast<TempoListItem *>(m_tableWidget->currentItem());
        haveCurrentItem = currentItem;
        if (haveCurrentItem) {
            // Make a key so we can make it current again if it still exists.
            currentItemKey.midiTicks = currentItem->getTime();
            currentItemKey.itemType = currentItem->getType();
            currentItemSelected = currentItem->isSelected();
        }
    }

    // Preserve scroll position.
    // ??? But in BankEditorDialog, the tree remembers its scroll position
    //     without any special code at all.  Why does this forget it?
    //     Does it actually forget it?  Can we rewrite to make sure it can
    //     remember without additional code?
    int scrollPos{0};
    if (m_tableWidget->verticalScrollBar())
        scrollPos = m_tableWidget->verticalScrollBar()->value();

    // Recreate list.

    m_tableWidget->clearContents();

    Composition *comp = &RosegardenDocument::currentDocument->getComposition();

    // Time Signatures
    if (m_timeSigCheckBox->isChecked()) {

        for (int timeSignatureIndex = 0;
             timeSignatureIndex < comp->getTimeSignatureCount();
             ++timeSignatureIndex) {

            const std::pair<timeT, Rosegarden::TimeSignature> sig =
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

            const QString timeString = comp->makeTimeString(
                    sig.first,
                    static_cast<Composition::TimeMode>(a_timeMode.get()));

            // Add a row to the table
            const int row = m_tableWidget->rowCount();
            m_tableWidget->insertRow(row);

            // Time
            QTableWidgetItem *item =
                    new QTableWidgetItem(timeString);
            item->setData(TimeRole, QVariant(qlonglong(sig.first)));
            item->setData(TypeRole, (int)Type::TimeSignature);
            m_tableWidget->setItem(row, 0, item);

            // Type
            item = new QTableWidgetItem(tr("Time Signature   "));
            m_tableWidget->setItem(row, 1, item);

            // Value
            item = new QTableWidgetItem(
                    QString("%1/%2   ").
                            arg(sig.second.getNumerator()).
                            arg(sig.second.getDenominator()));
            m_tableWidget->setItem(row, 2, item);

            // Properties
            item = new QTableWidgetItem(properties);
            m_tableWidget->setItem(row, 3, item);

            // Set current if it is the right one.
            // Setting current will clear any selection so we do it
            // before we set the selection.
            if (haveCurrentItem  &&
                currentItemKey.itemType == TempoListItem::TimeSignature) {
                if (sig.first == currentItemKey.midiTicks) {
#if broken  // ???
                    m_tableWidget->setCurrentItem(item);
                    item->setSelected(currentItemSelected);
#else
                    (void)currentItemSelected;
#endif
                }
            }
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

            // Add a row to the table
            const int row = m_tableWidget->rowCount();
            m_tableWidget->insertRow(row);

            // Time
            QTableWidgetItem *item =
                    new QTableWidgetItem(timeString);
            item->setData(TimeRole, QVariant(qlonglong(time)));
            item->setData(TypeRole, (int)Type::Tempo);
            m_tableWidget->setItem(row, 0, item);

            // Type
            item = new QTableWidgetItem(tr("Tempo   "));
            m_tableWidget->setItem(row, 1, item);

            // Value
            item = new QTableWidgetItem(desc);
            m_tableWidget->setItem(row, 2, item);

            // Set current if it is the right one.
            // Setting current will clear any selection so we do it
            // before we set the selection.
            if (haveCurrentItem  &&
                currentItemKey.itemType == TempoListItem::Tempo) {
                if (time == currentItemKey.midiTicks) {
#if broken  // ???
                    m_tableWidget->setCurrentItem(item);
                    item->setSelected(currentItemSelected);
#endif
                }
            }
        }
    }

    m_tableWidget->sortItems(0, Qt::AscendingOrder);


    // Restore Selection.

    bool haveSelection{false};

#if broken  // ???
    // For each item in the list...
    for (int itemIndex = 0;
         itemIndex < m_tableWidget->topLevelItemCount();
         ++itemIndex) {
        TempoListItem *item =
                dynamic_cast<TempoListItem *>(m_tableWidget->topLevelItem(itemIndex));
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
#endif

    if (haveSelection)
        enterActionState("have_selection");
    else
        leaveActionState("have_selection");

    // Restore scroll position.
    if (scrollPos  &&  m_tableWidget->verticalScrollBar())
        m_tableWidget->verticalScrollBar()->setValue(scrollPos);
}

void
TempoView2::makeInitialSelection(timeT time)
{
    // Select an item around the given time.

    // Note that this is complicated by the fact that Time Signatures
    // appear before Tempos.


#if broken  // ???
    TempoListItem *goodItem{nullptr};

    // For each item...
    for (int itemIndex = 0;
         itemIndex < m_tableWidget->topLevelItemCount();
         ++itemIndex) {
        TempoListItem *item =
                dynamic_cast<TempoListItem *>(m_tableWidget->topLevelItem(itemIndex));
        if (!item)
            continue;

        // Past the time we are looking for?  We're done.
        if (item->getTime() > time)
            break;

        // Keep track of the last item we examined.
        goodItem = item;
    }

    if (goodItem) {
        goodItem->setSelected(true);
        m_tableWidget->scrollToItem(goodItem);
    }
#else
    (void)time;
#endif
}

Segment *
TempoView2::getCurrentSegment()
{
    // TempoView2 does not deal in Segments.
    return nullptr;
}

void
TempoView2::slotEditDelete()
{
    MacroCommand *macroCommand = new MacroCommand(
            tr("Delete Tempo or Time Signature"));

#if broken  // ???
    // For each item in the list in reverse order...
    for (int itemIndex = m_tableWidget->topLevelItemCount() - 1;
         itemIndex >= 0;
         --itemIndex) {
        TempoListItem *item = dynamic_cast<TempoListItem *>(
                m_tableWidget->topLevelItem(itemIndex));
        if (!item)
            continue;

        // Skip any that aren't selected.
        if (!item->isSelected())
            continue;

        if (item->getType() == TempoListItem::TimeSignature) {
            macroCommand->addCommand(new RemoveTimeSignatureCommand(
                    item->getComposition(),
                    item->getIndex()));
        } else {  // Tempo
            macroCommand->addCommand(new RemoveTempoChangeCommand(
                    item->getComposition(),
                    item->getIndex()));
        }
    }
#endif

    if (macroCommand->haveCommands())
        CommandHistory::getInstance()->addCommand(macroCommand);
    else
        delete macroCommand;

    // No need to call updateList().  The CompositionObserver handlers
    // will be notified of the changes.
}

void
TempoView2::slotAddTempoChange()
{
    timeT insertTime = 0;

#if broken  // ???
    QList<QTreeWidgetItem *> selection = m_tableWidget->selectedItems();

    // If something is selected, use the time of that item.
    if (!selection.empty()) {
        TempoListItem *item =
            dynamic_cast<TempoListItem *>(selection.first());
        if (item)
            insertTime = item->getTime();
    }
#endif

    // Launch the TempoDialog.
    EditTempoController::self()->editTempo(
            this,  // parent
            insertTime,  // atTime
            true);  // timeEditable
}

void
TempoView2::slotAddTimeSignatureChange()
{
    timeT insertTime = 0;

#if broken  // ???
    QList<QTreeWidgetItem*> selection = m_tableWidget->selectedItems();

    // If something is selected, use the time of that item.
    if (!selection.empty()) {
        TempoListItem *item =
            dynamic_cast<TempoListItem *>(selection.first());
        if (item)
            insertTime = item->getTime();
    }
#endif

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
TempoView2::slotEditItem()
{
    QList<QTableWidgetItem *> selectedItems = m_tableWidget->selectedItems();
    if (selectedItems.empty())
        return;

    // These appear to be in order, so this will be the first column of
    // the first selected row.
    QTableWidgetItem *item = selectedItems[0];
    if (item->data(TimeRole) == QVariant())
        return;

    bool ok;
    const timeT time = item->data(TimeRole).toLongLong(&ok);
    if (!ok)
        return;

    const Type type = (Type)item->data(TypeRole).toInt(&ok);
    if (!ok)
        return;

    popupEditor(time, type);
}

void
TempoView2::slotSelectAll()
{
#if broken  // ???
    for (int i = 0; i < m_tableWidget->topLevelItemCount(); ++i) {
        m_tableWidget->topLevelItem(i)->setSelected(true);
    }
#endif
}

void
TempoView2::slotClearSelection()
{
#if broken  // ???
    for (int i = 0; i < m_tableWidget->topLevelItemCount(); ++i) {
        m_tableWidget->topLevelItem(i)->setSelected(false);
    }
#endif
}

void
TempoView2::initMenu()
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
TempoView2::slotFilterClicked(bool)
{
    updateList();
}

void
TempoView2::slotViewMusicalTimes()
{
    findAction("time_musical")->setChecked(true);
    findAction("time_real")->setChecked(false);
    findAction("time_raw")->setChecked(false);

    a_timeMode.set((int)Composition::TimeMode::MusicalTime);

    updateList();
}

void
TempoView2::slotViewRealTimes()
{
    findAction("time_musical")->setChecked(false);
    findAction("time_real")->setChecked(true);
    findAction("time_raw")->setChecked(false);

    a_timeMode.set((int)Composition::TimeMode::RealTime);

    updateList();
}

void
TempoView2::slotViewRawTimes()
{
    findAction("time_musical")->setChecked(false);
    findAction("time_real")->setChecked(false);
    findAction("time_raw")->setChecked(true);

    a_timeMode.set((int)Composition::TimeMode::RawTime);

    updateList();
}

void
TempoView2::popupEditor(timeT time, const Type type)
{
    switch (type)
    {

    case Type::Tempo:
        // Launch the TempoDialog.
        EditTempoController::self()->editTempo(
                this, time, true /* timeEditable */);
        break;

    case Type::TimeSignature:
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
TempoView2::updateWindowTitle()
{
    setWindowTitle(tr("%1 - Tempo and Time Signature Editor").
            arg(RosegardenDocument::currentDocument->getTitle()));
}

void
TempoView2::slotHelpRequested()
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
TempoView2::slotHelpAbout()
{
    new AboutDialog(this);
}

void
TempoView2::slotDocumentModified(bool /*modified*/)
{
    // Update the name in the window title in case we just did a Save As.
    updateWindowTitle();

    updateList();
}


}
