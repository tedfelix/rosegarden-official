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

#ifndef _ABS_SCROLL_AREA_H_
#define _ABS_SCROLL_AREA_H_

#include <QAbstractScrollArea>

/**This subclass of QAbstractScrollArea exists to try a hack whereby if the
 * transport is rolling, 90% of update() events are dropped.  This is an
 * experiment to see if a cheap bandaid can produce some useful results and save
 * a much deeper process of surgical refactoring of the whole segment update
 * glob of craziness.
 *
 * \author D. Michael McIntyre
 */
namespace Rosegarden
{

class AbstractScrollArea : public QAbstractScrollArea
{
    Q_OBJECT
public:
    AbstractScrollArea(QWidget *parent=0);
    ~AbstractScrollArea();
    void update();
protected:
    int m_updateCount;
};

}

#endif
