/*
    Rosegarden-4
    A sequencer and musical notation editor.

    This program is Copyright 2000-2003
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


#ifndef _TRACK_LABEL_H_
#define _TRACK_LABEL_H_

#include <qstring.h>
#include <qwidgetstack.h>

#include "Track.h"

class QTimer;
class QLabel;

/**
 * Specialises QLabel to create in effect a toggleable and hence
 * selectable label/label list.  In conjunction with TrackButtons
 * provides a framework for Track selection on the TrackCanvas.
 */
class TrackLabel : public QWidgetStack
{
Q_OBJECT
public:

    enum InstrumentTrackLabels
    {
        ShowTrack,
        ShowInstrument,
        ShowBoth
    };

    TrackLabel(Rosegarden::TrackId id,
               int position,
               QWidget *parent,
               const char *name=0);

    ~TrackLabel();

    // QLabel API delegation - applies on both labels
    void setIndent(int);

    QLabel* getInstrumentLabel() { return m_instrumentLabel; }
    QLabel* getTrackLabel()      { return m_trackLabel; }
    void setAlternativeLabel(const QString &label);
    void clearAlternativeLabel();
    void showLabel(InstrumentTrackLabels);

    // Encapsulates setting the label to highlighted or not
    //
    void setSelected(bool on);
    bool isSelected() const { return m_selected; }

    void setId(Rosegarden::TrackId id) { m_id = id; }
    Rosegarden::TrackId getId() const { return m_id; }

    int getPosition() const { return m_position; }
    void setPosition(int position) { m_position = position; }

signals:
    void clicked();

    // We emit this once we've renamed a track
    //
    void renameTrack(QString, int);

    void changeToInstrumentList();

protected:

    virtual void mousePressEvent(QMouseEvent *e);
    virtual void mouseReleaseEvent(QMouseEvent *e);
    virtual void mouseDoubleClickEvent(QMouseEvent *e);

    QLabel* getVisibleLabel();

    //--------------- Data members ---------------------------------

    QLabel              *m_instrumentLabel;
    QLabel              *m_trackLabel;
    QString              m_alternativeLabel;

    Rosegarden::TrackId  m_id;
    int                  m_position;
    bool                 m_selected;

    QTimer              *m_pressTimer;
};

#endif // _TRACK_LABEL_H_
