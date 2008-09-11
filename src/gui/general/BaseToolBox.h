
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

#ifndef _RG_BASETOOLBOX_H_
#define _RG_BASETOOLBOX_H_

#include <QMap>
#include <QObject>


class QWidget;
class QString;


namespace Rosegarden
{

class BaseTool;


/**
 * BaseToolBox : maintains a single instance of each registered tool
 *
 * Tools are fetched from a name
 */
//class BaseToolBox : public QObject
class BaseToolBox : public QObject
{
    Q_OBJECT

public:
    BaseToolBox(QWidget* parent);

    virtual BaseTool* getTool(const QString& toolName);

signals:
    void showContextHelp(const QString &);

protected:
    virtual BaseTool* createTool(const QString& toolName) = 0;

	// was: qdict<BaseTool> m_tools;
	QMap<QString, BaseTool> m_tools;		// was qdict  //### hm... QMap ..check later
	
};


}

#endif
