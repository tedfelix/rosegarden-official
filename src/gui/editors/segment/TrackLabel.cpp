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


#include "TrackLabel.h"

#include <klocale.h>
#include "base/Track.h"

#include <QInputDialog>
#include <QFont>
#include <QFrame>
#include <QLabel>
#include <QRegExp>
#include <QString>
#include <QTimer>
#include <QToolTip>
#include <QWidget>
#include <QStackedWidget>
#include <QValidator>
#include <QLayout>
#include <QHBoxLayout>
// #include <QEvent>
#include <QMouseEvent>

namespace Rosegarden
{

TrackLabel::TrackLabel(TrackId id,
                       int position,
                       QWidget *parent,
                       const char *name):
		QStackedWidget(parent),
        m_instrumentLabel(new QLabel(this)),
        m_trackLabel(new QLabel(this)),
        m_id(id),
        m_position(position)
{
	this->setObjectName( name );
	
    QFont font;
    font.setPointSize(font.pointSize() * 95 / 100);
    if (font.pixelSize() > 14)
        font.setPixelSize(14);
    font.setBold(false);
    m_instrumentLabel->setFont(font);
    m_trackLabel->setFont(font);
	
	this->setLayout( new QHBoxLayout() );
	
	m_instrumentLabel->setObjectName( "InstrumentLabel" );
	m_trackLabel->setObjectName( "TrackLabel" );
	
    layout()->addWidget(m_instrumentLabel);		//, ShowInstrument);
	layout()->addWidget(m_trackLabel);			//, ShowTrack);
	
// 	raiseWidget(ShowTrack);
	setCurrentWidget( m_trackLabel );
	
    m_instrumentLabel->setFrameShape(QFrame::NoFrame);
    m_trackLabel->setFrameShape(QFrame::NoFrame);

    m_pressTimer = new QTimer(this);

    connect(m_pressTimer, SIGNAL(timeout()),
            this, SIGNAL(changeToInstrumentList()));

    this->setToolTip(i18n("Click and hold with left mouse button to assign this Track to an Instrument."));

}

TrackLabel::~TrackLabel()
{}

void TrackLabel::setIndent(int i)
{
    m_instrumentLabel->setIndent(i);
    m_trackLabel->setIndent(i);
}

void TrackLabel::setAlternativeLabel(const QString &label)
{
    // recover saved original
    if (label.isEmpty()) {

        if (!m_alternativeLabel.isEmpty())
            m_instrumentLabel->setText(m_alternativeLabel);

        // do nothing if we've got nothing to swap
        return ;
    }

    // Store the current (first) label
    //
    if (m_alternativeLabel.isEmpty())
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
// 	raiseWidget(l);
	if( l == ShowTrack ){
		setCurrentWidget( m_trackLabel );
		
	} else if( l == ShowInstrument ){
		setCurrentWidget( m_instrumentLabel );
		
	}
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
    if( currentWidget() ){
        currentWidget()->update();
	}
}

void
TrackLabel::mousePressEvent(QMouseEvent *e)
{
    if (e->button() == Qt::RightButton) {

        emit clicked();
        emit changeToInstrumentList();

    } else if (e->button() == Qt::LeftButton) {

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

    if (e->button() == Qt::LeftButton) {
        emit clicked();
    }
}

void
TrackLabel::mouseDoubleClickEvent(QMouseEvent *e)
{
    if (e->button() != Qt::LeftButton)
        return ;

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

    QString newText = QInputDialog::getText(this,
										   i18n("Change track name"),
                                            i18n("Enter new track name"),
													QLineEdit::Normal,
                                            m_trackLabel->text(),
                                            &ok
										   );
//                                             &validator);

    if ( ok )
        emit renameTrack(newText, m_id);
}

}
#include "TrackLabel.moc"
