
/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2008 the Rosegarden development team.

    Parts of the configuration classes are taken from KMail.
    Copyright (C) 2000 The KMail Development Team.

    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#ifndef _RG_CONFIGURATIONPAGE_H_
#define _RG_CONFIGURATIONPAGE_H_

#include <QWidget>


class KConfig;


namespace Rosegarden
{

class RosegardenGUIDoc;


/**
 * This class borrowed from KMail
 * (c) 2000 The KMail Development Team
 */
class ConfigurationPage : public QWidget
{
    Q_OBJECT

public:
    ConfigurationPage(RosegardenGUIDoc *doc,
                      QWidget *parent=0, const char *name=0)
        : QWidget(parent, name), m_doc(doc), m_cfg(0), m_pageIndex(0) {}

    ConfigurationPage(QSettings cfg,
                      QWidget *parent=0, const char *name=0)
        : QWidget(parent, name), m_doc(0), m_cfg(cfg), m_pageIndex(0) {}

    ConfigurationPage(RosegardenGUIDoc *doc, QSettings cfg,
                      QWidget *parent=0, const char *name=0)
        : QWidget(parent, name), m_doc(doc), m_cfg(cfg), m_pageIndex(0) {}

    virtual ~ConfigurationPage() {};

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
    QSettings m_cfg;

    int m_pageIndex;
};


}

#endif
