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

#ifndef _DIALOGS_H_
#define _DIALOGS_H_

#include <kdialogbase.h>
#include <qstring.h>
#include <qcanvas.h>
#include <qspinbox.h>

#include <string>

#include "NotationTypes.h"
#include "editcommands.h"
#include "notepixmapfactory.h"

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


// Definitions of various simple dialogs that may be used in multiple
// different editing views.


class TimeSignatureDialog : public KDialogBase
{
    Q_OBJECT

public:
    TimeSignatureDialog(QWidget *parent,
			Rosegarden::TimeSignature defaultSig =
			Rosegarden::TimeSignature::DefaultTimeSignature,
			int barNo = 0, bool atStartOfBar = true,
			QString explanatoryText = 0);

    Rosegarden::TimeSignature getTimeSignature() const;

    enum Location {
	AsGiven, StartOfBar
    };

    Location getLocation() const;
    bool shouldNormalizeRests() const;

public slots:
    void slotNumUp();
    void slotNumDown();
    void slotDenomUp();
    void slotDenomDown();
    void slotUpdateCommonTimeButton();

protected:

    //--------------- Data members ---------------------------------

    Rosegarden::TimeSignature m_timeSignature;
    QLabel *m_numLabel;
    QLabel *m_denomLabel;
    QLabel *m_explanatoryLabel;

    QCheckBox *m_commonTimeButton;
    QCheckBox *m_hideSignatureButton;
    QCheckBox *m_normalizeRestsButton;
    QRadioButton *m_asGivenButton;
    QRadioButton *m_startOfBarButton;

    int m_barNo;
    bool m_atStartOfBar;
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
    RosegardenComboBox *m_keyCombo;
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

public slots:
    void slotUnitChanged(const QString &);
    void slotUntupledChanged(const QString &);
    void slotTupledChanged(const QString &);

protected:

    void updateUntupledCombo();
    void updateTupledCombo();
    void updateTimingDisplays();

    //--------------- Data members ---------------------------------

    KComboBox          *m_unitCombo;
    RosegardenComboBox *m_untupledCombo;
    RosegardenComboBox *m_tupledCombo;

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

protected:

    //--------------- Data members ---------------------------------

    QLineEdit *m_text;
    KComboBox *m_typeCombo;

    QLabel *m_staffAboveLabel;
    QLabel *m_textExampleLabel;
    QLabel *m_staffBelowLabel;

    NotePixmapFactory *m_notePixmapFactory;
    std::vector<std::string> m_styles;

    std::string getTextType() const;
    std::string getTextString() const;
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

    TempoDialog(QWidget *parent, RosegardenGUIDoc *doc);
    ~TempoDialog();

    // Set the position at which we're checking the tempo
    //
    void setTempoPosition(Rosegarden::timeT time);

public slots:
    virtual void slotOk();
    void slotActionChanged();
    void slotTempoChanged(const QString &);

signals:
    // Return results in this signal
    //
    void changeTempo(Rosegarden::timeT,  // tempo change time
                     double,             // tempo value
                     TempoDialog::TempoDialogAction); // tempo action

protected:
    void populateTempo();
    void updateBeatLabels(double newTempo);

    //--------------- Data members ---------------------------------

    RosegardenGUIDoc   *m_doc;
    Rosegarden::timeT   m_tempoTime;
    double              m_tempoValue;
    RosegardenSpinBox  *m_tempoValueSpinBox;

    QLabel	       *m_tempoBeatLabel;
    QLabel	       *m_tempoBeat;
    QLabel	       *m_tempoBeatsPerMinute;

    QLabel             *m_tempoTimeLabel;
    QLabel             *m_tempoBarLabel;
    QLabel             *m_tempoStatusLabel;
    
    QRadioButton       *m_tempoChangeHere;
    QRadioButton       *m_tempoChangeBefore;
    QLabel	       *m_tempoChangeBeforeAt;
    QRadioButton       *m_tempoChangeStartOfBar;
    QRadioButton       *m_tempoChangeGlobal;
    QCheckBox          *m_defaultBox;
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

protected:
    void redrawClefPixmap();

    //--------------- Data members ---------------------------------

    NotePixmapFactory *m_notePixmapFactory;
    Rosegarden::Clef m_clef;
    
    QLabel *m_clefLabel;
    QLabel *m_clefNameLabel;

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
    RescaleDialog(QWidget *parent);

    int getMultiplier();
    int getDivisor();

public slots:
    void slotFromChanged(int);
    void slotToChanged(int);

protected:
    int m_from;
    int m_to;
    QLabel *m_percent;
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

    RosegardenComboBox         *m_value1Combo;
    RosegardenComboBox         *m_value2Combo;
    RosegardenComboBox         *m_patternCombo;

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

protected slots:
    void slotPitchChanged(int);

private:
    QSpinBox *m_pitch;
    QCheckBox *m_range;
    QCheckBox *m_duplicate;
    KComboBox *m_clefs;
    QLabel *m_pitchLabel;
};


class InterpretDialog : public KDialogBase
{
    Q_OBJECT
public:
    InterpretDialog(QWidget *parent);

    // an OR from TransformsMenuInterpretCommand's constants
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
    int           m_totalTime;

    QLabel       *m_label;
    QLabel       *m_time;
    CountdownBar *m_progressBar;

    QPushButton  *m_stopButton;

    int           m_progressBarWidth;
    int           m_progressBarHeight;

    QAccel       *m_accelerators;
};

#endif
