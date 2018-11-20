
/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2018 the Rosegarden development team.

    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#ifndef RG_USEORNAMENTDIALOG_H
#define RG_USEORNAMENTDIALOG_H

#include "base/TriggerSegment.h"
#include "base/NotationTypes.h"
#include "gui/widgets/LineEdit.h"

#include <QDialog>

#include <string>
#include <vector>


class QWidget;
class LineEdit;
class QLabel;
class QCheckBox;
class QComboBox;


namespace Rosegarden
{

class Composition;


class UseOrnamentDialog : public QDialog
{
    Q_OBJECT

public:
    UseOrnamentDialog(QWidget *parent, Composition *);

    TriggerSegmentId getId() const;
    Mark getMark() const;
    bool getRetune() const;
    std::string getTimeAdjust() const;

public slots:
    void accept() override;
    void slotMarkChanged(int);

protected:
    void setupFromConfig();

    std::vector<Mark> m_marks;

    Composition   *m_composition;
    QComboBox     *m_ornament;
    QComboBox     *m_mark;
    QLabel        *m_textLabel;
    LineEdit      *m_text;
    QCheckBox     *m_retune;
    QComboBox     *m_adjustTime;
};


}

#endif
