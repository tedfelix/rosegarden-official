/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2015 the Rosegarden development team.
 
    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.
 
    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/


#include "ClefDialog.h"

#include "base/NotationTypes.h"
#include "gui/editors/notation/NotePixmapFactory.h"
#include "gui/widgets/BigArrowButton.h"
#include "gui/general/GUIPalette.h"
#include "misc/ConfigGroups.h"

#include <QDialog>
#include <QDialogButtonBox>
#include <QGroupBox>
#include <QLabel>
#include <QObject>
#include <QPixmap>
#include <QRadioButton>
#include <QString>
#include <QToolTip>
#include <QWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QSettings>


namespace Rosegarden
{

ClefDialog::ClefDialog(QWidget *parent,
                       NotePixmapFactory *npf,
                       Clef defaultClef,
                       bool showConversionOptions) :
        QDialog(parent),
        m_notePixmapFactory(npf),
        m_clef(defaultClef)
{
    setModal(true);
    setWindowTitle(tr("Clef"));

    QVBoxLayout *vboxLayout = new QVBoxLayout;
    setLayout(vboxLayout);

    QGroupBox *clefBox = new QGroupBox(tr("Clef"));
    QVBoxLayout *clefBoxLayout = new QVBoxLayout;
    clefBox->setLayout(clefBoxLayout);
    vboxLayout->addWidget(clefBox);

    // first layer (up button) (laying this out in a steries of stacked hbox and
    // vbox chunks inside a vbox instead of using a grid because I find it
    // easier to keep my head straight using this kind of layout)
    QWidget *topChunk = new QWidget;
    QHBoxLayout *topChunkLayout = new QHBoxLayout;
    topChunk->setLayout(topChunkLayout);

    m_octaveUp = new BigArrowButton(Qt::UpArrow);
    topChunkLayout->addWidget(m_octaveUp);
    m_octaveUp->setToolTip(tr("Up an Octave"));
    m_octaveUp->setMaximumWidth(50);

    clefBoxLayout->addWidget(topChunk);

    // second layer (left button, clef pixmap, right button)
    QWidget *midChunk = new QWidget;
    QHBoxLayout *midChunkLayout = new QHBoxLayout;
    midChunk->setLayout(midChunkLayout);

    BigArrowButton *clefDown = new BigArrowButton(Qt::LeftArrow);
    midChunkLayout->addWidget(clefDown);
    clefDown->setToolTip(tr("Lower clef"));

    QSettings settings;
    settings.beginGroup(GeneralOptionsConfigGroup);
    m_Thorn = settings.value("use_thorn_style", true).toBool();
    settings.endGroup();
    
    m_clefPixmap = new QLabel;
    // if no stylesheet, force a white background anyway, because the foreground
    // will be dark, and this used to be bordering on illegible in Classic
    QString localStyle = (m_Thorn ? 
            QString("background: #404040; color: white;")
                                :
            QString("background: white; color: black;"));
    m_clefPixmap->setStyleSheet(localStyle);
    midChunkLayout->addWidget(m_clefPixmap);

    BigArrowButton *clefUp = new BigArrowButton(Qt::RightArrow);
    midChunkLayout->addWidget(clefUp);
    clefUp->setToolTip( tr("Higher clef") );

    clefBoxLayout->addWidget(midChunk);

    // lower layer (down arrow)
    QWidget *lowChunk = new QWidget;
    QHBoxLayout *lowChunkLayout = new QHBoxLayout;
    lowChunk->setLayout(lowChunkLayout);

    m_octaveDown = new BigArrowButton(Qt::DownArrow);
    lowChunkLayout->addWidget(m_octaveDown);
    m_octaveDown->setToolTip(tr("Down an Octave"));
    m_octaveDown->setMaximumWidth(50);

    clefBoxLayout->addWidget(lowChunk);

    // sub-bass layer to fix the stubborn alignment problem
    QWidget *basChunk = new QWidget;
    QHBoxLayout *basChunkLayout = new QHBoxLayout;
    basChunk->setLayout(basChunkLayout);

    m_clefNameLabel = new QLabel(tr("Clef"));
    basChunkLayout->addWidget(m_clefNameLabel);
    m_clefNameLabel->setAlignment(Qt::AlignVCenter | Qt::AlignHCenter);

    clefBoxLayout->addWidget(basChunk);

    QGroupBox *conversionFrame = new QGroupBox(tr("Existing notes following clef change"));
    QVBoxLayout *conversionFrameLayout = new QVBoxLayout;
    conversionFrame->setLayout(conversionFrameLayout);
    vboxLayout->addWidget(conversionFrame);

    if (showConversionOptions) {
        m_noConversionButton = new QRadioButton(tr("Maintain current pitches"));
        conversionFrameLayout->addWidget(m_noConversionButton);

        m_changeOctaveButton = new QRadioButton(tr("Transpose into appropriate octave"));
        conversionFrameLayout->addWidget(m_changeOctaveButton);

        m_transposeButton = 0;

        //!!! why aren't we offering this option? does it not work? too difficult to describe?
        //
        // (I have no idea, but I doctored the following commented out code in case we ever
        // happen to want to ressurect it, although this seems unlikely.)
        //
        //m_transposeButton = new QRadioButton(tr("Maintain current positions on the staff"));
        
        settings.beginGroup("Clef_Dialog");        	
        m_changeOctaveButton->setChecked(settings.value("change_octave", true).toBool());
        m_noConversionButton->setChecked(settings.value("transpose", false).toBool());
        settings.endGroup();
    } else {
        m_noConversionButton = 0;
        m_changeOctaveButton = 0;
        m_transposeButton = 0;
        conversionFrame->hide();
    }

    // hook up the up/down left/right buttons
    QObject::connect(clefUp, SIGNAL(clicked()), this, SLOT(slotClefUp()));
    QObject::connect(clefDown, SIGNAL(clicked()), this, SLOT(slotClefDown()));
    QObject::connect(m_octaveUp, SIGNAL(clicked()), this, SLOT(slotOctaveUp()));
    QObject::connect(m_octaveDown, SIGNAL(clicked()), this, SLOT(slotOctaveDown()));

    redrawClefPixmap();

    // the strip of standard buttons at the bottom, sans the help button,
    // because I'm pretty much the one who would ever make context sensitive
    // help work, and I think there's little chance of ever doing that.
    QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    vboxLayout->addWidget(buttonBox);

    connect(buttonBox, SIGNAL(accepted()), this, SLOT(accept()));
    connect(buttonBox, SIGNAL(rejected()), this, SLOT(reject()));
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
    NotePixmapFactory::ColourType ct =
        m_Thorn ? NotePixmapFactory::PlainColourLight
                : NotePixmapFactory::PlainColour;
    m_notePixmapFactory->setSelected(false);
    m_notePixmapFactory->setShaded(false);
    QPixmap pmap = m_notePixmapFactory->makeClefDisplayPixmap(m_clef, ct);
    m_clefPixmap->setPixmap(pmap);

    m_clefNameLabel->setText(translatedClefName(m_clef));
}

QString
ClefDialog::translatedClefName(Clef clef)
{
    QString name;
    int octave = clef.getOctaveOffset();

    switch (octave) {
    case - 1:
        name = tr("%1 down an octave");
        break;
    case - 2:
        name = tr("%1 down two octaves");
        break;
    case 1:
        name = tr("%1 up an octave");
        break;
    case 2:
        name = tr("%1 up two octaves");
        break;
    default:
        name = "%1";
        break;
    }

    std::string type = clef.getClefType();
    if (type == Clef::Treble)
        name = name.arg(tr("Treble"));
    else if (type == Clef::French)
        name = name.arg(tr("French violin"));
    else if (type == Clef::Soprano)
        name = name.arg(tr("Soprano"));
    else if (type == Clef::Mezzosoprano)
        name = name.arg(tr("Mezzo-soprano"));
    else if (type == Clef::Alto)
        name = name.arg(tr("Alto"));
    else if (type == Clef::Tenor)
        name = name.arg(tr("Tenor"));
    else if (type == Clef::Baritone)
        name = name.arg(tr("C-baritone"));
    else if (type == Clef::Varbaritone)
        name = name.arg(tr("F-baritone"));
    else if (type == Clef::Bass)
        name = name.arg(tr("Bass"));
    else if (type == Clef::Subbass)
        name = name.arg(tr("Sub-bass"));

    return name;
}

void
ClefDialog::accept()
{
    QSettings settings;
    settings.beginGroup("Clef_Dialog");
    settings.setValue("change_octave", m_changeOctaveButton->isChecked());    
    settings.setValue("transpose", m_noConversionButton->isChecked());    
    settings.endGroup();
    QDialog::accept();
}


}
#include "ClefDialog.moc"
