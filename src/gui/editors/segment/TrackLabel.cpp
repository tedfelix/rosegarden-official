/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2011 the Rosegarden development team.
 
    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.
 
    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/


#include "TrackLabel.h"

#include "base/Track.h"
#include "gui/widgets/LineEdit.h"
#include "gui/widgets/InputDialog.h"

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
#include <QMouseEvent>
#include <QHeaderView>

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
    this->setObjectName(name);
    
    QFont font;
    font.setPointSize(font.pointSize() * 95 / 100);
    if (font.pixelSize() > 14)
        font.setPixelSize(14);
    font.setBold(false);
    m_instrumentLabel->setFont(font);
    m_trackLabel->setFont(font);

    m_instrumentLabel->setAutoFillBackground(true);
    m_trackLabel->setAutoFillBackground(true);
    
//    this->setLayout( new QHBoxLayout() );
//        layout()->setMargin(0);
    
    m_instrumentLabel->setObjectName("InstrumentLabel");
    m_trackLabel->setObjectName("TrackLabel");
    
//    layout()->addWidget(m_instrumentLabel);        //, ShowInstrument);
    addWidget(m_instrumentLabel);        //, ShowInstrument);
//    layout()->addWidget(m_trackLabel);            //, ShowTrack);
    addWidget(m_trackLabel);        //, ShowInstrument);
    
//     raiseWidget(ShowTrack);
    setCurrentWidget( m_trackLabel );
    
    m_instrumentLabel->setFrameShape(QFrame::NoFrame);
    m_trackLabel->setFrameShape(QFrame::NoFrame);

    m_pressTimer = new QTimer(this);

    connect(m_pressTimer, SIGNAL(timeout()),
            this, SIGNAL(changeToInstrumentList()));

    this->setToolTip(tr("<qt>"
                        "<p>Click to select all the segments on this track.</p>"
                        "<p>Shift+click to add to or to remove from the"
                        " selection all the segments on this track.</p>"
                        "<p>Click and hold with either mouse button to assign"
                        " this track to an instrument.</p>"
                        "</qt>"));

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
//     raiseWidget(l);
    if( l == ShowTrack ){
        setCurrentWidget( m_trackLabel );
        
    } else if( l == ShowInstrument ){
        setCurrentWidget( m_instrumentLabel );
        
    }
}

void
TrackLabel::setSelected(bool on)
{
//###
// NOTES: Using QPalette works fine if there is no stylesheet.  If there is a
// stylesheet, the QPalette-based stuff can no longer set the background if the
// background is controlled in any way by the stylesheet.  (This is apparently
// what the warnings in the API docs are all about.)
//
// We could use setObjectName() to change the name, and thus change how these
// widgets would be styled, but we'd have to unset and reset the entire
// stylesheet for that to work, apparently.  I've elected just to resort to hard
// code instead, and use spot stylesheets.  This is bound to be less complicated
// and have less overhead, though it comes with some side effects that may have
// to be revisited.
//
    QString localStyle = "";

    if (on) {
        m_selected = true;
        localStyle="QLabel { background-color: #AAAAAA; color: #FFFFFF; }";
    } else {
        m_selected = false;
        localStyle="QLabel { background-color: transparent; color: #000000; }";
    }

    m_instrumentLabel->setStyleSheet(localStyle);
    m_trackLabel->setStyleSheet(localStyle);

    if (currentWidget()){
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
        m_pressTimer->setSingleShot(true);
        m_pressTimer->start(200); // 200ms, single shot
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
    
    QString newText = InputDialog::getText(this,
                                           tr("Change track name"),
                                           tr("Enter new track name"),
                                           LineEdit::Normal,
                                           m_trackLabel->text(),
                                           &ok
                                           );
//                                             &validator);
//
//&&& what to do with these validators that aren't part of Q/InputDialog?  We
//could do something with them in InputDialog I suppose, but I'm not quite sure
//how that would work.

    if ( ok )
        emit renameTrack(newText, m_id);
}

}
#include "TrackLabel.moc"
