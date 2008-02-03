/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
 
    This program is Copyright 2000-2008
        Guillaume Laurent   <glaurent@telegraph-road.org>,
        Chris Cannam        <cannam@all-day-breakfast.com>,
        Richard Bown        <richard.bown@ferventsoftware.com>

    This file is Copyright 2006
	D. Michael McIntyre <dmmcintyr@users.sourceforge.net>
 
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


#include "PresetHandlerDialog.h"
#include <qlayout.h>
#include <kapplication.h>

#include <klocale.h>
#include "misc/Debug.h"
#include "document/ConfigGroups.h"
#include "CategoryElement.h"
#include "PresetElement.h"
#include "PresetGroup.h"
#include <kcombobox.h>
#include <kconfig.h>
#include <kdialogbase.h>
#include <qbuttongroup.h>
#include <qdialog.h>
#include <qframe.h>
#include <qgroupbox.h>
#include <qlabel.h>
#include <qstring.h>
#include <qvbox.h>
#include <qwidget.h>


namespace Rosegarden
{

PresetHandlerDialog::PresetHandlerDialog(QWidget *parent, bool fromNotation)
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
    m_categoryCombo = new KComboBox(frame);

    QLabel *inslabel = new QLabel(i18n("Instrument"), frame);
    m_instrumentCombo = new KComboBox(frame);

    QLabel *plylabel = new QLabel(i18n("Player Ability"), frame);
    m_playerCombo = new KComboBox(frame);
    m_playerCombo->insertItem(i18n("Amateur"));
    m_playerCombo->insertItem(i18n("Professional"));

    QGroupBox *scopeBox = new QButtonGroup
        (1, Horizontal, i18n("Scope"), frame);
    if (m_fromNotation) {
        QRadioButton *onlySelectedSegments = new
            QRadioButton(i18n("Only selected segments (EXPERIMENTAL)"), scopeBox);
        m_convertAllSegments = new 
            QRadioButton(i18n("All segments in this track (EXPERIMENTAL)"), scopeBox);
        onlySelectedSegments->setChecked(true);
    }
    else {
        QRadioButton *onlyNewSegments = new 
            QRadioButton(i18n("Only for new segments"), scopeBox);
        m_convertSegments = new 
            QRadioButton(i18n("Convert existing segments (EXPERIMENTAL)"), scopeBox);
        onlyNewSegments->setChecked(true);
    }
    
    layout->addMultiCellWidget(title, 0, 0, 0, 1, AlignLeft);
    layout->addWidget(catlabel, 1, 0, AlignRight);
    layout->addWidget(m_categoryCombo, 1, 1);
    layout->addWidget(inslabel, 2, 0, AlignRight);
    layout->addWidget(m_instrumentCombo, 2, 1);
    layout->addWidget(plylabel, 3, 0, AlignRight);
    layout->addWidget(m_playerCombo, 3, 1);
    layout->addMultiCellWidget(scopeBox, 4, 4, 0, 1, AlignLeft);

    populateCategoryCombo();
    // try to set to same category used previously
    m_config->setGroup(GeneralOptionsConfigGroup);
    m_categoryCombo->setCurrentItem(m_config->readNumEntry("category_combo_index", 0));

    // populate the instrument combo
    slotCategoryIndexChanged(m_categoryCombo->currentItem());

    // try to set to same instrument used previously
    m_config->setGroup(GeneralOptionsConfigGroup);
    m_instrumentCombo->setCurrentItem(m_config->readNumEntry("instrument_combo_index", 0));

    // set to same player used previously (this one can't fail, unlike the
    // others, because the contents of this combo are static)
    m_playerCombo->setCurrentItem(m_config->readNumEntry("player_combo_index", 0));

    if (m_fromNotation){
        m_convertAllSegments->setChecked(m_config->readBoolEntry("convert_all_segments", 0));
    }
    else {
    	m_convertSegments->setChecked(m_config->readBoolEntry("convert_segments", 0));
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
    PresetElement p = m_categories[m_categoryCombo->currentItem()].
                      getPresetByIndex(m_instrumentCombo->currentItem());

    return p.getClef();
}

int
PresetHandlerDialog::getTranspose()
{
    PresetElement p = m_categories[m_categoryCombo->currentItem()].
                      getPresetByIndex(m_instrumentCombo->currentItem());

    return p.getTranspose();
}

int
PresetHandlerDialog::getLowRange()
{
    PresetElement p = m_categories[m_categoryCombo->currentItem()].
                      getPresetByIndex(m_instrumentCombo->currentItem());
    // 0 == amateur
    // 1 == pro
    if (m_playerCombo->currentItem() == 0) {
        return p.getLowAm();
    } else {
        return p.getLowPro();
    }
}

int
PresetHandlerDialog::getHighRange()
{
    PresetElement p = m_categories[m_categoryCombo->currentItem()].
                      getPresetByIndex(m_instrumentCombo->currentItem());
    // 0 == amateur
    // 1 == pro
    if (m_playerCombo->currentItem() == 0) {
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

        m_categoryCombo->insertItem((*i).getName());
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

        m_instrumentCombo->insertItem((*i).getName());
    }

}

void
PresetHandlerDialog::slotOk()
{
    m_config->setGroup(GeneralOptionsConfigGroup);
    m_config->writeEntry("category_combo_index", m_categoryCombo->currentItem());
    m_config->writeEntry("instrument_combo_index", m_instrumentCombo->currentItem());
    m_config->writeEntry("player_combo_index", m_playerCombo->currentItem());

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
