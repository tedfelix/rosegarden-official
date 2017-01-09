
/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2017 the Rosegarden development team.

    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#ifndef RG_EVENTEDITDIALOG_H
#define RG_EVENTEDITDIALOG_H

#include "base/Event.h"
#include "gui/editors/notation/NotePixmapFactory.h"
#include <string>
#include <QDialog>


class QWidget;
class QString;
class QScrollArea;
class QLabel;
class QGridLayout;
class QFrame;
class QGroupBox;

namespace Rosegarden
{

class PropertyName;


class EventEditDialog : public QDialog
{
    Q_OBJECT

public:
    /**
     * Construct an event-edit dialog showing the properties of the
     * given event.  If editable is false, the user will not be allowed
     * to modify the event; otherwise the event will be editable and
     * the resulting edited version can subsequently be queried
     * through getEvent().
     */
    EventEditDialog(QWidget *parent,
                    const Event &event,
                    bool editable = true);

    bool isModified() const { return m_modified; }
    Event getEvent() const;

public slots:
    void slotEventTypeChanged(const QString &);
    void slotAbsoluteTimeChanged(int value);
    void slotDurationChanged(int value);
    void slotSubOrderingChanged(int value);

    void slotIntPropertyChanged(int);
    void slotRealTimePropertyChanged(int);
    void slotBoolPropertyChanged(bool);
    void slotStringPropertyChanged(const QString &);

    void slotPropertyDeleted();
    void slotPropertyMadePersistent();

protected:
    void addPersistentProperty(const PropertyName &);

    //--------------- Data members ---------------------------------
    NotePixmapFactory m_notePixmapFactory;

    QLabel *m_durationDisplay;
    QLabel *m_durationDisplayAux;

    QGroupBox	*m_persistentGrid;
    QGridLayout *m_persistentGridLay;
    int          m_persistentGridRow;

    QFrame      *m_nonPersistentGrid;

    QScrollArea *m_nonPersistentView;

    const Event &m_originalEvent;
    Event m_event;

    std::string m_type;
    timeT m_absoluteTime;
    timeT m_duration;
    int m_subOrdering;

    bool m_modified;
};

/*
 * A simpler event editor for use by the EventView and MatrixView
 * and people who want to remain sane.
 */

}

#endif
