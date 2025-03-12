/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2025 the Rosegarden development team.

    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#define RG_MODULE_STRING "[RestWidget]"
#define RG_NO_DEBUG_PRINT

#include "RestWidget.h"

#include "EditEvent.h"

#include "base/Composition.h"
#include "base/Event.h"
#include "base/NotationTypes.h"  // For Note::EventRestType...
#include "document/RosegardenDocument.h"
#include "gui/dialogs/TimeDialog.h"

#include <QGridLayout>
#include <QGroupBox>
#include <QLabel>
#include <QPushButton>
#include <QSpinBox>


namespace Rosegarden
{


RestWidget::RestWidget(EditEvent *parent, const Event &event) :
    EventWidget(parent),
    m_parent(parent)
{
    if (event.getType() != Note::EventRestType)
        return;

    // Main layout.
    // This is a "fake" layout that is needed to make sure there is a
    // layout at each parent/child level.  If we remove this layout, the
    // resizing from the parent down to the widgets becomes a mess.
    // Using QGridLayout because it is handy.  Any layout would do here.
    QGridLayout *mainLayout = new QGridLayout(this);

    // Rest Properties group box

    QGroupBox *propertiesGroup = new QGroupBox(tr("Rest Properties"), this);
    propertiesGroup->setContentsMargins(5, 5, 5, 5);
    mainLayout->addWidget(propertiesGroup);

    QGridLayout *propertiesLayout = new QGridLayout(propertiesGroup);
    propertiesLayout->setSpacing(5);

    int row{0};

    // Duration
    QLabel *durationLabel = new QLabel(tr("Duration:"), propertiesGroup);
    propertiesLayout->addWidget(durationLabel, row, 0);

    m_durationSpinBox = new QSpinBox(propertiesGroup);
    m_durationSpinBox->setMinimum(0);
    m_durationSpinBox->setMaximum(INT_MAX);
    m_durationSpinBox->setSingleStep(Note(Note::Shortest).getDuration());
    m_durationSpinBox->setValue(event.getDuration());
    propertiesLayout->addWidget(m_durationSpinBox, row, 1);

    QPushButton *durationEditButton =
            new QPushButton(tr("edit"), propertiesGroup);
    connect(durationEditButton, &QPushButton::clicked,
            this, &RestWidget::slotEditDuration);
    propertiesLayout->addWidget(durationEditButton, row, 2);

    ++row;

}

EventWidget::PropertyNameSet
RestWidget::getPropertyFilter() const
{
    return PropertyNameSet();
}

void
RestWidget::slotEditDuration(bool /*checked*/)
{
    Composition &composition =
            RosegardenDocument::currentDocument->getComposition();

    // This is needed to get the correct bar counts based on the current
    // time signature.  E.g. in 4/4, 3840 is one bar, in 2/4, 3840 is two bars.
    const timeT startTime = m_parent->getAbsoluteTime();

    TimeDialog dialog(
            this,  // parent
            tr("Edit Duration"),  // title
            &composition,  // composition
            startTime,  // startTime
            m_durationSpinBox->value(),  // defaultDuration
            1,  // minimumDuration
            true);  // constrainToCompositionDuration
    if (dialog.exec() == QDialog::Accepted)
        m_durationSpinBox->setValue(dialog.getTime());
}

void RestWidget::updateEvent(Event &event) const
{
    event.setDuration(m_durationSpinBox->value());
}


}
