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

#define RG_MODULE_STRING "[AudioStrip]"

#include "AudioStrip.h"

#include "misc/Debug.h"
#include "gui/widgets/Label.h"
#include "document/RosegardenDocument.h"
#include "gui/application/RosegardenMainWindow.h"
#include "base/Studio.h"

#include <QFont>
#include <QGridLayout>

namespace Rosegarden
{


AudioStrip::AudioStrip(QWidget *parent, unsigned i_id) :
    QWidget(parent),
    id(i_id),
    m_layout(new QGridLayout(this))
{
    QFont font;
    font.setPointSize(6);
    font.setBold(false);
    setFont(font);

    QFont boldFont(font);
    boldFont.setBold(true);

    // Label

    m_label = new Label(this);
    m_label->setFont(boldFont);
    m_label->setMaximumWidth(45);
    connect(m_label, SIGNAL(clicked()),
            SLOT(slotLabelClicked()));

    // Layout

    m_layout->addWidget(m_label, 0, 0, 1, 2, Qt::AlignLeft);

    updateWidgets();
}

AudioStrip::~AudioStrip()
{

}

void AudioStrip::updateWidgets()
{
    RosegardenDocument *doc = RosegardenMainWindow::self()->getDocument();
    Studio &studio = doc->getStudio();

    // Get the appropriate instrument based on the ID.
    Instrument *instrument = NULL;
    if (isInput())
        instrument = studio.getInstrumentById(id);

    // Update each widget efficiently.

    // Label

    if (isInput()) {
        m_label->setText(strtoqstr(instrument->getAlias()));
        m_label->setToolTip(strtoqstr(instrument->getAlias()) + "\n" +
                tr("Click to rename this instrument"));
    } else if (isSubmaster()) {
        m_label->setText(tr("Sub %1").arg(id));
    } else {  // Master
        m_label->setText(tr("Master"));
    }
}

void AudioStrip::slotLabelClicked()
{

}


}
