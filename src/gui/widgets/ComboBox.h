/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2015 the Rosegarden development team.

    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#ifndef RG_LINE_EDIT_H
#define RG_LINE_EDIT_H

#include <QLineEdit>

/**This subclass of QLineEdit uses a spot stylesheet to set its own background
 * color.
 *
 * The normal styling process has involved painting broad "widgetless" areas
 * with QWidget hacks.  These are necessary to paint certain void areas in the
 * main window background and in various other places that don't belong to
 * anything that can be addressed via a QSS selector.  These QWidget hacks
 * affect all child widgets, but it is normally possible to override the
 * background with highly specific selectors.  Not so for QLineEdit, it turns
 * out.  The only way to control its background after a QWidget hack upstream is
 * to use a local stylesheet directly in the code.  This class exists in order
 * to avoid having to pepper the code with stylesheet hacks for QLineEdit.
 *
 * \author D. Michael McIntyre
 */
namespace Rosegarden
{

class LineEdit : public QLineEdit
{
    Q_OBJECT
public:
    LineEdit(QWidget *parent=0);
    LineEdit(const QString&, QWidget *parent=0);
    ~LineEdit();
};

}

#endif
