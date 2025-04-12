/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2024 the Rosegarden development team.

    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#ifndef RG_NOTATIONCONFIGURATIONPAGE_H
#define RG_NOTATIONCONFIGURATIONPAGE_H

#include <string>
#include "TabbedConfigurationPage.h"
#include <QString>
#include <QStringList>


class QWidget;
class QPushButton;
class QLabel;
class QComboBox;
class QCheckBox;


namespace Rosegarden
{

class FontRequester;


/**
 * Notation Configuration page
 */
class NotationConfigurationPage : public TabbedConfigurationPage
{
    Q_OBJECT

public:
    explicit NotationConfigurationPage(QWidget *parent = nullptr);

    void apply() override;

    static QString iconLabel() { return tr("Notation"); }
    static QString title()     { return tr("Notation"); }
    static QString iconName()  { return "configure-notation"; }

protected slots:
    void slotFontComboChanged(int);
    void slotPopulateFontCombo(bool rescan);

protected:

    //--------------- Data members ---------------------------------

    QComboBox *m_font;
    QComboBox *m_singleStaffSize;
    QComboBox *m_multiStaffSize;
    FontRequester* m_textFont;
    FontRequester* m_sansFont;
    QLabel *m_fontOriginLabel;
    QLabel *m_fontCopyrightLabel;
    QLabel *m_fontMappedByLabel;
    QLabel *m_fontTypeLabel;
    QComboBox *m_layoutMode;
    QCheckBox *m_colourQuantize;
    QCheckBox *m_showUnknowns;
    QCheckBox *m_showInvisibles;
    QCheckBox *m_showRanges;
    QCheckBox *m_showCollisions;
    QComboBox *m_showTrackHeaders;
    QComboBox *m_noteStyle;
    QComboBox *m_insertType;
    QCheckBox *m_autoBeam;
    QCheckBox *m_autoTieBarlines;
    QCheckBox *m_collapseRests;
    QComboBox *m_pasteType;
    QCheckBox *m_preview;
    QCheckBox *m_quickEdit;
    QComboBox *m_accOctavePolicy;
    QComboBox *m_accBarPolicy;
    QComboBox *m_keySigCancelMode;
    QCheckBox *m_splitAndTie;
    QStringList m_untranslatedFont;
    QStringList m_untranslatedNoteStyle;
    QCheckBox *m_showRepeated;
    QCheckBox *m_editRepeated;
    QCheckBox *m_hideRedundantClefKey;
    QCheckBox *m_distributeVerses;

    void populateSizeCombo(QComboBox *combo, QString font, int defaultSize);
};


}

#endif
