
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

#ifndef _RG_TRACKLABEL_H_
#define _RG_TRACKLABEL_H_

#include "base/Track.h"
#include <QString>
#include <qwidgetstack.h>


class QWidget;
class QTimer;
class QMouseEvent;
class QLabel;


namespace Rosegarden
{



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

    TrackLabel(TrackId id,
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

    void setId(TrackId id) { m_id = id; }
    TrackId getId() const { return m_id; }

    int getPosition() const { return m_position; }
    void setPosition(int position) { m_position = position; }

signals:
    void clicked();

    // We emit this once we've renamed a track
    //
    void renameTrack(QString, TrackId);

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

    TrackId  m_id;
    int                  m_position;
    bool                 m_selected;

    QTimer              *m_pressTimer;
};


}

#endif
