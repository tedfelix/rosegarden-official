
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

#ifndef _RG_MARKERMODIFYDIALOG_H_
#define _RG_MARKERMODIFYDIALOG_H_

#include <kdialogbase.h>
#include <qstring.h>
#include <qlineedit.h>

#include "gui/widgets/TimeWidget.h"


namespace Rosegarden
{

class TimeWidget;
class RosegardenGUIDoc;
class Composition;


class MarkerModifyDialog : public KDialogBase
{
    Q_OBJECT
public:
    MarkerModifyDialog(QWidget *parent,
                       Composition *composition,
                       int time,
                       const QString &name,
                       const QString &des);

    QString getName() const { return m_nameEdit->text(); }
    QString getDescription() const { return m_desEdit->text(); }
    int getTime() const { return m_timeEdit->getTime(); }
    int getOriginalTime() const { return m_originalTime; }

protected:
    RosegardenGUIDoc             *m_doc;

    TimeWidget         *m_timeEdit;
    QLineEdit                    *m_nameEdit;
    QLineEdit                    *m_desEdit;

    int                           m_originalTime;
};




}

#endif
