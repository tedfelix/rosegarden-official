
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

#ifndef _RG_NOTATIONCONFIGURATIONPAGE_H_
#define _RG_NOTATIONCONFIGURATIONPAGE_H_

#include <string>
#include "TabbedConfigurationPage.h"
#include <qstring.h>
#include <qstringlist.h>
#include <klocale.h>


class QWidget;
class QPushButton;
class QLabel;
class QComboBox;
class QCheckBox;
class KFontRequester;
class KConfig;


namespace Rosegarden
{

class QuantizeParameters;


/**
 * Notation Configuration page
 */
class NotationConfigurationPage : public TabbedConfigurationPage
{
    Q_OBJECT

public:
    NotationConfigurationPage(KConfig *cfg,
                              QWidget *parent = 0, const char *name=0);

    virtual void apply();

    static QString iconLabel() { return i18n("Notation"); }
    static QString title()     { return i18n("Notation"); }
    static QString iconName()  { return "configure-notation"; }

protected slots:
    void slotFontComboChanged(int);
    void slotPopulateFontCombo(bool rescan);
    void slotViewButtonPressed();

protected:

    //--------------- Data members ---------------------------------

    QComboBox *m_font;
    QComboBox *m_singleStaffSize;
    QComboBox *m_multiStaffSize;
    QComboBox *m_printingSize;
    KFontRequester* m_textFont;
    KFontRequester* m_timeSigFont;
    QPushButton *m_viewButton;
    QLabel *m_fontOriginLabel;
    QLabel *m_fontCopyrightLabel;
    QLabel *m_fontMappedByLabel;
    QLabel *m_fontTypeLabel;
    QComboBox *m_layoutMode;
    QComboBox *m_spacing;
    QComboBox *m_proportion;
    QCheckBox *m_colourQuantize;
    QCheckBox *m_showUnknowns;
    QCheckBox *m_showInvisibles;
    QCheckBox *m_showRanges;
    QCheckBox *m_showCollisions;
    QComboBox *m_showTrackHeaders;
    QComboBox *m_noteStyle;
    QComboBox *m_insertType;
    QCheckBox *m_autoBeam;
    QCheckBox *m_collapseRests;
    QComboBox *m_pasteType;
    QComboBox *m_accOctavePolicy;
    QComboBox *m_accBarPolicy;
    QComboBox *m_keySigCancelMode;
    QCheckBox *m_splitAndTie;
    QuantizeParameters *m_quantizeFrame;
    QStringList m_untranslatedFont;
    QStringList m_untranslatedNoteStyle;

    void populateSizeCombo(QComboBox *combo, std::string font, int dfltSize);
};


}

#endif
