/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
 
    This program is Copyright 2000-2007
        Guillaume Laurent   <glaurent@telegraph-road.org>,
        Chris Cannam        <cannam@all-day-breakfast.com>,
        Richard Bown        <richard.bown@ferventsoftware.com>
 
    The moral rights of Guillaume Laurent, Chris Cannam, and Richard
    Bown to claim authorship of this work have been asserted.
 
    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.
 
    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/


#include "KeySignatureDialog.h"

#include <klocale.h>
#include "misc/Strings.h"
#include "base/NotationTypes.h"
#include "gui/editors/notation/NotePixmapFactory.h"
#include "gui/widgets/BigArrowButton.h"
#include <kcombobox.h>
#include <kdialogbase.h>
#include <qbuttongroup.h>
#include <qgroupbox.h>
#include <qhbox.h>
#include <qlabel.h>
#include <qobject.h>
#include <qpixmap.h>
#include <qradiobutton.h>
#include <qstring.h>
#include <qtooltip.h>
#include <qvbox.h>
#include <qwidget.h>
#include <qcheckbox.h>


namespace Rosegarden
{

KeySignatureDialog::KeySignatureDialog(QWidget *parent,
                                       NotePixmapFactory *npf,
                                       Clef clef,
                                       Rosegarden::Key defaultKey,
                                       bool showApplyToAll,
                                       bool showConversionOptions,
                                       QString explanatoryText) :
        KDialogBase(parent, 0, true, i18n("Key Change"), Ok | Cancel | Help),
        m_notePixmapFactory(npf),
        m_key(defaultKey),
        m_clef(clef),
        m_valid(true),
        m_ignoreComboChanges(false),
        m_explanatoryLabel(0),
        m_applyToAllButton(0),
        m_noPercussionCheckBox(0)
{
    setHelp("nv-signatures-key");

    QVBox *vbox = makeVBoxMainWidget();

    QHBox *keyBox = 0;
    QHBox *nameBox = 0;

    QGroupBox *keyFrame = new QGroupBox
                          (1, Horizontal, i18n("Key signature"), vbox);

    QGroupBox *transposeFrame = new QButtonGroup
                                (1, Horizontal, i18n("Key transposition"), vbox);

    QGroupBox *buttonFrame = new QButtonGroup
                             (1, Horizontal, i18n("Scope"), vbox);

    QButtonGroup *conversionFrame = new QButtonGroup
                                    (1, Horizontal, i18n("Existing notes following key change"), vbox);

    keyBox = new QHBox(keyFrame);
    nameBox = new QHBox(keyFrame);

    QLabel *explanatoryLabel = 0;
    if (explanatoryText) {
        explanatoryLabel = new QLabel(explanatoryText, keyFrame);
    }

    BigArrowButton *keyDown = new BigArrowButton(keyBox, Qt::LeftArrow);
    QToolTip::add
        (keyDown, i18n("Flatten"));

    m_keyLabel = new QLabel(i18n("Key"), keyBox);
    m_keyLabel->setAlignment(AlignVCenter | AlignHCenter);

    BigArrowButton *keyUp = new BigArrowButton(keyBox, Qt::RightArrow);
    QToolTip::add
        (keyUp, i18n("Sharpen"));

    m_keyCombo = new KComboBox(nameBox);
    m_majorMinorCombo = new KComboBox(nameBox);
    m_majorMinorCombo->insertItem(i18n("Major"));
    m_majorMinorCombo->insertItem(i18n("Minor"));
    if (m_key.isMinor()) {
        m_majorMinorCombo->setCurrentItem(m_majorMinorCombo->count() - 1);
    }

    regenerateKeyCombo();
    redrawKeyPixmap();
    m_explanatoryLabel = explanatoryLabel;

    m_keyLabel->setMinimumWidth(m_keyLabel->pixmap()->width());
    m_keyLabel->setMinimumHeight(m_keyLabel->pixmap()->height());

    m_yesTransposeButton =
        new QRadioButton(i18n("Transpose key according to segment transposition"),
                         transposeFrame);
    QRadioButton *noTransposeButton =
        new QRadioButton(i18n("Use specified key.  Do not transpose"), transposeFrame);
    m_yesTransposeButton->setChecked(true);

    // just to shut up the compiler warning about unused variable:
    noTransposeButton->setChecked(false);

    if (showApplyToAll) {
        QRadioButton *applyToOneButton =
            new QRadioButton(i18n("Apply to current segment only"),
                             buttonFrame);
        m_applyToAllButton =
            new QRadioButton(i18n("Apply to all segments at this time"),
                             buttonFrame);
        applyToOneButton->setChecked(true);
        m_noPercussionCheckBox =
            new QCheckBox(i18n("Exclude percussion segments"), buttonFrame);
        m_noPercussionCheckBox->setChecked(true);
        
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

bool
KeySignatureDialog::shouldBeTransposed() const
{
    return m_yesTransposeButton && m_yesTransposeButton->isChecked();
}

bool
KeySignatureDialog::shouldIgnorePercussion() const
{
    return m_noPercussionCheckBox && m_noPercussionCheckBox->isChecked();
}

void
KeySignatureDialog::slotKeyUp()
{
    bool sharp = m_key.isSharp();
    int ac = m_key.getAccidentalCount();
    if (ac == 0)
        sharp = true;
    if (sharp) {
        if (++ac > 7)
            ac = 7;
    } else {
        if (--ac < 1) {
            ac = 0;
            sharp = true;
        }
    }

    try {
        m_key = Rosegarden::Key(ac, sharp, m_key.isMinor());
        setValid(true);
    } catch (Rosegarden::Key::BadKeySpec s) {
        std::cerr << s.getMessage() << std::endl;
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
    if (ac == 0)
        sharp = false;
    if (sharp) {
        if (--ac < 0) {
            ac = 1;
            sharp = false;
        }
    } else {
        if (++ac > 7)
            ac = 7;
    }

    try {
        m_key = Rosegarden::Key(ac, sharp, m_key.isMinor());
        setValid(true);
    } catch (Rosegarden::Key::BadKeySpec s) {
        std::cerr << s.getMessage() << std::endl;
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
    if (m_explanatoryLabel)
        m_explanatoryLabel->hide();

    m_ignoreComboChanges = true;
    QString currentText = m_keyCombo->currentText();
    Rosegarden::Key::KeyList keys(Rosegarden::Key::getKeys(m_key.isMinor()));
    m_keyCombo->clear();

    std::sort(keys.begin(), keys.end(), KeyNameComparator());
    bool textSet = false;

    for (Rosegarden::Key::KeyList::iterator i = keys.begin();
            i != keys.end(); ++i) {

        QString name(strtoqstr(i->getName()));
        int space = name.find(' ');
        if (space > 0)
            name = name.left(space);

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
        QPixmap pmap =
            NotePixmapFactory::toQPixmap(m_notePixmapFactory->makeKeyDisplayPixmap(m_key, m_clef));
        m_keyLabel->setPixmap(pmap);
    } else {
        m_keyLabel->setText(i18n("No such key"));
    }
}

void
KeySignatureDialog::slotKeyNameChanged(const QString &s)
{
    if (m_ignoreComboChanges)
        return ;

    if (m_explanatoryLabel)
        m_explanatoryLabel->hide();

    std::string name(getKeyName(s, m_key.isMinor()));

    try {
        m_key = Rosegarden::Key(name);
        setValid(true);

        int space = name.find(' ');
        if (space > 0)
            name = name.substr(0, space);
        m_keyCombo->setEditText(strtoqstr(name));

    } catch (Rosegarden::Key::BadKeyName s) {
        std::cerr << s.getMessage() << std::endl;
        setValid(false);
    }

    redrawKeyPixmap();
}

void
KeySignatureDialog::slotMajorMinorChanged(const QString &s)
{
    if (m_ignoreComboChanges)
        return ;

    std::string name(getKeyName(m_keyCombo->currentText(), s == i18n("Minor")));

    try {
        m_key = Rosegarden::Key(name);
        setValid(true);
    } catch (Rosegarden::Key::BadKeyName s) {
        std::cerr << s.getMessage() << std::endl;
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
              : s);

    std::string name(qstrtostr(u));
    name = name + " " + (minor ? "minor" : "major");
    return name;
}

}
#include "KeySignatureDialog.moc"
