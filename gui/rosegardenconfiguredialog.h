// -*- c-basic-offset: 4 -*-

/*
    Rosegarden-4 v0.1
    A sequencer and musical notation editor.

    This program is Copyright 2000-2002
        Guillaume Laurent   <glaurent@telegraph-road.org>,
        Chris Cannam        <cannam@all-day-breakfast.com>,
        Richard Bown        <bownie@bownie.com>

    The moral right of the authors to claim authorship of this work
    has been asserted.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#ifndef _ROSEGARDENCONFIGUREDIALOG_H_
#define _ROSEGARDENCONFIGUREDIALOG_H_

//#include "rosegardenconfigure.h"

#include <kdialogbase.h>

class RosegardenGUIDoc;

namespace Rosegarden
{

/**
 * This class borrowed from KMail
 * (c) 2000 The KMail Development Team
 */
class ConfigurationPage : public QWidget
{
    Q_OBJECT

public:
    ConfigurationPage(RosegardenGUIDoc *doc,
                      QWidget *parent=0, const char *name=0 )
        : QWidget(parent, name), m_doc(doc), m_pageIndex(0) {}
    ~ConfigurationPage() {};

    /**
     * Should set the page up (ie. read the setting from the @ref
     * KConfig object into the widgets) after creating it in the
     * constructor. Called from @ref ConfigureDialog.
    */
//     virtual void setup() = 0;

    /**
     * Should apply the changed settings (ie. read the settings from
     * the widgets into the @ref KConfig object). Called from @ref
     * ConfigureDialog.
     */
    virtual void apply() = 0;

    /**
     * Should cleanup any temporaries after cancel. The default
     * implementation does nothing. Called from @ref
     * ConfigureDialog.
     */
    virtual void dismiss() {}

    void setPageIndex( int aPageIndex ) { m_pageIndex = aPageIndex; }
    int pageIndex() const { return m_pageIndex; }

protected:

    //--------------- Data members ---------------------------------

    RosegardenGUIDoc* m_doc;

    int m_pageIndex;
};

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

    static QString iconName() { return "misc"; }
    
protected:
    void addTab(QWidget *tab, const QString &title);

    //--------------- Data members ---------------------------------

    QTabWidget *m_tabWidget;

};


class GeneralConfigurationPage;
class PlaybackConfigurationPage;

class RosegardenConfigureDialog : public KDialogBase
{
Q_OBJECT
public:
    RosegardenConfigureDialog(RosegardenGUIDoc *doc,
                              QWidget *parent=0,
                              const char *name=0);
    ~RosegardenConfigureDialog();

protected slots:
    virtual void slotOk();
    virtual void slotApply();
    virtual void slotCancelOrClose();

    void slotActivateApply();

private:

    RosegardenGUIDoc *m_doc;
    GeneralConfigurationPage*  m_generalConfigurationPage;
    PlaybackConfigurationPage* m_playbackConfigurationPage;
};

}
 


#endif // _ROSEGARDENCONFIGUREDIALOG_H_
