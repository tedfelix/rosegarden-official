
/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2021 the Rosegarden development team.

    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#ifndef RG_MARKERMODIFYDIALOG_H
#define RG_MARKERMODIFYDIALOG_H

#include "base/Marker.h"
#include "gui/widgets/TimeWidget.h"
#include "gui/widgets/LineEdit.h"

#include <QDialog>
#include <QString>


namespace Rosegarden
{

class TimeWidget;
class RosegardenDocument;
class Composition;


class MarkerModifyDialog : public QDialog
{
    Q_OBJECT
public:
    MarkerModifyDialog(QWidget *parent,
                       Composition *composition,
                       int time,
                       const QString &name,
                       const QString &des);

    MarkerModifyDialog(QWidget *parent,
                       Composition *composition,
                       Marker *marker);

    QString getName() const { return m_nameEdit->text(); }
    QString getDescription() const { return m_desEdit->text(); }
    int getTime() const { return m_timeEdit->getTime(); }
    int getOriginalTime() const { return m_originalTime; }

protected:
    void initialise(Composition *composition,
                    int time,
                    const QString &name,
                    const QString &des);

    RosegardenDocument *m_doc;

    TimeWidget         *m_timeEdit;
    LineEdit           *m_nameEdit;
    LineEdit           *m_desEdit;

    int                m_originalTime;
};




}

#endif
