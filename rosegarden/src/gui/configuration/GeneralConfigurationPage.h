
/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.

    This program is Copyright 2000-2007
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

#ifndef _RG_GENERALCONFIGURATIONPAGE_H_
#define _RG_GENERALCONFIGURATIONPAGE_H_

#include "TabbedConfigurationPage.h"
#include "gui/editors/eventlist/EventView.h"
#include <qstring.h>
#include <qcheckbox.h>
#include <qspinbox.h>
#include <qcombobox.h>
#include <qlineedit.h>
#include <klocale.h>
#include <kiconloader.h>

class QWidget;
class KConfig;


namespace Rosegarden
{

class RosegardenGUIDoc;


/**
 * General Rosegarden Configuration page
 *
 * (application-wide settings)
 */
class GeneralConfigurationPage : public TabbedConfigurationPage
{
    Q_OBJECT

public:
    enum DoubleClickClient
    {
        NotationView,
        MatrixView,
        EventView
    };

    enum NoteNameStyle
    { 
        American,
        Local
    };

    GeneralConfigurationPage(RosegardenGUIDoc *doc,
                             KConfig *cfg,
                             QWidget *parent=0, const char *name=0);

    virtual void apply();

    static QString iconLabel() { return i18n("General"); }
    static QString title()     { return i18n("General Configuration"); }
    static QString iconName()  { return "configure-general"; }

signals:
    void updateAutoSaveInterval(unsigned int);
    void updateSidebarStyle(unsigned int);

protected slots:
    void slotShowStatus();

protected:
    int getCountInSpin()            { return m_countIn->value(); }
    int getDblClickClient()         { return m_client->currentItem(); }
    int getNoteNameStyle() { return m_nameStyle->currentItem(); }

    //--------------- Data members ---------------------------------
    RosegardenGUIDoc* m_doc;

    QComboBox* m_client;
    QSpinBox*  m_countIn;
    QCheckBox* m_toolContextHelp;
    QCheckBox* m_backgroundTextures;
    QCheckBox* m_notationBackgroundTextures;
    QCheckBox* m_matrixBackgroundTextures;
    QComboBox *m_autoSave;
    QComboBox* m_nameStyle;
    QComboBox* m_sidebarStyle;
    QComboBox* m_globalStyle;
    QCheckBox *m_jackTransport;

};




}

#endif
