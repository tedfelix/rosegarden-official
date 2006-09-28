/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
 
    This program is Copyright 2000-2006
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


#include "ClefDialog.h"

#include <klocale.h>
#include "base/NotationTypes.h"
#include "gui/editors/notation/NotePixmapFactory.h"
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


namespace Rosegarden
{

ClefDialog::ClefDialog(QWidget *parent,
                       NotePixmapFactory *npf,
                       Clef defaultClef,
                       bool showConversionOptions) :
        KDialogBase(parent, 0, true, i18n("Clef"), Ok | Cancel | Help),
        m_notePixmapFactory(npf),
        m_clef(defaultClef)
{
    setHelp("nv-signatures-clef");

    QVBox *vbox = makeVBoxMainWidget();

    QGroupBox *clefFrame = new QGroupBox
                           (1, Horizontal, i18n("Clef"), vbox);

    QButtonGroup *conversionFrame = new QButtonGroup
                                    (1, Horizontal, i18n("Existing notes following clef change"), vbox);

    QHBox *clefBox = new QHBox(clefFrame);

    BigArrowButton *clefDown = new BigArrowButton(clefBox, Qt::LeftArrow);
    QToolTip::add
        (clefDown, i18n("Lower clef"));

    QHBox *clefLabelBox = new QVBox(clefBox);

    m_octaveUp = new BigArrowButton(clefLabelBox, Qt::UpArrow);
    QToolTip::add
        (m_octaveUp, i18n("Up an Octave"));

    m_clefLabel = new QLabel(i18n("Clef"), clefLabelBox);
    m_clefLabel->setAlignment(AlignVCenter | AlignHCenter);

    m_octaveDown = new BigArrowButton(clefLabelBox, Qt::DownArrow);
    QToolTip::add
        (m_octaveDown, i18n("Down an Octave"));

    BigArrowButton *clefUp = new BigArrowButton(clefBox, Qt::RightArrow);
    QToolTip::add
        (clefUp, i18n("Higher clef"));

    m_clefNameLabel = new QLabel(i18n("Clef"), clefLabelBox);
    m_clefNameLabel->setAlignment(AlignVCenter | AlignHCenter);

    if (showConversionOptions) {
        m_noConversionButton =
            new QRadioButton
            (i18n("Maintain current pitches"), conversionFrame);
        m_changeOctaveButton =
            new QRadioButton
            (i18n("Transpose into appropriate octave"), conversionFrame);
        m_transposeButton = 0;

        //!!! why aren't we offering this option? does it not work? too difficult to describe?
        //	m_transposeButton =
        //	    new QRadioButton
        //	    (i18n("Maintain current positions on the staff"), conversionFrame);
        m_changeOctaveButton->setChecked(true);
    } else {
        m_noConversionButton = 0;
        m_changeOctaveButton = 0;
        m_transposeButton = 0;
        conversionFrame->hide();
    }

    QObject::connect(clefUp, SIGNAL(clicked()), this, SLOT(slotClefUp()));
    QObject::connect(clefDown, SIGNAL(clicked()), this, SLOT(slotClefDown()));
    QObject::connect(m_octaveUp, SIGNAL(clicked()), this, SLOT(slotOctaveUp()));
    QObject::connect(m_octaveDown, SIGNAL(clicked()), this, SLOT(slotOctaveDown()));

    redrawClefPixmap();
}

Clef
ClefDialog::getClef() const
{
    return m_clef;
}

ClefDialog::ConversionType

ClefDialog::getConversionType() const
{
    if (m_noConversionButton && m_noConversionButton->isChecked()) {
        return NoConversion;
    } else if (m_changeOctaveButton && m_changeOctaveButton->isChecked()) {
        return ChangeOctave;
    } else if (m_transposeButton && m_transposeButton->isChecked()) {
        return Transpose;
    }
    return NoConversion;
}

void
ClefDialog::slotClefUp()
{
    int octaveOffset = m_clef.getOctaveOffset();
    Clef::ClefList clefs(Clef::getClefs());

    for (Clef::ClefList::iterator i = clefs.begin();
            i != clefs.end(); ++i) {

        if (m_clef.getClefType() == i->getClefType()) {
            if (++i == clefs.end())
                i = clefs.begin();
            m_clef = Clef(i->getClefType(), octaveOffset);
            break;
        }
    }

    redrawClefPixmap();
}

void
ClefDialog::slotClefDown()
{
    int octaveOffset = m_clef.getOctaveOffset();
    Clef::ClefList clefs(Clef::getClefs());

    for (Clef::ClefList::iterator i = clefs.begin();
            i != clefs.end(); ++i) {

        if (m_clef.getClefType() == i->getClefType()) {
            if (i == clefs.begin())
                i = clefs.end();
            --i;
            m_clef = Clef(i->getClefType(), octaveOffset);
            break;
        }
    }

    redrawClefPixmap();
}

void
ClefDialog::slotOctaveUp()
{
    int octaveOffset = m_clef.getOctaveOffset();
    if (octaveOffset == 2)
        return ;

    ++octaveOffset;

    m_octaveDown->setEnabled(true);
    if (octaveOffset == 2) {
        m_octaveUp->setEnabled(false);
    }

    m_clef = Clef(m_clef.getClefType(), octaveOffset);
    redrawClefPixmap();
}

void
ClefDialog::slotOctaveDown()
{
    int octaveOffset = m_clef.getOctaveOffset();
    if (octaveOffset == -2)
        return ;

    --octaveOffset;

    m_octaveUp->setEnabled(true);
    if (octaveOffset == 2) {
        m_octaveDown->setEnabled(false);
    }

    m_clef = Clef(m_clef.getClefType(), octaveOffset);
    redrawClefPixmap();
}

void
ClefDialog::redrawClefPixmap()
{
    QPixmap pmap = NotePixmapFactory::toQPixmap
                   (m_notePixmapFactory->makeClefDisplayPixmap(m_clef));
    m_clefLabel->setPixmap(pmap);

    QString name;
    int octave = m_clef.getOctaveOffset();

    switch (octave) {
    case - 1:
        name = i18n("%1 down an octave");
        break;
    case - 2:
        name = i18n("%1 down two octaves");
        break;
    case 1:
        name = i18n("%1 up an octave");
        break;
    case 2:
        name = i18n("%1 up two octaves");
        break;
    default:
        name = "%1";
        break;
    }

    std::string type = m_clef.getClefType();
    if (type == Clef::Treble)
        name = name.arg(i18n("Treble"));
    else if (type == Clef::Soprano)
        name = name.arg(i18n("Soprano"));
    else if (type == Clef::Alto)
        name = name.arg(i18n("Alto"));
    else if (type == Clef::Tenor)
        name = name.arg(i18n("Tenor"));
    else if (type == Clef::Bass)
        name = name.arg(i18n("Bass"));

    m_clefNameLabel->setText(name);
}

}
