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

#ifndef RG_EDITEVENT_H
#define RG_EDITEVENT_H

#include "base/Event.h"

#include <QDialog>

#include <string>


class QWidget;
class QString;
class QSpinBox;
class QPushButton;
class QLabel;
class QGroupBox;
class QCheckBox;
class QComboBox;


namespace Rosegarden
{


class RosegardenDocument;
class LineEdit;
class EventWidgetStack;
class EventWidget;


/// "Edit Event" and "Insert Event" dialogs.
/**
 * AKA the "Event Editor".
 *
 * This replaces SimpleEventEditDialog and EventEditDialog.
 */
class EditEvent : public QDialog
{
    Q_OBJECT

public:

    EditEvent(QWidget *parent,
              RosegardenDocument *doc,
              const Event &event,
              bool inserting = false);  // Inserting or editing.

    /// Used by NoteWidget for TimeDialog.
    timeT getAbsoluteTime() const;

    /// Get the edited or new (for insertion) Event.
    Event getEvent();

public slots:

    void slotEventTypeChanged(int value);
    void slotAbsoluteTimeChanged(int value);
    void slotDurationChanged(int value);
    void slotNotationAbsoluteTimeChanged(int value);
    void slotNotationDurationChanged(int value);
    //void slotPitchChanged(int value);
    //void slotVelocityChanged(int value);
    void slotMetaChanged(const QString &);
    void slotEditAbsoluteTime();
    //void slotLockNotationChanged();
    //void slotEditPitch();
    void slotSysexLoad();
    void slotSysexSave();

private:

    RosegardenDocument *m_doc;

    Event m_event;

    std::string m_type;
    timeT m_absoluteTime;
    timeT m_duration;

    timeT m_notationAbsoluteTime;
    timeT m_notationDuration;


    // Widgets

    // Event type
    QComboBox *m_typeCombo{nullptr};
    QLabel *m_typeLabel{nullptr};

    // Absolute time
    QLabel *m_timeLabel;
    QSpinBox *m_timeSpinBox;
    QPushButton *m_timeEditButton;

    // Duration
    //QLabel *m_durationLabel;
    //QSpinBox *m_durationSpinBox;
    //QPushButton *m_durationEditButton;

    // Event
    EventWidgetStack *m_eventWidgetStack{nullptr};  // For insert.
    EventWidget *m_eventWidget{nullptr};  // For edit.

    // Setup the dialog for a new event type
    void updateWidgets();

#if 0
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
