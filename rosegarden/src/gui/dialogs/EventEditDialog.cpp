/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
 
    This program is Copyright 2000-2007
        Guillaume Laurent   <glaurent@telegraph-road.org>,
        Chris Cannam        <cannam@all-day-breakfast.com>,
        Richard Bown        <richard.bown@ferventsoftware.com>
 
    The moral rights of Guillaume Laurent, Chris Cannam, and Richard
    Bown to claim authorship of this work have been asserted.
 
    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.
 
    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/


#include "EventEditDialog.h"

#include <klocale.h>
#include "misc/Strings.h"
#include "base/Event.h"
#include "base/MidiTypes.h"
#include "base/NotationTypes.h"
#include "base/PropertyName.h"
#include "base/RealTime.h"
#include "gui/editors/notation/NotePixmapFactory.h"
#include <kdialogbase.h>
#include <kmessagebox.h>
#include <qcheckbox.h>
#include <qfont.h>
#include <qgrid.h>
#include <qgroupbox.h>
#include <qhbox.h>
#include <qlabel.h>
#include <qlineedit.h>
#include <qobject.h>
#include <qobjectlist.h>
#include <qpushbutton.h>
#include <qscrollview.h>
#include <qsize.h>
#include <qspinbox.h>
#include <qstring.h>
#include <qtooltip.h>
#include <qvbox.h>
#include <qwidget.h>


namespace Rosegarden
{

EventEditDialog::EventEditDialog(QWidget *parent,
                                 const Event &event,
                                 bool editable) :
        KDialogBase(parent, 0, true, i18n(editable ? "Advanced Event Edit" : "Advanced Event Viewer"),
                    (editable ? (Ok | Cancel) : Ok)),
        m_durationDisplay(0),
        m_durationDisplayAux(0),
        m_persistentGrid(0),
        m_nonPersistentGrid(0),
        m_nonPersistentView(0),
        m_originalEvent(event),
        m_event(event),
        m_type(event.getType()),
        m_absoluteTime(event.getAbsoluteTime()),
        m_duration(event.getDuration()),
        m_subOrdering(event.getSubOrdering()),
        m_modified(false)
{
    QVBox *vbox = makeVBoxMainWidget();

    QGroupBox *intrinsicBox = new QGroupBox
                              (1, Horizontal, i18n("Intrinsics"), vbox);

    QGrid *intrinsicGrid = new QGrid(4, QGrid::Horizontal, intrinsicBox);

    new QLabel(i18n("Event type: "), intrinsicGrid);
    new QLabel("", intrinsicGrid);
    new QLabel("", intrinsicGrid);
    QLineEdit *lineEdit = new QLineEdit(intrinsicGrid);
    lineEdit->setText(strtoqstr(event.getType()));

    new QLabel(i18n("Absolute time: "), intrinsicGrid);
    new QLabel("", intrinsicGrid);
    new QLabel("", intrinsicGrid);
    QSpinBox *absoluteTime = new QSpinBox
                             (INT_MIN, INT_MAX, Note(Note::Shortest).getDuration(), intrinsicGrid);
    absoluteTime->setValue(event.getAbsoluteTime());
    QObject::connect(absoluteTime, SIGNAL(valueChanged(int)),
                     this, SLOT(slotAbsoluteTimeChanged(int)));
    slotAbsoluteTimeChanged(event.getAbsoluteTime());

    new QLabel(i18n("Duration: "), intrinsicGrid);
    m_durationDisplay = new QLabel("(note)", intrinsicGrid);
    m_durationDisplay->setMinimumWidth(20);
    m_durationDisplayAux = new QLabel("(note)", intrinsicGrid);
    m_durationDisplayAux->setMinimumWidth(20);

    QSpinBox *duration = new QSpinBox
                         (0, INT_MAX, Note(Note::Shortest).getDuration(), intrinsicGrid);
    duration->setValue(event.getDuration());
    QObject::connect(duration, SIGNAL(valueChanged(int)),
                     this, SLOT(slotDurationChanged(int)));
    slotDurationChanged(event.getDuration());

    new QLabel(i18n("Sub-ordering: "), intrinsicGrid);
    new QLabel("", intrinsicGrid);
    new QLabel("", intrinsicGrid);

    QSpinBox *subOrdering = new QSpinBox( -100, 100, 1, intrinsicGrid);
    subOrdering->setValue(event.getSubOrdering());
    QObject::connect(subOrdering, SIGNAL(valueChanged(int)),
                     this, SLOT(slotSubOrderingChanged(int)));
    slotSubOrderingChanged(event.getSubOrdering());

    QGroupBox *persistentBox = new QGroupBox
                               (1, Horizontal, i18n("Persistent properties"), vbox);
    m_persistentGrid = new QGrid(4, QGrid::Horizontal, persistentBox);

    QLabel *label = new QLabel(i18n("Name"), m_persistentGrid);
    QFont font(label->font());
    font.setItalic(true);
    label->setFont(font);

    label = new QLabel(i18n("Type"), m_persistentGrid);
    label->setFont(font);
    label = new QLabel(i18n("Value"), m_persistentGrid);
    label->setFont(font);
    label = new QLabel("", m_persistentGrid);
    label->setFont(font);

    Event::PropertyNames p = event.getPersistentPropertyNames();

    for (Event::PropertyNames::iterator i = p.begin();
            i != p.end(); ++i) {
        addPersistentProperty(*i);
    }

    p = event.getNonPersistentPropertyNames();

    if (p.begin() == p.end()) {
        m_nonPersistentView = 0;
        m_nonPersistentGrid = 0;
    } else {

        QGroupBox *nonPersistentBox = new QGroupBox
                                      (1, Horizontal, i18n("Non-persistent properties"), vbox);
        new QLabel(i18n("These are cached values, lost if the event is modified."),
                   nonPersistentBox);

        m_nonPersistentView = new QScrollView(nonPersistentBox);
        //m_nonPersistentView->setHScrollBarMode(QScrollView::AlwaysOff);
        m_nonPersistentView->setResizePolicy(QScrollView::AutoOneFit);

        m_nonPersistentGrid = new QGrid
                              (4, QGrid::Horizontal, m_nonPersistentView->viewport());
        m_nonPersistentView->addChild(m_nonPersistentGrid);

        m_nonPersistentGrid->setSpacing(4);
        m_nonPersistentGrid->setMargin(5);

        label = new QLabel(i18n("Name       "), m_nonPersistentGrid);
        label->setFont(font);
        label = new QLabel(i18n("Type       "), m_nonPersistentGrid);
        label->setFont(font);
        label = new QLabel(i18n("Value      "), m_nonPersistentGrid);
        label->setFont(font);
        label = new QLabel("", m_nonPersistentGrid);
        label->setFont(font);

        for (Event::PropertyNames::iterator i = p.begin();
                i != p.end(); ++i) {

            new QLabel(strtoqstr(*i), m_nonPersistentGrid, strtoqstr(*i));
            new QLabel(strtoqstr(event.getPropertyTypeAsString(*i)), m_nonPersistentGrid, strtoqstr(*i));
            new QLabel(strtoqstr(event.getAsString(*i)), m_nonPersistentGrid, strtoqstr(*i));
            QPushButton *button = new QPushButton("P", m_nonPersistentGrid, strtoqstr(*i));
            button->setFixedSize(QSize(24, 24));
            QToolTip::add
                (button, i18n("Make persistent"));
            QObject::connect(button, SIGNAL(clicked()),
                             this, SLOT(slotPropertyMadePersistent()));
        }
    }
}

void
EventEditDialog::addPersistentProperty(const PropertyName &name)
{
    QLabel *label = new QLabel(strtoqstr(name), m_persistentGrid, strtoqstr(name));
    label->show();
    label = new QLabel(strtoqstr(m_originalEvent.getPropertyTypeAsString(name)),
                       m_persistentGrid, strtoqstr(name));
    label->show();

    PropertyType type(m_originalEvent.getPropertyType(name));
    switch (type) {

    case Int: {
                int min = INT_MIN, max = INT_MAX;
                // DMM - constrain program changes to a useful range of values
                // Might other types have a similar need for such limits?
                if (m_originalEvent.isa(ProgramChange::EventType)) {
                min = 0;
                max = 127;
            }
        QSpinBox *spinBox = new QSpinBox
                            (min, max, 1, m_persistentGrid, strtoqstr(name));
        spinBox->setValue(m_originalEvent.get<Int>(name));
        QObject::connect(spinBox, SIGNAL(valueChanged(int)),
                         this, SLOT(slotIntPropertyChanged(int)));
        spinBox->show();
        break;
    }
case UInt: {
        int min = 0;
        int max = UINT_MAX;
        if (m_originalEvent.isa(ProgramChange::EventType)) {
                min = 0;
                max = 65535;
            }
            QSpinBox *spinBox = new QSpinBox
                                (min, max, 1, m_persistentGrid, strtoqstr(name));
            spinBox->setValue(m_originalEvent.get<UInt>(name));
            QObject::connect(spinBox, SIGNAL(valueChanged(int)),
                             this, SLOT(slotIntPropertyChanged(int)));
            spinBox->show();
            break;
        }
    case RealTimeT: {
            RealTime realTime = m_originalEvent.get<RealTimeT>(name);

            QHBox* hbox = new QHBox(m_persistentGrid);

            // seconds
            //
            QSpinBox *spinBox = new QSpinBox
                                (INT_MIN, INT_MAX, 1,
                                 hbox, strtoqstr(name) + "%sec");
            spinBox->setValue(realTime.sec);

            QObject::connect(spinBox, SIGNAL(valueChanged(int)),
                             this, SLOT(slotRealTimePropertyChanged(int)));

            // nseconds
            //
            spinBox = new QSpinBox
                      (INT_MIN, INT_MAX, 1,
                       hbox, strtoqstr(name) + "%nsec");
            spinBox->setValue(realTime.nsec);

            QObject::connect(spinBox, SIGNAL(valueChanged(int)),
                             this, SLOT(slotRealTimePropertyChanged(int)));
            spinBox->show();
            break;
        }

    case Bool: {
            QCheckBox *checkBox = new QCheckBox
                                  ("", m_persistentGrid, strtoqstr(name));
            checkBox->setChecked(m_originalEvent.get<Bool>(name));
            QObject::connect(checkBox, SIGNAL(activated()),
                             this, SLOT(slotBoolPropertyChanged()));
            checkBox->show();
            break;
        }

    case String: {
            QLineEdit *lineEdit = new QLineEdit
                                  (strtoqstr(m_originalEvent.get<String>(name)),
                                   m_persistentGrid,
                                   strtoqstr(name));
            QObject::connect(lineEdit, SIGNAL(textChanged(const QString &)),
                             this, SLOT(slotStringPropertyChanged(const QString &)));
            lineEdit->show();
            break;
        }
    }

    QPushButton *button = new QPushButton("X", m_persistentGrid,
                                          strtoqstr(name));
    button->setFixedSize(QSize(24, 24));
    QToolTip::add
        (button, i18n("Delete this property"));
    QObject::connect(button, SIGNAL(clicked()),
                     this, SLOT(slotPropertyDeleted()));
    button->show();
}

Event
EventEditDialog::getEvent() const
{
    return Event(m_event, m_absoluteTime, m_duration, m_subOrdering);
}

void
EventEditDialog::slotEventTypeChanged(const QString &type)
{
    std::string t(qstrtostr(type));
    if (t != m_type) {
        m_modified = true;
        m_type = t;
    }
}

void
EventEditDialog::slotAbsoluteTimeChanged(int value)
{
    if (value == m_absoluteTime)
        return ;
    m_modified = true;
    m_absoluteTime = value;
}

void
EventEditDialog::slotDurationChanged(int value)
{
    timeT error = 0;
    m_durationDisplay->setPixmap
    (NotePixmapFactory::toQPixmap(m_notePixmapFactory.makeNoteMenuPixmap(timeT(value), error)));

    if (error >= value / 2) {
        m_durationDisplayAux->setText("++ ");
    } else if (error > 0) {
        m_durationDisplayAux->setText("+ ");
    } else if (error < 0) {
        m_durationDisplayAux->setText("- ");
    } else {
        m_durationDisplayAux->setText(" ");
    }

    if (timeT(value) == m_duration)
        return ;

    m_modified = true;
    m_duration = value;
}

void
EventEditDialog::slotSubOrderingChanged(int value)
{
    if (value == m_subOrdering)
        return ;
    m_modified = true;
    m_subOrdering = value;
}

void
EventEditDialog::slotIntPropertyChanged(int value)
{
    const QObject *s = sender();
    const QSpinBox *spinBox = dynamic_cast<const QSpinBox *>(s);
    if (!spinBox)
        return ;

    m_modified = true;
    QString propertyName = spinBox->name();
    m_event.set<Int>(qstrtostr(propertyName), value);
}

void
EventEditDialog::slotRealTimePropertyChanged(int value)
{
    const QObject *s = sender();
    const QSpinBox *spinBox = dynamic_cast<const QSpinBox *>(s);
    if (!spinBox)
        return ;

    m_modified = true;
    QString propertyFullName = spinBox->name();

    QString propertyName = propertyFullName.section('%', 0, 0),
                           nsecOrSec = propertyFullName.section('%', 1, 1);

    RealTime realTime = m_event.get<RealTimeT>(qstrtostr(propertyName));

    if (nsecOrSec == "sec")
        realTime.sec = value;
    else
        realTime.nsec = value;

    m_event.set<Int>(qstrtostr(propertyName), value);
}

void
EventEditDialog::slotBoolPropertyChanged()
{
    const QObject *s = sender();
    const QCheckBox *checkBox = dynamic_cast<const QCheckBox *>(s);
    if (!checkBox)
        return ;

    m_modified = true;
    QString propertyName = checkBox->name();
    bool checked = checkBox->isChecked();

    m_event.set<Bool>(qstrtostr(propertyName), checked);
}

void
EventEditDialog::slotStringPropertyChanged(const QString &value)
{
    const QObject *s = sender();
    const QLineEdit *lineEdit = dynamic_cast<const QLineEdit *>(s);
    if (!lineEdit)
        return ;

    m_modified = true;
    QString propertyName = lineEdit->name();
    m_event.set<String>(qstrtostr(propertyName), qstrtostr(value));
}

void
EventEditDialog::slotPropertyDeleted()
{
    const QObject *s = sender();
    const QPushButton *pushButton = dynamic_cast<const QPushButton *>(s);
    if (!pushButton)
        return ;

    QString propertyName = pushButton->name();

    if (KMessageBox::warningContinueCancel
            (this,
             i18n("Are you sure you want to delete the \"%1\" property?\n\n"
                  "Removing necessary properties may cause unexpected behavior.").
             arg(propertyName),
             i18n("Edit Event"),
             i18n("&Delete")) != KMessageBox::Continue)
        return ;

    m_modified = true;
    QObjectList *list = m_persistentGrid->queryList(0, propertyName, false);
    QObjectListIt i(*list);
    QObject *obj;
    while ((obj = i.current()) != 0) {
        ++i;
        delete obj;
    }
    delete list;

    m_event.unset(qstrtostr(propertyName));
}

void
EventEditDialog::slotPropertyMadePersistent()
{
    const QObject *s = sender();
    const QPushButton *pushButton = dynamic_cast<const QPushButton *>(s);
    if (!pushButton)
        return ;

    QString propertyName = pushButton->name();

    if (KMessageBox::warningContinueCancel
            (this,
             i18n("Are you sure you want to make the \"%1\" property persistent?\n\n"
                  "This could cause problems if it overrides a different "
                  "computed value later on.").
             arg(propertyName),
             i18n("Edit Event"),
             i18n("Make &Persistent")) != KMessageBox::Continue)
        return ;

    QObjectList *list = m_nonPersistentGrid->queryList(0, propertyName, false);
    QObjectListIt i(*list);
    QObject *obj;
    while ((obj = i.current()) != 0) {
        ++i;
        delete obj;
    }
    delete list;

    m_modified = true;
    addPersistentProperty(qstrtostr(propertyName));

    PropertyType type =
        m_originalEvent.getPropertyType(qstrtostr(propertyName));

    switch (type) {

    case Int:
        m_event.set<Int>
        (qstrtostr(propertyName),
         m_originalEvent.get<Int>
         (qstrtostr(propertyName)));
        break;

    case UInt:
        m_event.set<UInt>
        (qstrtostr(propertyName),
         m_originalEvent.get<UInt>
         (qstrtostr(propertyName)));
        break;

    case RealTimeT:
        m_event.set<RealTimeT>
        (qstrtostr(propertyName),
         m_originalEvent.get<RealTimeT>
         (qstrtostr(propertyName)));
        break;

    case Bool:
        m_event.set<Bool>
        (qstrtostr(propertyName),
         m_originalEvent.get<Bool>
         (qstrtostr(propertyName)));
        break;

    case String:
        m_event.set<String>
        (qstrtostr(propertyName),
         m_originalEvent.get<String>
         (qstrtostr(propertyName)));
        break;
    }
}

}
#include "EventEditDialog.moc"
