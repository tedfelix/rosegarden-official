/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
 
    This program is Copyright 2000-2008
        Guillaume Laurent   <glaurent@telegraph-road.org>,
        Chris Cannam        <cannam@all-day-breakfast.com>,
        Richard Bown        <richard.bown@ferventsoftware.com>
 
    The moral rights of Guillaume Laurent, Chris Cannam, and Richard
    Bown to claim authorship of this work have been asserted.
 
    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.
 
    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/


#include "ScrollBoxDialog.h"

#include "ScrollBox.h"
#include <kdialog.h>
#include <qframe.h>
#include <qpainter.h>
#include <qpixmap.h>
#include <qpoint.h>
#include <qsize.h>
#include <qwidget.h>


namespace Rosegarden
{

ScrollBoxDialog::ScrollBoxDialog(QWidget *parent,
                                 ScrollBox::SizeMode sizeMode,
                                 const char *name,
                                 WFlags flags) :
        KDialog(parent, name, flags),
        m_scrollbox(new ScrollBox(this, sizeMode))
{ }


ScrollBoxDialog::~ScrollBoxDialog()
{ }


void ScrollBoxDialog::closeEvent(QCloseEvent *e)
{
    e->accept();
    emit closed();
}

void ScrollBoxDialog::setPageSize(const QSize& s)
{
    m_scrollbox->setPageSize(s);
    setFixedHeight(m_scrollbox->height());
    setFixedWidth(m_scrollbox->width());
}

}
#include "ScrollBoxDialog.moc"
