/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2025 the Rosegarden development team.

    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#define RG_MODULE_STRING "[EditEvent]"
#define RG_NO_DEBUG_PRINT

#include "EditEvent.h"

#include "EventWidget.h"

#include "base/Event.h"
#include "base/MidiTypes.h"
#include "base/NotationTypes.h"
#include "gui/editors/guitar/Chord.h"
#include "misc/Strings.h"
#include "misc/Debug.h"
#include "misc/ConfigGroups.h"  // for WindowGeometryConfigGroup
#include "gui/dialogs/TimeDialog.h"
#include "sound/Midi.h"

#include <QComboBox>
#include <QDialog>
#include <QDialogButtonBox>
#include <QGroupBox>
#include <QGridLayout>
#include <QHeaderView>
#include <QLabel>
#include <QMenu>
#include <QMessageBox>
#include <QPushButton>
#include <QSpinBox>
#include <QString>
#include <QTableWidget>
#include <QWidget>
#include <QVBoxLayout>
#include <QSettings>


namespace Rosegarden
{


EditEvent::EditEvent(QWidget *parent, const Event &event) :
    QDialog(parent),
    m_event(event)
{
    setModal(true);
    setWindowTitle(tr("Edit Event"));

    QVBoxLayout *mainLayout = new QVBoxLayout(this);


    // Properties Group

    QGroupBox *propertiesGroup = new QGroupBox(tr("Properties"), this);
    propertiesGroup->setContentsMargins(5, 5, 5, 5);

    QGridLayout *propertiesLayout = new QGridLayout(propertiesGroup);
    propertiesLayout->setSpacing(5);
    mainLayout->addWidget(propertiesGroup);

    int row{0};

    // Event type
    propertiesLayout->addWidget(
            new QLabel(tr("Event type:"), propertiesGroup), row, 0);

    m_typeLabel = new QLabel(propertiesGroup);
    m_typeLabel->setText(strtoqstr(m_event.getType()));
    propertiesLayout->addWidget(m_typeLabel, row, 1);

    ++row;

    // Absolute time
    propertiesLayout->addWidget(
            new QLabel(tr("Absolute time:"), propertiesGroup), row, 0);

    m_timeSpinBox = new QSpinBox(propertiesGroup);
    m_timeSpinBox->setMinimum(INT_MIN);
    m_timeSpinBox->setMaximum(INT_MAX);
    m_timeSpinBox->setSingleStep(Note(Note::Shortest).getDuration());
    m_timeSpinBox->setValue(m_event.getAbsoluteTime());
    propertiesLayout->addWidget(m_timeSpinBox, row, 1);

    m_timeEditButton = new QPushButton(tr("edit"), propertiesGroup);
    connect(m_timeEditButton, &QPushButton::clicked,
            this, &EditEvent::slotEditAbsoluteTime);
    propertiesLayout->addWidget(m_timeEditButton, row, 2);

    // Event Widget
    m_eventWidget = EventWidget::create(this, event);
    mainLayout->addWidget(m_eventWidget);


    // Advanced Properties Group

    QGroupBox *advancedPropertiesGroup =
            new QGroupBox(tr("Advanced Properties"), this);
    advancedPropertiesGroup->setContentsMargins(5, 5, 5, 5);

    QGridLayout *advancedPropertiesLayout =
            new QGridLayout(advancedPropertiesGroup);
    advancedPropertiesLayout->setSpacing(5);
    // Make sure stretch is 1 so this will fill the rest of the dialog.
    mainLayout->addWidget(advancedPropertiesGroup, 1);

    row = 0;

    // Sub Ordering
    advancedPropertiesLayout->addWidget(
            new QLabel(tr("Sub-ordering: ")), row, 0);

    m_subOrdering = new QSpinBox(advancedPropertiesGroup);
    m_subOrdering->setRange(-100, 100);
    m_subOrdering->setSingleStep(1);
    m_subOrdering->setValue(event.getSubOrdering());
    advancedPropertiesLayout->addWidget(m_subOrdering, row, 1);

    ++row;

    // Spacer
    advancedPropertiesLayout->setRowMinimumHeight(row, 10);

    ++row;

    advancedPropertiesLayout->addWidget(
            new QLabel(tr("Additional Properties")),
            row, 0, 1, 2, Qt::AlignHCenter);

    ++row;

    // Property table.

    m_propertyTable = new QTableWidget(advancedPropertiesGroup);
    m_propertyTable->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(m_propertyTable, &QWidget::customContextMenuRequested,
            this, &EditEvent::slotContextMenu);
    // Copy properties from m_event to the table.
    updatePropertyTable();
    advancedPropertiesLayout->addWidget(m_propertyTable, row, 0, 1, 2);
    // Make sure the table expands to fill any empty space.
    advancedPropertiesLayout->setRowStretch(row, 1);

    // Button Box
    QDialogButtonBox *buttonBox = new QDialogButtonBox(
            QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    mainLayout->addWidget(buttonBox);
    connect(buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);

    loadOptions();

    // Make sure the last column fills the widget.
    // Note: Must do this AFTER loadOptions() or else it will not work.
    m_propertyTable->horizontalHeader()->setStretchLastSection(true);
}

EditEvent::~EditEvent()
{
    saveOptions();
}

void
EditEvent::saveOptions()
{
    QSettings settings;

    // Save window geometry and toolbar/dock state
    settings.beginGroup(WindowGeometryConfigGroup);
    settings.setValue("Edit_Event_Geometry", saveGeometry());
    //settings.setValue("Edit_Event_State", saveState());
    settings.setValue("Edit_Event_Property_Table_Header_State",
            m_propertyTable->horizontalHeader()->saveState());
}

void
EditEvent::loadOptions()
{
    // Note that Wayland does not allow top-level window positioning.

    QSettings settings;
    settings.beginGroup(WindowGeometryConfigGroup);
    restoreGeometry(settings.value("Edit_Event_Geometry").toByteArray());
    //restoreState(settings.value("Edit_Event_State").toByteArray());
    m_propertyTable->horizontalHeader()->restoreState(
            settings.value("Edit_Event_Property_Table_Header_State").toByteArray());
    settings.endGroup();
}

timeT
EditEvent::getAbsoluteTime() const
{
    return m_timeSpinBox->value();
}

Event
EditEvent::getEvent()
{
    // Start with the original.
    Event event(m_event, m_timeSpinBox->value());

    // Let the widget make its changes.
    if (m_eventWidget)
        m_eventWidget->updateEvent(event);

    // Changes from the Advanced Properties.

    // Sub-Ordering
    event.setSubOrdering(m_subOrdering->value());

    // Set the remaining properties from the table.

    // For each row in the table...
    for (int row = 0; row < m_propertyTable->rowCount(); ++row) {
        QTableWidgetItem *nameItem = m_propertyTable->item(row, 0);
        if (!nameItem)
            continue;
        if (nameItem->text() == "")
            continue;
        PropertyName propertyName(qstrtostr(nameItem->text()));

        QTableWidgetItem *typeItem = m_propertyTable->item(row, 1);
        if (!typeItem)
            continue;
        QString type = typeItem->text();
        // If non-persistent...
        if (type.endsWith(" (n)")) {
            type = type.left(type.length() - 4);
            // ??? Probably want to preserve this and recreate below.  As it
            //     is now, all properties end up persistent.  It is difficult
            //     to track down non-persistent properties in the code, so I
            //     have no test case to verify this.
            //persistent = false
        }

        QTableWidgetItem *valueItem = m_propertyTable->item(row, 2);
        if (!valueItem)
            continue;
        QString value = valueItem->text();

        // See Property.cpp for the type names.
        // See ConfigurationXmlSubHandler::characters() for similar code.
        if (type == "Int") {
            event.set<Int>(propertyName, value.toInt());
        } else if (type == "String") {
            event.set<String>(propertyName, qstrtostr(value));
        } else if (type == "Bool") {
            event.set<Bool>(propertyName, value == "true");
        } else if (type == "RealTimeT") {
            // Unused.  Turns out this is only used for tempo segments which
            // the user cannot edit.  TempoTimestampProperty is the only
            // property that uses this.  See Composition::setTempoTimestamp().
            // We can leave this disabled for now.  If we eventually need this,
            // we will need to upgrade RealTime to be able to convert a string
            // to a RealTime value.
            //RealTime rt(value);
            //event.set<RealTimeT>(propertyName, rt);
        }
    }

    return event;
}

void
EditEvent::slotEditAbsoluteTime()
{
    TimeDialog dialog(this,  // parent
                      tr("Edit Event Time"),  // title
                      m_timeSpinBox->value(),  // defaultTime
                      true);  // constrainToCompositionDuration
    if (dialog.exec() == QDialog::Accepted)
        m_timeSpinBox->setValue(dialog.getTime());
}

void EditEvent::addProperty(const PropertyName &name)
{
    // Add a row to the table
    const int row = m_propertyTable->rowCount();
    m_propertyTable->insertRow(row);

    int col{0};

    // Name
    QTableWidgetItem *nameItem = new QTableWidgetItem(name.getName().c_str());
    m_propertyTable->setItem(row, col++, nameItem);

    // Type
    QString type = m_event.getPropertyTypeAsString(name).c_str();
    // Not persistent?  Add "(n)".
    if (!m_event.isPersistent(name))
        type += " (n)";
    QTableWidgetItem *item = new QTableWidgetItem(type);
    // Read-only
    item->setFlags(item->flags() & ~Qt::ItemIsEditable);
    m_propertyTable->setItem(row, col++, item);

    // Value
    item = new QTableWidgetItem(m_event.getAsString(name).c_str());
    m_propertyTable->setItem(row, col++, item);
}

void EditEvent::updatePropertyTable()
{
    // See EventListEditor.

    // Hide the vertical header
    m_propertyTable->verticalHeader()->hide();

    QStringList columnNames;
    columnNames << tr("Name");
    columnNames << tr("Type");
    columnNames << tr("Value");
    m_propertyTable->setColumnCount(columnNames.size());
    m_propertyTable->setHorizontalHeaderLabels(columnNames);

    // Add persistent properties to the table.

    // Get the property filter.
    std::set<PropertyName> propertyFilter;
    if (m_eventWidget)
        propertyFilter = m_eventWidget->getPropertyFilter();

    m_propertyTable->setRowCount(0);

    Event::PropertyNames propertyNames = m_event.getPersistentPropertyNames();

    // For each property, add to table.
    for (const PropertyName &propertyName : propertyNames) {
        // Skip any that need filtering.
        if (propertyFilter.find(propertyName) != propertyFilter.end())
            continue;
        addProperty(propertyName);
    }

    // Add non-persistent properties to the table.

    propertyNames = m_event.getNonPersistentPropertyNames();

    // For each property, add to table.
    for (const PropertyName &propertyName : propertyNames) {
        // Skip any that need filtering.
        if (propertyFilter.find(propertyName) != propertyFilter.end())
            continue;
        addProperty(propertyName);
    }
}

void EditEvent::addProperty2(const QString &type, const QString &value)
{
    // Add a row to the table
    const int row = m_propertyTable->rowCount();
    m_propertyTable->insertRow(row);

    // Assume persistent.

    int col{0};

    // Name
    QTableWidgetItem *nameItem = new QTableWidgetItem("newproperty");
    m_propertyTable->setItem(row, col++, nameItem);

    // Type
    QTableWidgetItem *item = new QTableWidgetItem(type);
    // Read-only
    item->setFlags(item->flags() & ~Qt::ItemIsEditable);
    m_propertyTable->setItem(row, col++, item);

    // Value
    item = new QTableWidgetItem(value);
    m_propertyTable->setItem(row, col++, item);

    m_propertyTable->scrollToBottom();
}

void EditEvent::slotAddInteger()
{
    addProperty2("Int", "0");
}

void EditEvent::slotAddString()
{
    addProperty2("String", "");
}

void EditEvent::slotAddBoolean()
{
    addProperty2("Bool", "false");
}

void EditEvent::slotDelete()
{
    QTableWidgetItem *item = m_propertyTable->currentItem();
    if (!item)
        return;

    const int row = item->row();

    if (item->column() != 0) {
        item = m_propertyTable->item(row, 0);
        if (!item)
            return;
    }

    int reply = QMessageBox::warning(
            this,
            tr("Rosegarden"),
            tr("About to delete property \"%1\".  Are you sure?").arg(item->text()),
            QMessageBox::Yes | QMessageBox::No);
    if (reply != QMessageBox::Yes)
        return;

    m_propertyTable->removeRow(row);
}

void EditEvent::slotContextMenu(const QPoint &pos)
{
    // If the context menu hasn't been created, create it.
    if (!m_contextMenu) {
        m_contextMenu = new QMenu(this);
        if (!m_contextMenu) {
            RG_WARNING << "slotContextMenu() : Couldn't create context menu.";
            return;
        }

        // Add Integer
        QAction *addIntegerAction = m_contextMenu->addAction(
                tr("Add Integer Property"));
        connect(addIntegerAction, &QAction::triggered,
                this, &EditEvent::slotAddInteger);

        // Add String
        QAction *addStringAction = m_contextMenu->addAction(
                tr("Add String Property"));
        connect(addStringAction, &QAction::triggered,
                this, &EditEvent::slotAddString);

        // Add Boolean
        QAction *addBooleanAction = m_contextMenu->addAction(
                tr("Add Boolean Property"));
        connect(addBooleanAction, &QAction::triggered,
                this, &EditEvent::slotAddBoolean);

        m_contextMenu->addSeparator();

        // Delete
        QAction *deleteAction = m_contextMenu->addAction(tr("Delete"));
        connect(deleteAction, &QAction::triggered,
                this, &EditEvent::slotDelete);
    }

    // Launch the context menu.
    m_contextMenu->exec(m_propertyTable->mapToGlobal(pos));
}


}
