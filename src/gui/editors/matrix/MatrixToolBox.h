/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2018 the Rosegarden development team.

    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#ifndef RG_MATRIXTOOLBOX_H
#define RG_MATRIXTOOLBOX_H

#include "gui/general/BaseToolBox.h"

class QString;


namespace Rosegarden
{

class MatrixWidget;
class MatrixScene;


/**
 * MatrixToolBox : maintains a single instance of each registered tool
 *
 * Tools are fetched by name
 */
class MatrixToolBox : public BaseToolBox
{
    Q_OBJECT

public:
    MatrixToolBox(MatrixWidget *parent);
    ~MatrixToolBox() override;

    void setScene(MatrixScene *scene);

protected:
    BaseTool *createTool(QString toolName) override;
    MatrixWidget *m_widget;
    MatrixScene *m_scene;
};

}

#endif
