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

#define RG_MODULE_STRING "[PresetHandlerDialog]"

#include "PresetHandlerDialog.h"

#include "CategoryElement.h"
#include "PresetElement.h"
#include "PresetGroup.h"

#include "misc/Strings.h"
#include "misc/Debug.h"
#include "misc/ConfigGroups.h"

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
#include <QApplication>
#include <QUrl>
#include <QDesktopServices>

// #define DEBUG_CATEGORIES

namespace Rosegarden
{

PresetHandlerDialog::PresetHandlerDialog(QWidget *parent,
                                         bool fromNotation)
        : QDialog(parent, 0),
        m_fromNotation(fromNotation)
{
    m_presets = new PresetGroup();
    m_categories = m_presets->getCategories();
    if (m_fromNotation) setWindowTitle(tr("Convert notation for..."));

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
    setWindowTitle(tr("Load track parameters preset"));
    QVBoxLayout *vboxLayout = new QVBoxLayout;
    setLayout(vboxLayout);

    QLabel *title = new QLabel(tr("Select preset track parameters for:"), this);
    if (m_fromNotation) title->setText(tr("Create appropriate notation for:"));
    vboxLayout->addWidget(title);

    QGroupBox *gbox = new QGroupBox(this);
    QGridLayout *gboxLayout = new QGridLayout;
    gbox->setLayout(gboxLayout);

    vboxLayout->addWidget(gbox);

    gboxLayout->addWidget(new QLabel(tr("Category")), 0, 0, Qt::AlignLeft);

    m_categoryCombo = new QComboBox(gbox);
    gboxLayout->addWidget(m_categoryCombo, 0, 1, Qt::AlignRight);

    // use longest string in database for setting width of combos (since I have
    // no luck with or understanding of how to use spacing or stretch &c to
    // accomplish the same sort of aim)
    QString metric(tr("Electronic organ (manual) (treble)"));
    m_categoryCombo->setMinimumContentsLength(metric.size());

    gboxLayout->addWidget(new QLabel(tr("Instrument")), 1, 0, Qt::AlignLeft);
    m_instrumentCombo = new QComboBox(gbox);
    m_instrumentCombo->setMinimumContentsLength(metric.size());
    gboxLayout->addWidget(m_instrumentCombo, 1, 1, Qt::AlignRight);

    gboxLayout->addWidget(new QLabel(tr("Player Ability")), 2, 0, Qt::AlignLeft);
    m_playerCombo = new QComboBox(gbox);
    m_playerCombo->addItem(tr("Amateur"));
    m_playerCombo->addItem(tr("Professional"));
    m_playerCombo->setMinimumContentsLength(metric.size());
    gboxLayout->addWidget(m_playerCombo, 2, 1, Qt::AlignRight);

    QGroupBox *scopeBox = new QGroupBox(tr("Scope"));
    QVBoxLayout *scopeBoxLayout = new QVBoxLayout;
    scopeBox->setLayout(scopeBoxLayout);

    vboxLayout->addWidget(scopeBox);

    if (m_fromNotation) {
        QRadioButton *onlySelectedSegments = new
            QRadioButton(tr("Only selected segments"));
        scopeBoxLayout->addWidget(onlySelectedSegments);
        m_convertAllSegments = new 
            QRadioButton(tr("All segments in this track"));
        scopeBoxLayout->addWidget(m_convertAllSegments);
        onlySelectedSegments->setChecked(true);
    }
    else {
        QRadioButton *onlyNewSegments = new 
            QRadioButton(tr("Only for new segments"));
        scopeBoxLayout->addWidget(onlyNewSegments);
        m_convertSegments = new 
            QRadioButton(tr("Convert existing segments"));
        scopeBoxLayout->addWidget(m_convertSegments);
        onlyNewSegments->setChecked(true);
    }

    populateCategoryCombo();

    // try to set to same category used previously
    QSettings settings;
    settings.beginGroup(PresetDialogConfigGroup);

    m_categoryCombo->setCurrentIndex(settings.value("category_combo_index", 0).toInt());

    // populate the instrument combo
    slotCategoryIndexChanged(m_categoryCombo->currentIndex());

    // try to set to same instrument used previously

    m_instrumentCombo->setCurrentIndex(settings.value("instrument_combo_index", 0).toInt());

    // set to same player used previously (this one can't fail, unlike the
    // others, because the contents of this combo are static)
    m_playerCombo->setCurrentIndex(settings.value("player_combo_index", 0).toInt());

    if (m_fromNotation){
        m_convertAllSegments->setChecked(qStrToBool(settings.value("convert_all_segments", "0")));
    }
    else {
        m_convertSegments->setChecked(qStrToBool(settings.value("convert_segments", "0")));
    }
    
    connect(m_categoryCombo, SIGNAL(activated(int)),
            SLOT(slotCategoryIndexChanged(int)));

    settings.endGroup();

    QDialogButtonBox *buttonBox
        = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel | QDialogButtonBox::Help);
    vboxLayout->addWidget(buttonBox);

    connect(buttonBox, SIGNAL(accepted()), this, SLOT(accept()));
    connect(buttonBox, SIGNAL(rejected()), this, SLOT(reject()));
    connect(buttonBox, SIGNAL(helpRequested()), this, SLOT(help()));
}

void
PresetHandlerDialog::help()
{
    // TRANSLATORS: if the manual is translated into your language, you can
    // change the two-letter language code in this URL to point to your language
    // version, eg. "http://rosegardenmusic.com/wiki/doc:manual-es" for the
    // Spanish version. If your language doesn't yet have a translation, feel
    // free to create one.
    QString helpURL = tr("http://rosegardenmusic.com/wiki/doc:manual-preset-handler-en");
    QDesktopServices::openUrl(QUrl(helpURL));
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

#ifdef DEBUG_CATEGORIES
        RG_DEBUG << "    adding category: " << (*i).getName() << endl;
#endif

        m_categoryCombo->addItem(QObject::tr((*i).getName().toStdString().c_str()));
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

#ifdef DEBUG_CATEGORIES
        RG_DEBUG << "    adding instrument: " << (*i).getName() << endl;
#endif

        m_instrumentCombo->addItem(QObject::tr((*i).getName().toStdString().c_str()));
    }

}

void
PresetHandlerDialog::accept()
{
    RG_DEBUG << "PresetHandlerDialog::accept()" << endl;

    QSettings settings;
    settings.beginGroup( PresetDialogConfigGroup );

    settings.setValue("category_combo_index", m_categoryCombo->currentIndex());
    settings.setValue("instrument_combo_index", m_instrumentCombo->currentIndex());
    settings.setValue("player_combo_index", m_playerCombo->currentIndex());

    if (m_fromNotation) {
        settings.setValue("convert_all_segments", m_convertAllSegments->isChecked());
    }
    else {
        settings.setValue("convert_segments", m_convertSegments->isChecked());
    }

    settings.endGroup();
    
    QDialog::accept();
}

}
