
/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2016 the Rosegarden development team.

    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#ifndef RG_TRACKLABEL_H
#define RG_TRACKLABEL_H

#include "base/Track.h"
#include <QString>
#include <QLabel>


class QWidget;
class QTimer;
class QMouseEvent;


namespace Rosegarden
{


/**
 * Specialises QLabel to create in effect a toggleable and hence
 * selectable label/label list.  In conjunction with TrackButtons
 * provides a framework for Track selection in the TrackEditor.
 */
class TrackLabel : public QLabel
{
    Q_OBJECT
public:

    TrackLabel(TrackId id,
               int position,
               QWidget *parent);

    /// Sets the track name (e.g. "Wicked Solo").
    /**
     * Be sure to call updateLabel() for the change to appear on the UI.
     *
     * @see getTrackName()
     */
    void setTrackName(const QString &text)  { m_trackName = text; }
    void setShortName(const QString &text)  { m_shortName = text; }
    /// Gets the track name (e.g. "Wicked Solo").  See setTrackName().
    QString getTrackName() const  { return m_trackName; }

    /// Sets the instrument's presentation name (e.g. "General MIDI Device  #1").
    /**
     * Be sure to call updateLabel() for the change to appear on the UI.
     *
     * "Presentation Name"?  Can we come up with something better?  I think
     * it is a combination of a device name and a channel #.  Need to
     * dig and find the proper terms.
     */
    void setPresentationName(const QString &text)
        { m_presentationName = text; }

    /// Set the instrument's program change name (e.g. "Acoustic Grand Piano").
    /**
     * Be sure to call updateLabel() for the change to appear on the UI.
     */
    void setProgramChangeName(const QString &text)
        { m_programChangeName = text; }

    /// Forces the presentation name to be displayed on the label.
    /**
     * This is used when the instrument popup menu is launched.
     *
     * Be sure to call updateLabel() for the change to appear on the UI.
     *
     * @see TrackButtons::slotInstrumentMenu()
     */
    void forcePresentationName(bool force)  { m_forcePresentationName = force; }

    /// Updates the label on the display.
    /**
     * Call this after calling any of the routines that modify the appearance
     * or content of the label to get the changes to appear on the UI.
     *
     * @see setTrackName()
     * @see setPresentationName()
     * @see setProgramChangeName()
     * @see setDisplayMode()
     * @see forcePresentationName()
     */
    void updateLabel();

    /// See setDisplayMode()
    enum DisplayMode
    {
        ShowTrack,
        ShowInstrument
    };

    /// Selects between showing track names and instrument names.
    /**
     * Be sure to call updateLabel() for the change to appear on the UI.
     */
    void setDisplayMode(DisplayMode mode)  { m_mode = mode; }

    /// Highlights this label as being selected.
    /**
     * @see isSelected()
     */
    void setSelected(bool selected);
    /// Whether this track is selected.
    /**
     * @see setSelected()
     */
    bool isSelected() const { return m_selected; }

    /// The Track ID from Track::getId().  See getId(), setPosition()
    void setId(TrackId id) { m_id = id; }
    /// See setId(), getPosition()
    TrackId getId() const { return m_id; }

    /// The position of this track on the UI.
    /**
     * @see setPosition()
     * @see getId()
     */
    int getPosition() const { return m_position; }
    /// See getPosition()
    void setPosition(int position) { m_position = position; }

signals:
    /// Sent on right-click, left-release, and left-double-click events.
    /**
     * This is connected to TrackButtons's clicked signal mapper.
     */
    void clicked();

    /// Sent when a track's name is changed.
    /**
     * Connected to TrackButtons::slotRenameTrack().
     */
    void renameTrack(QString longLabel, QString shortLabel, TrackId trackId);

    /// Sent on right-click to launch the instrument popup menu.
    /**
     * Connected to TrackButtons's instrument list signal mapper.
     */
    void changeToInstrumentList();

private:

    virtual void mousePressEvent(QMouseEvent *e);
    virtual void mouseReleaseEvent(QMouseEvent *e);
    virtual void mouseDoubleClickEvent(QMouseEvent *e);

    //--------------- Data members ---------------------------------

    /// The track name selected by the user (e.g. "Wicked Solo")
    QString              m_trackName;
    /// The short name selected by the user (e.g. "Wkd Slo")
    QString              m_shortName;
    /// Instrument Presentation Name (e.g. "General MIDI Device  #1")
    QString              m_presentationName;
    /// Instrument Program Change Name (e.g. "Acoustic Grand Piano")
    QString              m_programChangeName;

    /// Show track names or instrument names.
    DisplayMode          m_mode;

    bool                 m_forcePresentationName;

    /// The associated Track object's ID.  See Track::getId().
    TrackId              m_id;
    /// Position on the UI.
    int                  m_position;

    bool                 m_selected;

    /// Timer to support the "left-click and hold" behavior.
    /**
     * Connected to changeToInstrumentList() which brings up the
     * instrument popup menu.
     */
    QTimer              *m_pressTimer;
};


}

#endif
