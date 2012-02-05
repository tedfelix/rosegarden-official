
/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2012 the Rosegarden development team.

    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#ifndef _RG_TRACKLABEL_H_
#define _RG_TRACKLABEL_H_

#include "base/Track.h"
#include <QString>
#include <QStackedWidget>
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
 *
 * Actually, this specializes QStackedWidget and creates two QLabels which
 * can be switched to display either track name or instrument name.
 *
 * Suggestion: Rewrite this to be QLabel-derived to reduce the number of
 * widgets.  Keep the track name and instrument name as separate strings
 * and send the proper one to the label based on the current mode.  IOw,
 * where we call setCurrentWidget(), call setText() instead.
 */
class TrackLabel : public QLabel
{
    Q_OBJECT
public:

    TrackLabel(TrackId id,
               int position,
               QWidget *parent,
               const char *name=0);

    ~TrackLabel();

    // ??? Check all use of the term "label".  Should it be "name"?
    //     A name is displayed on a label.

    /// Sets the track name (e.g. "Wicked Solo").
    /**
     * Be sure to call updateLabel() for the change to appear on the UI.
     *
     * @see getTrackName()
     */
    void setTrackName(const QString &text)  { m_trackName = text; }
    /// Gets the track name (e.g. "Wicked Solo").  See setTrackName().
    QString getTrackName() const  { return m_trackName; }

    /// Sets the instrument's presentation name (e.g. "General MIDI Device  #1").
    /**
     * Be sure to call updateLabel() for the change to appear on the UI.
     *
     * "Presentation Name"?  What does that even mean?  I think
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
     * Call this after calling any of the set name routines to get the
     * changes to appear on the UI.
     *
     * @see setTrackName()
     * @see setPresentationName()
     * @see setProgramChangeName()
     * @see setDisplayMode()
     * @see forcePresentationName()
     */
    void updateLabel();

    enum DisplayMode
    {
        ShowTrack,
        ShowInstrument
    };

    /// Selects between showing track labels and instrument labels.
    /**
     * Be sure to call updateLabel() for the change to appear on the UI.
     */
    void setDisplayMode(DisplayMode mode);

    // Encapsulates setting the label to highlighted or not
    void setSelected(bool on);
    bool isSelected() const { return m_selected; }

    void setId(TrackId id) { m_id = id; }
    TrackId getId() const { return m_id; }

    int getPosition() const { return m_position; }
    void setPosition(int position) { m_position = position; }

signals:
    void clicked();

    // We emit this once we've renamed a track
    void renameTrack(QString, TrackId);

    void changeToInstrumentList();

protected:

    virtual void mousePressEvent(QMouseEvent *e);
    virtual void mouseReleaseEvent(QMouseEvent *e);
    virtual void mouseDoubleClickEvent(QMouseEvent *e);

    //--------------- Data members ---------------------------------

    // The track name selected by the user (e.g. "Wicked Solo")
    QString              m_trackName;
    // Presentation Name (e.g. "General MIDI Device  #1")
    QString              m_presentationName;
    // Program Change Name (e.g. "Acoustic Grand Piano")
    QString              m_programChangeName;

    DisplayMode          m_mode;

    bool                 m_forcePresentationName;

    TrackId              m_id;
    int                  m_position;
    bool                 m_selected;

    QTimer              *m_pressTimer;
};


}

#endif
