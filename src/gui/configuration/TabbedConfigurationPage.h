
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

#ifndef _RG_TABBEDCONFIGURATIONPAGE_H_
#define _RG_TABBEDCONFIGURATIONPAGE_H_

#include "ConfigurationPage.h"
#include <QString>


class QWidget;
class QTabWidget;
class KConfig;


namespace Rosegarden
{

class RosegardenGUIDoc;


/**
 * This class borrowed from KMail
 * (c) 2000 The KMail Development Team
 */
class TabbedConfigurationPage : public ConfigurationPage
{
    Q_OBJECT

public:
    TabbedConfigurationPage(RosegardenGUIDoc *doc,
                            QWidget *parent=0, const char *name=0);

    TabbedConfigurationPage(KConfig *cfg,
                            QWidget *parent=0, const char *name=0);

    TabbedConfigurationPage(RosegardenGUIDoc *doc,
                            KConfig *cfg,
                            QWidget *parent=0, const char *name=0);

    static QString iconName() { return "misc"; }
    
protected:
    void init();
    void addTab(QWidget *tab, const QString &title);

    //--------------- Data members ---------------------------------

    QTabWidget *m_tabWidget;

};


}

#endif
