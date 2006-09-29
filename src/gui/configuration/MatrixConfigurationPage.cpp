/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
 
    This program is Copyright 2000-2006
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


#include "MatrixConfigurationPage.h"

#include "document/ConfigGroups.h"
#include "ConfigurationPage.h"
#include "document/RosegardenGUIDoc.h"
#include "gui/editors/matrix/MatrixView.h"
#include "TabbedConfigurationPage.h"
#include <kconfig.h>
#include <qframe.h>
#include <qlabel.h>
#include <qstring.h>
#include <qtabwidget.h>
#include <qwidget.h>
#include <qlayout.h>

namespace Rosegarden
{

MatrixConfigurationPage::MatrixConfigurationPage(KConfig *cfg,
        QWidget *parent,
        const char *name) :
        TabbedConfigurationPage(cfg, parent, name)
{
    m_cfg->setGroup(MatrixView::ConfigGroup);

    QFrame *frame = new QFrame(m_tabWidget);
    QGridLayout *layout = new QGridLayout(frame,
                                          4, 2,  // nbrow, nbcol
                                          10, 5);

    layout->addWidget(new QLabel("Nothing here yet", frame), 0, 0);

    addTab(frame, i18n("General"));
}

void MatrixConfigurationPage::apply()
{
    m_cfg->setGroup(MatrixView::ConfigGroup);
}

}
#include "MatrixConfigurationPage.moc"
