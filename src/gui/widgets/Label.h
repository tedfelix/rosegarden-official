/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2017 the Rosegarden development team.

    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#ifndef RG_LABEL_H
#define RG_LABEL_H

#include <QLabel>
#include <QWheelEvent>


class QWidget;
class QWheelEvent;
class QMouseEvent;


namespace Rosegarden
{

/// Interactive QLabel Widget
class Label : public QLabel
{
    Q_OBJECT
public:
    explicit Label(const QString &text, QWidget *parent=0, Qt::WindowFlags f=0) :
        QLabel(text, parent, f)  { }

    // ??? Non-standard.  Used by Ui_RosegardenTransport.
    Label(QWidget *parent = 0, const char *name=0):
        QLabel(name, parent) {;}

signals:
    void clicked();
    void doubleClicked();
    void scrollWheel(int);

protected:
    virtual void mouseReleaseEvent(QMouseEvent * /*e*/)
        { emit clicked(); }

    virtual void mouseDoubleClickEvent(QMouseEvent * /*e*/)
        { emit doubleClicked(); }

    virtual void wheelEvent(QWheelEvent *e)
        { emit scrollWheel(e->delta()); }

};


}

#endif
