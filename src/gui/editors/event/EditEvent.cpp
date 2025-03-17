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
#include "document/RosegardenDocument.h"
#include "gui/editors/guitar/Chord.h"
#include "misc/Strings.h"
#include "misc/Debug.h"
#include "misc/ConfigGroups.h"  // for WindowGeometryConfigGroup
#include "gui/dialogs/TimeDialog.h"
#include "sound/Midi.h"

#include <QComboBox>
#include <QDialog>
#include <QDialogButtonBox>
#include <QCheckBox>
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


// ??? This seems really reusable, however, it also seems like these
//     sub orderings aren't actually used where they need to be used.
//     E.g. a search on Controller::EventSubOrdering turns up far fewer
//     places than expected.  Shouldn't it be used whenever a
//     controller Event is created?  Shouldn't this "if" be in Event's
//     ctors?
static int getSubOrdering(std::string eventType)
{
    if (eventType == Indication::EventType) {
        return Indication::EventSubOrdering;
    } else if (eventType == Clef::EventType) {
        return Clef::EventSubOrdering;
    } else if (eventType == ::Rosegarden::Key::EventType) {
        return ::Rosegarden::Key::EventSubOrdering;
    } else if (eventType == Text::EventType) {
        return Text::EventSubOrdering;
    } else if (eventType == Note::EventRestType) {
        return Note::EventRestSubOrdering;
    } else if (eventType == PitchBend::EventType) {
        return PitchBend::EventSubOrdering;
    } else if (eventType == Controller::EventType) {
        return Controller::EventSubOrdering;
    } else if (eventType == KeyPressure::EventType) {
        return KeyPressure::EventSubOrdering;
    } else if (eventType == ChannelPressure::EventType) {
        return ChannelPressure::EventSubOrdering;
    } else if (eventType == ProgramChange::EventType) {
        return ProgramChange::EventSubOrdering;
    } else if (eventType == SystemExclusive::EventType) {
        return SystemExclusive::EventSubOrdering;
    }

    return 0;
}

EditEvent::EditEvent(QWidget *parent, const Event &event, bool inserting) :
    QDialog(parent),
    m_event(event)
{
    setModal(true);
    setWindowTitle(inserting ? tr("Insert Event").toStdString().c_str() :
                               tr("Edit Event").toStdString().c_str());

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

    // If the user is inserting a new Event, provide them with a
    // combo box for selecting the event type.
    if (inserting) {

        m_typeCombo = new QComboBox(propertiesGroup);
        m_typeCombo->addItem(strtoqstr(Note::EventType));
        m_typeCombo->addItem(strtoqstr(Controller::EventType));
        m_typeCombo->addItem(strtoqstr(KeyPressure::EventType));
        m_typeCombo->addItem(strtoqstr(ChannelPressure::EventType));
        m_typeCombo->addItem(strtoqstr(ProgramChange::EventType));
        m_typeCombo->addItem(strtoqstr(SystemExclusive::EventType));
        m_typeCombo->addItem(strtoqstr(PitchBend::EventType));
        m_typeCombo->addItem(strtoqstr(Indication::EventType));
        m_typeCombo->addItem(strtoqstr(Text::EventType));
        m_typeCombo->addItem(strtoqstr(Note::EventRestType));
        m_typeCombo->addItem(strtoqstr(Clef::EventType));
        m_typeCombo->addItem(strtoqstr(::Rosegarden::Key::EventType));
        m_typeCombo->addItem(strtoqstr(Guitar::Chord::EventType));
        // NRPN::EventType
        // RPN::EventType
        connect(m_typeCombo,
                    static_cast<void(QComboBox::*)(int)>(&QComboBox::activated),
                this, &EditEvent::slotEventTypeChanged);
        propertiesLayout->addWidget(m_typeCombo, row, 1);

    } else {  // Display event type read-only.

        m_typeLabel = new QLabel(propertiesGroup);
        m_typeLabel->setText(strtoqstr(m_event.getType()));
        propertiesLayout->addWidget(m_typeLabel, row, 1);

    }

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
    if (inserting) {
        // For insert mode we need a QWidget with a QStackedLayout and each
        // of the event widgets loaded into that.
//        m_eventWidgetStack = EventWidgetStack::create(this);
//        mainLayout->addWidget(m_eventWidgetStack);
    } else {  // editing
        // For edit mode we only need the Event widget for the current
        // event type.
        m_eventWidget = EventWidget::create(this, event);
        mainLayout->addWidget(m_eventWidget);
    }


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
    // ??? Is this ok for insert mode?
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

    // If we are inserting a new Event...
    if (m_typeCombo) {
        // Create a new default Event of the right type.
        // Just need the type, but there is no type-only ctor.
        event = Event(qstrtostr(m_typeCombo->currentText()), m_timeSpinBox->value());
    }

    // Let the widget make its changes.
    //if (m_eventWidgetStack)  // inserting
    //    m_eventWidgetStack->updateEvent(event);
    if (m_eventWidget)  // editing
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

        QTableWidgetItem *valueItem = m_propertyTable->item(row, 2);
        if (!valueItem)
            continue;
        QString value = valueItem->text();

        // ??? This doesn't preserve persistent vs. non-persistent.  It
        //     makes all properties persistent.

        // See Property.cpp for the type names.
        // See ConfigurationXmlSubHandler::characters() for similar code.
        if (type == "Int") {
            event.set<Int>(propertyName, value.toInt());
        } else if (type == "String") {
            event.set<String>(propertyName, qstrtostr(value));
        } else if (type == "Bool") {
            event.set<Bool>(propertyName, value == "true");  // ??? Really?
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
EditEvent::slotEventTypeChanged(int value)
{
    // ??? I don't think we want to do this.  Instead, we should ask the user
    //     for the Event type to insert, insert a default Event of that type
    //     and then allow them to edit that default Event.  That would
    //     remove the need for insert mode and a widget stack.

    // ??? Does QComboBox::currentText() work here?  It's simpler.
    std::string type = qstrtostr(m_typeCombo->itemText(value));

    // Make sure the sub-ordering is appropriate.
    m_subOrdering->setValue(getSubOrdering(type));

    // ??? Flip the EventWidgetStack.

}

void
EditEvent::slotEditAbsoluteTime()
{
    Composition &composition =
            RosegardenDocument::currentDocument->getComposition();

    TimeDialog dialog(this, tr("Edit Event Time"),
                      &composition,
                      m_timeSpinBox->value(),
                      true);
    if (dialog.exec() == QDialog::Accepted)
        m_timeSpinBox->setValue(dialog.getTime());
}

void EditEvent::addProperty(const PropertyName &name)
{
    // Add a row to the table
    const int row = m_propertyTable->rowCount();
    m_propertyTable->insertRow(row);

    // Go with bold for persistent properties.
    // ??? Actually, bold is hard to read.
    const bool bold = m_event.isPersistent(name);
    QFont boldFont;

    int col{0};

    // Name
    QTableWidgetItem *nameItem = new QTableWidgetItem(name.getName().c_str());
    if (bold) {
        boldFont = nameItem->font();
        boldFont.setBold(true);
        nameItem->setFont(boldFont);
    }
    m_propertyTable->setItem(row, col++, nameItem);

    // Type
    QTableWidgetItem *item = new QTableWidgetItem(
            m_event.getPropertyTypeAsString(name).c_str());
    // ??? For now, make this read-only.  If we want to allow editing
    //     of this, we need a combo box with the types in it.
    item->setFlags(item->flags() & ~Qt::ItemIsEditable);
    if (bold)
        item->setFont(boldFont);
    m_propertyTable->setItem(row, col++, item);

    // Value
    item = new QTableWidgetItem(m_event.getAsString(name).c_str());
    if (bold)
        item->setFont(boldFont);
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
    //if (m_eventWidgetStack)
    //    propertyFilter = m_eventWidgetStack->getPropertyFilter();
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

    QFont boldFont;

    int col{0};

    // Name
    QTableWidgetItem *nameItem = new QTableWidgetItem("newproperty");
    boldFont = nameItem->font();
    boldFont.setBold(true);
    nameItem->setFont(boldFont);
    m_propertyTable->setItem(row, col++, nameItem);

    // Type
    QTableWidgetItem *item = new QTableWidgetItem(type);
    // ??? For now, make this read-only.  If we want to allow editing
    //     of this, we need a combo box with the types in it.
    item->setFlags(item->flags() & ~Qt::ItemIsEditable);
    item->setFont(boldFont);
    m_propertyTable->setItem(row, col++, item);

    // Value
    item = new QTableWidgetItem(value);
    item->setFont(boldFont);
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
