/*
    Rosegarden-4
    A sequencer and musical notation editor.

    This program is Copyright 2000-2005
        Guillaume Laurent   <glaurent@telegraph-road.org>,
        Chris Cannam        <cannam@all-day-breakfast.com>,
        Richard Bown        <bownie@bownie.com>

    The moral right of the authors to claim authorship of this work
    has been asserted.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/


#include <qlabel.h>
#include <qtimer.h>
#include <qvalidator.h>
#include <qtooltip.h>

#include <klineeditdlg.h>
#include <klocale.h>

#include "rosedebug.h"
#include "tracklabel.h"

TrackLabel::TrackLabel(Rosegarden::TrackId id,
                       int position,
                       QWidget *parent,
                       const char *name):
    QWidgetStack(parent, name),
    m_instrumentLabel(new QLabel(this)),
    m_trackLabel(new QLabel(this)),
    m_id(id),
    m_position(position)
{
    addWidget(m_instrumentLabel, ShowInstrument);
    addWidget(m_trackLabel, ShowTrack);
    raiseWidget(ShowTrack);

    m_instrumentLabel->setFrameShape(QFrame::NoFrame);
    m_trackLabel->setFrameShape(QFrame::NoFrame);

    m_pressTimer = new QTimer(this);

    connect(m_pressTimer, SIGNAL(timeout()),
            this, SIGNAL(changeToInstrumentList()));

    QToolTip::add(this, i18n("Click and hold with left mouse button to assign this Track to an Instrument."));

}

TrackLabel::~TrackLabel()
{
}

// void TrackLabel::setText(QString s)
// {
//     getVisibleLabel()->setText(s);
// }

// QString TrackLabel::text()
// {
//     return getVisibleLabel()->text();
// }

void TrackLabel::setIndent(int i)
{
    m_instrumentLabel->setIndent(i);
    m_trackLabel->setIndent(i);
}

void TrackLabel::setAlternativeLabel(const QString &label)
{
    // recover saved original
    if (label.isEmpty()) {

        if(!m_alternativeLabel.isEmpty())
            m_instrumentLabel->setText(m_alternativeLabel);

        // do nothing if we've got nothing to swap
        return;
    }

    // Store the current (first) label 
    //
    if(m_alternativeLabel.isEmpty())
        m_alternativeLabel = m_instrumentLabel->text();

    // set new label
    m_instrumentLabel->setText(label);
}

void TrackLabel::clearAlternativeLabel()
{
    m_alternativeLabel = "";
}


void TrackLabel::showLabel(InstrumentTrackLabels l)
{
    raiseWidget(l);
}



void
TrackLabel::setSelected(bool on)
{
    if (on) {
        m_selected = true;
	
        m_instrumentLabel->setPaletteBackgroundColor(colorGroup().highlight());
        m_instrumentLabel->setPaletteForegroundColor(colorGroup().highlightedText());
	m_trackLabel->setPaletteBackgroundColor(colorGroup().highlight());
        m_trackLabel->setPaletteForegroundColor(colorGroup().highlightedText());

    } else {
        m_selected = false;
	
        m_instrumentLabel->setPaletteBackgroundColor(colorGroup().background());
	m_trackLabel->setPaletteBackgroundColor(colorGroup().background());
        m_instrumentLabel->setPaletteForegroundColor(colorGroup().text());
        m_trackLabel->setPaletteForegroundColor(colorGroup().text());
    }
    if (visibleWidget()) visibleWidget()->update();
}

void
TrackLabel::mousePressEvent(QMouseEvent *e)
{
    if (e->button() == RightButton) {

        emit clicked();
        emit changeToInstrumentList();

    } else if (e->button() == LeftButton) {

        // start a timer on this hold
        m_pressTimer->start(200, true); // 200ms, single shot
    }
}

void
TrackLabel::mouseReleaseEvent(QMouseEvent *e)
{
    // stop the timer if running
    if (m_pressTimer->isActive())
        m_pressTimer->stop();

    if (e->button() == LeftButton) {
        emit clicked();
    }
}

void
TrackLabel::mouseDoubleClickEvent(QMouseEvent *e)
{
    if (e->button() != LeftButton)
        return;

    // Highlight this label alone and cheat using
    // the clicked signal
    //
    emit clicked();

    // Just in case we've got our timing wrong - reapply
    // this label highlight
    //
    setSelected(true);

    bool ok = false;

    QRegExpValidator validator(QRegExp(".*"), this); // empty is OK

    QString newText = KLineEditDlg::getText(i18n("Change track name"),
                                            i18n("Enter new track name"),
                                            m_trackLabel->text(),
                                            &ok,
                                            this,
					    &validator);

    if ( ok )
        emit renameTrack(newText, m_id);
}

QLabel* TrackLabel::getVisibleLabel()
{
    return dynamic_cast<QLabel*>(visibleWidget());
}
#include "tracklabel.moc"
