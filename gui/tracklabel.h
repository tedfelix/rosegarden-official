/*
    Rosegarden-4 v0.1
    A sequencer and musical notation editor.

    This program is Copyright 2000-2002
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
#include <qlabel.h>
#include <qtimer.h>
#include <qinputdialog.h>
#include <iostream>

// Specialises QLabel to create in effect a toggleable and
// hence selectable label/label list.  In conjunction with
// TrackButtons provides a framework for Track selection
// on the TrackCanvas.
//
//

class TrackLabel : public QLabel
{
Q_OBJECT
public:
    TrackLabel(const int &position,
               QWidget *parent,
               const char *name=0,
               WFlags f=0);

    ~TrackLabel();

    // Encapsulates setting the label to highlighted or not
    //
    void setLabelHighlight(const bool &on);

    bool isSelected() { return m_selected; }
    int getPosition() { return m_position; }

protected:
    virtual void mousePressEvent(QMouseEvent *e);
    virtual void mouseReleaseEvent(QMouseEvent *e);
    virtual void mouseDoubleClickEvent(QMouseEvent *e);

public slots:
        void slotChangeToInstrumentList();

signals:
    // Our version of released() has an int id associated with it
    //
    void released(int);

    // We emit this once we've renamed a track
    //
    void renameTrack(QString, int);

    void changeToInstrumentList(int);

private:

    int  m_position;
    bool m_selected;

    QTimer *m_pressTimer;

};

#endif // _TRACK_LABEL_H_
