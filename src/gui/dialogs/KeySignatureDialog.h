
/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2016 the Rosegarden development team.

    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#ifndef RG_KEYSIGNATUREDIALOG_H
#define RG_KEYSIGNATUREDIALOG_H

#include "base/NotationTypes.h"
#include <string>
#include <QDialog>
#include <QString>
#include <QCheckBox>


class QWidget;
class QRadioButton;
class QLabel;
class QComboBox;
class QCheckBox;


namespace Rosegarden
{

class NotePixmapFactory;


class KeySignatureDialog : public QDialog
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
                       Clef clef,
                       Rosegarden::Key defaultKey =
                       Rosegarden::Key::DefaultKey,
                       bool showApplyToAll = true,
                       bool showConversionOptions = true,
                       QString explanatoryText = "");

    bool isValid() const;
    ::Rosegarden::Key getKey() const;

    bool shouldApplyToAll() const;
    bool shouldBeTransposed() const; 
    ConversionType getConversionType() const;
    bool shouldIgnorePercussion() const;

public slots:
    void slotKeyUp();
    void slotKeyDown();
    void slotKeyNameChanged(int);
    void slotMajorMinorChanged(const QString &);
    void slotHelpRequested();

protected:

    void redrawKeyPixmap();
    void regenerateKeyCombo();
    void setValid(bool valid);
    std::string getKeyName(const QString &s, bool minor);
    bool m_Thorn;

    //--------------- Data members ---------------------------------

    NotePixmapFactory *m_notePixmapFactory;

    Rosegarden::Key m_key;
    Clef m_clef;
    bool m_valid;
    bool m_ignoreComboChanges;

    QLabel *m_keyPixmap;
    QComboBox *m_keyCombo;
    QComboBox *m_majorMinorCombo;
    QLabel *m_explanatoryLabel;

    QRadioButton *m_applyToAllButton;
    QRadioButton *m_yesTransposeButton;

    QRadioButton *m_noConversionButton;
    QRadioButton *m_convertButton;
    QRadioButton *m_transposeButton;

    QCheckBox *m_noPercussionCheckBox;
};



}

#endif
