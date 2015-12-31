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

#ifndef RG_THORN_STYLE_H
#define RG_THORN_STYLE_H

#include <QProxyStyle>
#include <QIcon>
#include <rosegardenprivate_export.h>

namespace Rosegarden
{


/** Subclass a widget style so we can define our own custom icons for
 * QMessageBox and so on and make them look better in the Thorn style.
 */
class ROSEGARDENPRIVATE_EXPORT ThornStyle : public QProxyStyle
{
    Q_OBJECT

public:
    ThornStyle();
    ~ThornStyle();

protected slots:
    QIcon standardIconImplementation(StandardPixmap standardIcon,
                                     const QStyleOption * option = 0,
                                     const QWidget * widget = 0) const;
};    

}

#endif
