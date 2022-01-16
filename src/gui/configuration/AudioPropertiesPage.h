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

#ifndef RG_AUDIOPROPERTIESPAGE_H
#define RG_AUDIOPROPERTIESPAGE_H

#include "TabbedConfigurationPage.h"

#include <QString>

class QWidget;
class QPushButton;
class QLabel;


namespace Rosegarden
{


class RosegardenDocument;


/// Audio Properties page.  Document-wide settings.
class AudioPropertiesPage : public TabbedConfigurationPage
{
    Q_OBJECT
public:
    AudioPropertiesPage(QWidget *parent);

    void apply() override;

    static QString iconLabel() { return tr("Audio"); }
    static QString title()     { return tr("Audio Settings"); }
    static QString iconName()  { return "configure-audio"; }

private slots:

    /// Called by the Change Path button.
    void slotChoosePath();

private:

    QLabel *m_path;
    QLabel *m_diskSpace;
    QLabel *m_minutesAtStereo;

    QPushButton *m_changePathButton;

    /// Display remaining disk space and recording time at current path.
    void updateWidgets();

};


}

#endif
