// -*- c-basic-offset: 4 -*-

/*
    Rosegarden-4
    A sequencer and musical notation editor.

    This program is Copyright 2000-2006
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

#ifndef _DIALOGS_H_
#define _DIALOGS_H_

#include <kdialogbase.h>
#include <qstring.h>
#include <qcanvas.h>
#include <qspinbox.h>
#include <kcombobox.h>

#include <string>

#include "NotationTypes.h"
#include "editcommands.h"
#include "notepixmapfactory.h"
#include "commondialogs.h" // HSpinBox
#include "Composition.h" // tempoT

class QWidget;
class QLineEdit;
class QCheckBox;
class QLabel;
class RosegardenComboBox;
class KComboBox;
class QGroupBox;
class QRadioButton;
class QVButtonGroup;
class NotePixmapFactory;
class QGrid;
class RosegardenGUIDoc;
class RosegardenSpinBox;
class QTextEdit;
class QAccel;
class RosegardenQuantizeParameters;
class RosegardenPitchChooser;
class BigArrowButton;
class InstrumentParameterBox;
class RosegardenTimeWidget;


// Definitions of various simple dialogs that may be used in multiple
// different editing views.


class TimeSignatureDialog : public KDialogBase
{
    Q_OBJECT

public:
    TimeSignatureDialog(QWidget *parent,
			Rosegarden::Composition *composition,
			Rosegarden::timeT insertionTime,
			Rosegarden::TimeSignature defaultSig =
			    Rosegarden::TimeSignature::DefaultTimeSignature,
			bool timeEditable = false,
			QString explanatoryText = 0);

    Rosegarden::TimeSignature getTimeSignature() const;

    Rosegarden::timeT getTime() const;
    bool shouldNormalizeRests() const;

public slots:
    void slotNumUp();
    void slotNumDown();
    void slotDenomUp();
    void slotDenomDown();
    void slotUpdateCommonTimeButton();

protected:
    //--------------- Data members ---------------------------------

    Rosegarden::Composition *m_composition;
    Rosegarden::TimeSignature m_timeSignature;
    Rosegarden::timeT m_time;

    QLabel *m_numLabel;
    QLabel *m_denomLabel;
    QLabel *m_explanatoryLabel;

    QCheckBox *m_commonTimeButton;
    QCheckBox *m_hideSignatureButton;
    QCheckBox *m_hideBarsButton;
    QCheckBox *m_normalizeRestsButton;

    QRadioButton *m_asGivenButton;
    QRadioButton *m_startOfBarButton;

    RosegardenTimeWidget *m_timeEditor;
};


class KeySignatureDialog : public KDialogBase
{
    Q_OBJECT

public:
    enum ConversionType {
	NoConversion,
	Convert,
	Transpose
    };

    KeySignatureDialog(QWidget *parent,
		       NotePixmapFactory *npf,
		       Rosegarden::Clef clef,
		       Rosegarden::Key defaultKey =
		       Rosegarden::Key::DefaultKey,
		       bool showApplyToAll = true,
		       bool showConversionOptions = true,
		       QString explanatoryText = 0);

    bool isValid() const;
    Rosegarden::Key getKey() const;

    bool shouldApplyToAll() const;
    ConversionType getConversionType() const;

public slots:
    void slotKeyUp();
    void slotKeyDown();
    void slotKeyNameChanged(const QString &);
    void slotMajorMinorChanged(const QString &);

protected:

    void redrawKeyPixmap();
    void regenerateKeyCombo();
    void setValid(bool valid);

    //--------------- Data members ---------------------------------

    NotePixmapFactory *m_notePixmapFactory;

    Rosegarden::Key m_key;
    Rosegarden::Clef m_clef;
    bool m_valid;
    bool m_ignoreComboChanges;

    QLabel *m_keyLabel;
    KComboBox *m_keyCombo;
    KComboBox *m_majorMinorCombo;
    QLabel *m_explanatoryLabel;

    QRadioButton *m_applyToAllButton;

    QRadioButton *m_noConversionButton;
    QRadioButton *m_convertButton;
    QRadioButton *m_transposeButton;

    std::string getKeyName(const QString &s, bool minor);
};


class PasteNotationDialog : public KDialogBase
{
    Q_OBJECT

public:
    PasteNotationDialog(QWidget *parent,
			PasteEventsCommand::PasteType defaultType);

    PasteEventsCommand::PasteType getPasteType() const;
    bool setAsDefault() const;

public slots:
    void slotPasteTypeChanged();

protected:

    //--------------- Data members ---------------------------------

    std::vector<QRadioButton *> m_pasteTypeButtons;
    QCheckBox *m_setAsDefaultButton;

    PasteEventsCommand::PasteType m_defaultType;
};


class TupletDialog : public KDialogBase
{
    Q_OBJECT

public:
    TupletDialog(QWidget *parent,
		 Rosegarden::Note::Type defaultUnitType,
		 Rosegarden::timeT maxDuration = 0);

    Rosegarden::Note::Type getUnitType() const;
    int getUntupledCount() const;
    int getTupledCount() const;
    bool hasTimingAlready() const;

public slots:
    void slotUnitChanged(const QString &);
    void slotUntupledChanged(const QString &);
    void slotTupledChanged(const QString &);
    void slotHasTimingChanged();

protected:

    void updateUntupledCombo();
    void updateTupledCombo();
    void updateTimingDisplays();

    //--------------- Data members ---------------------------------

    KComboBox *m_unitCombo;
    KComboBox *m_untupledCombo;
    KComboBox *m_tupledCombo;

    QCheckBox *m_hasTimingAlready;

    QGroupBox *m_timingDisplayBox;
    QLabel *m_selectionDurationDisplay;
    QLabel *m_untupledDurationCalculationDisplay;
    QLabel *m_untupledDurationDisplay;
    QLabel *m_tupledDurationCalculationDisplay;
    QLabel *m_tupledDurationDisplay;
    QLabel *m_newGapDurationCalculationDisplay;
    QLabel *m_newGapDurationDisplay;
    QLabel *m_unchangedDurationCalculationDisplay;
    QLabel *m_unchangedDurationDisplay;

    Rosegarden::timeT m_maxDuration;
};


class TextEventDialog : public KDialogBase
{
    Q_OBJECT

public:
    TextEventDialog(QWidget *parent,
		    NotePixmapFactory *npf,
		    Rosegarden::Text defaultText,
		    int maxLength = -1); // for Qt default

    Rosegarden::Text getText() const {
	return Rosegarden::Text(getTextString(), getTextType());
    }

public slots:
    void slotTextChanged(const QString &);
    void slotTypeChanged(const QString &);

    // convenience canned dynamic texts
    void slotDynamicShortcutChanged(const QString &);

    // convenience canned tempo texts
    void slotTempoShortcutChanged(const QString &);

    // special Lilypond directives
    void slotLilypondDirectiveChanged(const QString &);

protected:

    //--------------- Data members ---------------------------------

    QLineEdit *m_text;
    KComboBox *m_typeCombo;
    KComboBox *m_dynamicShortcutCombo;
    KComboBox *m_tempoShortcutCombo;
    KComboBox *m_lilypondDirectiveCombo;

    QLabel *m_staffAboveLabel;
    QLabel *m_textExampleLabel;
    QLabel *m_staffBelowLabel;
    QLabel *m_dynamicShortcutLabel;
    QLabel *m_tempoShortcutLabel;
    QLabel *m_directiveLabel;

    NotePixmapFactory *m_notePixmapFactory;
    std::vector<std::string> m_styles;
    std::vector<std::string> m_directives;

    std::string getTextType() const;
    std::string getTextString() const;
};


class PitchDialog : public KDialogBase
{
    Q_OBJECT
public:
    PitchDialog(QWidget *parent, QString title, int defaultPitch = 60);

    int getPitch() const;

protected:
    RosegardenPitchChooser *m_pitchChooser;
};


class TimeDialog : public KDialogBase
{
    Q_OBJECT
public:
    /// for absolute times
    TimeDialog(QWidget *parent, QString title, Rosegarden::Composition *composition,
	       Rosegarden::timeT defaultTime);

    /// for durations
    TimeDialog(QWidget *parent, QString title, Rosegarden::Composition *composition,
	       Rosegarden::timeT startTime, Rosegarden::timeT defaultDuration);

    Rosegarden::timeT getTime() const;

protected:
    RosegardenTimeWidget *m_timeWidget;
};
		     

class EventEditDialog : public KDialogBase
{
    Q_OBJECT

public:
    /**
     * Construct an event-edit dialog showing the properties of the
     * given event.  If editable is false, the user will not be allowed
     * to modify the event; otherwise the event will be editable and
     * the resulting edited version can subsequently be queried
     * through getEvent().
     */
    EventEditDialog(QWidget *parent,
		    const Rosegarden::Event &event,
		    bool editable = true);

    bool isModified() const { return m_modified; }
    Rosegarden::Event getEvent() const;

public slots:
    void slotEventTypeChanged(const QString &);
    void slotAbsoluteTimeChanged(int value);
    void slotDurationChanged(int value);
    void slotSubOrderingChanged(int value);

    void slotIntPropertyChanged(int);
    void slotRealTimePropertyChanged(int);
    void slotBoolPropertyChanged();
    void slotStringPropertyChanged(const QString &);

    void slotPropertyDeleted();
    void slotPropertyMadePersistent();

protected:
    void addPersistentProperty(const Rosegarden::PropertyName &);

    //--------------- Data members ---------------------------------
    NotePixmapFactory m_notePixmapFactory;

    QLabel *m_durationDisplay;
    QLabel *m_durationDisplayAux;

    QGrid *m_persistentGrid;
    QGrid *m_nonPersistentGrid;

    QScrollView *m_nonPersistentView;

    const Rosegarden::Event &m_originalEvent;
    Rosegarden::Event m_event;

    std::string m_type;
    Rosegarden::timeT m_absoluteTime;
    Rosegarden::timeT m_duration;
    int m_subOrdering;

    bool m_modified;
};

/*
 * A simpler event editor for use by the EventView and MatrixView
 * and people who want to remain sane.
 */
class SimpleEventEditDialog : public KDialogBase
{
    Q_OBJECT
public:
    SimpleEventEditDialog(QWidget *parent,
                          RosegardenGUIDoc *doc,
		          const Rosegarden::Event &event,
		          bool inserting = false); // inserting or editing

    bool isModified() const { return m_modified; }
    Rosegarden::Event getEvent();

    // Setup the dialog for a new event type
    void setupForEvent();

public slots:
    void slotEventTypeChanged(int value);
    void slotAbsoluteTimeChanged(int value);
    void slotDurationChanged(int value);
    void slotNotationAbsoluteTimeChanged(int value);
    void slotNotationDurationChanged(int value);
    void slotPitchChanged(int value);
    void slotVelocityChanged(int value);
    void slotMetaChanged(const QString &);
    void slotEditAbsoluteTime();
    void slotEditNotationAbsoluteTime();
    void slotEditDuration();
    void slotEditNotationDuration();
    void slotLockNotationChanged();
    void slotEditPitch();
    void slotSysexLoad();
    void slotSysexSave();

protected:
    Rosegarden::Event        m_event;
    RosegardenGUIDoc        *m_doc;

    std::string              m_type;
    Rosegarden::timeT        m_absoluteTime;
    Rosegarden::timeT        m_notationAbsoluteTime;
    Rosegarden::timeT        m_duration;
    Rosegarden::timeT        m_notationDuration;

    KComboBox               *m_typeCombo;
    QLabel                  *m_typeLabel;

    QLabel                  *m_timeLabel;
    QLabel                  *m_durationLabel;
    QLabel                  *m_pitchLabel;
    QLabel                  *m_velocityLabel;
    QLabel                  *m_metaLabel;
    QLabel                  *m_controllerLabel;
    QLabel                  *m_controllerLabelValue;

    QSpinBox                *m_timeSpinBox;
    QSpinBox                *m_durationSpinBox;
    QSpinBox                *m_pitchSpinBox;
    QSpinBox                *m_velocitySpinBox;

    QPushButton             *m_timeEditButton;
    QPushButton             *m_durationEditButton;
    QPushButton             *m_pitchEditButton;
    QPushButton             *m_sysexLoadButton;
    QPushButton             *m_sysexSaveButton;

    QGroupBox               *m_notationGroupBox;
    QLabel                  *m_notationTimeLabel;
    QLabel                  *m_notationDurationLabel;
    QSpinBox                *m_notationTimeSpinBox;
    QSpinBox                *m_notationDurationSpinBox;
    QPushButton             *m_notationTimeEditButton;
    QPushButton             *m_notationDurationEditButton;
    QCheckBox               *m_lockNotationValues;

    QLineEdit               *m_metaEdit;

    bool                     m_modified;
};


class TempoDialog : public KDialogBase
{
    Q_OBJECT
public:
    typedef enum{
        AddTempo,
        ReplaceTempo,
        AddTempoAtBarStart,
        GlobalTempo,
        GlobalTempoWithDefault
    } TempoDialogAction;

    TempoDialog(QWidget *parent, RosegardenGUIDoc *doc,
		bool timeEditable = false);
    ~TempoDialog();

    // Set the position at which we're checking the tempo
    //
    void setTempoPosition(Rosegarden::timeT time);

public slots:
    virtual void slotOk();
    void slotActionChanged();
    void slotTempoChanged(const QString &);
    void slotTempoConstantClicked();
    void slotTempoRampToNextClicked();
    void slotTempoRampToTargetClicked();
    void slotTargetChanged(const QString &);

signals:
    // Return results in this signal
    //
    void changeTempo(Rosegarden::timeT,  // tempo change time
                     Rosegarden::tempoT,  // tempo value
		     Rosegarden::tempoT,  // target tempo value
                     TempoDialog::TempoDialogAction); // tempo action

protected:
    void populateTempo();
    void updateBeatLabels(double newTempo);

    //--------------- Data members ---------------------------------

    RosegardenGUIDoc     *m_doc;
    Rosegarden::timeT     m_tempoTime;
    HSpinBox  	  	 *m_tempoValueSpinBox;

    QRadioButton         *m_tempoConstant;
    QRadioButton         *m_tempoRampToNext;
    QRadioButton         *m_tempoRampToTarget;
    HSpinBox             *m_tempoTargetSpinBox; 

    QLabel	         *m_tempoBeatLabel;
    QLabel	         *m_tempoBeat;
    QLabel	         *m_tempoBeatsPerMinute;

    RosegardenTimeWidget *m_timeEditor;

    QLabel               *m_tempoTimeLabel;
    QLabel               *m_tempoBarLabel;
    QLabel               *m_tempoStatusLabel;
    
    QRadioButton         *m_tempoChangeHere;
    QRadioButton         *m_tempoChangeBefore;
    QLabel	         *m_tempoChangeBeforeAt;
    QRadioButton         *m_tempoChangeStartOfBar;
    QRadioButton         *m_tempoChangeGlobal;
    QCheckBox            *m_defaultBox;
};


class ClefDialog : public KDialogBase
{
    Q_OBJECT

public:
    enum ConversionType {
	NoConversion,
	ChangeOctave,
	Transpose
    };

    ClefDialog(QWidget *parent,
	       NotePixmapFactory *npf,
	       Rosegarden::Clef defaultClef,
	       bool showConversionOptions = true);

    Rosegarden::Clef getClef() const;
    ConversionType getConversionType() const;

public slots:
    void slotClefUp();
    void slotClefDown();
    void slotOctaveUp();
    void slotOctaveDown();

protected:
    void redrawClefPixmap();

    //--------------- Data members ---------------------------------

    NotePixmapFactory *m_notePixmapFactory;
    Rosegarden::Clef m_clef;
    
    QLabel *m_clefLabel;
    QLabel *m_clefNameLabel;

    BigArrowButton *m_octaveUp;
    BigArrowButton *m_octaveDown;

    QRadioButton *m_noConversionButton;
    QRadioButton *m_changeOctaveButton;
    QRadioButton *m_transposeButton;   
};


class QuantizeDialog : public KDialogBase
{
    Q_OBJECT

public:
    QuantizeDialog(QWidget *parent, bool inNotation = false);
    
    /// Returned quantizer object is on heap -- caller must delete
    Rosegarden::Quantizer *getQuantizer() const;

protected:
    RosegardenQuantizeParameters *m_quantizeFrame;
};


class RescaleDialog : public KDialogBase
{
    Q_OBJECT

public:
    RescaleDialog(QWidget *parent,
		  Rosegarden::Composition *composition, // for TimeWidget calculations
		  Rosegarden::timeT startTime,
		  Rosegarden::timeT originalDuration,
		  bool showCloseGapOption);

    Rosegarden::timeT getNewDuration();
    bool shouldCloseGap();

protected:
    RosegardenTimeWidget *m_newDuration;
    QCheckBox *m_closeGap;
};
    

class FileMergeDialog : public KDialogBase
{
    Q_OBJECT

public:
    FileMergeDialog(QWidget *parent, QString fileName, bool timingsDiffer);

    int getMergeOptions();
    
private:
    KComboBox *m_choice;
    QCheckBox *m_useTimings;
};


// Locate a file
//
class FileLocateDialog : public KDialogBase
{
    Q_OBJECT

public:
    FileLocateDialog(QWidget *parent,
                     const QString &file,
                     const QString &path);

    QString getDirectory() { return m_path; }
    QString getFilename() { return m_file; }

protected:
    virtual void slotUser1();
    virtual void slotUser2();
    virtual void slotUser3();

    QString m_file;
    QString m_path;

};
  
class AudioPlayingDialog : public KDialogBase
{
    Q_OBJECT

public:
    AudioPlayingDialog(QWidget *parent, const QString &label);

signals:

};

class AudioSplitDialog : public KDialogBase
{
    Q_OBJECT
public:
    AudioSplitDialog(QWidget *parent,
                     Rosegarden::Segment *segment,
                     RosegardenGUIDoc *doc);

    // Draw an audio preview over the segment and draw
    // the potential splits along it.
    //
    void drawPreview();
    void drawSplits(int threshold);

    // Get the threshold
    //
    int getThreshold() { return m_thresholdSpin->value(); }

public slots:
    void slotThresholdChanged(int);

protected:
    RosegardenGUIDoc              *m_doc;
    Rosegarden::Segment           *m_segment;
    QCanvas                       *m_canvas;
    QCanvasView                   *m_canvasView;
    QSpinBox                      *m_thresholdSpin;

    int                            m_canvasWidth;
    int                            m_canvasHeight;
    int                            m_previewWidth;
    int                            m_previewHeight;

    std::vector<QCanvasRectangle*> m_previewBoxes;

};


class LyricEditDialog : public KDialogBase
{
    Q_OBJECT

public:
    LyricEditDialog(QWidget *parent, Rosegarden::Segment *segment);

    QString getLyricData();

protected:
    Rosegarden::Segment *m_segment;

    QTextEdit           *m_textEdit;

    void unparse();
};

// -------------  EventParameterDialog -------------
//
class EventParameterDialog : public KDialogBase
{
    Q_OBJECT

public:
    EventParameterDialog(QWidget *parent,
                         const QString &name,                      // name
                         const Rosegarden::PropertyName &property, // property
                         int startValue);                          // start

    int getValue1();
    int getValue2();
    Rosegarden::PropertyPattern getPattern();

public slots:
    void slotPatternSelected(int value);

protected:
    //--------------- Data members ---------------------------------
    Rosegarden::PropertyName    m_property;
    Rosegarden::PropertyPattern m_pattern;

    KComboBox         *m_value1Combo;
    KComboBox         *m_value2Combo;
    KComboBox         *m_patternCombo;

    QLabel                     *m_value1Label;
    QLabel                     *m_value2Label;

};


// ---------------- CompositionLengthDialog -----------
class CompositionLengthDialog : public KDialogBase
{
    Q_OBJECT
public:
    CompositionLengthDialog(QWidget *parent,
                            Rosegarden::Composition *composition);

    Rosegarden::timeT getStartMarker();
    Rosegarden::timeT getEndMarker();

protected:

    QSpinBox                *m_startMarkerSpinBox;
    QSpinBox                *m_endMarkerSpinBox;
    Rosegarden::Composition *m_composition;
};


class SplitByPitchDialog : public KDialogBase
{
    Q_OBJECT
public:
    SplitByPitchDialog(QWidget *parent);

    int getPitch();

    bool getShouldRange();
    bool getShouldDuplicateNonNoteEvents();
    int getClefHandling(); // actually SegmentSplitByPitchCommand::ClefHandling

private:
    RosegardenPitchChooser *m_pitch;

    QCheckBox *m_range;
    QCheckBox *m_duplicate;
    KComboBox *m_clefs;
};


class SplitByRecordingSrcDialog : public KDialogBase
{
    Q_OBJECT
public:
    SplitByRecordingSrcDialog(QWidget *parent, RosegardenGUIDoc *doc);
    
    int getChannel();
    int getDevice();
    
private:
    std::vector<int> m_deviceIds;
    KComboBox *m_channel;
    KComboBox *m_device;
};

class InterpretDialog : public KDialogBase
{
    Q_OBJECT
public:
    InterpretDialog(QWidget *parent);

    // an OR from AdjustMenuInterpretCommand's constants
    int getInterpretations();

protected slots:
    void slotAllBoxChanged();

private:
    QCheckBox *m_allInterpretations;
    QCheckBox *m_applyTextDynamics;
    QCheckBox *m_applyHairpins;
    QCheckBox *m_stressBeats;
    QCheckBox *m_articulate;
};


class ShowSequencerStatusDialog : public KDialogBase
{
    Q_OBJECT
public:
    ShowSequencerStatusDialog(QWidget *parent);
};


// Timer dialog for counting down
//

class CountdownBar : public QFrame
{
    Q_OBJECT
public:
    CountdownBar(QWidget *parent, int width, int height);
    void setPosition(int position);

protected:
    virtual void paintEvent(QPaintEvent *e);

    int m_width;
    int m_height;
    int m_position;
};

class CountdownDialog : public QDialog // KDialogBase
{
    Q_OBJECT

public:
    CountdownDialog(QWidget *parent, int seconds = 300);

    void setLabel(const QString &label);
    void setElapsedTime(int seconds);

    int getTotalTime() const { return m_totalTime; }
    void setTotalTime(int seconds);

    QAccel* getAccelerators() { return m_accelerators; }

signals:
    void completed(); // m_totalTime has elapsed
    void stopped();   // someone pushed the stop button

protected:
    void setPastEndMode();

    bool          m_pastEndMode;

    int           m_totalTime;

    QLabel       *m_label;
    QLabel       *m_time;
    CountdownBar *m_progressBar;

    QPushButton  *m_stopButton;

    int           m_progressBarWidth;
    int           m_progressBarHeight;

    QAccel       *m_accelerators;
};

class ManageMetronomeDialog : public KDialogBase
{
    Q_OBJECT

public:
    ManageMetronomeDialog(QWidget *parent, RosegardenGUIDoc *doc);

    void setModified(bool value);

public slots:
    void slotOk();
    void slotApply();
    void slotSetModified();
    void slotResolutionChanged(int);
    void slotPreviewPitch(int);
    void slotInstrumentChanged(int);
    void slotPitchSelectorChanged(int);
    void slotPitchChanged(int);
    void populate(int dev);

protected:

    //--------------- Data members ---------------------------------

    RosegardenGUIDoc       *m_doc;

    KComboBox              *m_metronomeDevice;
    KComboBox              *m_metronomeInstrument;
    KComboBox              *m_metronomeResolution;
    KComboBox              *m_metronomePitchSelector;
    RosegardenPitchChooser *m_metronomePitch;
    QSpinBox               *m_metronomeBarVely;
    QSpinBox               *m_metronomeBeatVely;
    QSpinBox               *m_metronomeSubBeatVely;
    InstrumentParameterBox *m_instrumentParameterBox;
    QCheckBox              *m_playEnabled;
    QCheckBox              *m_recordEnabled;
    	
    bool                   m_modified;
    Rosegarden::MidiByte   m_barPitch;
    Rosegarden::MidiByte   m_beatPitch;
    Rosegarden::MidiByte   m_subBeatPitch;
};

class LilypondOptionsDialog : public KDialogBase
{
    Q_OBJECT

public:
    LilypondOptionsDialog(QWidget *parent,
			  QString windowCaption = "",
			  QString heading = "");

public slots:
    void slotOk();

protected:
    QComboBox *m_lilyLanguage;
    QComboBox *m_lilyPaperSize;
    QComboBox *m_lilyFontSize;
    QCheckBox *m_lilyExportHeaders;
    QCheckBox *m_lilyExportLyrics;
    QCheckBox *m_lilyExportMidi;
    QCheckBox *m_lilyExportUnmuted;
    QCheckBox *m_lilyExportPointAndClick;
    QCheckBox *m_lilyExportBarChecks;
    QCheckBox *m_lilyExportBeams;
    QCheckBox *m_lilyExportStaffGroup;
    QCheckBox *m_lilyExportStaffMerge;
};


class ExportDeviceDialog : public KDialogBase
{
public:
    enum ExportType { ExportOne, ExportAll };
    
    ExportDeviceDialog(QWidget *parent, QString deviceName);
    
    ExportType getExportType();

protected:
    QRadioButton *m_exportAll;
    QRadioButton *m_exportOne;
};


class MakeOrnamentDialog : public KDialogBase
{
    Q_OBJECT

public:
    MakeOrnamentDialog(QWidget *parent, QString defaultName, int defaultBasePitch);
    
    QString getName() const;
    int getBasePitch() const;

protected:
    QLineEdit *m_name;
    RosegardenPitchChooser *m_pitch;
};


class UseOrnamentDialog : public KDialogBase
{
    Q_OBJECT

public:
    UseOrnamentDialog(QWidget *parent, Rosegarden::Composition *);

    Rosegarden::TriggerSegmentId getId() const;
    Rosegarden::Mark getMark() const;
    bool getRetune() const;
    std::string getTimeAdjust() const;

public slots:
    void slotOk();
    void slotMarkChanged(int);

protected:
    void setupFromConfig();

    std::vector<Rosegarden::Mark> m_marks;

    Rosegarden::Composition  *m_composition;
    KComboBox                *m_ornament;
    KComboBox                *m_mark;
    QLabel                   *m_textLabel;
    QLineEdit                *m_text;
    QCheckBox                *m_retune;
    KComboBox                *m_adjustTime;
};

class TriggerSegmentDialog : public KDialogBase
{
    Q_OBJECT

public:
    TriggerSegmentDialog(QWidget *parent, Rosegarden::Composition *);

    Rosegarden::TriggerSegmentId getId() const;
    bool getRetune() const;
    std::string getTimeAdjust() const;

public slots:
    void slotOk();

protected:
    void setupFromConfig();

    Rosegarden::Composition  *m_composition;
    KComboBox                *m_segment;
    QCheckBox                *m_retune;
    KComboBox                *m_adjustTime;
};

/**
 * ask the user to give us information about the selected audio segment for
 * Tempo calculations
 */
class BeatsBarsDialog : public KDialogBase
{
    Q_OBJECT
	
public:
    BeatsBarsDialog();
    BeatsBarsDialog(QWidget *parent);

    int getQuantity() { return m_spinBox->value(); }
    int getMode()     { return m_comboBox->currentItem();   } 

protected:
    QSpinBox  *m_spinBox;
    KComboBox *m_comboBox;
};


class IdentifyTextCodecDialog : public KDialogBase
{
    Q_OBJECT
    
public:
    IdentifyTextCodecDialog(QWidget *parent, std::string text);

    std::string getCodec() const { return m_codec; }

protected slots:
    void slotCodecSelected(int);

protected:
    std::string m_text;
    std::string m_codec;
    std::vector<std::string> m_codecs;
    QLabel *m_example;
};


#endif
