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
    TrackLabel(const int &trackNum,
               QWidget *parent,
               const char *name=0,
               WFlags f=0):
               QLabel(parent, name, f), m_trackNum(trackNum) {;}

    ~TrackLabel() {;}

    // Encapsulates setting the label to highlighted or not
    //
    void setLabelHighlight(const bool &on)
    {
        if (on)
        {
            m_selected = true;
            setBackgroundMode(PaletteBase);
        }
        else
        {
            m_selected = false;
            setBackgroundMode(PaletteBackground);
        }
    }

    // Overrides virtual release event and sends upwards
    // along with Label id - a bit like a QButtonGroup
    //
    virtual void mouseReleaseEvent(QMouseEvent *)
    {
        if (m_selected)
            setLabelHighlight(false);
        else
            setLabelHighlight(true);

        emit released(m_trackNum);
    }

    virtual void mouseDoubleClickEvent(QMouseEvent *)
    {
        // Highlight this label alone and cheat using
        // the released signal
        //
        emit released(m_trackNum);

        // Just in case we've got our timing wrong - reapply
        // this label highlight
        //
        setLabelHighlight(true);

        bool ok = false;
        QString newText = QInputDialog::getText(
                                     QString("Change track name"),
                                     QString("Enter new track name"),
                                     text(),
                                     &ok,
                                     this);

        if ( ok && !newText.isEmpty() )
            emit renameTrack(newText, m_trackNum);
    }

    bool isSelected() { return m_selected; }
    int trackNum() { return m_trackNum; }

signals:
    // Our version of released() has an int id associated with it
    //
    void released(int);

    // We emit this once we've renamed a track
    //
    void renameTrack(QString, int);

private:

    int  m_trackNum;
    bool m_selected;

};

#endif // _TRACK_LABEL_H_
