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

#include <qlabel.h>
#include <qtimer.h>

#include "Instrument.h"
#include "Track.h"

//  Display an Instrument name and can also send signals
//  to pop up an instrument list.
//

#ifndef _INSTRUMENTLABEL_H_
#define _INSTRUMENTLABEL_H_

class InstrumentLabel : public QLabel
{
Q_OBJECT
public:
    InstrumentLabel(const QString &label,
                    Rosegarden::TrackId track,
                    QWidget *parent=0,
                    const char *name=0);

    InstrumentLabel(Rosegarden::TrackId id,
                    QWidget *parent=0,
                    const char *name=0);
    ~InstrumentLabel();

    void setId(Rosegarden::TrackId id) { m_id = id; }
    Rosegarden::TrackId getId() const { return m_id; }
    QPoint getPressPosition() const { return m_pressPosition; }

    bool isSelected() const { return m_selected; }
    void setSelected(bool value);

public slots:
    void slotChangeToInstrumentList();
    //
    // Set an alternative label
    //
    void slotSetAlternativeLabel(const QString &label);
    void clearAlternativeLabel() { m_alternativeLabel = ""; }

signals:
    // Our version of released() has an int id associated with it
    //
    void released(int); // TODO - rename this to 'clicked(int)'

    void changeToInstrumentList(int);

protected:

    virtual void mousePressEvent(QMouseEvent *e);
    virtual void mouseReleaseEvent(QMouseEvent *e);

    Rosegarden::TrackId       m_id;

    QTimer                   *m_pressTimer;
    QPoint                    m_pressPosition;
    QString                   m_alternativeLabel;

    bool                      m_selected;

};


#endif // _INSTRUMENTLABEL_H_
