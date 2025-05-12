/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2025 the Rosegarden development team.
 
    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.
 
    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/


#include "TrackLabel.h"

#include "misc/Debug.h"
#include "gui/dialogs/TrackLabelDialog.h"
#include "misc/Preferences.h"
#include "misc/ConfigGroups.h"

#include <QFont>
#include <QFrame>
#include <QString>
#include <QTimer>
#include <QWidget>
#include <QMouseEvent>
#include <QSettings>


namespace
{
    // Colors

    QColor getNormalTextColor()
    {
        if (Rosegarden::Preferences::getTheme() ==
                Rosegarden::Preferences::DarkTheme)
            return QColor(Qt::white);
        else
            return QColor(Qt::black);
    }

    QColor getSelectedBackgroundColor()
    {
        if (Rosegarden::Preferences::getTheme() ==
                Rosegarden::Preferences::DarkTheme)
            return QColor(128+64, 128+64, 128+64);
        else
            return QColor(0xAA, 0xAA, 0xAA);
    }

    QColor getSelectedTextColor()
    {
        if (Rosegarden::Preferences::getTheme() ==
                Rosegarden::Preferences::DarkTheme)
            return QColor(Qt::black);
        else
            return QColor(Qt::white);
    }

    QColor getArchiveTextColor()
    {
        if (Rosegarden::Preferences::getTheme() ==
                Rosegarden::Preferences::DarkTheme)
            return QColor(128+32,128+32,128+32);
        else
            return QColor(Qt::black);
    }

}


namespace Rosegarden
{


TrackLabel::TrackLabel(TrackId id,
                       int position,
                       int trackHeight,
                       QWidget *parent) :
        QLabel(parent),
        m_mode(ShowTrack),
        m_forcePresentationName(false),
        m_id(id),
        m_position(position)
{
    setObjectName("TrackLabel");

    // Compute Label Width

    QFont font;
    font.setPixelSize(trackHeight * 85 / 100);
    setFont(font);

    QFontMetrics fontMetrics(font);

    // set the label width depending on the setting in Settings > Presentation
    QSettings settings;
    settings.beginGroup(GeneralOptionsConfigGroup);
    // Default to "2" for legacy wide.
    const int trackLabelWidth = settings.value("track_label_width", 2).toInt();
    // Write it back out so we can find it.
    settings.setValue("track_label_width", trackLabelWidth);

    int labelWidth{18};
    if (trackLabelWidth == 0)       // Narrow: 8 char
        labelWidth = 8;
    else if (trackLabelWidth == 1)  // Medium: 12 char
        labelWidth = 12;
    else if (trackLabelWidth == 2)  // Wide: 18
        labelWidth = 18;

    QString labelWidthString;
    // We use all "X" because the font is proportional and "X" is usually one
    // of the wider characters in a proportional font.
    labelWidthString.fill('X', labelWidth);

    setFixedWidth(fontMetrics.boundingRect(labelWidthString).width());
    
    setFixedHeight(trackHeight);

    setFrameShape(QFrame::NoFrame);

    m_pressTimer = new QTimer(this);

    connect(m_pressTimer, &QTimer::timeout,
            this, &TrackLabel::changeToInstrumentList);

    setToolTip(tr("<qt>"
                  "<p>Click to select all the segments on this track.</p>"
                  "<p>Shift+click to add to or to remove from the"
                  " selection all the segments on this track.</p>"
                  "<p>Click and hold with either mouse button to assign"
                  " this track to an instrument.</p>"
                  "</qt>"));

    // Trick setSelected() into making an actual change.
    m_selected = true;
    setSelected(false);
}

void
TrackLabel::updateLabel()
{
    if (m_forcePresentationName) {
        setText(m_presentationName);
        return;
    }

    if (m_mode == ShowTrack) {
        setText(m_trackName);
    } else if (m_mode == ShowInstrument) {
        if (m_programChangeName != "") {
            setText(m_programChangeName);
        } else {
            setText(m_presentationName);
        }
    }
}

void
TrackLabel::updatePalette()
{
    QPalette pal = palette();

    if (m_selected) {
        setAutoFillBackground(true);
        pal.setColor(QPalette::Window, getSelectedBackgroundColor());
        pal.setColor(QPalette::WindowText, getSelectedTextColor());
    } else {
        // Let the parent color show through.
        setAutoFillBackground(false);
        if (m_archived)
            pal.setColor(QPalette::WindowText, getArchiveTextColor());
        else
            pal.setColor(QPalette::WindowText, getNormalTextColor());
    }

    setPalette(pal);
}

void
TrackLabel::setSelected(bool selected)
{
    // No change?  Bail.
    if (selected == m_selected)
        return;

    m_selected = selected;

    updatePalette();
}

void
TrackLabel::setArchived(bool archived)
{
    // No change?  Bail.
    if (archived == m_archived)
        return;

    m_archived = archived;

    updatePalette();
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


    TrackLabelDialog dlg(this,
                         tr("Change track name"),
                         tr("Enter new track name"),
                         m_trackName,
                         tr("<qt>The track name is also the notation staff name, eg. &quot;Trumpet.&quot;</qt>"),
                         tr("Enter short name"),
                         m_shortName,
                         tr("<qt>The short name is an alternate name that appears each time the staff system wraps, eg. &quot;Tr.&quot;</qt>")
                         );

    if (dlg.exec() == QDialog::Accepted) {

        QString longLabel = dlg.getPrimaryText();
        QString shortLabel = dlg.getSecondaryText();

        emit renameTrack(longLabel, shortLabel, m_id);
    }

}

}
