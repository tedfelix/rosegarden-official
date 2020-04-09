
/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2018 the Rosegarden development team.

    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#ifndef RG_SIMPLEEVENTEDITDIALOG_H
#define RG_SIMPLEEVENTEDITDIALOG_H

#include "base/Event.h"
#include "gui/widgets/LineEdit.h"

#include <QDialog>

#include <string>


class QWidget;
class QString;
class QSpinBox;
class QPushButton;
class LineEdit;
class QLabel;
class QGroupBox;
class QCheckBox;
class QComboBox;


namespace Rosegarden
{

class RosegardenDocument;

/// "Edit Event" dialog
/**
 * AKA the "Event Editor".
 *
 * See also EventEditDialog.
 */
class SimpleEventEditDialog : public QDialog
{
    Q_OBJECT
public:
    SimpleEventEditDialog(QWidget *parent,
                          RosegardenDocument *doc,
                          const Event &event,
                          bool inserting = false); // inserting or editing

    bool isModified() const { return m_modified; }
    Event getEvent();

    // Setup the dialog for a new event type
    void setupForEvent();

public slots:
    void slotEventTypeChanged(int value);
    void slotAbsoluteTimeChanged(int value);
    void slotDurationChanged(int value);
    void slotNotationAbsoluteTimeChanged(int value);
    void slotNotationDurationChanged(int value);
    void slotPitchChanged(int value);
    void slotVelocityChanged(int value);
    void slotMetaChanged(const QString &);
    void slotEditAbsoluteTime();
    void slotEditNotationAbsoluteTime();
    void slotEditDuration();
    void slotEditNotationDuration();
    void slotLockNotationChanged();
    void slotEditPitch();
    void slotSysexLoad();
    void slotSysexSave();

protected:
    Event        m_event;
    RosegardenDocument        *m_doc;

    std::string              m_type;
    timeT        m_absoluteTime;
    timeT        m_notationAbsoluteTime;
    timeT        m_duration;
    timeT        m_notationDuration;

    QComboBox               *m_typeCombo;
    QLabel                  *m_typeLabel;

    QLabel                  *m_timeLabel;
    QLabel                  *m_durationLabel;
    QLabel                  *m_pitchLabel;
    QLabel                  *m_velocityLabel;
    QLabel                  *m_metaLabel;
    QLabel                  *m_controllerLabel;
    QLabel                  *m_controllerLabelValue;

    QSpinBox                *m_timeSpinBox;
    QSpinBox                *m_durationSpinBox;
    QSpinBox                *m_pitchSpinBox;
    QSpinBox                *m_velocitySpinBox;

    QPushButton             *m_timeEditButton;
    QPushButton             *m_durationEditButton;
    QPushButton             *m_pitchEditButton;
    QPushButton             *m_sysexLoadButton;
    QPushButton             *m_sysexSaveButton;

    QGroupBox               *m_notationGroupBox;
    QLabel                  *m_notationTimeLabel;
    QLabel                  *m_notationDurationLabel;
    QSpinBox                *m_notationTimeSpinBox;
    QSpinBox                *m_notationDurationSpinBox;
    QPushButton             *m_notationTimeEditButton;
    QPushButton             *m_notationDurationEditButton;
    QCheckBox               *m_lockNotationValues;

    LineEdit                *m_metaEdit;

    bool                     m_modified;
};



}

#endif
