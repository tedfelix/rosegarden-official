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

#include <qlabel.h>
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
    TrackLabel(const int &id, QWidget *parent, const char *name=0, WFlags f=0):
               QLabel(parent, name, f), m_id(id) {;}

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
    virtual void mouseReleaseEvent(QMouseEvent *mE)
    {
        if (m_selected)
            setLabelHighlight(false);
        else
            setLabelHighlight(true);

        emit released(m_id);
    }

    bool isSelected() { return m_selected; }
    int id() { return m_id; }

signals:
    // Our version of released() has an int id associated with it
    //
    void released(int);

private:

    int  m_id;
    bool m_selected;

};

#endif // _TRACK_LABEL_H_
