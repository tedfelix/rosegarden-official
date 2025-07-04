/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2025 the Rosegarden development team.

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
    explicit GeneralConfigurationPage(QWidget *parent);

    enum DoubleClickClient
    {
        NotationView,
        MatrixView,
        EventListEditor
    };

    enum NoteNameStyle
    {
        American,
        Local
    };

    // PDF Viewers
    struct PDFViewerInfo {
        QString name;
        QString command;
    };
    typedef std::vector<PDFViewerInfo> PDFViewers;
    static const PDFViewers pdfViewers;
    static int getDefaultPDFViewer();

    // File Printers
    struct FilePrinterInfo {
        QString name;
        QString command;
    };
    typedef std::vector<FilePrinterInfo> FilePrinters;
    static const FilePrinters filePrinters;
    static int getDefaultFilePrinter();

    enum MetronomeDuring
    {
        DuringCountIn,
        DuringRecord,
        DuringBoth
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
    QComboBox *m_enableMetronomeDuring;
    QComboBox *m_autoSaveInterval;
    QCheckBox *m_appendSuffixes;
    QCheckBox *m_useTrackName;
    QCheckBox *m_enableEditingDuringPlayback;
    QCheckBox *m_cleanRecentFilesList;
    QCheckBox *m_useJackTransport;
    QCheckBox *m_jackStopAtAutoStop;
    QCheckBox *m_stopPlaybackAtEnd;
    QCheckBox *m_jumpToLoop;
    QCheckBox *m_advancedLooping;
    QCheckBox *m_autoChannels;
    QCheckBox *m_lv2;
    QCheckBox *m_dynamicDrag;

    // Presentation tab
    QComboBox *m_theme;
    QComboBox *m_nameStyle;
    QCheckBox *m_backgroundTextures;
    QCheckBox *m_notationBackgroundTextures;
    QCheckBox *m_longTitles;
    QComboBox *m_trackSize;
    QComboBox *m_trackLabelWidth;
    QCheckBox *m_useNativeFileDialogs;

    // External Applications tab
    QComboBox *m_pdfViewer;
    QComboBox *m_filePrinter;

};


}

#endif
