/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */
/*
    Rosegarden
    A sequencer and musical notation editor.
    Copyright 2000-2009 the Rosegarden development team.
    See the AUTHORS file for more details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#include "SqueezedLabel.h"

namespace Rosegarden
{

/** ksqueezedtextlabel.h replacement: */
SqueezedLabel::SqueezedLabel(QString label, QWidget* parent) :
    QLabel(label, parent)
{
	//### maybe fix: not really the same as KSqueezedTextLabel,
	//    but should work for now.

    //!!! no, this does something totally different -- implement it,
    //!!! borrowing SV code for this?

	//this->setScaledContents( true );
}

SqueezedLabel::SqueezedLabel(QWidget* parent) :
    QLabel(parent)
{
	//### maybe fix: not really the same as KSqueezedTextLabel,
	//    but should work for now.

    //!!! no, this does something totally different -- implement it,
    //!!! borrowing SV code for this?

	//this->setScaledContents( true );
}
//SqueezedLabel::~SqueezedLabel(){
	//
//}

}

#include "SqueezedLabel.moc"
