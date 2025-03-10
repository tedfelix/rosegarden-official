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

#ifndef RG_EDITEVENT_H
#define RG_EDITEVENT_H

#include "base/Event.h"

#include <QDialog>

class QComboBox;
class QLabel;
class QMenu;
class QPushButton;
class QSpinBox;
class QString;
class QWidget;
class QTableWidget;


namespace Rosegarden
{


class EventWidgetStack;
class EventWidget;


/// The "Edit Event" dialog.
/**
 * AKA the "Event Editor".
 *
 * ??? Still debating whether this should have an Insert mode.
 *
 * This replaces SimpleEventEditDialog and EventEditDialog.
 */
class EditEvent : public QDialog
{
    Q_OBJECT

public:

    EditEvent(QWidget *parent,
              const Event &event,
              bool inserting = false);  // Inserting or editing.
    ~EditEvent();

    /// Used by NoteWidget for editing durations with TimeDialog.
    timeT getAbsoluteTime() const;

    /// Get the edited or new (for insertion) Event.
    Event getEvent();

private slots:

    void slotEventTypeChanged(int value);
    void slotEditAbsoluteTime();
    void slotContextMenu(const QPoint &pos);

    void slotAddInteger();
    void slotAddString();
    void slotAddBoolean();
    void slotDelete();

private:

    void saveOptions();
    void loadOptions();

    // Copy of the original Event to use as a starting point for the edited
    // Event.
    const Event m_event;


    // Widgets

    // Event type
    QComboBox *m_typeCombo{nullptr};  // For insert.
    QLabel *m_typeLabel{nullptr};  // For edit.

    // Absolute time
    QSpinBox *m_timeSpinBox;
    QPushButton *m_timeEditButton;

    // Event
    EventWidgetStack *m_eventWidgetStack{nullptr};  // For insert.
    EventWidget *m_eventWidget{nullptr};  // For edit.

    // Sub-ordering
    QSpinBox *m_subOrdering;

    // Property Table
    QTableWidget *m_propertyTable;
    /// Add a single property to m_propertyTable.
    void addProperty(const PropertyName &name);
    /// Copy all properties from m_event to m_propertyTable.
    void updatePropertyTable();
    QMenu *m_contextMenu;
    /// For the context menu.
    void addProperty2(const QString &type, const QString &value);

#if 0
    // Unused code.  Clean this up at some point.  Keeping for reference,
    // though this is all in SimpleEventEditDialog.

    // Setup the dialog for a new event type
    void updateWidgets();

    void slotSysexLoad();
    void slotSysexSave();
    //void slotPitchChanged(int value);
    //void slotVelocityChanged(int value);

    std::string m_type;
    //timeT m_duration;
    //timeT m_notationAbsoluteTime;
    timeT m_notationDuration;

    QLabel *m_pitchLabel;
    QSpinBox *m_pitchSpinBox;
    QPushButton *m_pitchEditButton;

    QLabel *m_velocityLabel;
    QSpinBox *m_velocitySpinBox;

    QLabel *m_metaLabel;
    LineEdit *m_metaEdit;

    QLabel *m_controllerLabel;
    QLabel *m_controllerLabelValue;

    QPushButton *m_sysexLoadButton;
    QPushButton *m_sysexSaveButton;

    // Notation Group Box
    QGroupBox *m_notationGroupBox;
    QLabel *m_notationTimeLabel;
    QLabel *m_notationDurationLabel;
    QSpinBox *m_notationTimeSpinBox;
    QSpinBox *m_notationDurationSpinBox;
    QPushButton *m_notationTimeEditButton;
    QPushButton *m_notationDurationEditButton;
    QCheckBox *m_lockNotationValues;
#endif

};


}

#endif
