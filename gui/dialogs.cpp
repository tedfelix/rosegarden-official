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
#include "rosedebug.h"

#include <qwidget.h>
#include <qlineedit.h>
#include <qlabel.h>
#include <qhbox.h>
#include <qvbox.h>
#include <qcombobox.h>
#include <qcheckbox.h>
#include <qgroupbox.h>
#include <qradiobutton.h>
#include <qbuttongroup.h>
#include <qlayout.h>
#include <qtooltip.h>

#include <klocale.h>
#include <karrowbutton.h>

using Rosegarden::TimeSignature;
using Rosegarden::Note;
using Rosegarden::timeT;


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
    return m_lineEdit->text().latin1();
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

    QObject::connect(numDown,   SIGNAL(pressed()), this, SLOT(slotNumDown()));
    QObject::connect(numUp,     SIGNAL(pressed()), this, SLOT(slotNumUp()));
    QObject::connect(denomDown, SIGNAL(pressed()), this, SLOT(slotDenomDown()));
    QObject::connect(denomUp,   SIGNAL(pressed()), this, SLOT(slotDenomUp()));

    groupBox = new QButtonGroup(1, Horizontal, i18n("Scope"), vbox);

    if (!m_atStartOfBar) {

	// let's forget about start-of-composition for the moment
	QString scopeText;
	if (m_barNo != 0 || !m_atStartOfBar) {
	    if (m_atStartOfBar) {
		scopeText = QString
		    (i18n("Insertion point is at start of bar %1."))
		    .arg(m_barNo);
	    } else {
		scopeText = QString
		    (i18n("Insertion point is in the middle of bar %1."))
		    .arg(m_barNo);
	    }
	} else {
	    scopeText = QString
		(i18n("Insertion point is at start of composition."));
	}
	
	new QLabel(scopeText, groupBox);
	m_asGivenButton = new QRadioButton
	    (i18n("Start bar %1 here").arg(barNo + 1), groupBox);

	if (!m_atStartOfBar) {
	    m_startOfBarButton = new QRadioButton
		(i18n("Change time from start of bar %1")
		 .arg(m_barNo), groupBox);
	    m_startOfBarButton->setChecked(true);
	} else {
	    m_asGivenButton->setChecked(true);
	}
    } else {
	new QLabel(i18n("Time change will take effect at the start of bar %1.")
		   .arg(barNo), groupBox);
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
    QObject::connect(m_hideSignatureButton, SIGNAL(released()), this,
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
    
    QObject::connect(keyUp, SIGNAL(pressed()), this, SLOT(slotKeyUp()));
    QObject::connect(keyDown, SIGNAL(pressed()), this, SLOT(slotKeyDown()));
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

	QString name(i->getName().c_str());
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
	m_keyCombo->setEditText(name.c_str());

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
    
    std::string name(u.latin1());
    name = name + " " + (minor ? "minor" : "major");
    return name;
}


PasteNotationDialog::PasteNotationDialog(QWidget *parent,
					 PasteCommand::PasteType defaultType) :
    KDialogBase(parent, 0, true, i18n("Paste"), Ok | Cancel),
    m_defaultType(defaultType)
{
    QVBox *vbox = makeVBoxMainWidget();

    QButtonGroup *pasteTypeGroup = new QButtonGroup
	(1, Horizontal, i18n("Paste type"), vbox);

    m_restrictedButton = new QRadioButton
	(i18n("Paste into an existing gap [\"restricted\"]"), pasteTypeGroup);
    if (m_defaultType == PasteCommand::Restricted) {
	m_restrictedButton->setChecked(true);
    }
    m_simpleButton = new QRadioButton
	(i18n("Erase existing events to make room [\"simple\"]"), pasteTypeGroup);
    if (m_defaultType == PasteCommand::Simple) {
	m_simpleButton->setChecked(true);
    }
    m_openAndPasteButton = new QRadioButton
	(i18n("Move existing events out of the way [\"open-n-paste\"]"), pasteTypeGroup);
    if (m_defaultType == PasteCommand::OpenAndPaste) {
	m_openAndPasteButton->setChecked(true);
    }
    m_noteOverlayButton = new QRadioButton
	(i18n("Overlay notes, tying against present notes [\"note-overlay\"]"), pasteTypeGroup);
    if (m_defaultType == PasteCommand::NoteOverlay) {
	m_noteOverlayButton->setChecked(true);
    }
    m_matrixOverlayButton = new QRadioButton
	(i18n("Overlay notes, ignoring present notes [\"matrix-overlay\"]"), pasteTypeGroup);
    if (m_defaultType == PasteCommand::MatrixOverlay) {
	m_matrixOverlayButton->setChecked(true);
    }

    QButtonGroup *setAsDefaultGroup = new QButtonGroup
	(1, Horizontal, i18n("Options"), vbox);

    m_setAsDefaultButton = new QCheckBox
	(i18n("Make this the default paste type"), setAsDefaultGroup);
    m_setAsDefaultButton->setChecked(true);

    QObject::connect(m_restrictedButton, SIGNAL(released()),
		     this, SLOT(slotPasteTypeChanged()));
    QObject::connect(m_simpleButton, SIGNAL(released()),
		     this, SLOT(slotPasteTypeChanged()));
    QObject::connect(m_openAndPasteButton, SIGNAL(released()),
		     this, SLOT(slotPasteTypeChanged()));
    QObject::connect(m_noteOverlayButton, SIGNAL(released()),
		     this, SLOT(slotPasteTypeChanged()));
    QObject::connect(m_matrixOverlayButton, SIGNAL(released()),
		     this, SLOT(slotPasteTypeChanged()));
}

PasteCommand::PasteType
PasteNotationDialog::getPasteType() const
{
    if (m_restrictedButton->isChecked()) {
	return PasteCommand::Restricted;
    } else if (m_simpleButton->isChecked()) {
	return PasteCommand::Simple;
    } else if (m_noteOverlayButton->isChecked()) {
	return PasteCommand::NoteOverlay;
    } else if (m_matrixOverlayButton->isChecked()) {
	return PasteCommand::MatrixOverlay;
    } else {
	return PasteCommand::OpenAndPaste;
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
	    ((std::string("menu-") + note.getReferenceName()).c_str());
	m_unitCombo->insertItem(pmap, (note.getEnglishName() + "s").c_str());
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
				 std::string defaultText,
				 std::string defaultType,
				 int maxLength) :
    KDialogBase(parent, 0, true, i18n("Text"), Ok | Cancel),
    m_notePixmapFactory(npf)
{
    QVBox *vbox = makeVBoxMainWidget();

    QGroupBox *entryBox = new QGroupBox
	(1, Horizontal, i18n("Text specification"), vbox);
    QGroupBox *exampleBox = new QGroupBox
	(1, Horizontal, i18n("Example"), vbox);

    QGrid *entryGrid = new QGrid(2, QGrid::Horizontal, entryBox);

    new QLabel(i18n("Text:"), entryGrid);
    m_text = new QLineEdit(entryGrid);
    m_text->setText(defaultText.c_str());
    if (maxLength > 0) m_text->setMaxLength(maxLength);

    new QLabel(i18n("Style:"), entryGrid);
    m_typeCombo = new QComboBox(false, entryGrid);
    std::vector<std::string> styles = Rosegarden::Text::getUserStyles();
    for (unsigned int i = 0; i < styles.size(); ++i) {
	m_typeCombo->insertItem(styles[i].c_str());
	if (styles[i] == defaultType) {
	    m_typeCombo->setCurrentItem(m_typeCombo->count() - 1);
	}
    }

    QVBox *exampleVBox = new QVBox(exampleBox);
    m_staffAboveLabel = new QLabel(i18n("Staff"), exampleVBox);
    m_textExampleLabel = new QLabel(i18n("Example"), exampleVBox);
    m_staffBelowLabel = new QLabel(i18n("Staff"), exampleVBox);
 
    QObject::connect(m_text, SIGNAL(textChanged(const QString &)),
		     this, SLOT(slotTextChanged(const QString &)));
    QObject::connect(m_typeCombo, SIGNAL(activated(const QString &)),
		     this, SLOT(slotTypeChanged(const QString &)));

    m_text->setFocus();
    slotTypeChanged(getTextType().c_str());
}

std::string
TextEventDialog::getTextType() const
{
    return std::string(m_typeCombo->currentText().latin1());
}

std::string
TextEventDialog::getText() const
{
    return std::string(m_text->text().latin1());
}

void
TextEventDialog::slotTextChanged(const QString &text)
{
    std::string type(getTextType());
    Rosegarden::Text rtext(text.latin1(), type);
    m_textExampleLabel->setPixmap(m_notePixmapFactory->makeTextPixmap(rtext));
}

void
TextEventDialog::slotTypeChanged(const QString &qtype)
{
    std::string type(qtype.latin1());
    Rosegarden::Text rtext(getText(), type);
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

