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

#include "gui/widgets/AudioRouteMenu.h"
#include "misc/Debug.h"
#include "gui/widgets/InputDialog.h"
#include "gui/widgets/Label.h"
#include "document/RosegardenDocument.h"
#include "gui/application/RosegardenMainWindow.h"
#include "base/Studio.h"

#include <QFont>
#include <QGridLayout>

namespace Rosegarden
{


AudioStrip::AudioStrip(QWidget *parent, InstrumentId id) :
    QWidget(parent),
    m_id(NoInstrument),
    m_label(NULL),
    m_input(NULL),
    m_output(NULL),
    m_layout(new QGridLayout(this))
{
    QFont font;
    font.setPointSize(6);
    font.setBold(false);
    setFont(font);

    QFont boldFont(font);
    boldFont.setBold(true);

    // We have to have an id in order to create the proper widgets and
    // initialize them.  If we don't, don't worry about it.  Handle it
    // later in setId().
    if (id != NoInstrument)
        setId(id);
}

AudioStrip::~AudioStrip()
{

}

void AudioStrip::setId(InstrumentId id)
{
    // No change?  Bail.
    if (m_id == id)
        return;

    m_id = id;

    // If the widgets haven't been created yet, create them.
    if (!m_label)
        createWidgets();

    // Pass on the new id to widgets that care.

    if (m_input)
        m_input->setInstrument(m_id);

    if (m_output)
        m_output->setInstrument(m_id);
}

void AudioStrip::createWidgets()
{
    // No ID yet?  Bail.
    if (m_id == NoInstrument)
        return;

    QFont boldFont(font());
    boldFont.setBold(true);

    // Label

    m_label = new Label(this);
    m_label->setFont(boldFont);
    m_label->setMaximumWidth(45);
    connect(m_label, SIGNAL(clicked()),
            SLOT(slotLabelClicked()));

    // Input

    if (isInput()) {
        m_input = new AudioRouteMenu(this,
                                     AudioRouteMenu::In,
                                     AudioRouteMenu::Compact,
                                     m_id);
        m_input->getWidget()->setToolTip(tr("Record input source"));
        m_input->getWidget()->setMaximumWidth(45);
    }

    // Output

    if (isInput()) {
        m_output = new AudioRouteMenu(this,
                                      AudioRouteMenu::Out,
                                      AudioRouteMenu::Compact,
                                      m_id);
        m_output->getWidget()->setToolTip(tr("Output destination"));
        m_output->getWidget()->setMaximumWidth(45);
    }

    // Layout

    m_layout->addWidget(m_label, 0, 0, 1, 2, Qt::AlignLeft);

    if (m_input)
        m_layout->addWidget(m_input->getWidget(), 1, 0, 1, 2, Qt::AlignLeft);

    if (m_output)
        m_layout->addWidget(m_output->getWidget(), 2, 0, 1, 2, Qt::AlignLeft);
}

void AudioStrip::updateWidgets()
{
    RosegardenDocument *doc = RosegardenMainWindow::self()->getDocument();
    Studio &studio = doc->getStudio();

    // Get the appropriate instrument based on the ID.
    Instrument *instrument = NULL;
    if (isInput())
        instrument = studio.getInstrumentById(m_id);

    // Update each widget efficiently.

    // Label

    if (isInput()) {
        m_label->setText(strtoqstr(instrument->getAlias()));
        m_label->setToolTip(strtoqstr(instrument->getAlias()) + "\n" +
                tr("Click to rename this instrument"));
    } else if (isSubmaster()) {
        m_label->setText(tr("Sub %1").arg(m_id));
    } else {  // Master
        m_label->setText(tr("Master"));
    }

    // Input

    if (m_input)
        m_input->updateWidget();

    // Output

    if (m_output)
        m_output->updateWidget();
}

void AudioStrip::slotLabelClicked()
{
    // Can only change alias on input strips.
    if (!isInput())
        return;

    QString oldAlias = m_label->text();
    bool ok = false;

    QString newAlias = InputDialog::getText(
            this,  // parent
            tr("Rosegarden"),  // title
            tr("Enter instrument alias:"),  // label
            LineEdit::Normal,  // mode (echo)
            m_label->text(),  // text
            &ok);  // ok

    // Cancelled?  Bail.
    if (!ok)
        return;

    // No change?  Bail.
    if (newAlias == oldAlias)
        return;

    RosegardenDocument *doc = RosegardenMainWindow::self()->getDocument();
    Studio &studio = doc->getStudio();

    // Get the appropriate instrument based on the ID.
    Instrument *instrument = studio.getInstrumentById(m_id);

    instrument->setAlias(newAlias.toStdString());
    // ??? For now, we need this to update AIPP.  Over time, this will go
    //     away, and only the call to slotDocumentModified() will be needed.
    instrument->changed();
    doc->slotDocumentModified();
}


}
