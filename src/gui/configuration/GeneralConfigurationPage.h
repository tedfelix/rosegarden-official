/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2015 the Rosegarden development team.

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
#include "gui/editors/eventlist/EventView.h"

#include <QString>
#include <QCheckBox>
#include <QSpinBox>
#include <QComboBox>


class QWidget;


namespace Rosegarden
{

class RosegardenDocument;


/**
 * General Rosegarden Configuration page
 *
 * (application-wide settings)
 */
class GeneralConfigurationPage : public TabbedConfigurationPage
{
    Q_OBJECT

public:
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

    enum GraphicsSystem
    {
        Raster,
        Native,
        OpenGL
    };

    GeneralConfigurationPage(RosegardenDocument *doc, QWidget *parent = 0);

    virtual void apply();

    static QString iconLabel() { return tr("General"); }
    static QString title()     { return tr("General Configuration"); }
    static QString iconName()  { return "configure-general"; }

signals:
    void updateAutoSaveInterval(unsigned int);

protected slots:
    void slotShowStatus();

protected:
    int getCountInSpin()            { return m_countIn->value(); }
    int getDblClickClient()         { return m_client->currentIndex(); }
    int getNoteNameStyle()          { return m_nameStyle->currentIndex(); }
    int getAppendLabel()            { return m_appendLabel->isChecked(); }
    int getPdfViewer()              { return m_pdfViewer->currentIndex(); }
    int getFilePrinter()            { return m_filePrinter->currentIndex(); }
    int getGraphicsSystem()         { return m_graphicsSystem->currentIndex(); }
    
    //--------------- Data members ---------------------------------
    RosegardenDocument* m_doc;

    QComboBox* m_client;
    QSpinBox  *m_countIn;
    QCheckBox *m_toolContextHelp;
    QCheckBox *m_backgroundTextures;
    QCheckBox *m_notationBackgroundTextures;
    QCheckBox *m_matrixBackgroundTextures;
    QComboBox *m_autoSave;
    QComboBox *m_nameStyle;
    QComboBox *m_globalStyle;
    QCheckBox *m_appendLabel;
    QCheckBox *m_jackTransport;
    QCheckBox *m_Thorn;
    QCheckBox *m_longTitles;

    QComboBox *m_pdfViewer;
    QComboBox *m_filePrinter;
    QComboBox *m_graphicsSystem;

    unsigned int m_lastGraphicsSystemIndex;
};

}

#endif
