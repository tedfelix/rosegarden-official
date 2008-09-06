/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2008 the Rosegarden development team.

    This file is Copyright 2006
	D. Michael McIntyre <dmmcintyr@users.sourceforge.net>
 
    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.
 
    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/


#include "PresetHandlerDialog.h"
#include <QLayout>
#include <kapplication.h>

#include <klocale.h>
#include "misc/Debug.h"
#include "document/ConfigGroups.h"
#include "CategoryElement.h"
#include "PresetElement.h"
#include "PresetGroup.h"
#include <QComboBox>
#include <kconfig.h>
#include <QDialog>
#include <QDialogButtonBox>
#include <QGroupBox>
#include <QDialog>
#include <QFrame>
#include <QGroupBox>
#include <QLabel>
#include <QString>
#include <QWidget>
#include <QVBoxLayout>


namespace Rosegarden
{

PresetHandlerDialog::PresetHandlerDialog(QDialogButtonBox::QWidget *parent, bool fromNotation)
        : KDialogBase(parent, "presethandlerdialog", true, i18n("Load track parameters preset"), Ok | Cancel, Ok),
        m_config(kapp->config()),
        m_fromNotation(fromNotation)
{
    m_presets = new PresetGroup();
    m_categories = m_presets->getCategories();
    if (m_fromNotation) setCaption(i18n("Convert notation for..."));

    initDialog();
}

PresetHandlerDialog::~PresetHandlerDialog()
{
    // delete m_presets
    if (m_presets != NULL) {
        delete m_presets;
    }
}

void
PresetHandlerDialog::initDialog()
{
    RG_DEBUG << "PresetHandlerDialog::initDialog()" << endl;

    QVBox *vBox = makeVBoxMainWidget();

    QFrame *frame = new QFrame(vBox);

    QGridLayout *layout = new QGridLayout(frame, 6, 5, 10, 5);

    QLabel *title = new QLabel(i18n("Select preset track parameters for:"), frame);
    if (m_fromNotation) title->setText(i18n("Create appropriate notation for:"));

    QLabel *catlabel = new QLabel(i18n("Category"), frame);
    m_categoryCombo = new QComboBox(frame);

    QLabel *inslabel = new QLabel(i18n("Instrument"), frame);
    m_instrumentCombo = new QComboBox(frame);

    QLabel *plylabel = new QLabel(i18n("Player Ability"), frame);
    m_playerCombo = new QComboBox(frame);
    m_playerCombo->addItem(i18n("Amateur"));
    m_playerCombo->addItem(i18n("Professional"));

    QGroupBox *scopeBox = new QGroupBox
        (1, Horizontal, i18n("Scope"), frame);
    if (m_fromNotation) {
        QRadioButton *onlySelectedSegments = new
            QRadioButton(i18n("Only selected segments"), scopeBox);
        m_convertAllSegments = new 
            QRadioButton(i18n("All segments in this track"), scopeBox);
        onlySelectedSegments->setChecked(true);
    }
    else {
        QRadioButton *onlyNewSegments = new 
            QRadioButton(i18n("Only for new segments"), scopeBox);
        m_convertSegments = new 
            QRadioButton(i18n("Convert existing segments"), scopeBox);
        onlyNewSegments->setChecked(true);
    }
    
    layout->addWidget(title, 0, 0, 0- 0+1, 1- 1, AlignLeft);
    layout->addWidget(catlabel, 1, 0, AlignRight);
    layout->addWidget(m_categoryCombo, 1, 1);
    layout->addWidget(inslabel, 2, 0, AlignRight);
    layout->addWidget(m_instrumentCombo, 2, 1);
    layout->addWidget(plylabel, 3, 0, AlignRight);
    layout->addWidget(m_playerCombo, 3, 1);
    layout->addWidget(scopeBox, 4, 0, 0+1, 1- 1, AlignLeft);

    populateCategoryCombo();
    // try to set to same category used previously
    QSettings m_config;
    m_config.beginGroup( GeneralOptionsConfigGroup );
    // 
    // FIX-manually-(GW), add:
    // m_config.endGroup();		// corresponding to: m_config.beginGroup( GeneralOptionsConfigGroup );
    //  

    m_categoryCombo->setCurrentIndex( m_config.value("category_combo_index", 0).toInt() );

    // populate the instrument combo
    slotCategoryIndexChanged(m_categoryCombo->currentIndex());

    // try to set to same instrument used previously
    QSettings m_config;
    m_config.beginGroup( GeneralOptionsConfigGroup );
    // 
    // FIX-manually-(GW), add:
    // m_config.endGroup();		// corresponding to: m_config.beginGroup( GeneralOptionsConfigGroup );
    //  

    m_instrumentCombo->setCurrentIndex( m_config.value("instrument_combo_index", 0).toInt() );

    // set to same player used previously (this one can't fail, unlike the
    // others, because the contents of this combo are static)
    m_playerCombo->setCurrentIndex( m_config.value("player_combo_index", 0).toInt() );

    if (m_fromNotation){
        m_convertAllSegments->setChecked( qStrToBool( m_config.value("convert_all_segments", "0" ) ) );
    }
    else {
    	m_convertSegments->setChecked( qStrToBool( m_config.value("convert_segments", "0" ) ) );
    }
    	 
    
    connect(m_categoryCombo, SIGNAL(activated(int)),
            SLOT(slotCategoryIndexChanged(int)));
}

QString
PresetHandlerDialog::getName()
{
    return m_instrumentCombo->currentText();
}

int
PresetHandlerDialog::getClef()
{
    PresetElement p = m_categories[m_categoryCombo->currentIndex()].
                      getPresetByIndex(m_instrumentCombo->currentIndex());

    return p.getClef();
}

int
PresetHandlerDialog::getTranspose()
{
    PresetElement p = m_categories[m_categoryCombo->currentIndex()].
                      getPresetByIndex(m_instrumentCombo->currentIndex());

    return p.getTranspose();
}

int
PresetHandlerDialog::getLowRange()
{
    PresetElement p = m_categories[m_categoryCombo->currentIndex()].
                      getPresetByIndex(m_instrumentCombo->currentIndex());
    // 0 == amateur
    // 1 == pro
    if (m_playerCombo->currentIndex() == 0) {
        return p.getLowAm();
    } else {
        return p.getLowPro();
    }
}

int
PresetHandlerDialog::getHighRange()
{
    PresetElement p = m_categories[m_categoryCombo->currentIndex()].
                      getPresetByIndex(m_instrumentCombo->currentIndex());
    // 0 == amateur
    // 1 == pro
    if (m_playerCombo->currentIndex() == 0) {
        return p.getHighAm();
    } else {
        return p.getHighPro();
    }
}

bool
PresetHandlerDialog::getConvertAllSegments()
{
    if (m_fromNotation) {
        return m_convertAllSegments && m_convertAllSegments->isChecked();
    }
    else {
        return m_convertSegments && m_convertSegments->isChecked();
    }
}

bool
PresetHandlerDialog::getConvertOnlySelectedSegments()
{
    if (m_fromNotation) {
        return m_convertAllSegments && !m_convertAllSegments->isChecked();
    }
    else {
        return false;
    }
}

void
PresetHandlerDialog::populateCategoryCombo()
{
    RG_DEBUG << "PresetHandlerDialog::populateCategoryCombo()" << endl;

    for (CategoriesContainer::iterator i = m_categories.begin();
            i != m_categories.end(); ++i) {

        RG_DEBUG << "    adding category: " << (*i).getName() << endl;

        m_categoryCombo->addItem((*i).getName());
    }
}

void
PresetHandlerDialog::slotCategoryIndexChanged(int index)
{
    RG_DEBUG << "PresetHandlerDialog::slotCategoryIndexChanged(" << index << ")" << endl;

    CategoryElement e = m_categories[index];
    ElementContainer c = e.getPresets();

    m_instrumentCombo->clear();

    for (ElementContainer::iterator i = c.begin();
            i != c.end(); ++i) {

        RG_DEBUG << "    adding instrument: " << (*i).getName() << endl;

        m_instrumentCombo->addItem((*i).getName());
    }

}

void
PresetHandlerDialog::slotOk()
{
    QSettings m_config;
    m_config.beginGroup( GeneralOptionsConfigGroup );
    // 
    // FIX-manually-(GW), add:
    // m_config.endGroup();		// corresponding to: m_config.beginGroup( GeneralOptionsConfigGroup );
    //  

    m_config->writeEntry("category_combo_index", m_categoryCombo->currentIndex());
    m_config->writeEntry("instrument_combo_index", m_instrumentCombo->currentIndex());
    m_config->writeEntry("player_combo_index", m_playerCombo->currentIndex());

    if (m_fromNotation) {
        m_config->writeEntry("convert_all_segments", m_convertAllSegments->isChecked());
    }
    else {
        m_config->writeEntry("convert_segments", m_convertSegments->isChecked());
    }
    
    QDialog::accept();
}

}
#include "PresetHandlerDialog.moc"
