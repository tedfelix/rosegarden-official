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

#include <vector>

#include <kdialogbase.h>

class RosegardenGUIDoc;
class KConfig;

namespace Rosegarden
{
class ConfigurationPage;

class ConfigureDialogBase : public KDialogBase
{
    Q_OBJECT
public:
    ConfigureDialogBase(QWidget *parent=0,
                        const char *name=0);
    virtual ~ConfigureDialogBase();

    typedef std::vector<ConfigurationPage*> configurationpages;

protected slots:
    virtual void slotOk();
    virtual void slotApply();
    virtual void slotCancelOrClose();

    virtual void slotActivateApply();

protected:

    configurationpages m_configurationPages;
};

class ConfigureDialog : public ConfigureDialogBase
{
public:
    ConfigureDialog(RosegardenGUIDoc *doc,
                    KConfig* cfg,
                    QWidget *parent=0,
                    const char *name=0);
protected:
    RosegardenGUIDoc *m_doc;
};

class DocumentConfigureDialog : public ConfigureDialogBase
{
public:
    DocumentConfigureDialog(RosegardenGUIDoc *doc,
                            QWidget *parent=0,
                            const char *name=0);

protected:
    RosegardenGUIDoc *m_doc;
};


}

#endif // _ROSEGARDENCONFIGUREDIALOG_H_
