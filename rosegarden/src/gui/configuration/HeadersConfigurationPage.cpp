
/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.

    This program is Copyright 2000-2007
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

#include "HeadersConfigurationPage.h"

#include "document/ConfigGroups.h"
#include "document/RosegardenGUIDoc.h"

#include <kconfig.h>
#include <klocale.h>
#include <qgroupbox.h>
#include <qlabel.h>
#include <qstring.h>
#include <qtabwidget.h>
#include <qtooltip.h>
#include <qvbox.h>
#include <qwidget.h>

namespace Rosegarden
{

HeadersConfigurationPage::HeadersConfigurationPage(QWidget *parent,
	RosegardenGUIDoc *doc) :
	QVBox(parent),
	m_doc(doc)
{
    //
    // LilyPond export: Headers
    //

    QGroupBox *headersBox = new QGroupBox
                           (1, Horizontal,
                            i18n("Printable headers"), this);

    QFrame *frameHeaders = new QFrame(headersBox);
    QGridLayout *layoutHeaders = new QGridLayout(frameHeaders, 10, 6, 10, 5);

    // grab user headers from metadata
    Configuration metadata = (&m_doc->getComposition())->getMetadata();
    std::vector<std::string> propertyNames = metadata.getPropertyNames();
    std::vector<PropertyName> fixedKeys =
	CompositionMetadataKeys::getFixedKeys();

    for (unsigned int index = 0; index < fixedKeys.size(); index++) {
	std::string key = fixedKeys[index].getName();
	std::string header = "";
	for (unsigned int i = 0; i < propertyNames.size(); ++i) {
	    std::string property = propertyNames [i];
	    if (property == key) {
		header = metadata.get<String>(property);
	    }
	}

	unsigned int row = 0, col = 0, width = 1;
	QLineEdit *editHeader = new QLineEdit( QString( strtoqstr( header ) ), frameHeaders );
	if (key == headerDedication) {  
	    m_editDedication = editHeader;
	    row = 0; col = 2; width = 2;
	} else if (key == headerTitle) {       
	    m_editTitle = editHeader;	
	    row = 1; col = 1; width = 4;
	} else if (key == headerSubtitle) {
	    m_editSubtitle = editHeader;
	    row = 2; col = 1; width = 4;
	} else if (key == headerSubsubtitle) { 
	    m_editSubsubtitle = editHeader;
	    row = 3; col = 2; width = 2;
	} else if (key == headerPoet) {        
	    m_editPoet = editHeader;
	    row = 4; col = 0; width = 2;
	} else if (key == headerInstrument) {  
	    m_editInstrument = editHeader;
	    row = 4; col = 2; width = 2;
	} else if (key == headerComposer) {    
	    m_editComposer = editHeader;
	    row = 4; col = 4; width = 2; 
	} else if (key == headerMeter) {       
	    m_editMeter = editHeader;
	    row = 5; col = 0; width = 3; 
	} else if (key == headerArranger) {    
	    m_editArranger = editHeader;
	    row = 5; col = 3; width = 3; 
	} else if (key == headerPiece) {       
	    m_editPiece = editHeader;
	    row = 6; col = 0; width = 3; 
	} else if (key == headerOpus) {        
	    m_editOpus = editHeader;
	    row = 6; col = 3; width = 3; 
	} else if (key == headerCopyright) {   
	    m_editCopyright = editHeader;
	    row = 8; col = 1; width = 4; 
	} else if (key == headerTagline) {     
	    m_editTagline = editHeader;
	    row = 9; col = 1; width = 4; 
	}

	// editHeader->setReadOnly( true );
	editHeader->setAlignment( (col == 0 ? Qt::AlignLeft : (col >= 3 ? Qt::AlignRight : Qt::AlignCenter) ));

	layoutHeaders->addMultiCellWidget(editHeader, row, row, col, col+(width-1) );

	//
	// ToolTips
	//
	QToolTip::add( editHeader, key );
    }
    QLabel *separator = new QLabel(i18n("The composition comes here."), frameHeaders);
    separator->setAlignment( Qt::AlignCenter );
    layoutHeaders->addMultiCellWidget(separator, 7, 7, 1, 4 );
}


void HeadersConfigurationPage::apply()
{
    KConfig *config = kapp->config();
    config->setGroup(NotationViewConfigGroup);

    //
    // Update header fields
    //

    Configuration &metadata = (&m_doc->getComposition())->getMetadata();
    metadata.set<String>(CompositionMetadataKeys::Dedication, qstrtostr(m_editDedication->text()));
    metadata.set<String>(CompositionMetadataKeys::Title, qstrtostr(m_editTitle->text()));
    metadata.set<String>(CompositionMetadataKeys::Subtitle, qstrtostr(m_editSubtitle->text()));
    metadata.set<String>(CompositionMetadataKeys::Subsubtitle, qstrtostr(m_editSubsubtitle->text()));
    metadata.set<String>(CompositionMetadataKeys::Poet, qstrtostr(m_editPoet->text()));
    metadata.set<String>(CompositionMetadataKeys::Composer, qstrtostr(m_editComposer->text()));
    metadata.set<String>(CompositionMetadataKeys::Meter, qstrtostr(m_editMeter->text()));
    metadata.set<String>(CompositionMetadataKeys::Opus, qstrtostr(m_editOpus->text()));
    metadata.set<String>(CompositionMetadataKeys::Arranger, qstrtostr(m_editArranger->text()));
    metadata.set<String>(CompositionMetadataKeys::Instrument, qstrtostr(m_editInstrument->text()));
    metadata.set<String>(CompositionMetadataKeys::Piece, qstrtostr(m_editPiece->text()));
    metadata.set<String>(CompositionMetadataKeys::Copyright, qstrtostr(m_editCopyright->text()));
    metadata.set<String>(CompositionMetadataKeys::Tagline, qstrtostr(m_editTagline->text()));

    m_doc->slotDocumentModified();
}

}
#include "HeadersConfigurationPage.moc"
