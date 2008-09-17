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
#include <QApplication>

#include <klocale.h>
#include "misc/Strings.h"
#include "misc/Debug.h"
#include "document/ConfigGroups.h"
#include "CategoryElement.h"
#include "PresetElement.h"
#include "PresetGroup.h"
#include <QComboBox>
#include <QSettings>
#include <QDialog>
#include <QDialogButtonBox>
#include <QGroupBox>
#include <QFrame>
#include <QLabel>
#include <QString>
#include <QWidget>
#include <QVBoxLayout>


namespace Rosegarden
{

PresetHandlerDialog::PresetHandlerDialog(QDialogButtonBox::QWidget *parent,
                                         bool fromNotation)
        : QDialog(parent),
        m_fromNotation(fromNotation)
{
    m_presets = new PresetGroup();
    m_categories = m_presets->getCategories();
    if (m_fromNotation) setWindowTitle(i18n("Convert notation for..."));

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

    setModal(true);
    setWindowTitle(i18n("Load track parameters preset"));
    QGridLayout *metagrid = new QGridLayout;
    setLayout(metagrid);

    QFrame *frame = new QFrame;
    frame->setContentsMargins(10, 10, 10, 10);
    QGridLayout *layout = new QGridLayout;
    layout->setSpacing(5);
    frame->setLayout(layout);

    metagrid->addWidget(frame, 0, 0);

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

    QGroupBox *scopeBox = new QGroupBox(i18n("Scope"));
    QVBoxLayout *scopeBoxLayout = new QVBoxLayout;
    if (m_fromNotation) {
        QRadioButton *onlySelectedSegments = new
            QRadioButton(i18n("Only selected segments"));
        scopeBoxLayout->addWidget(onlySelectedSegments);
        m_convertAllSegments = new 
            QRadioButton(i18n("All segments in this track"));
        scopeBoxLayout->addWidget(m_convertAllSegments);
        onlySelectedSegments->setChecked(true);
    }
    else {
        QRadioButton *onlyNewSegments = new 
            QRadioButton(i18n("Only for new segments"));
        scopeBoxLayout->addWidget(onlyNewSegments);
        m_convertSegments = new 
            QRadioButton(i18n("Convert existing segments"));
        scopeBoxLayout->addWidget(m_convertSegments);
        onlyNewSegments->setChecked(true);
    }
    scopeBox->setLayout(scopeBoxLayout);

    layout->addWidget(title, 0, 0, 0- 0+1, 1- 1, Qt::AlignLeft);
    layout->addWidget(catlabel, 1, 0, Qt::AlignRight);
    layout->addWidget(m_categoryCombo, 1, 1);
    layout->addWidget(inslabel, 2, 0, Qt::AlignRight);
    layout->addWidget(m_instrumentCombo, 2, 1);
    layout->addWidget(plylabel, 3, 0, Qt::AlignRight);
    layout->addWidget(m_playerCombo, 3, 1);
    layout->addWidget(scopeBox, 4, 0, 0+1, 1- 1, Qt::AlignLeft);

    populateCategoryCombo();
    // try to set to same category used previously
    QSettings settings;
    settings.beginGroup( GeneralOptionsConfigGroup );

    m_categoryCombo->setCurrentIndex( settings.value("category_combo_index", 0).toInt() );

    // populate the instrument combo
    slotCategoryIndexChanged(m_categoryCombo->currentIndex());

    // try to set to same instrument used previously

    //###settings.beginGroup( GeneralOptionsConfigGroup );
    m_instrumentCombo->setCurrentIndex( settings.value("instrument_combo_index", 0).toInt() );

    // set to same player used previously (this one can't fail, unlike the
    // others, because the contents of this combo are static)
    m_playerCombo->setCurrentIndex( settings.value("player_combo_index", 0).toInt() );

    if (m_fromNotation){
        m_convertAllSegments->setChecked( qStrToBool( settings.value("convert_all_segments", "0" ) ) );
    }
    else {
    	m_convertSegments->setChecked( qStrToBool( settings.value("convert_segments", "0" ) ) );
    }
    
    connect(m_categoryCombo, SIGNAL(activated(int)),
            SLOT(slotCategoryIndexChanged(int)));

    settings.endGroup();

    QDialogButtonBox *buttonBox
        = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    metagrid->addWidget(buttonBox, 1, 0);
    metagrid->setRowStretch(0, 10);
    connect(buttonBox, SIGNAL(accepted()), this, SLOT(accept()));
    connect(buttonBox, SIGNAL(rejected()), this, SLOT(reject()));
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
    QSettings settings;
    settings.beginGroup( GeneralOptionsConfigGroup );

    settings.setValue("category_combo_index", m_categoryCombo->currentIndex());
    settings.setValue("instrument_combo_index", m_instrumentCombo->currentIndex());
    settings.setValue("player_combo_index", m_playerCombo->currentIndex());

    if (m_fromNotation) {
        settings.setValue("convert_all_segments", m_convertAllSegments->isChecked());
    }
    else {
        settings.setValue("convert_segments", m_convertSegments->isChecked());
    }
    
    QDialog::accept();

    settings.endGroup();
}

}
#include "PresetHandlerDialog.moc"
