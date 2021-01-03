/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2021 the Rosegarden development team.
 
    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.
 
    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/


#include <QCloseEvent>
#include "ScrollBoxDialog.h"

#include "ScrollBox.h"
#include <QFrame>
#include <QPainter>
#include <QPixmap>
#include <QPoint>
#include <QSize>
#include <QWidget>


namespace Rosegarden
{

ScrollBoxDialog::ScrollBoxDialog(QWidget *parent,
                                 ScrollBox::SizeMode sizeMode,
                                 const char *name
//                                  WFlags flags
								) :
        QDialog(parent),	//, name, flags),
        m_scrollbox(new ScrollBox(this, sizeMode))
{
	setObjectName( name ); 
}


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
