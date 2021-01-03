/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2021 the Rosegarden development team.
 
    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.
 
    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#define RG_MODULE_STRING "[EventEditDialog]"

#include "EventEditDialog.h"

#include "misc/Strings.h"
#include "base/Event.h"
#include "base/MidiTypes.h"
#include "base/NotationTypes.h"
#include "base/PropertyName.h"
#include "base/RealTime.h"
#include "gui/editors/notation/NotePixmapFactory.h"
#include "gui/widgets/LineEdit.h"

#include <QDialog>
#include <QDialogButtonBox>
#include <QMessageBox>
#include <QCheckBox>
#include <QFont>
#include <QGroupBox>
#include <QLabel>
#include <QObject>
#include <QObjectList>
#include <QPushButton>
#include <QScrollArea>
#include <QSize>
#include <QSpinBox>
#include <QString>
#include <QToolTip>
#include <QWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>



namespace Rosegarden
{

EventEditDialog::EventEditDialog(QWidget *parent,
                                 const Event &event,
                                 bool editable) :
        QDialog(parent),
        m_durationDisplay(nullptr),
        m_durationDisplayAux(nullptr),
        m_persistentGrid(nullptr),
        m_persistentGridLay(nullptr),
        m_persistentGridRow(0),
        m_nonPersistentGrid(nullptr),
        m_nonPersistentView(nullptr),
        m_originalEvent(event),
        m_event(event),
        m_type(event.getType()),
        m_absoluteTime(event.getAbsoluteTime()),
        m_duration(event.getDuration()),
        m_subOrdering(event.getSubOrdering()),
        m_modified(false)
{
    setModal(true);
    setWindowTitle(tr(editable ?
                          tr("Advanced Event Edit").toStdString().c_str() :
                          tr("Advanced Event Viewer").toStdString().c_str()));

    QGridLayout *metagrid = new QGridLayout;
    setLayout(metagrid);
    QWidget *vbox = new QWidget(this);
    QVBoxLayout *vboxLayout = new QVBoxLayout;
    metagrid->addWidget(vbox, 0, 0);


    QGroupBox *intrinsicGrid = new QGroupBox(tr("Intrinsics"), vbox);
    QGridLayout *intrinsicGridLay = new QGridLayout(intrinsicGrid);
    vboxLayout->addWidget(intrinsicGrid);

    int row = 0;

    intrinsicGridLay->addWidget(new QLabel(tr("Event type: ")), row, 0);
    intrinsicGridLay->addWidget(new QLabel("", intrinsicGrid), row, 1);
    intrinsicGridLay->addWidget(new QLabel("", intrinsicGrid), row, 2);
    LineEdit *lineEdit = new LineEdit(intrinsicGrid);
    lineEdit->setText(strtoqstr(event.getType()));
    intrinsicGridLay->addWidget(lineEdit, row, 3);
    row++;

    intrinsicGridLay->addWidget(new QLabel(tr("Absolute time: ")), row, 0);
    intrinsicGridLay->addWidget(new QLabel(""), row, 1);
    intrinsicGridLay->addWidget(new QLabel(""), row, 2);
    QSpinBox *absoluteTime = new QSpinBox;
    absoluteTime->setRange(INT_MIN, INT_MAX);
    absoluteTime->setSingleStep(Note(Note::Shortest).getDuration());
    absoluteTime->setValue(event.getAbsoluteTime());
    intrinsicGridLay->addWidget(absoluteTime, row, 3);
    row++;

    QObject::connect(absoluteTime, SIGNAL(valueChanged(int)),
                     this, SLOT(slotAbsoluteTimeChanged(int)));
    slotAbsoluteTimeChanged(event.getAbsoluteTime());

    intrinsicGridLay->addWidget(new QLabel(tr("Duration: ")), row, 0);
    m_durationDisplay = new QLabel("(note)");
    m_durationDisplay->setMinimumWidth(20);
    intrinsicGridLay->addWidget(m_durationDisplay, row, 1);
    m_durationDisplayAux = new QLabel("(note)");
    m_durationDisplayAux->setMinimumWidth(20);
    intrinsicGridLay->addWidget(m_durationDisplayAux, row, 2);
    QSpinBox *duration = new QSpinBox;
    duration->setRange(0, INT_MAX);
    duration->setSingleStep(Note(Note::Shortest).getDuration());
    duration->setValue(event.getDuration());
    intrinsicGridLay->addWidget(duration, row, 3);
    row++;

    QObject::connect(duration, SIGNAL(valueChanged(int)),
                     this, SLOT(slotDurationChanged(int)));
    slotDurationChanged(event.getDuration());

    intrinsicGridLay->addWidget(new QLabel(tr("Sub-ordering: ")), row, 0);
    intrinsicGridLay->addWidget(new QLabel(""), row, 1);
    intrinsicGridLay->addWidget(new QLabel(""), row, 2);
    QSpinBox *subOrdering = new QSpinBox;
    subOrdering->setRange(-100, 100);
    subOrdering->setSingleStep(1);
    subOrdering->setValue(event.getSubOrdering());
    intrinsicGridLay->addWidget(subOrdering, row, 3);

    QObject::connect(subOrdering, SIGNAL(valueChanged(int)),
                     this, SLOT(slotSubOrderingChanged(int)));
    slotSubOrderingChanged(event.getSubOrdering());

    intrinsicGrid->setLayout(intrinsicGridLay);

    m_persistentGrid = new QGroupBox( tr("Persistent properties"), vbox );
    m_persistentGridLay = new QGridLayout(m_persistentGrid);
    vboxLayout->addWidget(m_persistentGrid);

    m_persistentGridRow = 0;

    QLabel *label = new QLabel(tr("Name"), m_persistentGrid);
    QFont font(label->font());
    font.setItalic(true);
    label->setFont(font);
    m_persistentGridLay->addWidget(label, m_persistentGridRow, 0);

    label = new QLabel(tr("Type"), m_persistentGrid);
    label->setFont(font);
    m_persistentGridLay->addWidget(label, m_persistentGridRow, 1);

    label = new QLabel(tr("Value"), m_persistentGrid);
    label->setFont(font);
    m_persistentGridLay->addWidget(label, m_persistentGridRow, 2);

    label = new QLabel("", m_persistentGrid);
    label->setFont(font);
    m_persistentGridLay->addWidget(label, m_persistentGridRow, 3);

    m_persistentGridRow++;

    Event::PropertyNames p = event.getPersistentPropertyNames();

    for (Event::PropertyNames::iterator i = p.begin();
            i != p.end(); ++i) {
        addPersistentProperty(*i);
    }

    m_persistentGrid->setLayout(m_persistentGridLay);

    p = event.getNonPersistentPropertyNames();

    if (p.begin() == p.end()) {
        m_nonPersistentView = nullptr;
        m_nonPersistentGrid = nullptr;
    } else {

        // create bottom area container (group-box) for non-persistent-events view
        QGroupBox *nonPersistentBox = 
            new QGroupBox( tr("Non-persistent properties"));
        QVBoxLayout *nonPersistentBoxLayout = new QVBoxLayout;
        vboxLayout->addWidget(nonPersistentBox);

        // insert label
        nonPersistentBoxLayout->addWidget(new QLabel(tr(
                "These are cached values, lost if the event is modified.")));


        m_nonPersistentGrid = new QFrame;
        m_nonPersistentGrid->setContentsMargins(5, 5, 5, 5);
        QGridLayout *nonPersistentGridLay = new QGridLayout;
        nonPersistentGridLay->setSpacing(4);

        int nonPersistentGridRow = 0;

        label = new QLabel(tr("Name       "));
        label->setFont(font);
        nonPersistentGridLay->addWidget(label, nonPersistentGridRow, 0);

        label = new QLabel(tr("Type       "));
        label->setFont(font);
        nonPersistentGridLay->addWidget(label, nonPersistentGridRow, 1);

        label = new QLabel(tr("Value      "));
        label->setFont(font);
        nonPersistentGridLay->addWidget(label, nonPersistentGridRow, 2);

        label = new QLabel("", m_nonPersistentGrid);
        label->setFont(font);
        nonPersistentGridLay->addWidget(label, nonPersistentGridRow, 3);

        nonPersistentGridRow++;

        for (Event::PropertyNames::iterator i = p.begin();
                i != p.end(); ++i) {

            label = new QLabel(strtoqstr(*i), m_nonPersistentGrid);
            label->setObjectName(strtoqstr(*i));
            nonPersistentGridLay->addWidget(label, nonPersistentGridRow, 0);

            label = new QLabel(strtoqstr(event.getPropertyTypeAsString(*i)));
            label->setObjectName(strtoqstr(*i));
            nonPersistentGridLay->addWidget(label, nonPersistentGridRow, 1);

            label = new QLabel(strtoqstr(event.getAsString(*i)));
            label->setObjectName(strtoqstr(*i));
            nonPersistentGridLay->addWidget(label, nonPersistentGridRow, 2);

            QPushButton *button = new QPushButton("P");
            button->setObjectName(strtoqstr(*i));
            button->setFixedSize(QSize(24, 24));
            nonPersistentGridLay->addWidget(label, nonPersistentGridRow, 3);
            button->setToolTip(tr("Make persistent"));
            QObject::connect(button, &QAbstractButton::clicked,
                                this, &EventEditDialog::slotPropertyMadePersistent);

            nonPersistentGridRow++;
        }

        m_nonPersistentGrid->setLayout(nonPersistentGridLay);


        // insert grid into scroll area and scroll area into group-box
        m_nonPersistentView = new QScrollArea;
        m_nonPersistentView->setWidget(m_nonPersistentGrid);
        m_nonPersistentView->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
//        m_nonPersistentView->setResizePolicy( QScrollView::AutoOneFit );
        nonPersistentBoxLayout->addWidget(m_nonPersistentView);

        nonPersistentBox->setLayout(nonPersistentBoxLayout);
    }

    vbox->setLayout(vboxLayout);

    QDialogButtonBox *buttonBox = new QDialogButtonBox(
                                        editable ? (QDialogButtonBox::Ok |
                                                    QDialogButtonBox::Cancel)
                                                  : QDialogButtonBox::Ok);
    metagrid->addWidget(buttonBox, 1, 0);
    metagrid->setRowStretch(0, 10);
    connect(buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
}

void
EventEditDialog::addPersistentProperty(const PropertyName &name)
{
    QLabel *label = new QLabel(strtoqstr(name), m_persistentGrid);
    label->setObjectName(strtoqstr(name));
    m_persistentGridLay->addWidget(label, m_persistentGridRow, 0);
    label->show();

    label = new QLabel(strtoqstr(m_originalEvent.getPropertyTypeAsString(name)));
    label->setObjectName(strtoqstr(name));
    m_persistentGridLay->addWidget(label, m_persistentGridRow, 1);
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
            QSpinBox *spinBox = new QSpinBox;
            spinBox->setRange(min, max);
            spinBox->setSingleStep(1);
            spinBox->setObjectName(strtoqstr(name));
            spinBox->setValue(m_originalEvent.get<Int>(name));
            m_persistentGridLay->addWidget(spinBox, m_persistentGridRow, 2);
            QObject::connect(spinBox, SIGNAL(valueChanged(int)),
                              this, SLOT(slotIntPropertyChanged(int)));
            spinBox->show();
            break;
        }
    case RealTimeT: {
            RealTime realTime = m_originalEvent.get<RealTimeT>(name);

            QWidget *hbox = new QWidget(m_persistentGrid);
            QHBoxLayout *hboxLayout = new QHBoxLayout;

            // seconds
            //
            QSpinBox *spinBox = new QSpinBox;
            spinBox->setRange(INT_MIN, INT_MAX);
            spinBox->setSingleStep(1);
            spinBox->setObjectName(strtoqstr(name));
            spinBox->setSuffix(tr("sec"));
            spinBox->setValue(realTime.sec);
            hboxLayout->addWidget(spinBox);

            QObject::connect(spinBox, SIGNAL(valueChanged(int)),
                              this, SLOT(slotRealTimePropertyChanged(int)));
            spinBox->show();

            // nseconds
            //
            spinBox = new QSpinBox;
            spinBox->setRange(INT_MIN, INT_MAX);
            spinBox->setSingleStep( 1 );
            spinBox->setObjectName(strtoqstr(name));
            spinBox->setSuffix(tr("nsec"));
            spinBox->setValue(realTime.nsec);
            hboxLayout->addWidget(spinBox);

            QObject::connect(spinBox, SIGNAL(valueChanged(int)),
                              this, SLOT(slotRealTimePropertyChanged(int)));
            spinBox->show();

            hbox->setObjectName(strtoqstr(name));
            m_persistentGridLay->addWidget(hbox, m_persistentGridRow, 2);
            hbox->setLayout(hboxLayout);
            break;
        }

    case Bool: {
            QCheckBox *checkBox = new QCheckBox("");
            checkBox->setObjectName(strtoqstr(name));
            checkBox->setChecked(m_originalEvent.get<Bool>(name));
            m_persistentGridLay->addWidget(checkBox, m_persistentGridRow, 2);
            QObject::connect(checkBox, &QAbstractButton::toggled,
                              this, &EventEditDialog::slotBoolPropertyChanged);
            checkBox->show();
            break;
        }

    case String: {
            LineEdit *lineEdit = new LineEdit
                                  (strtoqstr(m_originalEvent.get<String>(name)));
            lineEdit->setObjectName(strtoqstr(name));
            m_persistentGridLay->addWidget(lineEdit, m_persistentGridRow, 2);
            QObject::connect(lineEdit, &QLineEdit::textChanged,
                             this, &EventEditDialog::slotStringPropertyChanged);
            lineEdit->show();
            break;
        }
    }

    QPushButton *button = new QPushButton("X");
    button->setObjectName(strtoqstr(name));
    button->setFixedSize(QSize(24, 24));
    button->setToolTip(tr("Delete this property"));
    m_persistentGridLay->addWidget(button, m_persistentGridRow, 3);
    QObject::connect(button, &QAbstractButton::clicked,
                     this, &EventEditDialog::slotPropertyDeleted);
    button->show();

    m_persistentGridRow++;
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
        (m_notePixmapFactory.makeNoteMenuPixmap(timeT(value), error));

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
    QString propertyName = spinBox->objectName();
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
    QString propertyFullName = spinBox->objectName();

    QString propertyName = propertyFullName.section('%', 0, 0),
                           nsecOrSec = propertyFullName.section('%', 1, 1);

    // RealTime realTime = m_event.get<RealTimeT>(qstrtostr(propertyName));

    // if (nsecOrSec == "sec")
    //     realTime.sec = value;
    // else
    //     realTime.nsec = value;

    m_event.set<Int>(qstrtostr(propertyName), value);
}

void
EventEditDialog::slotBoolPropertyChanged(bool)
{
    const QObject *s = sender();
    const QCheckBox *checkBox = dynamic_cast<const QCheckBox *>(s);
    if (!checkBox)
        return ;

    m_modified = true;
    QString propertyName = checkBox->objectName();
    bool checked = checkBox->isChecked();

    m_event.set<Bool>(qstrtostr(propertyName), checked);
}

void
EventEditDialog::slotStringPropertyChanged(const QString &value)
{
    const QObject *s = sender();
    const LineEdit *lineEdit = dynamic_cast<const LineEdit *>(s);
    if (!lineEdit)
        return ;

    m_modified = true;
    QString propertyName = lineEdit->objectName();
    m_event.set<String>(qstrtostr(propertyName), qstrtostr(value));
}

void
EventEditDialog::slotPropertyDeleted()
{
    const QObject *s = sender();
    const QPushButton *pushButton = dynamic_cast<const QPushButton *>(s);
    if (!pushButton)
        return ;

    QString propertyName = pushButton->objectName();


    QMessageBox msgBox(QMessageBox::Warning, tr("Edit Event"),
             tr("Are you sure you want to delete the \"%1\" property?\n\n"
                  "Removing necessary properties may cause unexpected behavior.")
                  .arg(propertyName),
             QMessageBox::Cancel, this);
    QPushButton *ok = msgBox.addButton(tr("&Delete"),
                                       QMessageBox::AcceptRole);
    msgBox.exec();
    if (msgBox.clickedButton() != ok)
        return;

    m_modified = true;

    QList<QWidget *> list =
        m_persistentGrid->findChildren<QWidget *>(propertyName);

    QList<QWidget *>::iterator i;
    for (i=list.begin(); i!=list.end(); ++i)
        delete *i;

    m_event.unset(qstrtostr(propertyName));
}

void
EventEditDialog::slotPropertyMadePersistent()
{
    const QObject *s = sender();
    const QPushButton *pushButton = dynamic_cast<const QPushButton *>(s);
    if (!pushButton)
        return ;

    QString propertyName = pushButton->objectName();

    QMessageBox msgBox(QMessageBox::Warning, tr("Edit Event"),
             tr("Are you sure you want to make the \"%1\" property persistent?"
                  "\n\nThis could cause problems if it overrides a different "
                  "computed value later on.").arg(propertyName),
             QMessageBox::Cancel, this);
    QPushButton *ok = msgBox.addButton(tr("Make &Persistent"),
                                       QMessageBox::AcceptRole);
    msgBox.exec();
    if (msgBox.clickedButton() != ok)
        return;

    QList<QWidget *> list =
        m_nonPersistentGrid->findChildren<QWidget *>(propertyName);

    QList<QWidget *>::iterator i;
    for (i=list.begin(); i!=list.end(); ++i)
        delete *i;

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
