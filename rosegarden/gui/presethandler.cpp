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
 
//#include <string>
//#include <set>
//#include <map>
#include <vector>

#include <qstring.h>
#include <qxml.h>
#include <qfileinfo.h>

#include <klocale.h>
#include <kstandarddirs.h>
#include <kapplication.h>

#include "rosestrings.h"
#include "rosedebug.h"
#include "clefindex.h"

//#include "Exception.h"

#include "presethandler.h"

using std::string;
//using std::map;
//using std::set;
//using std::cout;
//using std::cerr;
//using std::endl;

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

    PresetElement *e  = new PresetElement(name, clef, transpose, highAm, lowAm,
	                                  highPro, lowPro);
    m_presets.push_back(*e);
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
    m_name(""),
    m_clef(false),
    m_transpose(false),
    m_amateur(false),
    m_pro(false)
{
    m_presetDirectory = KGlobal::dirs()->findResource("appdata", "presets/");

    QString mapFileName = QString("%1/presets.xml")
        .arg(m_presetDirectory);

    QFileInfo mapFileInfo(mapFileName);

    if (!mapFileInfo.isReadable()) {
	throw MappingFileReadFailed
	    (qstrtostr(i18n("Can't open preset file %1").
		       arg(mapFileName)));
    }

    QFile mapFile(mapFileName);

    QXmlInputSource source(mapFile);
    QXmlSimpleReader reader;
    reader.setContentHandler(this);
    reader.setErrorHandler(this);
    bool ok = reader.parse(source);
    mapFile.close();

    if (!ok) {
        throw MappingFileReadFailed(qstrtostr(m_errorString));
    }

//    m_categories = new
}

PresetGroup::~PresetGroup()
{
/*    for (SystemFontMap::iterator i = m_systemFontCache.begin();
	 i != m_systemFontCache.end(); ++i) {
	delete i->second;
    } */
}

bool
PresetGroup::startElement(const QString &, const QString &,
                          const QString &qName,
                          const QXmlAttributes &attributes)
{
    QString lcName = qName.lower();

    // if we have the start of a category tag
    if (lcName = "category") {

	// try to extract the name attribute
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
	    CategoryElement *ce = new CategoryElement(m_elCategoryName);
	    m_categories.push_back(*ce);
	}
    }

    // if we have the start of an instrument tag
    if (lcName = "instrument") {

	// try to extract the name attribute
	QString s = attributes.value("name");
	if (s) {
	    m_elInstrumentName = s;
	    m_name = true;

	    // increment the current instrument number
	    m_lastInstrument = m_currentInstrument;
	    m_currentInstrument++;
	 }
    }

    // convert text clef name to combo box index using the enum in clefindex.h
    if (lcName = "clef") {
	QString s = attributes.value("type");
	if (s) {
	    if (s == "treble") m_elClef = TrebleClef;
	    else if (s == "bass") m_elClef = BassClef;
	    else if (s == "alto") m_elClef = AltoClef;
	    else if (s == "tenor") m_elClef = TenorClef;
	    else if (s == "guitar") m_elClef = GuitarClef;
	    else if (s == "xylophone") m_elClef = XylophoneClef;
	    else if (s == "celesta") m_elClef = CelestaClef;
	    else if (s == "oldCelesta") m_elClef = OldCelestaClef;
	    else if (s == "soprano") m_elClef = SopranoClef;
	    else if (s == "two-bar") m_elClef = TwoBarClef;
	    m_clef = true;
	}
    }

    if (lcName = "transpose") {
	QString s = attributes.value("value");
	if (s) {
	    m_elTranspose = s.toInt();
	    m_transpose = true;
	}
    }

    if (lcName = "range") {
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
		// critical error
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
		// critical error
	    }
	} else {
	    // critical error
	}
    }

    // once we have assembled all the bits, create a new PresetElement
    if (m_name && m_clef && m_transpose && m_amateur && m_pro) {
	m_categories[m_currentCategory].addPreset(m_elInstrumentName,
		                                   m_elClef,
						   m_elTranspose,
						   m_elLowAm,
						   m_elHighAm,
						   m_elLowPro,	
						   m_elHighPro);
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

} // startElement

bool
PresetGroup::error(const QXmlParseException& exception)
{
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
    : KDialogBase(parent, "presethandlerdialog", true, i18n("Load track parameters preset"), Ok|Cancel, Ok)/*,
    cfg(kapp->config()) */
{
    initDialog();
}

PresetHandlerDialog::~PresetHandlerDialog()
{
    // nothing
}

void
PresetHandlerDialog::initDialog()
{
    // make a little dialog with three combos in it and plumb it up
}

QString
PresetHandlerDialog::getName()
{
    return m_instrumentCombo->currentText();
}

int
PresetHandlerDialog::getClef()
{
    // TODO
}

int
PresetHandlerDialog::getTranspose()
{
    // return:
    // PresetElement->getTranspose() for the preset element at instrument combo index
    // in the category element at category combo index in the PresetGroup n
    //
    // group->category(cat_comboIndex)->element(ins_comboIndex)->getTranspose()
}

int
PresetHandlerDialog::getLowRange()
{
    // if range combo is amateur use high/lowAm
    // else high/lowPro
}

int
PresetHandlerDialog::getHighRange()
{
    // as above
}

void
 PresetHandlerDialog::populateCategoryCombo()
{
    // new ParameterGroup
    // for iterator thingie { populate the combo}
}

void
PresetHandlerDialog::slotCategoryIndexChanged(int index)
{
    // for category [index] { populate the instrument combo }
}

/*void
PresetHandlerDialog::slotOK()
{
    // write the amateur/pro setting to config and
    slotApply();
}*/

#include "presethandler.moc"
