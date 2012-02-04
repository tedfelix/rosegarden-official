
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
class TrackLabel : public QStackedWidget
{
    Q_OBJECT
public:

    TrackLabel(TrackId id,
               int position,
               QWidget *parent,
               const char *name=0);

    ~TrackLabel();

    /// Label indent in pixels.  See QLabel::setIndent().
    void setIndent(int pixels);

    // ??? Check all use of the term "label".  Should it be "name"?
    //     A name is displayed on a label.

    /// Sets the instrument's presentation name (e.g. "General MIDI Device  #1").
    /**
     * "Presentation Name"?  What does that even mean?  I think
     * it is a combination of a device name and a channel #.  Need to
     * dig and find the proper terms.
     */
    void setPresentationName(const QString &text)
        { m_instrumentLabel->setText(text); }

    /// Sets the track name (e.g. Melody).  See getTrackName().
    void setTrackName(const QString &text)  { m_trackLabel->setText(text); }
    /// Gets the track name (e.g. Melody).  See setTrackName().
    QString getTrackName() const  { return m_trackLabel->text(); }

    /// Set the instrument's program change name.
    /**
     * Typically, this is used as follows:
     *
     *   1. setPresentationName() is called with the "presentation name"
     *      which looks like "General MIDI Device  #1".  This is set as
     *      the text on the instrument label.
     *   2. setProgramChangeName() is called with the "program name" which
     *      looks like "Acoustic Grand Piano".
     *   3. setProgramChangeName() notices the presentation name is in the
     *      instrument label already, so it stores it as the presentation
     *      name.
     *
     * On future calls, a "program change name" like "Acoustic Grand Piano"
     * is usually sent into this routine and that is what is displayed.
     * If the user turns off the "Program" checkbox in the Instrument
     * Parameters box, an empty string is sent, and the "presentation name"
     * is displayed on the track label.
     *
     * Suggestion: Can we simplify this by offering two routines:
     *
     *   - setPresentationName() sets an m_presentationName
     *   - setProgramChangeName() sets an m_programChangeName
     *
     * Then if m_programChangeName is ever "", we would fall back on
     * displaying m_presentationName.
     *
     * @see clearPresentationName()
     */
    void setProgramChangeName(const QString &programChangeName);

    /// Clears any stored presentation name.
    /**
     * The next call to setProgramChangeName() will store the current
     * instrument label text as the presentation name.
     */
    void clearPresentationName();

    enum DisplayMode
    {
        ShowTrack,
        ShowInstrument
    };

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

    QLabel* getVisibleLabel();

    //--------------- Data members ---------------------------------

    // Typically this displays the program change name (e.g. "Acoustic Grand
    // Piano").  However, if the instrument doesn't send a program change,
    // this displays m_presentationName.
    QLabel              *m_instrumentLabel;

    // Displays the track name selected by the user.
    QLabel              *m_trackLabel;

    // Presentation Name (e.g. "General MIDI Device  #1")
    QString              m_presentationName;

    TrackId              m_id;
    int                  m_position;
    bool                 m_selected;

    QTimer              *m_pressTimer;
};


}

#endif
