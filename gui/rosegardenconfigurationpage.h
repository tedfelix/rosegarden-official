// -*- c-basic-offset: 4 -*-

/*
    Rosegarden-4
    A sequencer and musical notation editor.

    This program is Copyright 2000-2003
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

#ifndef _ROSEGARDENCONFIGUREPAGE_H_
#define _ROSEGARDENCONFIGUREPAGE_H_

#include <qspinbox.h>
#include <qcombobox.h>
#include <qslider.h>
#include <qlineedit.h>

#include <klocale.h>

#include <string>

#include "Device.h"
#include "config.h"

class RosegardenGUIDoc;
class QTabWidget;
class QLineEdit;
class QPushButton;
class QRadioButton;
class QLabel;
class QCheckBox;
class KListView;
class RosegardenQuantizeParameters;

namespace Rosegarden
{

/**
 * This class borrowed from KMail
 * (c) 2000 The KMail Development Team
 */
class ConfigurationPage : public QWidget
{
    Q_OBJECT

public:
    ConfigurationPage(RosegardenGUIDoc *doc,
                      QWidget *parent=0, const char *name=0)
        : QWidget(parent, name), m_doc(doc), m_cfg(0), m_pageIndex(0) {}

    ConfigurationPage(KConfig *cfg,
                      QWidget *parent=0, const char *name=0)
        : QWidget(parent, name), m_doc(0), m_cfg(cfg), m_pageIndex(0) {}

    ConfigurationPage(RosegardenGUIDoc *doc, KConfig *cfg,
                      QWidget *parent=0, const char *name=0)
        : QWidget(parent, name), m_doc(doc), m_cfg(cfg), m_pageIndex(0) {}

    virtual ~ConfigurationPage() {};

    /**
     * Should set the page up (ie. read the setting from the @ref
     * KConfig object into the widgets) after creating it in the
     * constructor. Called from @ref ConfigureDialog.
    */
//     virtual void setup() = 0;

    /**
     * Should apply the changed settings (ie. read the settings from
     * the widgets into the @ref KConfig object). Called from @ref
     * ConfigureDialog.
     */
    virtual void apply() = 0;

    /**
     * Should cleanup any temporaries after cancel. The default
     * implementation does nothing. Called from @ref
     * ConfigureDialog.
     */
    virtual void dismiss() {}

    void setPageIndex( int aPageIndex ) { m_pageIndex = aPageIndex; }
    int pageIndex() const { return m_pageIndex; }

protected:

    //--------------- Data members ---------------------------------

    RosegardenGUIDoc* m_doc;
    KConfig* m_cfg;

    int m_pageIndex;
};

/**
 * This class borrowed from KMail
 * (c) 2000 The KMail Development Team
 */
class TabbedConfigurationPage : public ConfigurationPage
{
    Q_OBJECT

public:
    TabbedConfigurationPage(RosegardenGUIDoc *doc,
                            QWidget *parent=0, const char *name=0);

    TabbedConfigurationPage(KConfig *cfg,
                            QWidget *parent=0, const char *name=0);

    TabbedConfigurationPage(RosegardenGUIDoc *doc,
                            KConfig *cfg,
                            QWidget *parent=0, const char *name=0);

    static QString iconName() { return "misc"; }
    
protected:
    void init();
    void addTab(QWidget *tab, const QString &title);

    //--------------- Data members ---------------------------------

    QTabWidget *m_tabWidget;

};

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

    GeneralConfigurationPage(KConfig *cfg,
                             QWidget *parent=0, const char *name=0);

    virtual void apply();

    static QString iconLabel() { return i18n("General"); }
    static QString title()     { return i18n("General Configuration"); }

    int getCountInSpin()            { return m_countIn->value(); }
    int getDblClickClient()         { return m_client->currentItem(); }
    QString getExternalAudioEditor() { return m_externalAudioEditorPath->text(); }
    int getNoteNameStyle() { return m_nameStyle->currentItem(); }

signals:
    void updateAutoSaveInterval(unsigned int);

protected slots:
    void slotFileDialog();


protected:

    //--------------- Data members ---------------------------------

    QComboBox* m_client;
    QSpinBox*  m_countIn;
    QSpinBox*  m_midiPitchOctave;
    QLineEdit* m_externalAudioEditorPath;
    QCheckBox* m_backgroundTextures;
    QSpinBox*  m_autosaveInterval;
    QComboBox* m_nameStyle;
};

/**
 * Notation Configuration page
 */
class NotationConfigurationPage : public TabbedConfigurationPage
{
    Q_OBJECT;

public:
    NotationConfigurationPage(KConfig *cfg,
                              QWidget *parent = 0, const char *name=0);

    virtual void apply();

    static QString iconLabel() { return i18n("Notation"); }
    static QString title()     { return i18n("Notation"); }

protected slots:
    void slotFontComboChanged(const QString &);

protected:

    //--------------- Data members ---------------------------------

    QComboBox *m_font;
    QComboBox *m_singleStaffSize;
    QComboBox *m_multiStaffSize;
    QLabel *m_fontOriginLabel;
    QLabel *m_fontCopyrightLabel;
    QLabel *m_fontMappedByLabel;
    QLabel *m_fontTypeLabel;
    QComboBox *m_layoutMode;
    QComboBox *m_spacing;
    QCheckBox *m_colourQuantize;
    QCheckBox *m_showUnknowns;
    QComboBox *m_noteStyle;
    QComboBox *m_insertType;
    QCheckBox *m_autoBeam;
    QCheckBox *m_collapseRests;
    QComboBox *m_pasteType;
    RosegardenQuantizeParameters *m_quantizeFrame;

    // Lilypond export:
    QComboBox *m_lilyPaperSize;
    QComboBox *m_lilyFontSize;
    QCheckBox *m_lilyExportHeaders;
    QCheckBox *m_lilyExportLyrics;
    QCheckBox *m_lilyExportMidi;
    QCheckBox *m_lilyExportUnmuted;
    QCheckBox *m_lilyExportPointAndClick;
    QCheckBox *m_lilyExportBarChecks;

    void populateSizeCombo(QComboBox *combo, std::string font, int dfltSize);
};

/**
 * Notation Configuration page
 */
class MatrixConfigurationPage : public TabbedConfigurationPage
{
    Q_OBJECT;

public:
    MatrixConfigurationPage(KConfig *cfg,
                            QWidget *parent = 0, const char *name=0);

    virtual void apply();

    static QString iconLabel() { return i18n("Matrix"); }
    static QString title()     { return i18n("Matrix"); }

protected slots:

protected:

    //--------------- Data members ---------------------------------
};

/**
 * Latency Configuration page
 *
 * (application-wide settings)
 */
class LatencyConfigurationPage : public TabbedConfigurationPage
{
    Q_OBJECT

public:
    LatencyConfigurationPage(RosegardenGUIDoc *doc,
                             KConfig *cfg,
                             QWidget *parent=0, const char *name=0);

    virtual void apply();

    static QString iconLabel() { return i18n("Latency"); }
    static QString title()     { return i18n("Sequencer Latency"); }

    int getReadAheadValue() { return m_readAhead->value(); }
    int getPlaybackValue()  { return m_playback->value(); }

    int getJACKPlaybackValue() { return m_jackPlayback->value(); }
    int getJACKRecordValue() { return m_jackRecord->value(); }

protected slots:
    // Get the latest latency values from the sequencer
    //
    void slotFetchLatencyValues();


    // To ensure that read ahead is never less than playback
    //
    void slotReadAheadChanged(int);
    void slotPlaybackChanged(int);

protected:

    //--------------- Data members ---------------------------------

    QSlider* m_readAhead;
    QSlider* m_playback;
    QLabel*  m_readAheadLabel;
    QLabel*  m_playbackLabel;

    QSlider* m_jackPlayback;
    QSlider* m_jackRecord;

    QPushButton* m_fetchLatencyValues;
};


/**
 * Document Meta-information page
 *
 * (document-wide settings)
 */
class DocumentMetaConfigurationPage : public TabbedConfigurationPage
{
    Q_OBJECT
public:
    DocumentMetaConfigurationPage(RosegardenGUIDoc *doc,
                                  QWidget *parent = 0, const char *name = 0);
    virtual void apply();

    static QString iconLabel() { return i18n("About"); }
    static QString title() { return i18n("About"); }

protected slots:
    void slotAddNewProperty();
    void slotDeleteProperty();
    
protected:

    //--------------- Data members ---------------------------------

    KListView *m_metadata;
};


/**
 * Audio Configuration page
 *
 * (document-wide settings)
 */
class AudioConfigurationPage : public TabbedConfigurationPage
{
    Q_OBJECT
public:
    AudioConfigurationPage(RosegardenGUIDoc *doc,
                           QWidget *parent=0, const char *name=0);
    virtual void apply();

    static QString iconLabel() { return i18n("Audio"); }
    static QString title()     { return i18n("Audio Settings"); }

protected slots:
    void slotFileDialog();

    // Work out and display remaining disk space and time left 
    // at current path.
    //
    void calculateStats();

protected:

    //--------------- Data members ---------------------------------

    QLabel           *m_path;
    QLabel           *m_diskSpace;
    QLabel           *m_minutesAtStereo;

    QPushButton      *m_changePathButton;
};


// -----------  SequencerConfigurationage -----------------
//

class SequencerConfigurationPage : public TabbedConfigurationPage
{
    Q_OBJECT
public:
    SequencerConfigurationPage(RosegardenGUIDoc *doc,
                               KConfig *cfg,
                               QWidget *parent=0,
                               const char *name=0);

    virtual void apply();

    static QString iconLabel() { return i18n("Sequencer"); }
    static QString title()     { return i18n("Sequencer Settings"); }

protected slots:

    void slotShowStatus();

protected:

    // General
    QLineEdit *m_sequencerArguments;
    QCheckBox *m_sendControllersAtPlay;

    // Latency
    QCheckBox *m_midiClockEnabled;
    QComboBox *m_jackTransport;
    QComboBox *m_mmcTransport;

    // Recording
    QComboBox *m_recordDevice;
    std::vector<Rosegarden::DeviceId> m_devices;

#ifdef HAVE_LIBJACK
    // Number of JACK input ports our RG client creates - 
    // this decides how many audio input destinations
    // we have.
    //
    QSpinBox     *m_jackInputs;

#endif // HAVE_LIBJACK

    // How many minutes of audio recording should we allow?
    //
    QSpinBox     *m_audioRecordMinutes;

};
 
}

#endif // _ROSEGARDENCONFIGUREPAGE_H_
