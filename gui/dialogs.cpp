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
					 Rosegarden::TimeSignature sig) :
    KDialogBase(parent, 0, true, i18n("Time Signature"), Ok | Cancel),
    m_timeSignature(sig)
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
}

void
TimeSignatureDialog::slotNumDown()
{
    int n = m_timeSignature.getNumerator();
    if (--n >= 1) {
	m_timeSignature = TimeSignature(n, m_timeSignature.getDenominator());
	m_numLabel->setText(QString("%1").arg(n));
    }
}

void
TimeSignatureDialog::slotNumUp()
{
    int n = m_timeSignature.getNumerator();
    if (++n <= 99) {
	m_timeSignature = TimeSignature(n, m_timeSignature.getDenominator());
	m_numLabel->setText(QString("%1").arg(n));
    }
}

void
TimeSignatureDialog::slotDenomDown()
{
    int n = m_timeSignature.getDenominator();
    if ((n /= 2) >= 1) {
	m_timeSignature = TimeSignature(m_timeSignature.getNumerator(), n);
	m_denomLabel->setText(QString("%1").arg(n));
    }
}

void
TimeSignatureDialog::slotDenomUp()
{
    int n = m_timeSignature.getDenominator();
    if ((n *= 2) <= 64) {
	m_timeSignature = TimeSignature(m_timeSignature.getNumerator(), n);
	m_denomLabel->setText(QString("%1").arg(n));
    }
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
					 PasteNotationCommand::PasteType defaultType) :
    KDialogBase(parent, 0, true, i18n("Paste"), Ok | Cancel),
    m_defaultType(defaultType)
{
    QVBox *vbox = makeVBoxMainWidget();

    QButtonGroup *pasteTypeGroup = new QButtonGroup
	(1, Horizontal, i18n("Paste type"), vbox);

    m_pasteIntoGapButton = new QRadioButton
	(i18n("Paste into an existing gap [\"restricted\"]"), pasteTypeGroup);
    if (m_defaultType == PasteNotationCommand::PasteIntoGap) {
	m_pasteIntoGapButton->setChecked(true);
    }
    m_pasteDestructiveButton = new QRadioButton
	(i18n("Erase existing events to make room [\"simple\"]"), pasteTypeGroup);
    if (m_defaultType == PasteNotationCommand::PasteDestructive) {
	m_pasteDestructiveButton->setChecked(true);
    }
    m_openAndPasteButton = new QRadioButton
	(i18n("Move existing events out of the way [\"open-n-paste\"]"), pasteTypeGroup);
    if (m_defaultType == PasteNotationCommand::OpenAndPaste) {
	m_openAndPasteButton->setChecked(true);
    }
    m_pasteOverlayButton = new QRadioButton
	(i18n("Overlay notes, tying against present notes [\"note-overlay\"]"), pasteTypeGroup);
    if (m_defaultType == PasteNotationCommand::PasteOverlay) {
	m_pasteOverlayButton->setChecked(true);
    }
    m_pasteOverlayRawButton = new QRadioButton
	(i18n("Overlay notes, ignoring present notes [\"matrix-overlay\"]"), pasteTypeGroup);
    if (m_defaultType == PasteNotationCommand::PasteOverlayRaw) {
	m_pasteOverlayRawButton->setChecked(true);
    }

    QButtonGroup *setAsDefaultGroup = new QButtonGroup
	(1, Horizontal, i18n("Options"), vbox);

    m_setAsDefaultButton = new QCheckBox
	(i18n("Make this the default paste type"), setAsDefaultGroup);
    m_setAsDefaultButton->setChecked(true);

    QObject::connect(m_pasteIntoGapButton, SIGNAL(released()),
		     this, SLOT(slotPasteTypeChanged()));
    QObject::connect(m_pasteDestructiveButton, SIGNAL(released()),
		     this, SLOT(slotPasteTypeChanged()));
    QObject::connect(m_openAndPasteButton, SIGNAL(released()),
		     this, SLOT(slotPasteTypeChanged()));
    QObject::connect(m_pasteOverlayButton, SIGNAL(released()),
		     this, SLOT(slotPasteTypeChanged()));
    QObject::connect(m_pasteOverlayRawButton, SIGNAL(released()),
		     this, SLOT(slotPasteTypeChanged()));
}

PasteNotationCommand::PasteType
PasteNotationDialog::getPasteType() const
{
    if (m_pasteIntoGapButton->isChecked()) {
	return PasteNotationCommand::PasteIntoGap;
    } else if (m_pasteDestructiveButton->isChecked()) {
	return PasteNotationCommand::PasteDestructive;
    } else if (m_pasteOverlayButton->isChecked()) {
	return PasteNotationCommand::PasteOverlay;
    } else if (m_pasteOverlayRawButton->isChecked()) {
	return PasteNotationCommand::PasteOverlayRaw;
    } else {
	return PasteNotationCommand::OpenAndPaste;
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

