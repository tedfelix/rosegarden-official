// -*- c-basic-offset: 4 -*-

/*
    Rosegarden-4
    A sequencer and musical notation editor.

    This program is Copyright 2000-2006
        Guillaume Laurent   <glaurent@telegraph-road.org>,
        Chris Cannam        <cannam@all-day-breakfast.com>,
        Richard Bown        <bownie@bownie.com>

    This file is Copyright 2006
	D. Michael McIntyre <dmmcintyr@users.sourceforge.net>

    The moral right of the authors to claim authorship of this work
    has been asserted.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/
 
#include <vector>

#include <qstring.h>
#include <qxml.h>
#include <qfileinfo.h>
#include <qlabel.h>
#include <qframe.h>
#include <qvbox.h>
#include <qlayout.h>
#include <qregexp.h>

#include <klocale.h>
#include <kstandarddirs.h>
#include <kapplication.h>
#include <kconfig.h>
#include <kcombobox.h>

#include "constants.h"
#include "rosestrings.h"
#include "rosedebug.h"
#include "clefindex.h"

#include "Exception.h"

#include "presethandler.h"


////////////////////
//                //
// Preset Element //
//                //
////////////////////
PresetElement::PresetElement(QString name,
			     int clef,
			     int transpose,
			     int highAm,
			     int lowAm,
			     int highPro,
			     int lowPro) : 
    m_name (name),
    m_clef (clef),
    m_transpose (transpose),
    m_highAm (highAm),
    m_lowAm (lowAm),
    m_highPro (highPro),
    m_lowPro (lowPro)
{
    RG_DEBUG << "PresetElement::PresetElement(" << endl
	     << "    name = " << name << endl 
	     << "    clef = " << clef << endl
	     << "    trns.= " << transpose << endl
	     << "    higH = " << highAm << endl
	     << "    lowA = " << lowAm << endl
	     << "    higP = " << highPro << endl
	     << "    lowP = " << lowPro << ")" << endl;
}

PresetElement::~PresetElement()
{
    // nothing to do
}


/////////////////////
//                 //
// CategoryElement //
//                 //
/////////////////////
CategoryElement::CategoryElement(QString name) :
    m_name(name)
{
}

CategoryElement::~CategoryElement()
{
    // nothing to do
}

void
CategoryElement::addPreset(QString name,
			   int clef,
			   int transpose,
			   int highAm,
			   int lowAm,
			   int highPro,
			   int lowPro) 
{
    RG_DEBUG << "CategoryElement::addPreset(...): adding new PresetElement" << endl;

    PresetElement e(name, clef, transpose, highAm, lowAm,
	            highPro, lowPro);
    m_categoryPresets.push_back(e);
}


/////////////////
//             //
// PresetGroup //
//             //
/////////////////

PresetGroup::PresetGroup() :
    m_errorString(i18n("unknown error")),
    m_elCategoryName(""),
    m_elInstrumentName(""),
    m_elClef(0),
    m_elTranspose(0),
    m_elLowAm(0),
    m_elHighAm(0),
    m_elLowPro(0),
    m_elHighPro(0),
    m_lastCategory(-1),
    m_currentCategory(-1),
    m_lastInstrument(-1),
    m_currentInstrument(-1),
    m_name(false),
    m_clef(false),
    m_transpose(false),
    m_amateur(false),
    m_pro(false)
{
    m_presetDirectory = KGlobal::dirs()->findResource("appdata", "presets/");

    QString language = KGlobal::locale()->language();

    QString presetFileName = QString("%1/presets-%2.xml")
        .arg(m_presetDirectory).arg(language);

    if (!QFileInfo(presetFileName).isReadable()) {
	
	RG_DEBUG << "Failed to open " << presetFileName << endl;

	language.replace(QRegExp("_.*$"), "");
	presetFileName = QString("%1/presets-%2.xml")
	    .arg(m_presetDirectory).arg(language);

	if (!QFileInfo(presetFileName).isReadable()) {
	
	    RG_DEBUG << "Failed to open " << presetFileName << endl;

	    presetFileName = QString("%1/presets.xml")
		.arg(m_presetDirectory);
	    
	    if (!QFileInfo(presetFileName).isReadable()) {

		RG_DEBUG << "Failed to open " << presetFileName << endl;

		throw PresetFileReadFailed
		    (qstrtostr(i18n("Can't open preset file %1").
			       arg(presetFileName)));
	    }
	}
    }

    QFile presetFile(presetFileName);

    QXmlInputSource source(presetFile);
    QXmlSimpleReader reader;
    reader.setContentHandler(this);
    reader.setErrorHandler(this);
    bool ok = reader.parse(source);
    presetFile.close();

    if (!ok) {
        throw PresetFileReadFailed(qstrtostr(m_errorString));
    }
}

PresetGroup::~PresetGroup()
{
    //!!! do I have anything to do here?
}

bool
PresetGroup::startElement(const QString &, const QString &,
                          const QString &qName,
                          const QXmlAttributes &attributes)
{
    QString lcName = qName.lower();

//    RG_DEBUG << "PresetGroup::startElement: processing starting element: " << lcName << endl;

    if (lcName == "category") {

	QString s = attributes.value("name");
	if (s) { 
	    m_elCategoryName = s;
	    // increment the current category number
	    m_lastCategory = m_currentCategory;
            m_currentCategory++;

	    // reset the instrument counter going into the new category
	    m_lastInstrument = -1;
	    m_currentInstrument = -1;

	    RG_DEBUG << "PresetGroup::startElement: adding category " << m_elCategoryName << " last: "
		     << m_lastCategory << " curr: " << m_currentCategory << endl;

	    // add new CategoryElement to m_categories, in order to contain
	    // subsequent PresetElements
	    CategoryElement ce(m_elCategoryName);
	    m_categories.push_back(ce);
	}

    } else if (lcName == "instrument") {

	QString s = attributes.value("name");
	if (s) {
	    m_elInstrumentName = s;
	    m_name = true;

	    // increment the current instrument number
	    m_lastInstrument = m_currentInstrument;
	    m_currentInstrument++;
	 }

    } else if (lcName == "clef") {
	QString s = attributes.value("type");
	if (s) {
	    if (s == "treble") m_elClef = TrebleClef;
	    else if (s == "bass") m_elClef = BassClef;
	    else if (s == "crotales") m_elClef = CrotalesClef;
	    else if (s == "xylophone") m_elClef = XylophoneClef;
	    else if (s == "guitar") m_elClef = GuitarClef;
	    else if (s == "contrabass") m_elClef = ContrabassClef;
	    else if (s == "celesta") m_elClef = CelestaClef;
	    else if (s == "oldCelesta") m_elClef = OldCelestaClef;
	    else if (s == "soprano") m_elClef = SopranoClef;
	    else if (s == "alto") m_elClef = AltoClef;
	    else if (s == "tenor") m_elClef = TenorClef;
	    else if (s == "two-bar") m_elClef = TwoBarClef;
	    else {
		RG_DEBUG << "startElement: processed unrecognized clef type: " << s << endl;
	    }
	    m_clef = true;
	}

    } else if (lcName == "transpose") {
	QString s = attributes.value("value");
	if (s) {
	    m_elTranspose = s.toInt();
	    m_transpose = true;
	}

    } else if (lcName == "range") {
	QString s = attributes.value("class");

	if (s == "amateur") {
	    s = attributes.value("low");
	    if (s) {
		m_elLowAm = s.toInt();
		m_amateur = true;
	    }

	    s = attributes.value("high");
	    if (s && m_amateur) {
		m_elHighAm = s.toInt();
	    } else {
		return false;
	    }

	} else if (s == "professional") {
	    s = attributes.value("low");
	    if (s) {
		m_pro = true;
		m_elLowPro = s.toInt();
	    }

	    s = attributes.value("high");
	    if (s && m_pro) {
		m_elHighPro = s.toInt();
	    } else {
		return false;
	    }
	} else {
	    return false;
	}
    }

//    RG_DEBUG << "PresetGroup::startElement(): accumulating flags:" << endl
//	     << "     name: " << (m_name ? "true" : "false") << endl
//             << "     clef: " << (m_clef ? "true" : "false") << endl
//	     << "transpose: " << (m_transpose ? "true" : "false") << endl
//	     << "  am. rng: " << (m_amateur ? "true" : "false") << endl
//	     << "  pro rng: " << (m_pro ? "true" : "false") << endl;

    // once we have assembled all the bits, create a new PresetElement
    if (m_name && m_clef && m_transpose && m_amateur && m_pro) {
	m_categories[m_currentCategory].addPreset(m_elInstrumentName,
		                                   m_elClef,
						   m_elTranspose,
						   m_elHighAm,
						   m_elLowAm,
						   m_elHighPro,
						   m_elLowPro);	
	// increment the current instrument 
	//!!! (is this ever going to be needed?)
	m_lastInstrument = m_currentInstrument;
	m_currentInstrument++;

	// reset the "do we have a whole preset yet?" flags
	m_name = false;
	m_clef = false;
	m_transpose = false;
	m_amateur = false;
	m_pro = false; 
    } 

    return true;

} // startElement

bool
PresetGroup::error(const QXmlParseException& exception)
{
    RG_DEBUG << "PresetGroup::error(): jubilation and glee, we have an error, whee!" << endl;

    m_errorString = QString("%1 at line %2, column %3: %4")
	.arg(exception.message())
	.arg(exception.lineNumber())
	.arg(exception.columnNumber())
	.arg(m_errorString);
    return QXmlDefaultHandler::error(exception);
}

bool
PresetGroup::fatalError(const QXmlParseException& exception)
{
    RG_DEBUG << "PresetGroup::fatalError(): double your jubilation, and triple your glee, a fatal error doth it be!" << endl;
    m_errorString = QString("%1 at line %2, column %3: %4")
	.arg(exception.message())
	.arg(exception.lineNumber())
	.arg(exception.columnNumber())
	.arg(m_errorString);
    return QXmlDefaultHandler::fatalError(exception);
}


/////////////////////////
//                     //
// PresetHandlerDialog //
//                     //
/////////////////////////
PresetHandlerDialog::PresetHandlerDialog(QWidget *parent)
    : KDialogBase(parent, "presethandlerdialog", true, i18n("Load track parameters preset"), Ok|Cancel, Ok),
    m_config(kapp->config()) 
{
    m_presets = new PresetGroup();
    m_categories = m_presets->getCategories();

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

    QGridLayout *layout = new QGridLayout(frame, 5, 5, 10, 5);

    QLabel *title = new QLabel(i18n("Select preset track parameters for:"), frame);

    QLabel *catlabel = new QLabel(i18n("Category"),frame);
    m_categoryCombo = new KComboBox(frame);
    
    QLabel *inslabel = new QLabel(i18n("Instrument"),frame);
    m_instrumentCombo = new KComboBox(frame);

    QLabel *plylabel = new QLabel(i18n("Player Ability"), frame);
    m_playerCombo = new KComboBox(frame);
    m_playerCombo->insertItem(i18n("Amateur"));
    m_playerCombo->insertItem(i18n("Professional"));

    layout->addMultiCellWidget(title, 0, 0, 0, 1, AlignLeft);
    layout->addWidget(catlabel, 1, 0, AlignRight);
    layout->addWidget(m_categoryCombo, 1, 1);
    layout->addWidget(inslabel, 2, 0, AlignRight);
    layout->addWidget(m_instrumentCombo, 2, 1);
    layout->addWidget(plylabel, 3, 0, AlignRight);
    layout->addWidget(m_playerCombo, 3, 1);

    populateCategoryCombo();
    // try to set to same category used previously
    m_config->setGroup(Rosegarden::GeneralOptionsConfigGroup);
    m_categoryCombo->setCurrentItem(m_config->readNumEntry("category_combo_index", 0));

    // populate the instrument combo
    slotCategoryIndexChanged(m_categoryCombo->currentItem());

    // try to set to same instrument used previously
    m_config->setGroup(Rosegarden::GeneralOptionsConfigGroup);
    m_instrumentCombo->setCurrentItem(m_config->readNumEntry("instrument_combo_index", 0));

    // set to same player used previously (this one can't fail, unlike the
    // others, because the contents of this combo are static)
    m_playerCombo->setCurrentItem(m_config->readNumEntry("player_combo_index", 0));

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
    m_config->setGroup(Rosegarden::GeneralOptionsConfigGroup);
    m_config->writeEntry("category_combo_index", m_categoryCombo->currentItem());
    m_config->writeEntry("instrument_combo_index", m_instrumentCombo->currentItem());
    m_config->writeEntry("player_combo_index", m_playerCombo->currentItem());

    QDialog::accept();
}

#include "presethandler.moc"
