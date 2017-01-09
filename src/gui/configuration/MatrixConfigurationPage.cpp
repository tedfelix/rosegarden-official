/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2017 the Rosegarden development team.
 
    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.
 
    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/


#include "MatrixConfigurationPage.h"

#include "misc/ConfigGroups.h"
#include "ConfigurationPage.h"
#include "document/RosegardenDocument.h"
#include "misc/ConfigGroups.h"
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

MatrixConfigurationPage::MatrixConfigurationPage(QWidget *parent) :
        TabbedConfigurationPage(parent)
{
    QFrame *frame = new QFrame(m_tabWidget);
    frame->setContentsMargins(10, 10, 10, 10);
    QGridLayout *layout = new QGridLayout(frame);
    layout->setSpacing(5);

    layout->addWidget(new QLabel("Nothing here yet", frame), 0, 0);

    addTab(frame, tr("General"));
}

void MatrixConfigurationPage::apply()
{
    //@@@ Next two lines not need.  Commented out.
    //@@@ QSettings settings;
    //@@@ settings.beginGroup( MatrixViewConfigGroup );
}

}
