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

class QWidget;
class QLineEdit;
class QCheckBox;
class QLabel;
class QComboBox;
class NotePixmapFactory;


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
			Rosegarden::TimeSignature::DefaultTimeSignature);
    Rosegarden::TimeSignature getTimeSignature() const {
	return m_timeSignature;
    }

protected:
    Rosegarden::TimeSignature m_timeSignature;
    QLabel *m_numLabel;
    QLabel *m_denomLabel;

public slots:
    void slotNumUp();
    void slotNumDown();
    void slotDenomUp();
    void slotDenomDown();
};


class KeySignatureDialog : public KDialogBase
{
    Q_OBJECT

public:
    KeySignatureDialog(QWidget *parent,
		       NotePixmapFactory *npf,
		       Rosegarden::Clef clef,
		       Rosegarden::Key defaultKey =
		       Rosegarden::Key::DefaultKey,
		       bool showApplyToAll = true,
		       bool showTranspose = true);

    bool isValid() const;
    Rosegarden::Key getKey() const;

    bool shouldApplyToAll() const;
    bool shouldTranspose() const;

protected:
    NotePixmapFactory *m_notePixmapFactory;

    Rosegarden::Key m_key;
    Rosegarden::Clef m_clef;
    bool m_valid;
    bool m_ignoreComboChanges;

    QLabel *m_keyLabel;
    QComboBox *m_keyCombo;
    QComboBox *m_majorMinorCombo;

    QCheckBox *m_transposeButton;
    QCheckBox *m_applyToAllButton;

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


#endif
