
/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2008 the Rosegarden development team.

    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#ifndef _RG_TRACKHEADER_H_
#define _RG_TRACKHEADER_H_

#include <Q3Header>

#include <QHeaderView>	// replaces Q3Header
#include <QPaintEvent>

class QWidget;
// class QPaintEvent;


namespace Rosegarden
{



class TrackHeader : public Q3Header		//QHeaderView
{

public:
    TrackHeader(int number,
                          QWidget *parent=0,
                          const char *name=0
			   ):
				Q3Header(number, parent, name) {;}
	
	/*
	QHeader(Qt::Horizontal, parent) 
	{
		this->setObjectName(name);
		this->setNumber();
	}
	*/
	
	~TrackHeader();

protected:
    virtual void paintEvent(QPaintEvent *pe);
//     void paintSection(QPainter * p, int index, QRect fr);
//     void paintSectionLabel (QPainter * p, int index, const QRect & fr);
//     QRect sRect (int index);
    
private:

};



}

#endif
