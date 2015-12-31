
/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2016 the Rosegarden development team.

    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#ifndef RG_PROPERTYBOX_H
#define RG_PROPERTYBOX_H

#include <QSize>
#include <QString>
#include <QWidget>


class QPaintEvent;


namespace Rosegarden
{



/**
 * We use a ControlBox to help modify events on the ruler - set tools etc.
 * and provide extra information or options.
 *
 */
class PropertyBox : public QWidget
{
    Q_OBJECT

public:
    PropertyBox(QString label,
               int width,
               int height,
               QWidget *parent=0);

    virtual QSize sizeHint() const;
    virtual QSize minimumSizeHint() const;

protected:
    virtual void paintEvent(QPaintEvent *);

    //--------------- Data members ---------------------------------

    QString m_label;
    int     m_width;
    int     m_height;
};


}

#endif
