/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2017 the Rosegarden development team.

    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#ifndef RG_MUSICXMLOPTIONSDIALOG_H
#define RG_MUSICXMLOPTIONSDIALOG_H

#include <QDialog>
#include <QString>

#include "gui/configuration/HeadersConfigurationPage.h"

class QWidget;
class QCheckBox;
class QComboBox;

namespace Rosegarden
{

class RosegardenDocument;
class HeadersConfigurationPage;

class MusicXMLOptionsDialog : public QDialog
{
    Q_OBJECT

public:
    MusicXMLOptionsDialog(QWidget *parent,
                          RosegardenDocument *doc,
                          QString windowCaption = "",
                          QString heading = "");

public slots:
    void slotApply();
    void accept();
    void help();

protected:
    RosegardenDocument *m_doc;
    QComboBox *m_mxmlVersion;
    QComboBox *m_mxmlDTDType;
    QCheckBox *m_mxmlExportStaffGroup;
    QComboBox *m_mxmlExportSelection;
    QComboBox *m_mxmlMultiStave;
    QComboBox *m_mxmlExportPercussion;
    QCheckBox *m_mxmlUseOctaveShift;
    HeadersConfigurationPage *m_headersPage;

    void populateDefaultValues();
};



}

#endif
