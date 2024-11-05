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

#define RG_MODULE_STRING "[TempoAndTimeSignatureEditor]"
#define RG_NO_DEBUG_PRINT

#include "TempoAndTimeSignatureEditor.h"

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
    constexpr int IndexRole = Qt::UserRole + 2;

}


namespace Rosegarden
{


TempoAndTimeSignatureEditor::TempoAndTimeSignatureEditor(timeT openTime)
{

    updateWindowTitle();

    setStatusBar(new QStatusBar(this));

    // Connect for changes so we can update the list.
    connect(RosegardenDocument::currentDocument,
                &RosegardenDocument::documentModified,
            this, &TempoAndTimeSignatureEditor::slotDocumentModified);

    initMenu();

    // Create frame and layout.
    // ??? Why is this a QFrame?  QWidget should be enough.
    m_frame = new QFrame(this);
    m_frame->setMinimumSize(700, 300);
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
            this, &TempoAndTimeSignatureEditor::slotFilterClicked);
    filterGroupLayout->addWidget(m_tempoCheckBox);

    // Time Signature
    m_timeSigCheckBox = new QCheckBox(tr("Time Signature"), m_filterGroup);
    m_timeSigCheckBox->setChecked(a_timeSignatureFilter.get());
    connect(m_timeSigCheckBox, &QCheckBox::clicked,
            this, &TempoAndTimeSignatureEditor::slotFilterClicked);
    filterGroupLayout->addWidget(m_timeSigCheckBox);

    // Fill the rest of the empty space to keep the widgets together.
    filterGroupLayout->addStretch(1);

    // Tempo/Time Signature List
    m_tableWidget = new QTableWidget(m_frame);
    m_mainLayout->addWidget(m_tableWidget);
    // Disable double-click editing of each field.
    m_tableWidget->setEditTriggers(QAbstractItemView::NoEditTriggers);
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
    m_tableWidget->setColumnWidth(0, 110);
    m_tableWidget->setColumnWidth(1, 120);
    //m_tableWidget->setMinimumWidth(700);
    connect(m_tableWidget, &QTableWidget::cellDoubleClicked,
            this, &TempoAndTimeSignatureEditor::slotPopupEditor);

    //m_frame->setMinimumSize(m_mainLayout->minimumSize());

    // Update the list.
    updateTable();
    makeInitialSelection(openTime);

    // Restore window geometry and header state.
    QSettings settings;
    settings.beginGroup(WindowGeometryConfigGroup);
    restoreGeometry(settings.value("Tempo_View2_Geometry").toByteArray());
    //restoreState(settings.value("Tempo_View2_State").toByteArray());
    m_tableWidget->horizontalHeader()->restoreState(
            settings.value("Tempo_View2_Header_State").toByteArray());
    settings.endGroup();

    // Make sure the last column fills the widget.
    // Note: Must do this AFTER restoreState() or else it will not work.
    m_tableWidget->horizontalHeader()->setStretchLastSection(true);

    m_doc->getComposition().addObserver(this);

}

TempoAndTimeSignatureEditor::~TempoAndTimeSignatureEditor()
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
TempoAndTimeSignatureEditor::closeEvent(QCloseEvent *e)
{
    // Let RosegardenMainWindow know we are going down.
    emit closing();

    EditViewBase::closeEvent(e);
}

void
TempoAndTimeSignatureEditor::tempoChanged(const Composition * /*comp*/)
{
    updateTable();
}

void
TempoAndTimeSignatureEditor::timeSignatureChanged(const Composition * /*comp*/)
{
    updateTable();
}

void
TempoAndTimeSignatureEditor::updateTable()
{
    // Preserve Selection.

    // We use a key instead of an index because indexes change.  Keys
    // do not.  This guarantees that we recreate the original selection
    // as closely as possible.
    struct Key {
        timeT midiTicks{0};
        Type itemType{Type::TimeSignature};

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

    // For each row...
    for (int row = 0; row < m_tableWidget->rowCount(); ++row) {
        QTableWidgetItem *item = m_tableWidget->item(row, 0);
        if (!item)
            continue;

        // Not selected?  Try the next...
        if (!item->isSelected())
            continue;

        // Create key.
        Key key;
        bool ok{false};
        key.midiTicks = item->data(TimeRole).toLongLong(&ok);
        if (!ok)
            continue;
        key.itemType = (Type)item->data(TypeRole).toInt(&ok);
        if (!ok)
            continue;

        // Add to set.
        selectionSet.insert(key);
    }

    // Preserve the "current item".

    bool haveCurrentItem;
    Key currentItemKey;
    int currentItemColumn;

    // Scope to avoid accidentally reusing currentItem after it is gone.
    {
        // The "current item" has the focus outline which is only
        // visible if it happens to be selected.
        QTableWidgetItem *currentItem = m_tableWidget->currentItem();
        haveCurrentItem = currentItem;
        if (haveCurrentItem) {
            int row = m_tableWidget->row(currentItem);
            currentItemColumn = m_tableWidget->column(currentItem);

            // We need the column 0 item since it is the only one with data.
            QTableWidgetItem *item0 = m_tableWidget->item(row, 0);

            // Make a key so we can make it current again if it still exists.
            currentItemKey.midiTicks = item0->data(TimeRole).toLongLong();
            currentItemKey.itemType = (Type)item0->data(TypeRole).toInt();
        }
    }

    // Recreate list.

    // Clear the list completely.
    m_tableWidget->setRowCount(0);

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

            // Use a QVariant so that the table sorts properly.
            const QVariant timeVariant = comp->makeTimeVariant(
                    sig.first,
                    static_cast<Composition::TimeMode>(a_timeMode.get()));

            // Add a row to the table
            const int row = m_tableWidget->rowCount();
            m_tableWidget->insertRow(row);

            // Time
            QTableWidgetItem *item = new QTableWidgetItem;
            item->setData(Qt::EditRole, timeVariant);
            item->setData(TimeRole, QVariant(qlonglong(sig.first)));
            item->setData(TypeRole, (int)Type::TimeSignature);
            item->setData(IndexRole, timeSignatureIndex);
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
            if (haveCurrentItem  &&
                currentItemKey.itemType == Type::TimeSignature  &&
                currentItemKey.midiTicks == sig.first) {
                item = m_tableWidget->item(row, currentItemColumn);
                if (item)
                    m_tableWidget->setCurrentItem(item);
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

            // Use a QVariant so that the table sorts properly.
            const QVariant timeVariant = comp->makeTimeVariant(
                    time, static_cast<Composition::TimeMode>(a_timeMode.get()));

            // Add a row to the table
            const int row = m_tableWidget->rowCount();
            m_tableWidget->insertRow(row);

            // Time
            QTableWidgetItem *item = new QTableWidgetItem;
            item->setData(Qt::EditRole, timeVariant);
            item->setData(TimeRole, QVariant(qlonglong(time)));
            item->setData(TypeRole, (int)Type::Tempo);
            item->setData(IndexRole, tempoIndex);
            m_tableWidget->setItem(row, 0, item);

            // Type
            item = new QTableWidgetItem(tr("Tempo   "));
            m_tableWidget->setItem(row, 1, item);

            // Value
            item = new QTableWidgetItem(desc);
            m_tableWidget->setItem(row, 2, item);

            // Properties
            // Put an empty one in or things get strange.
            item = new QTableWidgetItem();
            m_tableWidget->setItem(row, 3, item);

            // Set current if it is the right one.
            if (haveCurrentItem  &&
                currentItemKey.itemType == Type::Tempo  &&
                currentItemKey.midiTicks == time) {
                item = m_tableWidget->item(row, currentItemColumn);
                if (item)
                    m_tableWidget->setCurrentItem(item);
            }
        }
    }

    m_tableWidget->sortItems(0, Qt::AscendingOrder);


    // Restore Selection.

    bool haveSelection{false};

    // For each row...
    for (int row = 0; row < m_tableWidget->rowCount(); ++row) {
        QTableWidgetItem *item = m_tableWidget->item(row, 0);
        if (!item)
            continue;

        // Create key.
        Key key;
        key.midiTicks = item->data(TimeRole).toLongLong();
        key.itemType = (Type)item->data(TypeRole).toInt();

        // Not selected?  Try the next.
        if (selectionSet.find(key) == selectionSet.end())
            continue;

        // Select the entire row.
        // For each column...
        for (int col = 0; col < m_tableWidget->columnCount(); ++col) {
            QTableWidgetItem *item = m_tableWidget->item(row, col);
            if (!item)
                continue;
            item->setSelected(true);
        }

        haveSelection = true;
    }

    if (haveSelection)
        enterActionState("have_selection");
    else
        leaveActionState("have_selection");
}

void
TempoAndTimeSignatureEditor::makeInitialSelection(timeT time)
{
    // Select an item around the given time.

    QTableWidgetItem *foundItem{nullptr};
    int foundRow{0};

    // For each row...
    for (int row = 0; row < m_tableWidget->rowCount(); ++row) {
        QTableWidgetItem *item = m_tableWidget->item(row, 0);
        bool ok;
        const timeT itemTime = item->data(TimeRole).toLongLong(&ok);
        if (!ok)
            continue;

        // Past the time we are looking for?  We're done.
        if (itemTime > time)
            break;

        // Keep track of the last item we examined.
        foundItem = item;
        foundRow = row;
    }

    // Nothing found?  Bail.
    if (!foundItem)
        return;

    // Make it current so the keyboard works correctly.
    m_tableWidget->setCurrentItem(foundItem);

    // Select the entire row.
    // For each column...
    for (int col = 0; col < m_tableWidget->columnCount(); ++col) {
        QTableWidgetItem *item = m_tableWidget->item(foundRow, col);
        if (!item)
            continue;
        item->setSelected(true);
    }

    // Yield to the event loop so that the UI will be rendered before calling
    // scrollToItem().
    qApp->processEvents();

    // Make sure the item is visible.
    m_tableWidget->scrollToItem(foundItem);
}

void
TempoAndTimeSignatureEditor::select(timeT time, Type type)
{
    QTableWidgetItem *foundItem{nullptr};
    int foundRow{0};

    // For each row...
    for (int row = 0; row < m_tableWidget->rowCount(); ++row) {
        QTableWidgetItem *item = m_tableWidget->item(row, 0);
        bool ok;
        const timeT itemTime = item->data(TimeRole).toLongLong(&ok);
        if (!ok)
            continue;
        Type itemType = (Type)item->data(TypeRole).toInt(&ok);
        if (!ok)
            continue;

        // Found it?  We're done.
        if (itemTime == time  &&  itemType == type) {
            foundItem = item;
            foundRow = row;
            break;
        }
    }

    // Nothing found?  Bail.
    if (!foundItem)
        return;

    // Make it current so the keyboard works correctly.
    m_tableWidget->setCurrentItem(foundItem);

    // Select the entire row.
    // For each column...
    for (int col = 0; col < m_tableWidget->columnCount(); ++col) {
        QTableWidgetItem *item = m_tableWidget->item(foundRow, col);
        if (!item)
            continue;
        item->setSelected(true);
    }

    // Yield to the event loop so that the UI will be rendered before calling
    // scrollToItem().
    qApp->processEvents();

    // Make sure the item is visible.
    m_tableWidget->scrollToItem(foundItem);
}

Segment *
TempoAndTimeSignatureEditor::getCurrentSegment()
{
    // TempoAndTimeSignatureEditor does not deal in Segments.
    return nullptr;
}

void
TempoAndTimeSignatureEditor::slotEditDelete()
{
    Composition *composition =
            &RosegardenDocument::currentDocument->getComposition();

    MacroCommand *macroCommand = new MacroCommand(
            tr("Delete Tempo or Time Signature"));

    // For each row in reverse order...
    for (int row = m_tableWidget->rowCount() - 1; row >= 0; --row) {
        QTableWidgetItem *item = m_tableWidget->item(row, 0);
        if (!item)
            continue;

        // Skip any that aren't selected.
        if (!item->isSelected())
            continue;

        bool ok;
        const Type type = (Type)item->data(TypeRole).toInt(&ok);
        if (!ok)
            continue;

        const int index = item->data(IndexRole).toInt(&ok);
        if (!ok)
            continue;

        if (type == Type::TimeSignature) {
            macroCommand->addCommand(new RemoveTimeSignatureCommand(
                    composition, index));
        } else {  // Tempo
            macroCommand->addCommand(new RemoveTempoChangeCommand(
                    composition, index));
        }

    }

    if (macroCommand->haveCommands())
        CommandHistory::getInstance()->addCommand(macroCommand);
    else
        delete macroCommand;

    // No need to call updateList().  The CompositionObserver handlers
    // will be notified of the changes.
}

void
TempoAndTimeSignatureEditor::slotAddTempoChange()
{
    timeT insertTime{0};

    QList<QTableWidgetItem *> selectedItems = m_tableWidget->selectedItems();
    if (!selectedItems.empty()) {
        // These appear to be in order, so this will be the first column of
        // the first selected row.
        QTableWidgetItem *item = selectedItems[0];
        if (item->data(TimeRole) != QVariant())
            insertTime = item->data(TimeRole).toLongLong();
    }

    // Launch the TempoDialog.
    EditTempoController::self()->editTempo(
            this,  // parent
            insertTime,  // atTime
            true);  // timeEditable

    select(insertTime, Type::Tempo);
}

void
TempoAndTimeSignatureEditor::slotAddTimeSignatureChange()
{
    timeT insertTime{0};

    QList<QTableWidgetItem *> selectedItems = m_tableWidget->selectedItems();
    if (!selectedItems.empty()) {
        // These appear to be in order, so this will be the first column of
        // the first selected row.
        QTableWidgetItem *item = selectedItems[0];
        if (item->data(TimeRole) != QVariant())
            insertTime = item->data(TimeRole).toLongLong();
    }

    Composition &composition =
            RosegardenDocument::currentDocument->getComposition();
    Rosegarden::TimeSignature sig = composition.getTimeSignatureAt(insertTime);

    TimeSignatureDialog dialog(this, &composition, insertTime, sig, true);

    if (dialog.exec() != QDialog::Accepted)
        return;

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

    select(insertTime, Type::TimeSignature);

}

void
TempoAndTimeSignatureEditor::slotEditItem()
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
TempoAndTimeSignatureEditor::slotPopupEditor(int row, int /*col*/)
{
    // Get the row,0 item
    QTableWidgetItem *item = m_tableWidget->item(row, 0);
    if (!item)
        return;

    // Get time and type
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
TempoAndTimeSignatureEditor::slotSelectAll()
{
    for (int row = 0; row < m_tableWidget->rowCount(); ++row) {
        for (int col = 0; col < m_tableWidget->columnCount(); ++col) {
            QTableWidgetItem *item = m_tableWidget->item(row, col);
            if (!item)
                continue;
            item->setSelected(true);
        }
    }
}

void
TempoAndTimeSignatureEditor::slotClearSelection()
{
    for (int row = 0; row < m_tableWidget->rowCount(); ++row) {
        for (int col = 0; col < m_tableWidget->columnCount(); ++col) {
            QTableWidgetItem *item = m_tableWidget->item(row, col);
            if (!item)
                continue;
            item->setSelected(false);
        }
    }
}

void
TempoAndTimeSignatureEditor::initMenu()
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
TempoAndTimeSignatureEditor::slotFilterClicked(bool)
{
    updateTable();
}

void
TempoAndTimeSignatureEditor::slotViewMusicalTimes()
{
    findAction("time_musical")->setChecked(true);
    findAction("time_real")->setChecked(false);
    findAction("time_raw")->setChecked(false);

    a_timeMode.set((int)Composition::TimeMode::MusicalTime);

    updateTable();
}

void
TempoAndTimeSignatureEditor::slotViewRealTimes()
{
    findAction("time_musical")->setChecked(false);
    findAction("time_real")->setChecked(true);
    findAction("time_raw")->setChecked(false);

    a_timeMode.set((int)Composition::TimeMode::RealTime);

    updateTable();
}

void
TempoAndTimeSignatureEditor::slotViewRawTimes()
{
    findAction("time_musical")->setChecked(false);
    findAction("time_real")->setChecked(false);
    findAction("time_raw")->setChecked(true);

    a_timeMode.set((int)Composition::TimeMode::RawTime);

    updateTable();
}

void
TempoAndTimeSignatureEditor::popupEditor(timeT time, const Type type)
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
TempoAndTimeSignatureEditor::updateWindowTitle()
{
    setWindowTitle(tr("%1 - Tempo and Time Signature Editor").
            arg(RosegardenDocument::currentDocument->getTitle()));
}

void
TempoAndTimeSignatureEditor::slotHelpRequested()
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
TempoAndTimeSignatureEditor::slotHelpAbout()
{
    new AboutDialog(this);
}

void
TempoAndTimeSignatureEditor::slotDocumentModified(bool /*modified*/)
{
    // Update the name in the window title in case we just did a Save As.
    updateWindowTitle();

    updateTable();
}


}
