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

#include "dialogs.h"
#include "notepixmapfactory.h"
#include "rosestrings.h"
#include "rosedebug.h"
#include "rosegardenguidoc.h"
#include "widgets.h"

#include "RealTime.h"

#include <qlineedit.h>
#include <qlabel.h>
#include <qhbox.h>
#include <qvbox.h>
#include <qspinbox.h>
#include <qcombobox.h>
#include <qcheckbox.h>
#include <qgroupbox.h>
#include <qradiobutton.h>
#include <qpushbutton.h>
#include <qbuttongroup.h>
#include <qlayout.h>
#include <qtooltip.h>
#include <qobjectlist.h>
#include <qmessagebox.h>
#include <qgrid.h>
#include <qbitmap.h>
#include <qspinbox.h>
#include <qvalidator.h>
#include <qvbuttongroup.h>

#include <klocale.h>
#include <karrowbutton.h>

using Rosegarden::TimeSignature;
using Rosegarden::Note;
using Rosegarden::timeT;
using Rosegarden::Quantizer;


SimpleTextDialog::SimpleTextDialog(QWidget *parent, int maxLength) :
    KDialogBase(parent, 0, true, i18n("Text"), Ok | Cancel)
{
    QHBox *w = makeHBoxMainWidget();
    new QLabel(i18n("Text:"), w);
    m_lineEdit = new QLineEdit(w);
    if (maxLength > 0) m_lineEdit->setMaxLength(maxLength);
    m_lineEdit->setFocus();
}

std::string
SimpleTextDialog::getText() const
{
    return qstrtostr(m_lineEdit->text());
}


class BigArrowButton : public KArrowButton
{
public:
    BigArrowButton(QWidget *parent = 0, Qt::ArrowType arrow = Qt::UpArrow,
		   const char *name = 0) :
	KArrowButton(parent, arrow, name) { }
    virtual ~BigArrowButton() { } 

    virtual QSize sizeHint() const {
	return QSize(20, 20);
    }
};
    

TimeSignatureDialog::TimeSignatureDialog(QWidget *parent,
					 Rosegarden::TimeSignature sig,
					 int barNo, bool atStartOfBar) :
    KDialogBase(parent, 0, true, i18n("Time Signature"), Ok | Cancel),
    m_timeSignature(sig),
    m_commonTimeButton(0),
    m_hideSignatureButton(0),
    m_normalizeRestsButton(0),
    m_asGivenButton(0),
    m_startOfBarButton(0),
    m_barNo(barNo),
    m_atStartOfBar(atStartOfBar)
{
    static QFont *timeSigFont = 0;

    if (timeSigFont == 0) {
	timeSigFont = new QFont("new century schoolbook", 8, QFont::Bold);
	timeSigFont->setPixelSize(20);
    }

    QVBox *vbox = makeVBoxMainWidget();
    QGroupBox *groupBox = new QGroupBox
	(1, Horizontal, i18n("Time signature"), vbox);
    QHBox *numBox = new QHBox(groupBox);
    QHBox *denomBox = new QHBox(groupBox);

    BigArrowButton *numDown   = new BigArrowButton(numBox, Qt::LeftArrow);
    BigArrowButton *denomDown = new BigArrowButton(denomBox, Qt::LeftArrow);

    m_numLabel   = new QLabel
	(QString("%1").arg(m_timeSignature.getNumerator()), numBox);
    m_denomLabel = new QLabel
	(QString("%1").arg(m_timeSignature.getDenominator()), denomBox);

    m_numLabel->setAlignment(AlignHCenter | AlignVCenter);
    m_denomLabel->setAlignment(AlignHCenter | AlignVCenter);

    m_numLabel->setFont(*timeSigFont);
    m_denomLabel->setFont(*timeSigFont);

    BigArrowButton *numUp     = new BigArrowButton(numBox, Qt::RightArrow);
    BigArrowButton *denomUp   = new BigArrowButton(denomBox, Qt::RightArrow);

    QObject::connect(numDown,   SIGNAL(clicked()), this, SLOT(slotNumDown()));
    QObject::connect(numUp,     SIGNAL(clicked()), this, SLOT(slotNumUp()));
    QObject::connect(denomDown, SIGNAL(clicked()), this, SLOT(slotDenomDown()));
    QObject::connect(denomUp,   SIGNAL(clicked()), this, SLOT(slotDenomUp()));

    groupBox = new QButtonGroup(1, Horizontal, i18n("Scope"), vbox);

    if (!m_atStartOfBar) {

	// let's forget about start-of-composition for the moment
	QString scopeText;
	if (m_barNo != 0 || !m_atStartOfBar) {
	    if (m_atStartOfBar) {
		scopeText = QString
		    (i18n("Insertion point is at start of bar %1."))
		    .arg(m_barNo + 1);
	    } else {
		scopeText = QString
		    (i18n("Insertion point is in the middle of bar %1."))
		    .arg(m_barNo + 1);
	    }
	} else {
	    scopeText = QString
		(i18n("Insertion point is at start of composition."));
	}
	
	new QLabel(scopeText, groupBox);
	m_asGivenButton = new QRadioButton
	    (i18n("Start bar %1 here").arg(m_barNo + 2), groupBox);

	if (!m_atStartOfBar) {
	    m_startOfBarButton = new QRadioButton
		(i18n("Change time from start of bar %1")
		 .arg(m_barNo + 1), groupBox);
	    m_startOfBarButton->setChecked(true);
	} else {
	    m_asGivenButton->setChecked(true);
	}
    } else {
	new QLabel(i18n("Time change will take effect at the start of bar %1.")
		   .arg(barNo + 1), groupBox);
    }

    groupBox = new QGroupBox(1, Horizontal, i18n("Options"), vbox);
    m_hideSignatureButton = new QCheckBox
	(i18n("Make the new time signature hidden"), groupBox);
    m_hideSignatureButton->setChecked(false);
    m_commonTimeButton = new QCheckBox
	(i18n("Show as common time"), groupBox);
    m_commonTimeButton->setChecked(true);
    m_normalizeRestsButton = new QCheckBox
	(i18n("Normalize subsequent rests"), groupBox);
    m_normalizeRestsButton->setChecked(true);
    QObject::connect(m_hideSignatureButton, SIGNAL(clicked()), this,
		     SLOT(slotUpdateCommonTimeButton()));
    slotUpdateCommonTimeButton();
}

Rosegarden::TimeSignature
TimeSignatureDialog::getTimeSignature() const
{
    TimeSignature ts(m_timeSignature.getNumerator(),
		     m_timeSignature.getDenominator(),
		     (m_commonTimeButton &&
		      m_commonTimeButton->isEnabled() &&
		      m_commonTimeButton->isChecked()),
		     (m_hideSignatureButton &&
		      m_hideSignatureButton->isEnabled() &&
		      m_hideSignatureButton->isChecked()));
    return ts;
}


void
TimeSignatureDialog::slotNumDown()
{
    int n = m_timeSignature.getNumerator();
    if (--n >= 1) {
	m_timeSignature = TimeSignature(n, m_timeSignature.getDenominator());
	m_numLabel->setText(QString("%1").arg(n));
    }
    slotUpdateCommonTimeButton();
}

void
TimeSignatureDialog::slotNumUp()
{
    int n = m_timeSignature.getNumerator();
    if (++n <= 99) {
	m_timeSignature = TimeSignature(n, m_timeSignature.getDenominator());
	m_numLabel->setText(QString("%1").arg(n));
    }
    slotUpdateCommonTimeButton();
}

void
TimeSignatureDialog::slotDenomDown()
{
    int n = m_timeSignature.getDenominator();
    if ((n /= 2) >= 1) {
	m_timeSignature = TimeSignature(m_timeSignature.getNumerator(), n);
	m_denomLabel->setText(QString("%1").arg(n));
    }
    slotUpdateCommonTimeButton();
}

void
TimeSignatureDialog::slotDenomUp()
{
    int n = m_timeSignature.getDenominator();
    if ((n *= 2) <= 64) {
	m_timeSignature = TimeSignature(m_timeSignature.getNumerator(), n);
	m_denomLabel->setText(QString("%1").arg(n));
    }
    slotUpdateCommonTimeButton();
}

void
TimeSignatureDialog::slotUpdateCommonTimeButton()
{
    if (!m_hideSignatureButton || !m_hideSignatureButton->isChecked()) {
	if (m_timeSignature.getDenominator() == m_timeSignature.getNumerator()) {
	    if (m_timeSignature.getNumerator() == 4) {
		m_commonTimeButton->setText(i18n("Display as common time"));
		m_commonTimeButton->setEnabled(true);
		return;
	    } else if (m_timeSignature.getNumerator() == 2) {
		m_commonTimeButton->setText(i18n("Display as cut common time"));
		m_commonTimeButton->setEnabled(true);
		return;
	    }
	}
    }
    m_commonTimeButton->setEnabled(false);
}


TimeSignatureDialog::Location
TimeSignatureDialog::getLocation() const
{
    if (m_asGivenButton && m_asGivenButton->isChecked()) {
	return AsGiven;
    } else if (m_startOfBarButton && m_startOfBarButton->isChecked()) {
	return StartOfBar;
    }
    return AsGiven;
}

bool
TimeSignatureDialog::shouldNormalizeRests() const
{
    return (m_normalizeRestsButton && m_normalizeRestsButton->isEnabled() &&
	    m_normalizeRestsButton->isChecked());
}


KeySignatureDialog::KeySignatureDialog(QWidget *parent,
				       NotePixmapFactory *npf,
				       Rosegarden::Clef clef,
				       Rosegarden::Key defaultKey,
				       bool showApplyToAll,
				       bool showConversionOptions) :
    KDialogBase(parent, 0, true, i18n("Key Change"), Ok | Cancel),
    m_notePixmapFactory(npf),
    m_key(defaultKey),
    m_clef(clef),
    m_valid(true),
    m_ignoreComboChanges(false)
{
    QVBox *vbox = makeVBoxMainWidget();

    QHBox *keyBox = 0;
    QHBox *nameBox = 0;

    QGroupBox *keyFrame = new QGroupBox
	(1, Horizontal, i18n("Key signature"), vbox);

    QGroupBox *buttonFrame = new QButtonGroup
	(1, Horizontal, i18n("Scope"), vbox);
    
    QButtonGroup *conversionFrame = new QButtonGroup
	(1, Horizontal, i18n("Existing notes following key change"), vbox);
	
    keyBox = new QHBox(keyFrame);
    nameBox = new QHBox(keyFrame);
    
    BigArrowButton *keyDown = new BigArrowButton(keyBox, Qt::LeftArrow);
    QToolTip::add(keyDown, i18n("Flatten"));

    m_keyLabel = new QLabel(i18n("Key"), keyBox);
    m_keyLabel->setAlignment(AlignVCenter | AlignHCenter);

    BigArrowButton *keyUp = new BigArrowButton(keyBox, Qt::RightArrow);
    QToolTip::add(keyUp, i18n("Sharpen"));

    m_keyCombo = new QComboBox(true, nameBox);
    m_majorMinorCombo = new QComboBox(false, nameBox);
    m_majorMinorCombo->insertItem("Major");
    m_majorMinorCombo->insertItem("Minor");
    if (m_key.isMinor()) {
	m_majorMinorCombo->setCurrentItem(m_majorMinorCombo->count() - 1);
    }

    regenerateKeyCombo();
    redrawKeyPixmap();

    m_keyLabel->setMinimumWidth(m_keyLabel->pixmap()->width());
    m_keyLabel->setMinimumHeight(m_keyLabel->pixmap()->height());

    if (showApplyToAll) {
	QRadioButton *applyToOneButton =
	    new QRadioButton(i18n("Apply to current segment only"),
			     buttonFrame);
	m_applyToAllButton =
	    new QRadioButton(i18n("Apply to all segments at this time"),
			     buttonFrame);
	applyToOneButton->setChecked(true);
    } else {
	m_applyToAllButton = 0;
	buttonFrame->hide();
    }
    
    if (showConversionOptions) {
	m_noConversionButton =
	    new QRadioButton
	    (i18n("Maintain current pitches"), conversionFrame);
	m_convertButton =
	    new QRadioButton
	    (i18n("Maintain current accidentals"), conversionFrame);
	m_transposeButton =
	    new QRadioButton
	    (i18n("Transpose into this key"), conversionFrame);
	m_noConversionButton->setChecked(true);
    } else {
	m_noConversionButton = 0;
	m_convertButton = 0;
	m_transposeButton = 0;
	conversionFrame->hide();
    }
    
    QObject::connect(keyUp, SIGNAL(clicked()), this, SLOT(slotKeyUp()));
    QObject::connect(keyDown, SIGNAL(clicked()), this, SLOT(slotKeyDown()));
    QObject::connect(m_keyCombo, SIGNAL(activated(const QString &)),
		     this, SLOT(slotKeyNameChanged(const QString &)));
    QObject::connect(m_keyCombo, SIGNAL(textChanged(const QString &)),
		     this, SLOT(slotKeyNameChanged(const QString &)));
    QObject::connect(m_majorMinorCombo, SIGNAL(activated(const QString &)),
		     this, SLOT(slotMajorMinorChanged(const QString &)));
}

KeySignatureDialog::ConversionType
KeySignatureDialog::getConversionType() const
{
    if (m_noConversionButton && m_noConversionButton->isChecked()) {
	return NoConversion;
    } else if (m_convertButton && m_convertButton->isChecked()) {
	return Convert;
    } else if (m_transposeButton && m_transposeButton->isChecked()) {
	return Transpose;
    }
    return NoConversion;
}

bool
KeySignatureDialog::shouldApplyToAll() const
{
    return m_applyToAllButton && m_applyToAllButton->isChecked();
}

void
KeySignatureDialog::slotKeyUp()
{
    bool sharp = m_key.isSharp();
    int ac = m_key.getAccidentalCount();
    if (sharp) {
	if (++ac > 7) ac = 7;
    } else {
	if (--ac < 1) {
	    ac = 0;
	    sharp = true;
	}
    }
    
    try {
	m_key = Rosegarden::Key(ac, sharp, m_key.isMinor());
	setValid(true);
    } catch (Rosegarden::Key::BadKeySpec) {
	setValid(false);
    }

    regenerateKeyCombo();
    redrawKeyPixmap();
}

void
KeySignatureDialog::slotKeyDown()
{
    bool sharp = m_key.isSharp();
    int ac = m_key.getAccidentalCount();
    if (sharp) {
	if (--ac < 0) {
	    ac = 1;
	    sharp = false;
	}
    } else {
	if (++ac > 7) ac = 7;
    }
    
    try {
	m_key = Rosegarden::Key(ac, sharp, m_key.isMinor());
	setValid(true);
    } catch (Rosegarden::Key::BadKeySpec) {
	setValid(false);
    }

    regenerateKeyCombo();
    redrawKeyPixmap();
}

struct KeyNameComparator
{
    bool operator()(const Rosegarden::Key &k1, const Rosegarden::Key &k2) {
	return (k1.getName() < k2.getName());
    }
};

void
KeySignatureDialog::regenerateKeyCombo()
{
    m_ignoreComboChanges = true;
    QString currentText = m_keyCombo->currentText();
    Rosegarden::Key::KeySet keys(Rosegarden::Key::getKeys(m_key.isMinor()));
    m_keyCombo->clear();

    std::sort(keys.begin(), keys.end(), KeyNameComparator());
    bool textSet = false;

    for (Rosegarden::Key::KeySet::iterator i = keys.begin();
	 i != keys.end(); ++i) {

	QString name(strtoqstr(i->getName()));
	int space = name.find(' ');
	if (space > 0) name = name.left(space);

	m_keyCombo->insertItem(name);

	if (m_valid && (*i == m_key)) {
	    m_keyCombo->setCurrentItem(m_keyCombo->count() - 1);
	    textSet = true;
	}
    }

    if (!textSet) {
	m_keyCombo->setEditText(currentText);
    }
    m_ignoreComboChanges = false;
}

bool
KeySignatureDialog::isValid() const
{
    return m_valid;
}

Rosegarden::Key
KeySignatureDialog::getKey() const
{
    return m_key;
}

void
KeySignatureDialog::redrawKeyPixmap()
{
    if (m_valid) {
	QCanvasPixmap pmap =
	    m_notePixmapFactory->makeKeyDisplayPixmap(m_key, m_clef);
	m_keyLabel->setPixmap(pmap);
    } else {
	m_keyLabel->setText(i18n("No such key"));
    }
}

void
KeySignatureDialog::slotKeyNameChanged(const QString &s)
{
    if (m_ignoreComboChanges) return;

    std::string name(getKeyName(s, m_key.isMinor()));
    
    try {
	m_key = Rosegarden::Key(name);
	setValid(true);

	int space = name.find(' ');
	if (space > 0) name = name.substr(0, space);
	m_keyCombo->setEditText(strtoqstr(name));

    } catch (Rosegarden::Key::BadKeyName) {
	setValid(false);
    }

    redrawKeyPixmap();
}

void
KeySignatureDialog::slotMajorMinorChanged(const QString &s)
{
    if (m_ignoreComboChanges) return;

    std::string name(getKeyName(m_keyCombo->currentText(), s == "Minor"));

    try {
	m_key = Rosegarden::Key(name);
	setValid(true);
    } catch (Rosegarden::Key::BadKeyName) {
	setValid(false);
    }

    regenerateKeyCombo();
    redrawKeyPixmap();
}

void
KeySignatureDialog::setValid(bool valid)
{
    m_valid = valid;
    enableButton(Ok, m_valid);
}

std::string
KeySignatureDialog::getKeyName(const QString &s, bool minor)
{
    QString u((s.length() >= 1) ? (s.left(1).upper() + s.right(s.length() - 1))
			        :  s);
    
    std::string name(qstrtostr(u));
    name = name + " " + (minor ? "minor" : "major");
    return name;
}


PasteNotationDialog::PasteNotationDialog(QWidget *parent,
					 PasteEventsCommand::PasteType defaultType) :
    KDialogBase(parent, 0, true, i18n("Paste"), Ok | Cancel),
    m_defaultType(defaultType)
{
    QVBox *vbox = makeVBoxMainWidget();

    QButtonGroup *pasteTypeGroup = new QButtonGroup
	(1, Horizontal, i18n("Paste type"), vbox);

    m_restrictedButton = new QRadioButton
	(i18n("Paste into an existing gap [\"restricted\"]"), pasteTypeGroup);
    if (m_defaultType == PasteEventsCommand::Restricted) {
	m_restrictedButton->setChecked(true);
    }
    m_simpleButton = new QRadioButton
	(i18n("Erase existing events to make room [\"simple\"]"), pasteTypeGroup);
    if (m_defaultType == PasteEventsCommand::Simple) {
	m_simpleButton->setChecked(true);
    }
    m_openAndPasteButton = new QRadioButton
	(i18n("Move existing events out of the way [\"open-n-paste\"]"), pasteTypeGroup);
    if (m_defaultType == PasteEventsCommand::OpenAndPaste) {
	m_openAndPasteButton->setChecked(true);
    }
    m_noteOverlayButton = new QRadioButton
	(i18n("Overlay notes, tying against present notes [\"note-overlay\"]"), pasteTypeGroup);
    if (m_defaultType == PasteEventsCommand::NoteOverlay) {
	m_noteOverlayButton->setChecked(true);
    }
    m_matrixOverlayButton = new QRadioButton
	(i18n("Overlay notes, ignoring present notes [\"matrix-overlay\"]"), pasteTypeGroup);
    if (m_defaultType == PasteEventsCommand::MatrixOverlay) {
	m_matrixOverlayButton->setChecked(true);
    }

    QButtonGroup *setAsDefaultGroup = new QButtonGroup
	(1, Horizontal, i18n("Options"), vbox);

    m_setAsDefaultButton = new QCheckBox
	(i18n("Make this the default paste type"), setAsDefaultGroup);
    m_setAsDefaultButton->setChecked(true);

    QObject::connect(m_restrictedButton, SIGNAL(clicked()),
		     this, SLOT(slotPasteTypeChanged()));
    QObject::connect(m_simpleButton, SIGNAL(clicked()),
		     this, SLOT(slotPasteTypeChanged()));
    QObject::connect(m_openAndPasteButton, SIGNAL(clicked()),
		     this, SLOT(slotPasteTypeChanged()));
    QObject::connect(m_noteOverlayButton, SIGNAL(clicked()),
		     this, SLOT(slotPasteTypeChanged()));
    QObject::connect(m_matrixOverlayButton, SIGNAL(clicked()),
		     this, SLOT(slotPasteTypeChanged()));
}

PasteEventsCommand::PasteType
PasteNotationDialog::getPasteType() const
{
    if (m_restrictedButton->isChecked()) {
	return PasteEventsCommand::Restricted;
    } else if (m_simpleButton->isChecked()) {
	return PasteEventsCommand::Simple;
    } else if (m_noteOverlayButton->isChecked()) {
	return PasteEventsCommand::NoteOverlay;
    } else if (m_matrixOverlayButton->isChecked()) {
	return PasteEventsCommand::MatrixOverlay;
    } else {
	return PasteEventsCommand::OpenAndPaste;
    }
}

bool
PasteNotationDialog::setAsDefault() const
{
    return m_setAsDefaultButton->isChecked();
}

void
PasteNotationDialog::slotPasteTypeChanged()
{
    m_setAsDefaultButton->setChecked(m_defaultType == getPasteType());
}



TupletDialog::TupletDialog(QWidget *parent, Note::Type defaultUnitType,
			   timeT maxDuration) :
    KDialogBase(parent, 0, true, i18n("Tuplet"), Ok | Cancel),
    m_maxDuration(maxDuration)
{
    QVBox *vbox = makeVBoxMainWidget();

    QGroupBox *timingBox = new QGroupBox
	(1, Horizontal, i18n("New timing for tuplet group"), vbox);

    if (m_maxDuration > 0) {

	// bit of a sanity check
	if (maxDuration < Note(Note::Semiquaver).getDuration()) {
	    maxDuration = Note(Note::Semiquaver).getDuration();
	}

	Note::Type maxUnitType =
	    Note::getNearestNote(maxDuration/2, 0).getNoteType();
	if (defaultUnitType > maxUnitType) defaultUnitType = maxUnitType;
    }

    QGrid *timingGrid = new QGrid(3, QGrid::Horizontal, timingBox);

    new QLabel(i18n("Play "), timingGrid);
    m_untupledCombo = new QComboBox(true, timingGrid);

    m_unitCombo = new QComboBox(false, timingGrid);
    NotePixmapFactory npf;

    for (Note::Type t = Note::Shortest; t <= Note::Longest; ++t) {
	Note note(t);
	if (maxDuration > 0 && (2 * note.getDuration() > maxDuration)) break;
	QPixmap pmap = npf.makeToolbarPixmap
	    (strtoqstr((std::string("menu-") + note.getReferenceName())));
	m_unitCombo->insertItem(pmap, strtoqstr(note.getEnglishName() + "s"));
	if (defaultUnitType == t) {
	    m_unitCombo->setCurrentItem(m_unitCombo->count() - 1);
	}
    }
    
    updateUntupledCombo();

    new QLabel(i18n("in the time of  "), timingGrid);
    m_tupledCombo = new QComboBox(true, timingGrid);
    updateTupledCombo();

    QGroupBox *timingDisplayBox = new QGroupBox
	(1, Horizontal, i18n("Timing calculations"), vbox);

    QGrid *timingDisplayGrid = new QGrid(3, QGrid::Horizontal, timingDisplayBox);

    if (maxDuration > 0) {

	new QLabel(i18n("Selected region:"), timingDisplayGrid);
	new QLabel("", timingDisplayGrid);
	m_selectionDurationDisplay = new QLabel("x", timingDisplayGrid);
	m_selectionDurationDisplay->setAlignment(int(QLabel::AlignVCenter |
						     QLabel::AlignRight));
    } else {
	m_selectionDurationDisplay = 0;
    }
    
    new QLabel(i18n("Group with current timing:"), timingDisplayGrid);
    m_untupledDurationCalculationDisplay = new QLabel("x", timingDisplayGrid);
    m_untupledDurationDisplay = new QLabel("x", timingDisplayGrid);
    m_untupledDurationDisplay->setAlignment(int(QLabel::AlignVCenter |
						QLabel::AlignRight));

    new QLabel(i18n("Group with new timing:"), timingDisplayGrid);
    m_tupledDurationCalculationDisplay = new QLabel("x", timingDisplayGrid);
    m_tupledDurationDisplay = new QLabel("x", timingDisplayGrid);
    m_tupledDurationDisplay->setAlignment(int(QLabel::AlignVCenter |
					      QLabel::AlignRight));

    new QLabel(i18n("Gap created by timing change:"), timingDisplayGrid);
    m_newGapDurationCalculationDisplay = new QLabel("x", timingDisplayGrid);
    m_newGapDurationDisplay = new QLabel("x", timingDisplayGrid);
    m_newGapDurationDisplay->setAlignment(int(QLabel::AlignVCenter |
					      QLabel::AlignRight));

    if (maxDuration > 0) {

	new QLabel(i18n("Unchanged at end of selection:"), timingDisplayGrid);
	m_unchangedDurationCalculationDisplay = new QLabel
	    ("x", timingDisplayGrid);
	m_unchangedDurationDisplay = new QLabel("x", timingDisplayGrid);
	m_unchangedDurationDisplay->setAlignment(int(QLabel::AlignVCenter |
						     QLabel::AlignRight));

    } else {
	m_unchangedDurationDisplay = 0;
    }

    updateTimingDisplays();

    QObject::connect(m_unitCombo, SIGNAL(activated(const QString &)),
		     this, SLOT(slotUnitChanged(const QString &)));

    QObject::connect(m_untupledCombo, SIGNAL(activated(const QString &)),
		     this, SLOT(slotUntupledChanged(const QString &)));
    QObject::connect(m_untupledCombo, SIGNAL(textChanged(const QString &)),
		     this, SLOT(slotUntupledChanged(const QString &)));

    QObject::connect(m_tupledCombo, SIGNAL(activated(const QString &)),
		     this, SLOT(slotTupledChanged(const QString &)));
    QObject::connect(m_tupledCombo, SIGNAL(textChanged(const QString &)),
		     this, SLOT(slotTupledChanged(const QString &)));
}

Note::Type
TupletDialog::getUnitType() const
{
    return Note::Shortest + m_unitCombo->currentItem();
}

int
TupletDialog::getUntupledCount() const
{
    bool isNumeric = true;
    int count = m_untupledCombo->currentText().toInt(&isNumeric);
    if (count == 0 || !isNumeric) return 1;
    else return count;
}

int
TupletDialog::getTupledCount() const
{
    bool isNumeric = true;
    int count = m_tupledCombo->currentText().toInt(&isNumeric);
    if (count == 0 || !isNumeric) return 1;
    else return count;
}



void
TupletDialog::updateUntupledCombo()
{
    // Untupled combo can contain numbers up to the maximum
    // duration divided by the unit duration.  If there's no
    // maximum, we'll have to put in some likely values and
    // allow the user to edit it.  Both the numerical combos
    // should possibly be spinboxes, except I think I like
    // being able to "suggest" a few values

    int maxValue = 12;

    if (m_maxDuration) {
	maxValue = m_maxDuration / Note(getUnitType()).getDuration();
    }

    QString previousText = m_untupledCombo->currentText();
    if (previousText.toInt() == 0) {
	if (maxValue < 3) previousText = QString("%1").arg(maxValue);
	else previousText = "3";
    }

    m_untupledCombo->clear();
    bool setText = false;

    for (int i = 1; i <= maxValue; ++i) {
	QString text = QString("%1").arg(i);
	m_untupledCombo->insertItem(text);
	if (text == previousText) {
	    m_untupledCombo->setCurrentItem(m_untupledCombo->count() - 1);
	    setText = true;
	}
    }

    if (!setText) {
	m_untupledCombo->setEditText(previousText);
    }
}

void
TupletDialog::updateTupledCombo()
{
    // should contain all positive integers less than the
    // largest value in the untupled combo.  In principle
    // we can support values larger, but we can't quite
    // do the tupleting transformation yet

    int untupled = getUntupledCount();

    QString previousText = m_tupledCombo->currentText();
    if (previousText.toInt() == 0 ||
	previousText.toInt() > untupled) {
	if (untupled < 2) previousText = QString("%1").arg(untupled);
	else previousText = "2";
    }

    m_tupledCombo->clear();

    for (int i = 1; i < untupled; ++i) {
	QString text = QString("%1").arg(i);
	m_tupledCombo->insertItem(text);
	if (text == previousText) {
	    m_tupledCombo->setCurrentItem(m_tupledCombo->count() - 1);
	}
    }
}

void
TupletDialog::updateTimingDisplays()
{
    timeT unitDuration = Note(getUnitType()).getDuration();

    int untupledCount = getUntupledCount();
    int tupledCount = getTupledCount();

    timeT untupledDuration = unitDuration * untupledCount;
    timeT tupledDuration = unitDuration * tupledCount;

    if (m_selectionDurationDisplay) {
	m_selectionDurationDisplay->setText(QString("%1").arg(m_maxDuration));
    }

    m_untupledDurationCalculationDisplay->setText
	(QString("  %1 x %2 = ").arg(untupledCount).arg(unitDuration));
    m_untupledDurationDisplay->setText
	(QString("%1").arg(untupledDuration));

    m_tupledDurationCalculationDisplay->setText
	(QString("  %1 x %2 = ").arg(tupledCount).arg(unitDuration));
    m_tupledDurationDisplay->setText
	(QString("%1").arg(tupledDuration));

    m_newGapDurationCalculationDisplay->setText
	(QString("  %1 - %2 = ").arg(untupledDuration).arg(tupledDuration));
    m_newGapDurationDisplay->setText
	(QString("%1").arg(untupledDuration - tupledDuration));

    if (m_selectionDurationDisplay && m_unchangedDurationDisplay) {
	if (m_maxDuration != untupledDuration) {
	    m_unchangedDurationCalculationDisplay->setText
		(QString("  %1 - %2 = ").arg(m_maxDuration).arg(untupledDuration));
	} else {
	    m_unchangedDurationCalculationDisplay->setText("");
	}
	m_unchangedDurationDisplay->setText
	    (QString("%1").arg(m_maxDuration - untupledDuration));
    }
}

void
TupletDialog::slotUnitChanged(const QString &)
{
    updateUntupledCombo();
    updateTupledCombo();
    updateTimingDisplays();
}

void
TupletDialog::slotUntupledChanged(const QString &)
{
    updateTupledCombo();
    updateTimingDisplays();
}

void
TupletDialog::slotTupledChanged(const QString &)
{
    updateTimingDisplays();
}



TextEventDialog::TextEventDialog(QWidget *parent,
				 NotePixmapFactory *npf,
				 Rosegarden::Text defaultText,
				 int maxLength) :
    KDialogBase(parent, 0, true, i18n("Text"), Ok | Cancel),
    m_notePixmapFactory(npf),
    m_styles(Rosegarden::Text::getUserStyles())
{
    QVBox *vbox = makeVBoxMainWidget();

    QGroupBox *entryBox = new QGroupBox
	(1, Horizontal, i18n("Specification"), vbox);
    QGroupBox *exampleBox = new QGroupBox
	(1, Horizontal, i18n("Preview"), vbox);

    QGrid *entryGrid = new QGrid(2, QGrid::Horizontal, entryBox);

    new QLabel(i18n("Text:  "), entryGrid);
    m_text = new QLineEdit(entryGrid);
    m_text->setText(strtoqstr(defaultText.getText()));
    if (maxLength > 0) m_text->setMaxLength(maxLength);

    new QLabel(i18n("Style:  "), entryGrid);
    m_typeCombo = new QComboBox(false, entryGrid);

    for (unsigned int i = 0; i < m_styles.size(); ++i) {

	std::string style = m_styles[i];

	std::string styleName;
	styleName += (char)toupper(style[0]);
	styleName += style.substr(1);

	int uindex = styleName.find('_');
	if (uindex > 0) {
	    styleName =
		styleName.substr(0, uindex) + " " +
		styleName.substr(uindex + 1);
	}

	m_typeCombo->insertItem(strtoqstr(styleName));

	if (style == defaultText.getTextType()) {
	    m_typeCombo->setCurrentItem(m_typeCombo->count() - 1);
	}
    }

    QVBox *exampleVBox = new QVBox(exampleBox);
    
    int ls = m_notePixmapFactory->getLineSpacing();

    int mapWidth = 200;
    QPixmap map(mapWidth, ls * 5 + 1);
    QBitmap mask(mapWidth, ls * 5 + 1);

    map.fill();
    mask.fill(Qt::color0);

    QPainter p, pm;

    p.begin(&map);
    pm.begin(&mask);

    p.setPen(Qt::black);
    pm.setPen(Qt::white);
    
    for (int i = 0; i < 5; ++i) {
	p.drawLine(0, ls * i, mapWidth-1, ls * i);
	pm.drawLine(0, ls * i, mapWidth-1, ls * i);
    }

    p.end();
    pm.end();

    map.setMask(mask);

    m_staffAboveLabel = new QLabel("staff", exampleVBox);
    m_staffAboveLabel->setPixmap(map);

    m_textExampleLabel = new QLabel(i18n("Example"), exampleVBox);

    m_staffBelowLabel = new QLabel("staff", exampleVBox);
    m_staffBelowLabel->setPixmap(map);

    QObject::connect(m_text, SIGNAL(textChanged(const QString &)),
		     this, SLOT(slotTextChanged(const QString &)));
    QObject::connect(m_typeCombo, SIGNAL(activated(const QString &)),
		     this, SLOT(slotTypeChanged(const QString &)));

    m_text->setFocus();
    slotTypeChanged(strtoqstr(getTextType()));
}

std::string
TextEventDialog::getTextType() const
{
    return m_styles[m_typeCombo->currentItem()];
}

std::string
TextEventDialog::getTextString() const
{
    return std::string(qstrtostr(m_text->text()));
}

void
TextEventDialog::slotTextChanged(const QString &qtext)
{
    std::string type(getTextType());
    std::string text(qstrtostr(qtext));

    if (text == "") text = "Sample";
    if (text.length() > 20) {
	text = text.substr(0, 20) + "...";
    }

    Rosegarden::Text rtext(text, type);
    m_textExampleLabel->setPixmap(m_notePixmapFactory->makeTextPixmap(rtext));
}

void
TextEventDialog::slotTypeChanged(const QString &)
{
    std::string type(getTextType());
    std::string text(getTextString());

    if (text == "") text = "Sample";
    if (text.length() > 20) {
	text = text.substr(0, 20) + "...";
    }

    Rosegarden::Text rtext(text, type);
    m_textExampleLabel->setPixmap(m_notePixmapFactory->makeTextPixmap(rtext));

    if (type == Rosegarden::Text::Dynamic ||
	type == Rosegarden::Text::LocalDirection ||
	type == Rosegarden::Text::UnspecifiedType ||
	type == Rosegarden::Text::Lyric) {

	m_staffAboveLabel->show();
	m_staffBelowLabel->hide();

    } else {

	m_staffAboveLabel->hide();
	m_staffBelowLabel->show();
    }
}


EventEditDialog::EventEditDialog(QWidget *parent,
				 NotePixmapFactory *npf,
				 const Rosegarden::Event &event,
				 bool editable) :
    KDialogBase(parent, 0, true, i18n(editable ? "Edit Event" : "View Event"),
		(editable ? (Ok | Cancel) : Ok)),
    m_notePixmapFactory(npf),
    m_originalEvent(event),
    m_event(event),
    m_type(event.getType()),
    m_absoluteTime(event.getAbsoluteTime()),
    m_duration(event.getDuration()),
    m_subOrdering(event.getSubOrdering()),
    m_modified(false)
{
    QVBox *vbox = makeVBoxMainWidget();

    QGroupBox *intrinsicBox = new QGroupBox
	(1, Horizontal, i18n("Intrinsics"), vbox);

    QGrid *intrinsicGrid = new QGrid(4, QGrid::Horizontal, intrinsicBox);

    new QLabel(i18n("Event type: "), intrinsicGrid);
    new QLabel("", intrinsicGrid);
    new QLabel("", intrinsicGrid);
    QLineEdit *lineEdit = new QLineEdit(intrinsicGrid);
    lineEdit->setText(strtoqstr(event.getType()));

    new QLabel(i18n("Absolute time: "), intrinsicGrid);
    new QLabel("", intrinsicGrid);
    new QLabel("", intrinsicGrid);
    QSpinBox *absoluteTime = new QSpinBox
	(INT_MIN, INT_MAX, Note(Note::Shortest).getDuration(), intrinsicGrid);
    absoluteTime->setValue(event.getAbsoluteTime());
    QObject::connect(absoluteTime, SIGNAL(valueChanged(int)),
		     this, SLOT(slotAbsoluteTimeChanged(int)));
    slotAbsoluteTimeChanged(event.getAbsoluteTime());

    new QLabel(i18n("Duration: "), intrinsicGrid);
    m_durationDisplay = new QLabel("(note)", intrinsicGrid);
    m_durationDisplay->setMinimumWidth(20);
    m_durationDisplayAux = new QLabel("(note)", intrinsicGrid);
    m_durationDisplayAux->setMinimumWidth(20);

    QSpinBox *duration = new QSpinBox
	(0, INT_MAX, Note(Note::Shortest).getDuration(), intrinsicGrid);
    duration->setValue(event.getDuration());
    QObject::connect(duration, SIGNAL(valueChanged(int)),
		     this, SLOT(slotDurationChanged(int)));
    slotDurationChanged(event.getDuration());

    new QLabel(i18n("Sub-ordering: "), intrinsicGrid);
    new QLabel("", intrinsicGrid);
    new QLabel("", intrinsicGrid);
    
    QSpinBox *subOrdering = new QSpinBox(-100, 100, 1, intrinsicGrid);
    subOrdering->setValue(event.getSubOrdering());
    QObject::connect(subOrdering, SIGNAL(valueChanged(int)),
		     this, SLOT(slotSubOrderingChanged(int)));
    slotSubOrderingChanged(event.getSubOrdering());

    QGroupBox *persistentBox = new QGroupBox
	(1, Horizontal, i18n("Persistent properties"), vbox);
    m_persistentGrid = new QGrid(4, QGrid::Horizontal, persistentBox);

    QLabel *label = new QLabel(i18n("Name"), m_persistentGrid);
    QFont font(label->font());
    font.setItalic(true);
    label->setFont(font);
    
    label = new QLabel(i18n("Type"), m_persistentGrid);
    label->setFont(font);
    label = new QLabel(i18n("Value"), m_persistentGrid);
    label->setFont(font);
    label = new QLabel("", m_persistentGrid);
    label->setFont(font);

    Rosegarden::Event::PropertyNames p = event.getPersistentPropertyNames();

    for (Rosegarden::Event::PropertyNames::iterator i = p.begin();
	 i != p.end(); ++i) {
	addPersistentProperty(*i);
    }

    QGroupBox *nonPersistentBox = new QGroupBox
	(1, Horizontal, i18n("Non-persistent properties"), vbox);
    new QLabel(i18n("These are cached values, lost if the event is modified."),
	       nonPersistentBox);

    m_nonPersistentGrid = new QGrid
	(4, QGrid::Horizontal, nonPersistentBox);
    m_nonPersistentGrid->setSpacing(4);

    label = new QLabel(i18n("Name       "), m_nonPersistentGrid);
    label->setFont(font);
    label = new QLabel(i18n("Type       "), m_nonPersistentGrid);
    label->setFont(font);
    label = new QLabel(i18n("Value      "), m_nonPersistentGrid);
    label->setFont(font);
    label = new QLabel("", m_nonPersistentGrid);
    label->setFont(font);

    p = event.getNonPersistentPropertyNames();

    for (Rosegarden::Event::PropertyNames::iterator i = p.begin();
	 i != p.end(); ++i) {

	new QLabel(strtoqstr(*i), m_nonPersistentGrid, strtoqstr(*i));
	new QLabel(strtoqstr(event.getPropertyTypeAsString(*i)), m_nonPersistentGrid, strtoqstr(*i));
	new QLabel(strtoqstr(event.getAsString(*i)), m_nonPersistentGrid, strtoqstr(*i));
	QPushButton *button = new QPushButton("P", m_nonPersistentGrid, strtoqstr(*i));
	button->setFixedSize(QSize(24, 24));
	QToolTip::add(button, i18n("Make persistent"));
	QObject::connect(button, SIGNAL(clicked()),
			 this, SLOT(slotPropertyMadePersistent()));
    }
}


void
EventEditDialog::addPersistentProperty(const Rosegarden::PropertyName &name)
{
    QLabel *label = new QLabel(strtoqstr(name), m_persistentGrid, strtoqstr(name));
    label->show();
    label = new QLabel(strtoqstr(m_originalEvent.getPropertyTypeAsString(name)),
		       m_persistentGrid, strtoqstr(name));
    label->show();

    Rosegarden::PropertyType type(m_originalEvent.getPropertyType(name));
    switch (type) {
	
    case Rosegarden::Int:
    {
	QSpinBox *spinBox = new QSpinBox
	    (INT_MIN, INT_MAX, 1, m_persistentGrid, strtoqstr(name));
	spinBox->setValue(m_originalEvent.get<Rosegarden::Int>(name));
	QObject::connect(spinBox, SIGNAL(valueChanged(int)),
			 this, SLOT(slotIntPropertyChanged(int)));
	spinBox->show();
	break;
    }
    
    case Rosegarden::Bool:
    {
	QCheckBox *checkBox = new QCheckBox
	    ("", m_persistentGrid, strtoqstr(name));
	checkBox->setChecked(m_originalEvent.get<Rosegarden::Bool>(name));
	QObject::connect(checkBox, SIGNAL(clicked()),
			 this, SLOT(slotBoolPropertyChanged()));
	checkBox->show();
	break;
    }
    
    case Rosegarden::String:
    {
	QLineEdit *lineEdit = new QLineEdit
	    (strtoqstr(m_originalEvent.get<Rosegarden::String>(name)),
	     m_persistentGrid,
	     strtoqstr(name));
	QObject::connect(lineEdit, SIGNAL(textChanged(const QString &)),
			 this, SLOT(slotStringPropertyChanged(const QString &)));
	lineEdit->show();
	break;
    }
    }
    
    QPushButton *button = new QPushButton("X", m_persistentGrid,
					  strtoqstr(name));
    button->setFixedSize(QSize(24, 24));
    QToolTip::add(button, i18n("Delete this property"));
    QObject::connect(button, SIGNAL(clicked()),
		     this, SLOT(slotPropertyDeleted()));
    button->show();
}


Rosegarden::Event
EventEditDialog::getEvent() const
{
    return Rosegarden::Event(m_event, m_absoluteTime, m_duration, m_subOrdering);
}


void
EventEditDialog::slotEventTypeChanged(const QString &type)
{
    std::string t(qstrtostr(type));
    if (t != m_type) {
	m_modified = true;
	m_type = t;
    }
}

void
EventEditDialog::slotAbsoluteTimeChanged(int value)
{
    if (value == m_absoluteTime) return;
    m_modified = true;
    m_absoluteTime = value;
}

void
EventEditDialog::slotDurationChanged(int value)
{
    Note nearestNote = Note::getNearestNote(timeT(value), 1);
    std::string noteName = nearestNote.getReferenceName();
    noteName = "menu-" + noteName;
    QPixmap map = m_notePixmapFactory->makeToolbarPixmap(strtoqstr(noteName));

    m_durationDisplay->setPixmap(map);

    timeT nearestDuration = nearestNote.getDuration();
    if (timeT(value) >= nearestDuration * 2) {
	m_durationDisplayAux->setText("++ ");
    } else if (timeT(value) > nearestDuration) {
	m_durationDisplayAux->setText("+ ");
    } else if (timeT(value) < nearestDuration) {
	m_durationDisplayAux->setText("- ");
    } else {
	m_durationDisplayAux->setText(" ");
    }

    if (value == m_duration) return;

    m_modified = true;
    m_duration = value;
}

void
EventEditDialog::slotSubOrderingChanged(int value)
{
    if (value == m_subOrdering) return;
    m_modified = true;
    m_subOrdering = value;
}

void
EventEditDialog::slotIntPropertyChanged(int value) 
{
    const QObject *s = sender();
    const QSpinBox *spinBox = dynamic_cast<const QSpinBox *>(s);
    if (!spinBox) return;

    m_modified = true;
    QString propertyName = spinBox->name();
    m_event.set<Rosegarden::Int>(qstrtostr(propertyName), value);
}

void
EventEditDialog::slotBoolPropertyChanged()
{ 
    const QObject *s = sender();
    const QCheckBox *checkBox = dynamic_cast<const QCheckBox *>(s);
    if (!checkBox) return;

    m_modified = true;
    QString propertyName = checkBox->name();
    bool checked = checkBox->isChecked();

    m_event.set<Rosegarden::Bool>(qstrtostr(propertyName), checked);
}

void
EventEditDialog::slotStringPropertyChanged(const QString &value)
{
    const QObject *s = sender();
    const QLineEdit *lineEdit = dynamic_cast<const QLineEdit *>(s);
    if (!lineEdit) return;
    
    m_modified = true;
    QString propertyName = lineEdit->name();
    m_event.set<Rosegarden::String>(qstrtostr(propertyName), qstrtostr(value));
}

void
EventEditDialog::slotPropertyDeleted()
{
    const QObject *s = sender();
    const QPushButton *pushButton = dynamic_cast<const QPushButton *>(s);
    if (!pushButton) return;

    QString propertyName = pushButton->name();

    if (QMessageBox::warning
	(this, i18n("Edit Event"),
	 i18n("Are you sure you want to delete the \"%1\" property?\n\n"
	      "Removing necessary properties may cause unexpected behaviour.").
	 arg(propertyName),
	 i18n("&Delete"), i18n("&Cancel"), 0, 1) != 0) return;

    m_modified = true;
    QObjectList *list = m_persistentGrid->queryList(0, propertyName, false);
    QObjectListIt i(*list);
    QObject *obj;
    while ((obj = i.current()) != 0) {
	++i;
	delete obj;
    }
    delete list;
    
    m_event.unset(qstrtostr(propertyName));
}

void
EventEditDialog::slotPropertyMadePersistent()
{
    const QObject *s = sender();
    const QPushButton *pushButton = dynamic_cast<const QPushButton *>(s);
    if (!pushButton) return;

    QString propertyName = pushButton->name();

    if (QMessageBox::warning
	(this, i18n("Edit Event"),
	 i18n("Are you sure you want to make the \"%1\" property persistent?\n\n"
	      "This could cause problems if it overrides a different "
	      "computed value later on.").
	 arg(propertyName),
	 i18n("Make &Persistent"), i18n("&Cancel"), 0, 1) != 0) return;

    QObjectList *list = m_nonPersistentGrid->queryList(0, propertyName, false);
    QObjectListIt i(*list);
    QObject *obj;
    while ((obj = i.current()) != 0) {
	++i;
	delete obj;
    }
    delete list;

    m_modified = true;
    addPersistentProperty(qstrtostr(propertyName));

    Rosegarden::PropertyType type =
	m_originalEvent.getPropertyType(qstrtostr(propertyName));

    switch (type) {

    case Rosegarden::Int:
	m_event.set<Rosegarden::Int>
	    (qstrtostr(propertyName),
	     m_originalEvent.get<Rosegarden::Int>
	     (qstrtostr(propertyName)));
	break;

    case Rosegarden::Bool:
	m_event.set<Rosegarden::Bool>
	    (qstrtostr(propertyName),
	     m_originalEvent.get<Rosegarden::Bool>
	     (qstrtostr(propertyName)));
	break;

    case Rosegarden::String:
	m_event.set<Rosegarden::String>
	    (qstrtostr(propertyName),
	     m_originalEvent.get<Rosegarden::String>
	     (qstrtostr(propertyName)));
	break;
    }
}

class TempoValidator : public QDoubleValidator
{
public:
    TempoValidator(QWidget *parent, const char *name = 0):
        QDoubleValidator(parent, name) {;}

    TempoValidator(double bottom, double top, int decimals,
                   QWidget *parent, const char *name = 0):
        QDoubleValidator(bottom, top, decimals, parent, name) {;}
    virtual ~TempoValidator() {;}

    /*
    virtual void fixup(QString &input) const
    {
        // do nothing for the moment
    }
    */

private:
};



TempoDialog::TempoDialog(QWidget *parent, RosegardenGUIDoc *doc):
    KDialogBase(parent, 0, true, i18n("Tempo"), Ok | Cancel),
    m_doc(doc),
    m_tempoTime(0),
    m_tempoValue(0.0)
{
    QVBox *vbox = makeVBoxMainWidget();
    QGroupBox *groupBox = new QGroupBox(3, Horizontal, i18n("Tempo"), vbox);
    //groupBox->setAlignment(AlignHCenter);

    // Set tempo
    new QLabel(i18n("New tempo"), groupBox);
    m_tempoValueSpinBox = new RosegardenSpinBox(groupBox);
    m_tempoValueSpinBox->setMinValue(1);
    m_tempoValueSpinBox->setMaxValue(1000);
    new QLabel(i18n("bpm"), groupBox);

    // create a validator
    TempoValidator *validator = new TempoValidator(1.0, 1000.0, 6, this);
    m_tempoValueSpinBox->setValidator(validator);

    // Scope Box
    QGroupBox *scopeBox = new QGroupBox(1, Horizontal,
                                        i18n("Scope"), vbox);
//    m_optionButtons->addSpace(10); // 10 pix space

    QHBox *currentBox = new QHBox(scopeBox);
    new QLabel(i18n("The pointer is currently at "), currentBox);
    m_tempoTimeLabel = new QLabel(currentBox);
    QHBox *currentBarBox = new QHBox(scopeBox);
    new QLabel(currentBarBox);
    m_tempoBarLabel = new QLabel(currentBarBox);

    m_optionButtons = new QVButtonGroup(scopeBox);

    // id == 0
    QRadioButton *tempoChangeHere = new QRadioButton
	(i18n("This tempo applies from here onwards"),
	 m_optionButtons);

    // id == 1
    m_tempoChangeBefore = new QRadioButton(m_optionButtons);
    m_tempoChangeBeforeAt = new QLabel(m_optionButtons);

    // id == 2
    m_tempoChangeGlobal = new QRadioButton(m_optionButtons);

    QHBox *optionHBox = new QHBox(m_optionButtons);
    new QLabel(optionHBox);
    m_defaultBox = new QCheckBox
	(i18n("Also make this the default for new segments"), optionHBox);
    new QLabel(optionHBox);

    // disable initially
    m_defaultBox->setDisabled(true);

    connect(m_optionButtons, SIGNAL(pressed(int)),
            SLOT(slotRadioButtonPressed(int)));

//    m_optionButtons->setButton(0);
    tempoChangeHere->setChecked(true);

    populateTempo();
}

TempoDialog::~TempoDialog()
{
}

void
TempoDialog::setTempoPosition(Rosegarden::timeT time)
{
    m_tempoTime = time;
    populateTempo();
}

// Using current m_tempoTime
//
void
TempoDialog::populateTempo()
{
    Rosegarden::Composition &comp = m_doc->getComposition();

    double tempo = comp.getTempoAt(m_tempoTime);
    QString tempoString;
    tempoString.sprintf("%4.6f", tempo);

    // We have to frig this by setting both integer tempo
    // and the special text field.
    //
    m_tempoValueSpinBox->setValue((int)tempo);
    m_tempoValueSpinBox->setSpecialValueText(tempoString);

    Rosegarden::RealTime tempoTime= comp.getElapsedRealTime(m_tempoTime);
    QString milliSeconds;
    milliSeconds.sprintf("%03ld", tempoTime.usec / 1000);
    m_tempoTimeLabel->setText(i18n("%1.%2 s").arg(tempoTime.sec)
			      .arg(milliSeconds));

    int barNo = comp.getBarNumber(m_tempoTime) + 1;
    if (comp.getBarStart(barNo) == m_tempoTime) {
	m_tempoBarLabel->setText
	    (i18n("(at the start of bar %1)").arg(barNo));
    } else {
	m_tempoBarLabel->setText(
	    i18n("(in the middle of bar %1)").arg(barNo));
    }

    int tempoChangeNo = comp.getTempoChangeNumberAt(m_tempoTime);
    if (tempoChangeNo >= 0) {

	m_tempoChangeBefore->setText
	    (i18n("This tempo replaces the last tempo change"));

	timeT lastTempoTime = comp.getRawTempoChange(tempoChangeNo).first;
	Rosegarden::RealTime lastRT = comp.getElapsedRealTime(lastTempoTime);
	QString lastms;
	lastms.sprintf("%03ld", lastRT.usec / 1000);
	int lastBar = comp.getBarNumber(lastTempoTime) + 1;
	m_tempoChangeBeforeAt->setText
	    (i18n("    (at %1.%2 s, in bar %3)").arg(lastRT.sec)
	     .arg(lastms).arg(lastBar));

	m_tempoChangeBefore->show();
	m_tempoChangeBeforeAt->show();

    } else {
	m_tempoChangeBefore->hide();
	m_tempoChangeBeforeAt->hide();
    }

    if (comp.getTempoChangeCount() > 0) {

	m_tempoChangeGlobal->setText
	    (i18n("This tempo applies to the whole segment"));
	m_tempoChangeGlobal->show();
	m_defaultBox->show();

    } else {
	m_tempoChangeGlobal->hide();
	m_defaultBox->hide();
    }
}

void
TempoDialog::slotOk()
{
    double tempoDouble = m_tempoValueSpinBox->getDoubleValue();

    // Check for freakiness in the returned results - 
    // if we can't believe the double value then use
    // the value on the SpinBox itself - we just have 
    // to lose the precision.
    //
    if ((int)tempoDouble != m_tempoValueSpinBox->value())
        tempoDouble = m_tempoValueSpinBox->value();

    TempoDialogAction action =
        (TempoDialogAction)m_optionButtons->id(m_optionButtons->selected());

    if (action == GlobalTempo && m_defaultBox->isChecked())
        action = GlobalTempoWithDefault;

    emit changeTempo(m_tempoTime,
                     tempoDouble,
                     action);
    delete this;
}


void
TempoDialog::slotRadioButtonPressed(int id)
{
    if (id == 2)
        m_defaultBox->setDisabled(false);
    else
        m_defaultBox->setDisabled(true);
}



ClefDialog::ClefDialog(QWidget *parent,
		       NotePixmapFactory *npf,
		       Rosegarden::Clef defaultClef,
		       bool showConversionOptions) :
    KDialogBase(parent, 0, true, i18n("Clef"), Ok | Cancel),
    m_notePixmapFactory(npf),
    m_clef(defaultClef)
{
    QVBox *vbox = makeVBoxMainWidget();

    QGroupBox *clefFrame = new QGroupBox
	(1, Horizontal, i18n("Clef"), vbox);
    
    QButtonGroup *conversionFrame = new QButtonGroup
	(1, Horizontal, i18n("Existing notes following clef change"), vbox);
	
    QHBox *clefBox = new QHBox(clefFrame);
    
    BigArrowButton *clefDown = new BigArrowButton(clefBox, Qt::LeftArrow);
    QToolTip::add(clefDown, i18n("Lower clef"));

    QHBox *clefLabelBox = new QVBox(clefBox);
    
    m_clefLabel = new QLabel(i18n("Clef"), clefLabelBox);
    m_clefLabel->setAlignment(AlignVCenter | AlignHCenter);

    m_clefNameLabel = new QLabel(i18n("Clef"), clefLabelBox);
    m_clefNameLabel->setAlignment(AlignVCenter | AlignHCenter);

    BigArrowButton *clefUp = new BigArrowButton(clefBox, Qt::RightArrow);
    QToolTip::add(clefUp, i18n("Higher clef"));
      
    if (showConversionOptions) {
	m_noConversionButton =
	    new QRadioButton
	    (i18n("Maintain current pitches"), conversionFrame);
	m_changeOctaveButton =
	    new QRadioButton
	    (i18n("Transpose into appropriate octave"), conversionFrame);
	m_transposeButton = 0;
//	m_transposeButton =
//	    new QRadioButton
//	    (i18n("Maintain current positions on the staff"), conversionFrame);
	m_changeOctaveButton->setChecked(true);
    } else {
	m_noConversionButton = 0;
	m_changeOctaveButton = 0;
	m_transposeButton = 0;
	conversionFrame->hide();
    }
    
    QObject::connect(clefUp, SIGNAL(clicked()), this, SLOT(slotClefUp()));
    QObject::connect(clefDown, SIGNAL(clicked()), this, SLOT(slotClefDown()));

    redrawClefPixmap();
}

Rosegarden::Clef
ClefDialog::getClef() const
{
    return m_clef;
}

ClefDialog::ConversionType
ClefDialog::getConversionType() const
{
    if (m_noConversionButton && m_noConversionButton->isChecked()) {
	return NoConversion;
    } else if (m_changeOctaveButton && m_changeOctaveButton->isChecked()) {
	return ChangeOctave;
    } else if (m_transposeButton && m_transposeButton->isChecked()) {
	return Transpose;
    }
    return NoConversion;
}

void
ClefDialog::slotClefUp()
{
    Rosegarden::Clef::ClefList clefs(Rosegarden::Clef::getClefs());

    for (Rosegarden::Clef::ClefList::iterator i = clefs.begin();
	 i != clefs.end(); ++i) {

	if (m_clef == *i) {
	    if (++i == clefs.end()) i = clefs.begin();
	    m_clef = *i;
	    break;
	}
    }

    redrawClefPixmap();
}


void
ClefDialog::slotClefDown()
{
    Rosegarden::Clef::ClefList clefs(Rosegarden::Clef::getClefs());

    for (Rosegarden::Clef::ClefList::iterator i = clefs.begin();
	 i != clefs.end(); ++i) {

	if (m_clef == *i) {
	    if (i == clefs.begin()) i = clefs.end();
	    m_clef = *--i;
	    break;
	}
    }

    redrawClefPixmap();
}

void
ClefDialog::redrawClefPixmap()
{
    QCanvasPixmap pmap = m_notePixmapFactory->makeClefDisplayPixmap(m_clef);
    m_clefLabel->setPixmap(pmap);
    QString name(strtoqstr(m_clef.getClefType()));
    name = name.left(1).upper() + name.right(name.length() - 1);
    m_clefNameLabel->setText(name);
}


QuantizeDialog::QuantizeDialog(QWidget *parent,
			       std::string source,
			       std::string target) :
    KDialogBase(parent, 0, true, i18n("Quantize"), Ok | Cancel),
    m_source(source),
    m_target(target),
    m_standardQuantizations
        (Rosegarden::StandardQuantization::getStandardQuantizations())
{
    QVBox *vbox = makeVBoxMainWidget();

    QGroupBox *quantizeBox = new QGroupBox
	(1, Horizontal, i18n("Basic quantization"), vbox);

    QHBox *typeBox = new QHBox(quantizeBox);
    new QLabel(i18n("Quantize:"), typeBox);
    m_typeCombo = new QComboBox(false, typeBox);
    m_typeCombo->insertItem(i18n("Event positions only"));
    m_typeCombo->insertItem(i18n("Event positions and durations"));
    m_typeCombo->insertItem(i18n("Event positions, and round durations to exact notes"));

    QHBox *unitBox = new QHBox(quantizeBox);
    new QLabel(i18n("Base duration unit:"), unitBox);
    m_unitCombo = new QComboBox(false, unitBox);

    NotePixmapFactory npf;
    QPixmap noMap = npf.makeToolbarPixmap("menu-no-note");

    for (unsigned int i = 0; i < m_standardQuantizations.size(); ++i) {
	std::string noteName = m_standardQuantizations[i].noteName;
	QPixmap pmap = noMap;
	if (noteName != "") {
	    noteName = "menu-" + noteName;
	    pmap = npf.makeToolbarPixmap(strtoqstr(noteName));
	}
	QString qname = strtoqstr(m_standardQuantizations[i].description);
	m_unitCombo->insertItem(pmap, qname);
	if (m_standardQuantizations[i].unit ==
	    Note(Note::Semiquaver).getDuration()) {
		m_unitCombo->setCurrentItem(m_unitCombo->count() - 1);
	}
    }
    
    m_noteQuantizeBox = new QGroupBox
	(1, Horizontal, i18n("Note rounding"), vbox);

    QHBox *dotsBox = new QHBox(m_noteQuantizeBox);
    new QLabel(i18n("Maximum number of dots:"), dotsBox);
    m_dotsCombo = new QComboBox(false, dotsBox);
    m_dotsCombo->insertItem("0");
    m_dotsCombo->insertItem("1");
    m_dotsCombo->insertItem("2");
    m_dotsCombo->insertItem("3");
    m_dotsCombo->setCurrentItem(2);

    m_legatoButton = new QCheckBox
	(i18n("Only round durations up if following rests permit"),
	 m_noteQuantizeBox);
    
    m_noteQuantizeBox->setEnabled(false);

    connect(m_typeCombo, SIGNAL(activated(int)), SLOT(slotTypeChanged(int)));
    connect(m_unitCombo, SIGNAL(activated(int)), SLOT(slotUnitChanged(int)));
    connect(m_dotsCombo, SIGNAL(activated(int)), SLOT(slotDotsChanged(int)));
    connect(m_legatoButton, SIGNAL(activated()), SLOT(slotLegatoChanged()));
}

Quantizer
QuantizeDialog::getQuantizer() const
{
    Quantizer::QuantizationType type;
    switch (m_typeCombo->currentItem()) {
    case 0: type = Quantizer::PositionQuantize; break;
    case 1: type = Quantizer::UnitQuantize; break;
    case 2: type = Quantizer::NoteQuantize; break;
    default: assert(0);
    }
    
    if (type == Quantizer::NoteQuantize) {
	if (m_legatoButton->isChecked()) {
	    type = Quantizer::LegatoQuantize;
	}
    }	

    int dots = m_dotsCombo->currentItem();

    Rosegarden::StandardQuantization quantization =
	m_standardQuantizations[m_unitCombo->currentItem()];

    return Quantizer(m_source, m_target, type, quantization.unit, dots);
}
	

void
QuantizeDialog::slotTypeChanged(int index)
{
    m_noteQuantizeBox->setEnabled(index == 2);
}

void
QuantizeDialog::slotUnitChanged(int)
{
    //...
}

void
QuantizeDialog::slotDotsChanged(int)
{
    //...
}

void
QuantizeDialog::slotLegatoChanged()
{
    //...
}

