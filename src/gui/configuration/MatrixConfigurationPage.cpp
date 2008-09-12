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


#include "MatrixConfigurationPage.h"

#include "document/ConfigGroups.h"
#include "ConfigurationPage.h"
#include "document/RosegardenGUIDoc.h"
#include "document/ConfigGroups.h"
#include "gui/editors/matrix/MatrixView.h"
#include "TabbedConfigurationPage.h"
#include <QSettings>
#include <QFrame>
#include <QLabel>
#include <QString>
#include <QTabWidget>
#include <QWidget>
#include <QLayout>

namespace Rosegarden
{

MatrixConfigurationPage::MatrixConfigurationPage(QSettings cfg,
        QWidget *parent,
        const char *name) :
        TabbedConfigurationPage(cfg, parent, name)
//### JAS update function declaration / definition
{
    QFrame *frame = new QFrame(m_tabWidget);
    QGridLayout *layout = new QGridLayout(frame,
                                          4, 2,  // nbrow, nbcol
                                          10, 5);

    layout->addWidget(new QLabel("Nothing here yet", frame), 0, 0);

    addTab(frame, i18n("General"));
}

void MatrixConfigurationPage::apply()
{
    //@@@ Next two lines not need.  Commented out.
    //@@@ QSettings settings;
    //@@@ settings.beginGroup( MatrixViewConfigGroup );
}

}
#include "MatrixConfigurationPage.moc"
