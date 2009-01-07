/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2008 the Rosegarden development team.

    This file originally from Sonic Visualiser, copyright 2007 Queen
    Mary, University of London.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#include "ResourceFinder.h"

#include <QDir>
#include <QStringList>

namespace Rosegarden
{

//!!!
//### implement all this!


QString ResourceFinder::getResourcePath(QString a, QString res ){
	/**
		res is something like: "chords/user_chords.xml"
	**/
	QString ret;
	ret = getResourceDir("");
	return ret + res;
}



QString ResourceFinder::getResourceDir(QString subDir)
{
	/** with subDir empty, returns the root directory for resources for this application.
		with subDir set, returns the resource directory (root dir + subDir)
		subDir should not have a preceeding slash, nor a following slash: "chords"
	**/
	
	//### FIX !!!! return a valid directory !!
	QString ret;
	QStringList sl;
	if( subDir.isEmpty() ){
		ret = "rg_resources/";
	}else{
		sl << "rg_resources/";
		sl << subDir << "/";
		ret = sl.join("");
	}
	return ret;
}

QString ResourceFinder::getResourceSaveDir(QString subDir)
{
	//### implement
	return QString();
}

//QFileInfoList ResourceFinder::getResourceFiles(QString &resourceDir, QString &fileType)
QStringList ResourceFinder::getResourceFiles(QString resourceCat, QString fileExt)
{
	/**
		return list of files in resource-dir resourceCat with extension fileExt
		
		resourceCat is a the resource categorie name (sub-directory name) 
		without preceeding or following slashes: "chords"
		fileExt is the extension-name without dot: "xml"
	**/
	QString resourceDir = getResourceDir( resourceCat );
	
	//QFileInfoList fl;
	QStringList sl;
	
	QStringList filters;
	filters << fileExt;
	
	QDir dirx;
	dirx.setPath(resourceDir); 
	
	if( ! dirx.exists() )
		return sl;
	
//	QDir::entryInfoList ( const QStringList & nameFilters, Filters filters = NoFilter, SortFlags sort = NoSort )
//	fl = dirx.entryInfoList( filters, QDir::Files | QDir::Readable | QDir::CaseSensitive, QDir::Name );
	sl = dirx.entryList( filters, QDir::Files | QDir::Readable | QDir::CaseSensitive, QDir::Name );
	return sl;
	
}

}

