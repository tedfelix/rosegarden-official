
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

#ifndef _RG_TEXTEVENTDIALOG_H_
#define _RG_TEXTEVENTDIALOG_H_

#include "base/NotationTypes.h"
#include <string>
#include <kdialogbase.h>
#include <qstring.h>
#include <vector>


class QWidget;
class QLineEdit;
class QLabel;
class KComboBox;


namespace Rosegarden
{

class NotePixmapFactory;


class TextEventDialog : public KDialogBase
{
    Q_OBJECT

public:
    TextEventDialog(QWidget *parent,
                    NotePixmapFactory *npf,
                    Text defaultText,
                    int maxLength = -1); // for Qt default

    Text getText() const {
        return Text(getTextString(), getTextType());
    }

public slots:
    void slotTextChanged(const QString &);
    void slotTypeChanged(const QString &);

    /*
     * Save previous state of assorted widgets for restoration in the next
     * instance
     */
    void slotOK();

    // convenience canned texts
    void slotDynamicShortcutChanged(const QString &);
    void slotDirectionShortcutChanged(const QString &);
    void slotLocalDirectionShortcutChanged(const QString &);
    void slotTempoShortcutChanged(const QString &);
    void slotLocalTempoShortcutChanged(const QString &);

    //
    // special Lilypond directives, initial phase, as cheap text events; will
    // eventually move out of Text, and out of this dialog into
    // some other less cheesy interface 
    //
    void slotLilypondDirectiveChanged(const QString &);

protected:

    std::string getTextType() const;
    std::string getTextString() const;

    //--------------- Data members ---------------------------------

    QLineEdit *m_text;
    KComboBox *m_typeCombo;
    KComboBox *m_dynamicShortcutCombo;
    KComboBox *m_directionShortcutCombo;
    KComboBox *m_localDirectionShortcutCombo;
    KComboBox *m_tempoShortcutCombo;
    KComboBox *m_localTempoShortcutCombo;
    // temporary home:
    KComboBox *m_lilypondDirectiveCombo;


    QLabel *m_staffAboveLabel;
    QLabel *m_textExampleLabel;
    QLabel *m_staffBelowLabel;
    QLabel *m_dynamicShortcutLabel;
    QLabel *m_directionShortcutLabel;
    QLabel *m_localDirectionShortcutLabel;
    QLabel *m_tempoShortcutLabel;
    QLabel *m_localTempoShortcutLabel;
    // temporary home:
    QLabel *m_directiveLabel;

    QString m_prevChord;
    QString m_prevLyric;
    QString m_prevAnnotation;

    NotePixmapFactory *m_notePixmapFactory;
    std::vector<std::string> m_styles;
//    std::vector<std::string> m_directives;

};



}

#endif
