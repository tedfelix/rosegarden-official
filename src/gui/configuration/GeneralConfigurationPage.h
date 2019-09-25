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

#ifndef RG_GENERALCONFIGURATIONPAGE_H
#define RG_GENERALCONFIGURATIONPAGE_H

#include "TabbedConfigurationPage.h"

#include <QString>

class QCheckBox;
class QComboBox;
class QSpinBox;
class QWidget;


namespace Rosegarden
{


/**
 * General Rosegarden Configuration page
 *
 * (application-wide settings)
 */
class GeneralConfigurationPage : public TabbedConfigurationPage
{
    Q_OBJECT

public:
    GeneralConfigurationPage(QWidget *parent);

    enum DoubleClickClient
    {
        NotationView,
        MatrixView,
        EventView
    };

    enum NoteNameStyle
    { 
        American,
        Local
    };

    enum PdfViewer
    {
        Okular,
        Evince,
        Acroread,
        MuPDF,
        ePDFView,
        xdgOpen
    };

    enum FilePrinter
    {
        KPrinter,
        GtkLP,
        Lpr,
        Lp,
        HPLIP
    };

    void apply() override;

    // For ConfigureDialog
    static QString iconLabel() { return tr("General"); }
    static QString title()     { return tr("General Configuration"); }
    static QString iconName()  { return "configure-general"; }

signals:
    void updateAutoSaveInterval(unsigned int);

private slots:
    void slotShowStatus();

private:
    // Behavior tab
    QComboBox *m_openSegmentsIn;
    QSpinBox *m_countIn;
    QComboBox *m_autoSaveInterval;
    QCheckBox *m_appendSuffixes;
    QCheckBox *m_useTrackName;
    QCheckBox *m_enableUndoDuringPlayback;
    QCheckBox *m_enableSegmentSplitting;
    QCheckBox *m_useJackTransport;

    // Presentation tab
    QCheckBox *m_Thorn;
    QComboBox *m_nameStyle;
    QCheckBox *m_backgroundTextures;
    QCheckBox *m_notationBackgroundTextures;
    QCheckBox *m_longTitles;

    // External Applications tab
    QComboBox *m_pdfViewer;
    QComboBox *m_filePrinter;

};


}

#endif
