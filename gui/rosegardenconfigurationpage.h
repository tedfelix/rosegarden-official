// -*- c-basic-offset: 4 -*-

/*
    Rosegarden-4
    A sequencer and musical notation editor.

    This program is Copyright 2000-2004
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

#include <map>
#include <utility>
#include <vector>

#include <qspinbox.h>
#include <qcombobox.h>
#include <qcheckbox.h>
#include <qslider.h>
#include <qlineedit.h>

#include <klocale.h>
#include <kcolordialog.h>
#include <kcolorbutton.h>

#include <string>

#include "ColourMap.h"
#include "Device.h"
#include "config.h"
#include "widgets.h"
#include "colourwidgets.h"

class RosegardenGUIDoc;
class QTabWidget;
class QLineEdit;
class QPushButton;
class QRadioButton;
class QLabel;
class QCheckBox;
class RosegardenQuantizeParameters;
class KListView;
class QTable;

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

    GeneralConfigurationPage(RosegardenGUIDoc *doc,
                             KConfig *cfg,
                             QWidget *parent=0, const char *name=0);

    virtual void apply();

    static QString iconLabel() { return i18n("General"); }
    static QString title()     { return i18n("General Configuration"); }
    static QString iconName()  { return "configure"; }

    int getCountInSpin()            { return m_countIn->value(); }
    int getDblClickClient()         { return m_client->currentItem(); }
    bool getUseDefaultStudio()      { return m_studio->isChecked(); }
    QString getExternalAudioEditor() { return m_externalAudioEditorPath->text(); }
    int getNoteNameStyle() { return m_nameStyle->currentItem(); }

signals:
    void updateAutoSaveInterval(unsigned int);

protected slots:
    void slotFileDialog();


protected:

    //--------------- Data members ---------------------------------
    RosegardenGUIDoc* m_doc;

    QComboBox* m_client;
    QSpinBox*  m_countIn;
    QCheckBox* m_studio;
    QSpinBox*  m_midiPitchOctave;
    QLineEdit* m_externalAudioEditorPath;
    QCheckBox* m_backgroundTextures;
    QCheckBox *m_autosave;
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
    static QString iconName()  { return "fonts"; }

protected slots:
    void slotFontComboChanged(int);
    void slotViewButtonPressed();

protected:

    //--------------- Data members ---------------------------------

    QComboBox *m_font;
    QComboBox *m_singleStaffSize;
    QComboBox *m_multiStaffSize;
    QComboBox *m_printingSize;
    QPushButton *m_viewButton;
    QLabel *m_fontOriginLabel;
    QLabel *m_fontCopyrightLabel;
    QLabel *m_fontMappedByLabel;
    QLabel *m_fontTypeLabel;
    QComboBox *m_layoutMode;
    QComboBox *m_spacing;
    QComboBox *m_proportion;
    QCheckBox *m_colourQuantize;
    QCheckBox *m_showUnknowns;
    QCheckBox *m_showInvisibles;
    QComboBox *m_noteStyle;
    QComboBox *m_insertType;
    QCheckBox *m_autoBeam;
    QCheckBox *m_collapseRests;
    QComboBox *m_pasteType;
    QComboBox *m_accOctavePolicy;
    QComboBox *m_accBarPolicy;
    QComboBox *m_keySigCancelMode;
    RosegardenQuantizeParameters *m_quantizeFrame;
    QStringList m_untranslatedFont;
    QStringList m_untranslatedNoteStyle;

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

    int getJACKPlaybackValue() { return m_jackPlayback->value(); }
    int getJACKRecordValue() { return m_jackRecord->value(); }

protected slots:
    // Get the latest latency values from the sequencer
    //
    void slotFetchLatencyValues();

protected:

    //--------------- Data members ---------------------------------

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
    static QString iconName()  { return "contents"; }

    void selectMetadata(QString name);

protected slots:
    void slotAddNewProperty();
    void slotDeleteProperty();
    
protected:

    //--------------- Data members ---------------------------------

    KListView *m_fixed;
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
    static QString iconName()  { return "folder"; }

protected slots:
    void slotFileDialog();

    // Work out and display remaining disk space and time left 
    // at current path.
    //
    void calculateStats();

    void slotFoundMountPoint(const QString&,
                             unsigned long kBSize,
                             unsigned long kBUsed,
                             unsigned long kBAvail);
    
protected:

    //--------------- Data members ---------------------------------

    QLabel           *m_path;
    QLabel           *m_diskSpace;
    QLabel           *m_minutesAtStereo;

    QPushButton      *m_changePathButton;
};

/**
 * Colour Configuration Page
 *
 * (document-wide settings)
 */
class ColourConfigurationPage : public TabbedConfigurationPage
{
    Q_OBJECT
public:
    ColourConfigurationPage(RosegardenGUIDoc *doc,
                            QWidget *parent=0, const char *name=0);
    virtual void apply();

    void populate_table();

    static QString iconLabel() { return i18n("Color"); }
    static QString title()     { return i18n("Color Settings"); }
    static QString iconName()  { return "colorize"; }

signals:
    void docColoursChanged();

protected slots:
    void slotAddNew();
    void slotDelete();
    void slotTextChanged(unsigned int, QString);
    void slotColourChanged(unsigned int, QColor);

protected:
    RosegardenColourTable *m_colourtable;

    ColourMap m_map;
    RosegardenColourTable::ColourList m_listmap;

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
    static QString iconName()  { return "player_play"; }

#ifdef HAVE_LIBJACK
    QString getJackPath() { return m_jackPath->text(); }
#endif // HAVE_LIBJACK

protected slots:

    void slotReadAheadChanged(int);
    void slotAudioMixChanged(int);
    void slotAudioReadChanged(int);
    void slotAudioWriteChanged(int);
    void slotSmallFileChanged(int);

    void slotShowStatus();
    void slotJackToggled();

    void slotSoundFontToggled(bool);
    void slotSfxLoadPathChoose();
    void slotSoundFontChoose();

protected:

    int updateTimeSlider(int msec, int minPower, int maxPower, int multiplier,
			 QSlider *slider, QLabel *label, QString klabel);

    //--------------- Data members ---------------------------------

    // General
    QLineEdit *m_sequencerArguments;
    QCheckBox *m_sendControllersAtPlay;

    QCheckBox   *m_sfxLoadEnabled;
    QLineEdit   *m_sfxLoadPath;
    QPushButton *m_sfxLoadChoose;
    QLineEdit   *m_soundFontPath;
    QPushButton *m_soundFontChoose;


#ifdef HAVE_LIBJACK
    QCheckBox *m_startJack;
    QLineEdit *m_jackPath;
#endif // HAVE_LIBJACK

    // Sync and timing
    //
    QCheckBox *m_midiClockEnabled;
    QComboBox *m_timer;
    QComboBox *m_jackTransport;
    QComboBox *m_mmcTransport;
    QComboBox *m_mtcTransport;

    int      m_sampleRate;
    QSlider* m_readAhead;
    QSlider* m_audioMix;
    QSlider* m_audioRead;
    QSlider* m_audioWrite;
    QSlider* m_smallFile;
    QLabel*  m_readAheadLabel;
    QLabel*  m_audioMixLabel;
    QLabel*  m_audioReadLabel;
    QLabel*  m_audioWriteLabel;
    QLabel*  m_smallFileLabel;

#ifdef HAVE_LIBJACK
    // Number of JACK input ports our RG client creates - 
    // this decides how many audio input destinations
    // we have.
    //
    QCheckBox    *m_createFaderOuts;
    QCheckBox    *m_createSubmasterOuts;

    KComboBox    *m_lowLatencyMode;

#endif // HAVE_LIBJACK

    // How many minutes of audio recording should we allow?
    //
    QSpinBox     *m_audioRecordMinutes;

};
 
}

#endif // _ROSEGARDENCONFIGUREPAGE_H_
