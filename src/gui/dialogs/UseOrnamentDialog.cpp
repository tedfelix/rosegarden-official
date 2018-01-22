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


#include "UseOrnamentDialog.h"

#include "base/BaseProperties.h"
#include "misc/Strings.h"
#include "misc/ConfigGroups.h"
#include "base/Composition.h"
#include "base/NotationTypes.h"
#include "base/TriggerSegment.h"
#include "gui/editors/notation/NotePixmapFactory.h"
#include "gui/widgets/LineEdit.h"

#include <QComboBox>
#include <QSettings>
#include <QDialog>
#include <QDialogButtonBox>
#include <QCheckBox>
#include <QFrame>
#include <QGroupBox>
#include <QLabel>
#include <QString>
#include <QWidget>
#include <QVBoxLayout>
#include <QLayout>
#include <QApplication>


namespace Rosegarden
{

UseOrnamentDialog::UseOrnamentDialog(QWidget *parent,
                                     Composition *composition) :
        QDialog(parent),
        m_composition(composition)
{
    setModal(true);
    setWindowTitle(tr("Use Ornament"));
    QGridLayout *metagrid = new QGridLayout;
    setLayout(metagrid);
    QWidget *vbox = new QWidget(this);
    QVBoxLayout *vboxLayout = new QVBoxLayout;
    metagrid->addWidget(vbox, 0, 0);


    QLabel *label;

    QGroupBox *notationBox = new QGroupBox(tr("Notation"));
    vboxLayout->addWidget(notationBox);

    notationBox->setContentsMargins(5, 5, 5, 5);
    QGridLayout *layout = new QGridLayout;
    layout->setSpacing(5);

    label = new QLabel(tr("Display as:  "));
    layout->addWidget(label, 0, 0);

    m_mark = new QComboBox;
    layout->addWidget(m_mark, 0, 1);

    m_marks.push_back(Marks::Trill);
    m_marks.push_back(Marks::LongTrill);
    m_marks.push_back(Marks::TrillLine);
    m_marks.push_back(Marks::Turn);
    m_marks.push_back(Marks::Mordent);
    m_marks.push_back(Marks::MordentInverted);
    m_marks.push_back(Marks::MordentLong);
    m_marks.push_back(Marks::MordentLongInverted);

    const QString markLabels[] = {
                                     tr("Trill"), tr("Trill with line"), tr("Trill line only"),
                                     tr("Turn"), tr("Mordent"), tr("Inverted mordent"),
                                     tr("Long mordent"), tr("Long inverted mordent"),
                                 };

    for (size_t i = 0; i < m_marks.size(); ++i) {
        m_mark->addItem(NotePixmapFactory::makeMarkMenuPixmap(m_marks[i]),
                        markLabels[i]);
    }
    m_mark->addItem(tr("Text mark"));

    connect(m_mark, SIGNAL(activated(int)), this, SLOT(slotMarkChanged(int)));

    m_textLabel = new QLabel(tr("   Text:  "));
    layout->addWidget(m_textLabel, 0, 2);

    m_text = new LineEdit;
    layout->addWidget(m_text, 0, 3);
    notationBox->setLayout(layout);

    QGroupBox *performBox = new QGroupBox(tr("Performance"));
    vboxLayout->addWidget(performBox);
    vbox->setLayout(vboxLayout);

    performBox->setContentsMargins(5, 5, 5, 5);
    layout = new QGridLayout;
    layout->setSpacing(5);

    label = new QLabel(tr("Perform using triggered segment: "));
    layout->addWidget(label, 0, 0);

    m_ornament = new QComboBox;
    layout->addWidget(m_ornament, 0, 1);

    int n = 1;
    for (Composition::triggersegmentcontaineriterator i =
                m_composition->getTriggerSegments().begin();
            i != m_composition->getTriggerSegments().end(); ++i) {
        m_ornament->addItem
        (QString("%1. %2").arg(n++).arg(strtoqstr((*i)->getSegment()->getLabel())));
    }

    label = new QLabel(tr("Perform with timing: "));
    layout->addWidget(label, 1, 0);

    m_adjustTime = new QComboBox;
    layout->addWidget(m_adjustTime, 1, 1);

    m_adjustTime->addItem(tr("As stored"));
    m_adjustTime->addItem(tr("Truncate if longer than note"));
    m_adjustTime->addItem(tr("End at same time as note"));
    m_adjustTime->addItem(tr("Stretch or squash segment to note duration"));

    m_retune = new QCheckBox(tr("Adjust pitch to note"));
    m_retune->setChecked(true);

    layout->addWidget(m_retune, 2, 1);
    performBox->setLayout(layout);

    setupFromConfig();

    QDialogButtonBox *buttonBox
        = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    metagrid->addWidget(buttonBox, 1, 0);
    metagrid->setRowStretch(0, 10);
    connect(buttonBox, SIGNAL(accepted()), this, SLOT(accept()));
    connect(buttonBox, SIGNAL(rejected()), this, SLOT(reject()));
}

void
UseOrnamentDialog::setupFromConfig()
{
    QSettings settings;
    settings.beginGroup( NotationViewConfigGroup );

    Mark mark = qstrtostr(settings.value("useornamentmark", "trill").toString());
    int seg = settings.value("useornamentlastornament", 0).toInt() ;
    std::string timing = qstrtostr(settings.value("useornamenttiming",
            strtoqstr(BaseProperties::TRIGGER_SEGMENT_ADJUST_SQUISH)).toString());
    bool retune = qStrToBool( settings.value("useornamentretune", "true" ) ) ;

    size_t i = 0;
    for (i = 0; i < m_marks.size(); ++i) {
        if (mark == m_marks[i]) {
            m_mark->setCurrentIndex(i);
            m_text->setEnabled(false);
            break;
        }
    }
    if (i >= m_marks.size()) {
        m_mark->setCurrentIndex(m_marks.size());
        m_text->setEnabled(true);
        m_text->setText(strtoqstr(Marks::getTextFromMark(mark)));
    }

    if (seg >= 0 && seg < m_ornament->count())
        m_ornament->setCurrentIndex(seg);

    if (timing == BaseProperties::TRIGGER_SEGMENT_ADJUST_NONE) {
        m_adjustTime->setCurrentIndex(0);
    } else if (timing == BaseProperties::TRIGGER_SEGMENT_ADJUST_SQUISH) {
        m_adjustTime->setCurrentIndex(3);
    } else if (timing == BaseProperties::TRIGGER_SEGMENT_ADJUST_SYNC_START) {
        m_adjustTime->setCurrentIndex(1);
    } else if (timing == BaseProperties::TRIGGER_SEGMENT_ADJUST_SYNC_END) {
        m_adjustTime->setCurrentIndex(2);
    }

    m_retune->setChecked(retune);

    settings.endGroup();
}

TriggerSegmentId
UseOrnamentDialog::getId() const
{
    int ix = m_ornament->currentIndex();

    for (Composition::triggersegmentcontaineriterator i =
                m_composition->getTriggerSegments().begin();
            i != m_composition->getTriggerSegments().end(); ++i) {

        if (ix == 0)
            return (*i)->getId();
        --ix;
    }

    return 0;
}

Mark
UseOrnamentDialog::getMark() const
{
    if (int(m_marks.size()) > m_mark->currentIndex())
        return m_marks[m_mark->currentIndex()];
    else
        return Marks::getTextMark(qstrtostr(m_text->text()));
}

bool
UseOrnamentDialog::getRetune() const
{
    return m_retune->isChecked();
}

std::string
UseOrnamentDialog::getTimeAdjust() const
{
    int option = m_adjustTime->currentIndex();

    switch (option) {

    case 0:
        return BaseProperties::TRIGGER_SEGMENT_ADJUST_NONE;
    case 1:
        return BaseProperties::TRIGGER_SEGMENT_ADJUST_SYNC_START;
    case 2:
        return BaseProperties::TRIGGER_SEGMENT_ADJUST_SYNC_END;
    case 3:
        return BaseProperties::TRIGGER_SEGMENT_ADJUST_SQUISH;

    default:
        return BaseProperties::TRIGGER_SEGMENT_ADJUST_NONE;
    }
}

void
UseOrnamentDialog::slotMarkChanged(int i)
{
    if (i == 2) {
        m_text->setEnabled(true);
    } else {
        m_text->setEnabled(false);
    }
}

void
UseOrnamentDialog::accept()
{
    QSettings settings;
    settings.beginGroup( NotationViewConfigGroup );

    settings.setValue("useornamentmark", strtoqstr(getMark()));
    settings.setValue("useornamenttiming", strtoqstr(getTimeAdjust()));
    settings.setValue("useornamentretune", m_retune->isChecked());
    settings.setValue("useornamentlastornament", m_ornament->currentIndex());

    settings.endGroup();

    QDialog::accept();
}

}
