/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2008 the Rosegarden development team.
 
    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.
 
    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/


#include "UseOrnamentDialog.h"
#include <QLayout>
#include <QApplication>

#include "base/BaseProperties.h"
#include <klocale.h>
#include "misc/Strings.h"
#include "document/ConfigGroups.h"
#include "base/Composition.h"
#include "base/NotationTypes.h"
#include "base/TriggerSegment.h"
#include "gui/editors/notation/NotePixmapFactory.h"
#include <QComboBox>
#include <QSettings>
#include <QDialog>
#include <QDialogButtonBox>
#include <QCheckBox>
#include <QFrame>
#include <QGroupBox>
#include <QLabel>
#include <QLineEdit>
#include <QString>
#include <QWidget>
#include <QVBoxLayout>


namespace Rosegarden
{

UseOrnamentDialog::UseOrnamentDialog(QDialogButtonBox::QWidget *parent,
                                     Composition *composition) :
        KDialogBase(parent, "useornamentdialog", true, i18n("Use Ornament"),
                    Ok | Cancel, Ok),
        m_composition(composition)
{
    QVBox *vbox = makeVBoxMainWidget();
    QLabel *label;

    QGroupBox *notationBox = new QGroupBox(1, Horizontal, i18n("Notation"), vbox);

    QFrame *frame = new QFrame(notationBox);
    QGridLayout *layout = new QGridLayout(frame, 4, 1, 5, 5);

    label = new QLabel(i18n("Display as:  "), frame);
    layout->addWidget(label, 0, 0);

    m_mark = new QComboBox(frame);
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
                                     i18n("Trill"), i18n("Trill with line"), i18n("Trill line only"),
                                     i18n("Turn"), i18n("Mordent"), i18n("Inverted mordent"),
                                     i18n("Long mordent"), i18n("Long inverted mordent"),
                                 };

    for (size_t i = 0; i < m_marks.size(); ++i) {
        m_mark->addItem(NotePixmapFactory::toQPixmap
                           (NotePixmapFactory::makeMarkMenuPixmap(m_marks[i])),
                           markLabels[i]);
    }
    m_mark->addItem(i18n("Text mark"));

    connect(m_mark, SIGNAL(activated(int)), this, SLOT(slotMarkChanged(int)));

    m_textLabel = new QLabel(i18n("   Text:  "), frame);
    layout->addWidget(m_textLabel, 0, 2);

    m_text = new QLineEdit(frame);
    layout->addWidget(m_text, 0, 3);

    QGroupBox *performBox = new QGroupBox(1, Horizontal, i18n("Performance"), vbox);

    frame = new QFrame(performBox);
    layout = new QGridLayout(frame, 3, 2, 5, 5);

    label = new QLabel(i18n("Perform using triggered segment: "), frame);
    layout->addWidget(label, 0, 0);

    m_ornament = new QComboBox(frame);
    layout->addWidget(m_ornament, 0, 1);

    int n = 1;
    for (Composition::triggersegmentcontaineriterator i =
                m_composition->getTriggerSegments().begin();
            i != m_composition->getTriggerSegments().end(); ++i) {
        m_ornament->addItem
        (QString("%1. %2").arg(n++).arg(strtoqstr((*i)->getSegment()->getLabel())));
    }

    label = new QLabel(i18n("Perform with timing: "), frame);
    layout->addWidget(label, 1, 0);

    m_adjustTime = new QComboBox(frame);
    layout->addWidget(m_adjustTime, 1, 1);

    m_adjustTime->addItem(i18n("As stored"));
    m_adjustTime->addItem(i18n("Truncate if longer than note"));
    m_adjustTime->addItem(i18n("End at same time as note"));
    m_adjustTime->addItem(i18n("Stretch or squash segment to note duration"));

    m_retune = new QCheckBox(i18n("Adjust pitch to note"), frame);
    m_retune->setChecked(true);

    layout->addWidget(m_retune, 2, 1);

    setupFromConfig();
}

void
UseOrnamentDialog::setupFromConfig()
{
    QSettings config;
    config.beginGroup( NotationViewConfigGroup );
    // 
    // FIX-manually-(GW), add:
    // config.endGroup();		// corresponding to: config.beginGroup( NotationViewConfigGroup );
    //  


    Mark mark = qstrtostr(config.value("useornamentmark", "trill") );
    int seg = config.value("useornamentlastornament", 0).toInt() ;
    std::string timing = qstrtostr
                         (config->readEntry
                          ("useornamenttiming",
                           strtoqstr(BaseProperties::TRIGGER_SEGMENT_ADJUST_SQUISH)));
    bool retune = qStrToBool( config.value("useornamentretune", "true" ) ) ;

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
UseOrnamentDialog::slotOk()
{
    QSettings config;
    config.beginGroup( NotationViewConfigGroup );
    // 
    // FIX-manually-(GW), add:
    // config.endGroup();		// corresponding to: config.beginGroup( NotationViewConfigGroup );
    //  


    config.setValue("useornamentmark", strtoqstr(getMark()));
    config.setValue("useornamenttiming", strtoqstr(getTimeAdjust()));
    config.setValue("useornamentretune", m_retune->isChecked());
    config.setValue("useornamentlastornament", m_ornament->currentIndex());

    accept();
}

}
#include "UseOrnamentDialog.moc"
