/*
    Rosegarden-4 v0.1
    A sequencer and musical notation editor.

    This program is Copyright 2000-2002
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

#include <string>

#include "NotationTypes.h"
#include "editcommands.h"

class QWidget;
class QLineEdit;
class QCheckBox;
class QSpinBox;
class QLabel;
class QComboBox;
class QGroupBox;
class QRadioButton;
class QVButtonGroup;
class NotePixmapFactory;
class QGrid;
class RosegardenGUIDoc;
class RosegardenSpinBox;


// Definitions of various simple dialogs that may be used in multiple
// different editing views.


// okay, not much point in this -- it could be a QInputDialog.  I
// s'pose there might be some advantage in deriving from KDialogBase,
// and hey, I've written it now

class SimpleTextDialog : public KDialogBase
{
    Q_OBJECT

public:
    SimpleTextDialog(QWidget *parent, int maxLength = -1); // for Qt default
    std::string getText() const;

protected:
    QLineEdit *m_lineEdit;
};


class TimeSignatureDialog : public KDialogBase
{
    Q_OBJECT

public:
    TimeSignatureDialog(QWidget *parent,
			Rosegarden::TimeSignature defaultSig =
			Rosegarden::TimeSignature::DefaultTimeSignature,
			int barNo = 0, bool atStartOfBar = true);

    Rosegarden::TimeSignature getTimeSignature() const;

    enum Location {
	AsGiven, StartOfBar
    };

    Location getLocation() const;
    bool shouldNormalizeRests() const;

protected:
    Rosegarden::TimeSignature m_timeSignature;
    QLabel *m_numLabel;
    QLabel *m_denomLabel;

    QCheckBox *m_commonTimeButton;
    QCheckBox *m_hideSignatureButton;
    QCheckBox *m_normalizeRestsButton;
    QRadioButton *m_asGivenButton;
    QRadioButton *m_startOfBarButton;

    int m_barNo;
    bool m_atStartOfBar;

public slots:
    void slotNumUp();
    void slotNumDown();
    void slotDenomUp();
    void slotDenomDown();
    void slotUpdateCommonTimeButton();
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
		       bool showConversionOptions = true);

    bool isValid() const;
    Rosegarden::Key getKey() const;

    bool shouldApplyToAll() const;
    ConversionType getConversionType() const;

protected:
    NotePixmapFactory *m_notePixmapFactory;

    Rosegarden::Key m_key;
    Rosegarden::Clef m_clef;
    bool m_valid;
    bool m_ignoreComboChanges;

    QLabel *m_keyLabel;
    QComboBox *m_keyCombo;
    QComboBox *m_majorMinorCombo;

    QRadioButton *m_applyToAllButton;

    QRadioButton *m_noConversionButton;
    QRadioButton *m_convertButton;
    QRadioButton *m_transposeButton;

    void redrawKeyPixmap();
    void regenerateKeyCombo();
    void setValid(bool valid);
    std::string getKeyName(const QString &s, bool minor);

public slots:
    void slotKeyUp();
    void slotKeyDown();
    void slotKeyNameChanged(const QString &);
    void slotMajorMinorChanged(const QString &);
};


class PasteNotationDialog : public KDialogBase
{
    Q_OBJECT

public:
    PasteNotationDialog(QWidget *parent,
			PasteEventsCommand::PasteType defaultType);

    PasteEventsCommand::PasteType getPasteType() const;
    bool setAsDefault() const;

protected:
    QRadioButton *m_restrictedButton;
    QRadioButton *m_simpleButton;
    QRadioButton *m_openAndPasteButton;
    QRadioButton *m_noteOverlayButton;
    QRadioButton *m_matrixOverlayButton;
    
    QCheckBox *m_setAsDefaultButton;

    PasteEventsCommand::PasteType m_defaultType;

public slots:
    void slotPasteTypeChanged();
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

protected:
    QComboBox *m_unitCombo;
    QComboBox *m_untupledCombo;
    QComboBox *m_tupledCombo;

    QLabel *m_selectionDurationDisplay;
    QLabel *m_untupledDurationCalculationDisplay;
    QLabel *m_untupledDurationDisplay;
    QLabel *m_tupledDurationCalculationDisplay;
    QLabel *m_tupledDurationDisplay;
    QLabel *m_newGapDurationCalculationDisplay;
    QLabel *m_newGapDurationDisplay;
    QLabel *m_unchangedDurationCalculationDisplay;
    QLabel *m_unchangedDurationDisplay;

    void updateUntupledCombo();
    void updateTupledCombo();
    void updateTimingDisplays();

    Rosegarden::timeT m_maxDuration;

public slots:
    void slotUnitChanged(const QString &);
    void slotUntupledChanged(const QString &);
    void slotTupledChanged(const QString &);
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

protected:
    QLineEdit *m_text;
    QComboBox *m_typeCombo;

    QLabel *m_staffAboveLabel;
    QLabel *m_textExampleLabel;
    QLabel *m_staffBelowLabel;

    NotePixmapFactory *m_notePixmapFactory;
    std::vector<std::string> m_styles;

    std::string getTextType() const;
    std::string getTextString() const;

public slots:
    void slotTextChanged(const QString &);
    void slotTypeChanged(const QString &);
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
		    NotePixmapFactory *npf,
		    const Rosegarden::Event &event,
		    bool editable = true);

    bool isModified() const { return m_modified; }
    Rosegarden::Event getEvent() const;

protected:
    NotePixmapFactory *m_notePixmapFactory;

    const Rosegarden::Event &m_originalEvent;
    Rosegarden::Event m_event;

    QLabel *m_durationDisplay;
    QLabel *m_durationDisplayAux;

    QGrid *m_persistentGrid;
    QGrid *m_nonPersistentGrid;

    std::string m_type;
    Rosegarden::timeT m_absoluteTime;
    Rosegarden::timeT m_duration;
    int m_subOrdering;

    bool m_modified;

public slots:
    void slotEventTypeChanged(const QString &);
    void slotAbsoluteTimeChanged(int value);
    void slotDurationChanged(int value);
    void slotSubOrderingChanged(int value);

    void slotIntPropertyChanged(int);
    void slotBoolPropertyChanged();
    void slotStringPropertyChanged(const QString &);

    void slotPropertyDeleted();
    void slotPropertyMadePersistent();

protected:
    void addPersistentProperty(const Rosegarden::PropertyName &);
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

signals:
    // Return results in this signal
    //
    void changeTempo(Rosegarden::timeT,  // tempo change time
                     double,             // tempo value
                     TempoDialog::TempoDialogAction); // tempo action

protected:
    void populateTempo();

    RosegardenGUIDoc   *m_doc;
    Rosegarden::timeT   m_tempoTime;
    double              m_tempoValue;

    RosegardenSpinBox  *m_tempoValueSpinBox;
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

protected:
    NotePixmapFactory *m_notePixmapFactory;
    Rosegarden::Clef m_clef;
    
    QLabel *m_clefLabel;
    QLabel *m_clefNameLabel;

    QRadioButton *m_noConversionButton;
    QRadioButton *m_changeOctaveButton;
    QRadioButton *m_transposeButton;
    
    void redrawClefPixmap();
    
public slots:
    void slotClefUp();
    void slotClefDown();
};


class QuantizeDialog : public KDialogBase
{
    Q_OBJECT

public:
    QuantizeDialog(QWidget *parent,
		   std::string quantizeSource,
		   std::string quantizeTarget);
    
    Rosegarden::Quantizer getQuantizer() const;
    
protected:
    std::string m_source;
    std::string m_target;

    std::vector<Rosegarden::StandardQuantization>
    m_standardQuantizations;

    QComboBox *m_typeCombo;
    QComboBox *m_unitCombo;
    QGroupBox *m_noteQuantizeBox;
    QComboBox *m_dotsCombo;
    QCheckBox *m_legatoButton;
    QCheckBox *m_makeViableButton;
    QCheckBox *m_rebeamButton;

public slots:
    void slotTypeChanged(int);
    void slotUnitChanged(int);
    void slotDotsChanged(int);
    void slotLegatoChanged();
};
  

#endif
