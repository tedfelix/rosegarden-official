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

#include <qlabel.h>
#include <qtimer.h>

#include "Instrument.h"

//  Display an Instrument name and can also send signals
//  to pop up an instrument list.
//

#ifndef _INSTRUMENTLABEL_H_
#define _INSTRUMENTLABEL_H_

class InstrumentLabel : public QLabel
{
Q_OBJECT
public:
    InstrumentLabel(const QString & text, int position,
                    QWidget *parent=0, const char *name=0);

    InstrumentLabel(int position, QWidget *parent=0, const char *name=0);

    ~InstrumentLabel();

    int getPosition() const { return m_position; }

    QPoint getPressPosition() const { return m_pressPosition; }


public slots:
    void slotChangeToInstrumentList();
    void setLabelHighlight(bool value);


protected:
    virtual void mousePressEvent(QMouseEvent *e);
    virtual void mouseReleaseEvent(QMouseEvent *e);


signals:
    // Our version of released() has an int id associated with it
    //
    void released(int);

    void changeToInstrumentList(int);


private:
    int m_position;

    QTimer *m_pressTimer;
    QPoint m_pressPosition;


};


#endif // _INSTRUMENTLABEL_H_
