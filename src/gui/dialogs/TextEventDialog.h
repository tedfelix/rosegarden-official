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

#ifndef RG_TEXTEVENTDIALOG_H
#define RG_TEXTEVENTDIALOG_H

#include "base/NotationTypes.h"
#include "gui/widgets/LineEdit.h"

#include <string>
#include <QDialog>
#include <QString>
#include <vector>


class QWidget;
class LineEdit;
class QLabel;
class QComboBox;
class QSpinBox;
class QStackedWidget;

namespace Rosegarden
{

class NotePixmapFactory;


class TextEventDialog : public QDialog
{
    Q_OBJECT

public:
    TextEventDialog(QWidget *parent,
                    NotePixmapFactory *npf,
                    Text defaultText,
                    int maxLength = -1); // for Qt default

    Text getText() const;

public slots:
    void slotTextChanged(const QString &);
    void slotTypeChanged(const QString &);

    /**
     * Save previous state of assorted widgets for restoration in the next
     * instance
     */
    void slotOK();
    void slotHelpRequested();

    // convenience canned texts
    void slotDynamicShortcutChanged(const QString &);
    void slotDirectionShortcutChanged(const QString &);
    void slotLocalDirectionShortcutChanged(const QString &);
    void slotTempoShortcutChanged(const QString &);
    void slotLocalTempoShortcutChanged(const QString &);

    //
    // special LilyPond directives, initial phase, as cheap text events; will
    // eventually move out of Text, and out of this dialog into
    // some other less cheesy interface (or maybe not, four years later, oh
    // well)
    //
    void slotLilyPondDirectiveChanged(const QString &);

    void slotUpdateSize(int);

protected:

    std::string getTextType() const;
    std::string getTextString() const;

    //--------------- Data members ---------------------------------

    LineEdit *m_text;
    QComboBox *m_typeCombo;
    QSpinBox  *m_verseSpin;
    QComboBox *m_dynamicShortcutCombo;
    QComboBox *m_directionShortcutCombo;
    QComboBox *m_localDirectionShortcutCombo;
    QComboBox *m_tempoShortcutCombo;
    QComboBox *m_localTempoShortcutCombo;
    QLabel *m_blankWidget;
    // temporary home:
    QComboBox *m_lilyPondDirectiveCombo;


    QLabel *m_staffAboveLabel;
    QLabel *m_textExampleLabel;
    QLabel *m_staffBelowLabel;
    QLabel *m_dynamicShortcutLabel;
    QLabel *m_directionShortcutLabel;
    QLabel *m_localDirectionShortcutLabel;
    QLabel *m_tempoShortcutLabel;
    QLabel *m_localTempoShortcutLabel;
    QLabel *m_verseLabel;
    QLabel *m_blankLabel;
    // temporary home:
    QLabel *m_directiveLabel;

    QStackedWidget *m_optionLabel;
    QStackedWidget *m_optionWidget;

    QString m_prevChord;
    QString m_prevLyric;
    QString m_prevAnnotation;

    NotePixmapFactory *m_notePixmapFactory;
    std::vector<std::string> m_styles;
//    std::vector<std::string> m_directives;

};


}

#endif
