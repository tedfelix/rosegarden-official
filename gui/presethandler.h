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
    (based on gui/notefont.*)

    The moral right of the authors to claim authorship of this work
    has been asserted.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/
 
#ifndef _PRESET_HANDLER_H_
#define _PRESET_HANDLER_H_

#include <vector>

#include <qstring.h>
#include <qxml.h>

#include "Exception.h"

#include <kdialogbase.h>
#include <kcombobox.h>
#include <kconfig.h>

#include "clefindex.h"

class QWidget;
class QVBox;
class QVBoxLayout;
class QLabel;
class QVBox;
class QGridLayout;
class QLabel;
class QFrame;
class KComboBox;

class CategoryElement;
class PresetElement;

typedef std::vector<CategoryElement> CategoriesContainer;
typedef std::vector<PresetElement> ElementContainer;

/*
 * A container class for storing a set of data describing a real world
 * instrument for which one is writing musical notation
 */
class PresetElement
{
public:

    PresetElement(QString name,
		  int clef,
		  int transpose,
		  int highAm,
		  int lowAm,
		  int highPro,
		  int lowPro);

    ~PresetElement();

    // accessors
    QString getName()	 { return m_name;      }
    int getClef()    	 { return m_clef;      }
    int getTranspose()   { return m_transpose; }
    int getHighAm()	 { return m_highAm;    }     
    int getLowAm()	 { return m_lowAm;     }
    int getHighPro()	 { return m_highPro;   }
    int getLowPro() 	 { return m_lowPro;    }

private:
    QString m_name;
    int m_clef;
    int m_transpose;
    int m_highAm;
    int m_lowAm;
    int m_highPro;
    int m_lowPro;
}; // PresetElement


/*
 * A container class for storing a collection of PresetElement objects grouped
 * into the same musical category (eg. Flutes, Clarinets, Strings)
 */
class CategoryElement
{
public:
    CategoryElement(QString name);
    ~CategoryElement();

    void addPreset(QString name,
		  int clef,
		  int transpose,
		  int highAm,
		  int lowAm,
		  int highPro,
		  int lowPro);

    QString getName() { return m_name; }

    ElementContainer getPresets() { return m_categoryPresets; }
    PresetElement getPresetByIndex(int index) { return m_categoryPresets [index]; }

private:
    QString m_name;
    ElementContainer m_categoryPresets;
}; // CategoryElement


/*
 * Read presets.xml from disk and store a collection of PresetElement objects
 * which can then be used to populate and run the chooser GUI
 */
class PresetGroup : public QXmlDefaultHandler
{
public:
    typedef Rosegarden::Exception PresetFileReadFailed;

    PresetGroup(); // load and parse the XML mapping file
    ~PresetGroup();

    CategoriesContainer  getCategories() { return m_categories; }
    //CategoryElement getCategoryByIndex(int index) { return m_categories [index]; }

    // Xml handler methods:

    virtual bool startElement (const QString& namespaceURI, const QString& localName,
                               const QString& qName, const QXmlAttributes& atts);

    bool error(const QXmlParseException& exception);
    bool fatalError(const QXmlParseException& exception);

    // I don't think I have anything to do with this, but it must return true?
//    bool characters(const QString &) { return true; }

private:

    //--------------- Data members ---------------------------------
    CategoriesContainer m_categories;

    // For use when reading the XML file:
    QString m_errorString;
    QString m_presetDirectory;

    QString m_elCategoryName;
    QString m_elInstrumentName;
    int m_elClef;
    int m_elTranspose;
    int m_elLowAm;
    int m_elHighAm;
    int m_elLowPro;
    int m_elHighPro;

    int m_lastCategory;
    int m_currentCategory;
    int m_lastInstrument;
    int m_currentInstrument;

    bool m_name;
    bool m_clef;
    bool m_transpose;
    bool m_amateur;
    bool m_pro;

}; // PresetGroup

class PresetHandlerDialog : public KDialogBase
{
    Q_OBJECT

public:

    PresetHandlerDialog(QWidget* parent);
    ~PresetHandlerDialog();

    PresetGroup *m_presets;
    CategoriesContainer m_categories;

    KConfig *m_config;

    //-------[ accessor functions ]------------------------

    QString getName();

    int getClef();
    int getTranspose();
    int getLowRange();
    int getHighRange();

protected:

    //--------[ member functions ]-------------------------
    
    // initialize the dialog
    void initDialog();

    // populate the category combo
    void populateCategoryCombo();


    //---------[ data members ]-----------------------------

    static const char * const ConfigGroup;
    
    KComboBox	*m_categoryCombo;
    KComboBox	*m_instrumentCombo;
    KComboBox	*m_playerCombo;

protected slots:

    // de-populate and re-populate the Instrument combo when the category
    // changes.
    void slotCategoryIndexChanged(int index);

    // write out settings to kconfig data for next time and call accept()
    void slotOk();

}; // PresetHandlerDialog

#endif // _PRESET_HANDLER_H


