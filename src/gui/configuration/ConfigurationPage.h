/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2018 the Rosegarden development team.

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

#ifndef RG_CONFIGURATIONPAGE_H
#define RG_CONFIGURATIONPAGE_H

#include <QWidget>


namespace Rosegarden
{

class RosegardenDocument;


/**
 * This class borrowed from KMail
 * (c) 2000 The KMail Development Team
 */
class ConfigurationPage : public QWidget
{
    Q_OBJECT

public:
    ConfigurationPage(QWidget *parent) :
        QWidget(parent),
        m_doc(nullptr)
    {
    }

    ~ConfigurationPage() override {};

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

signals:

    // ConfigureDialog and others use this to enable the Apply button.
    void modified();

protected slots:

    virtual void slotModified();

protected:

    RosegardenDocument* m_doc;

};


}

#endif
