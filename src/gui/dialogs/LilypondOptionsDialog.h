
/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.

    This program is Copyright 2000-2008
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

#ifndef _RG_LILYPONDOPTIONSDIALOG_H_
#define _RG_LILYPONDOPTIONSDIALOG_H_

#include <kdialogbase.h>
#include <qstring.h>

#include "gui/configuration/HeadersConfigurationPage.h"

class QWidget;
class QCheckBox;
class QComboBox;
class QLineEdit;
class QLineEdit;

namespace Rosegarden
{

class RosegardenGUIDoc;
class HeadersConfigurationPage;

class LilypondOptionsDialog : public KDialogBase
{
    Q_OBJECT

public:
    LilypondOptionsDialog(QWidget *parent,
			  RosegardenGUIDoc *doc,
                          QString windowCaption = "",
                          QString heading = "");

    static void setDefaultLilypondVersion(QString version);

public slots:
    void slotApply();
    void slotOk();

protected:
    RosegardenGUIDoc *m_doc;
    QComboBox *m_lilyLanguage;
    QComboBox *m_lilyPaperSize;
    QComboBox *m_lilyFontSize;
    QComboBox *m_lilyTempoMarks;
    QComboBox *m_lilyExportSelection;
    QComboBox *m_lilyLyricsHAlignment;
    QCheckBox *m_lilyPaperLandscape;
    QCheckBox *m_lilyRaggedBottom;
    QCheckBox *m_lilyExportLyrics;
    QCheckBox *m_lilyExportMidi;
    QCheckBox *m_lilyExportPointAndClick;
    QCheckBox *m_lilyExportBeams;
    QCheckBox *m_lilyExportStaffGroup;
    QCheckBox *m_lilyExportStaffMerge;
    QComboBox *m_lilyMarkerMode;
    HeadersConfigurationPage *m_headersPage;

};



}

#endif
