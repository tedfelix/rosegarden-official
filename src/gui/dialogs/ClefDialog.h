
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

#ifndef RG_CLEFDIALOG_H
#define RG_CLEFDIALOG_H

#include "base/NotationTypes.h"
#include <QDialog>


class QWidget;
class QRadioButton;
class QLabel;


namespace Rosegarden
{

class BigArrowButton;
class NotePixmapFactory;


class ClefDialog : public QDialog
{
    Q_OBJECT

public:
    enum ConversionType {
        NoConversion,
        ChangeOctave,
        Transpose,
    };

    ClefDialog(QWidget *parent,
               NotePixmapFactory *npf,
               Clef defaultClef,
               bool showConversionOptions = true);

    Clef getClef() const;
    ConversionType getConversionType() const;

    // Code extracted from redrawClefPixmap() and made static to
    // reuse it from notation/Inconsistencies.h
    // TODO : Should be move in a better place (may be something
    //        like gui/general/Translation.cpp)
    static QString translatedClefName(Clef clef);

public slots:
    void slotClefUp();
    void slotClefDown();
    void slotOctaveUp();
    void slotOctaveDown();
    void accept();

protected:
    void redrawClefPixmap();
    bool m_Thorn;

    //--------------- Data members ---------------------------------

    NotePixmapFactory *m_notePixmapFactory;
    Clef m_clef;
    
    QLabel *m_clefPixmap;
    QLabel *m_clefNameLabel;

    BigArrowButton *m_octaveUp;
    BigArrowButton *m_octaveDown;

    QRadioButton *m_noConversionButton;
    QRadioButton *m_changeOctaveButton;
    QRadioButton *m_transposeButton;   
};



}

#endif
