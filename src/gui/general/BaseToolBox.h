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

#ifndef RG_BASETOOLBOX_H
#define RG_BASETOOLBOX_H

#include <QHash>
#include <QObject>


class QWidget;
class QString;


namespace Rosegarden
{

class BaseTool;


/**
 * BaseToolBox : maintains a single instance of each registered tool.
 * Tools are fetched by name.
 */
class BaseToolBox : public QObject
{
    Q_OBJECT

public:
    BaseToolBox(QWidget* parent);

    virtual BaseTool* getTool(QString toolName);

signals:
    void showContextHelp(const QString &);

protected:
    virtual BaseTool* createTool(QString toolName) = 0;

    QHash<QString, BaseTool*> m_tools;
};

}

#endif
