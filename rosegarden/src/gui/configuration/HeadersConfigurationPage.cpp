
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
#include "document/io/LilypondExporter.h"
#include "gui/widgets/CollapsingFrame.h"
#include "misc/Strings.h"

#include <kapplication.h>
#include <kconfig.h>
#include <klistview.h>
#include <klocale.h>
#include <qgroupbox.h>
#include <qlabel.h>
#include <qlayout.h>
#include <qlineedit.h>
#include <qpushbutton.h>
#include <qstring.h>
#include <qtabwidget.h>
#include <qtooltip.h>
#include <qvbox.h>
#include <qwidget.h>
#include <qfont.h>

namespace Rosegarden
{

HeadersConfigurationPage::HeadersConfigurationPage(QWidget *parent,
	RosegardenGUIDoc *doc) :
	QVBox(parent),
	m_doc(doc)
{
    //
    // LilyPond export: Printable headers
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

    std::set<std::string> shown;

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
	QLineEdit *editHeader = new QLineEdit(strtoqstr( header ), frameHeaders);
	QString trName;
	if (key == headerDedication) {  
	    m_editDedication = editHeader;
	    row = 0; col = 2; width = 2;
	    trName = i18n("Dedication");
	} else if (key == headerTitle) {       
	    m_editTitle = editHeader;	
	    row = 1; col = 1; width = 4;
	    trName = i18n("Title");
	} else if (key == headerSubtitle) {
	    m_editSubtitle = editHeader;
	    row = 2; col = 1; width = 4;
	    trName = i18n("Subtitle");
	} else if (key == headerSubsubtitle) { 
	    m_editSubsubtitle = editHeader;
	    row = 3; col = 2; width = 2;
	    trName = i18n("Subsubtitle");
	} else if (key == headerPoet) {        
	    m_editPoet = editHeader;
	    row = 4; col = 0; width = 2;
	    trName = i18n("Poet");
	} else if (key == headerInstrument) {  
	    m_editInstrument = editHeader;
	    row = 4; col = 2; width = 2;
	    trName = i18n("Instrument");
	} else if (key == headerComposer) {    
	    m_editComposer = editHeader;
	    row = 4; col = 4; width = 2; 
	    trName = i18n("Composer");
	} else if (key == headerMeter) {       
	    m_editMeter = editHeader;
	    row = 5; col = 0; width = 3; 
	    trName = i18n("Meter");
	} else if (key == headerArranger) {    
	    m_editArranger = editHeader;
	    row = 5; col = 3; width = 3; 
	    trName = i18n("Arranger");
	} else if (key == headerPiece) {       
	    m_editPiece = editHeader;
	    row = 6; col = 0; width = 3; 
	    trName = i18n("Piece");
	} else if (key == headerOpus) {        
	    m_editOpus = editHeader;
	    row = 6; col = 3; width = 3; 
	    trName = i18n("Opus");
	} else if (key == headerCopyright) {   
	    m_editCopyright = editHeader;
	    row = 8; col = 1; width = 4; 
	    trName = i18n("Copyright");
	} else if (key == headerTagline) {     
	    m_editTagline = editHeader;
	    row = 9; col = 1; width = 4; 
	    trName = i18n("Tagline");
	}

	// editHeader->setReadOnly( true );
	editHeader->setAlignment( (col == 0 ? Qt::AlignLeft : (col >= 3 ? Qt::AlignRight : Qt::AlignCenter) ));

	layoutHeaders->addMultiCellWidget(editHeader, row, row, col, col+(width-1) );

	//
	// ToolTips
	//
	QToolTip::add( editHeader, trName );

	shown.insert(key);
    }
    QLabel *separator = new QLabel(i18n("The composition comes here."), frameHeaders);
    separator->setAlignment( Qt::AlignCenter );
    layoutHeaders->addMultiCellWidget(separator, 7, 7, 1, 4 );

    //
    // LilyPond export: Non-printable headers
    //

    CollapsingFrame *otherHeadersBox = new CollapsingFrame
        (i18n("Non-printable headers"), this, "nonprintableheaders");
    QFrame *frameOtherHeaders = new QFrame(otherHeadersBox);
    otherHeadersBox->setWidgetFill(true);
    QFont font(otherHeadersBox->font());
    font.setBold(false);
    otherHeadersBox->setFont(font);
    otherHeadersBox->setWidget(frameOtherHeaders);

    // set default expansion to false for this group -- what a faff
    KConfig *config = kapp->config();
    QString groupTemp = config->group();
    config->setGroup("CollapsingFrame");
    bool expanded = config->readBoolEntry("nonprintableheaders", false);
    config->writeEntry("nonprintableheaders", expanded);
    config->setGroup(groupTemp);

    QGridLayout *layoutOtherHeaders = new QGridLayout(frameOtherHeaders, 2, 2, 10, 5);

    m_metadata = new KListView(frameOtherHeaders);
    m_metadata->addColumn(i18n("Name"));
    m_metadata->addColumn(i18n("Value"));
    m_metadata->setFullWidth(true);
    m_metadata->setItemsRenameable(true);
    m_metadata->setRenameable(0);
    m_metadata->setRenameable(1);
    m_metadata->setItemMargin(5);
    m_metadata->setDefaultRenameAction(QListView::Accept);
    m_metadata->setShowSortIndicator(true);

    std::vector<std::string> names(metadata.getPropertyNames());

    for (unsigned int i = 0; i < names.size(); ++i) {

        if (shown.find(names[i]) != shown.end())
            continue;

        QString name(strtoqstr(names[i]));

        // property names stored in lower case
        name = name.left(1).upper() + name.right(name.length() - 1);

        new KListViewItem(m_metadata, name,
                          strtoqstr(metadata.get<String>(names[i])));

        shown.insert(names[i]);
    }

    layoutOtherHeaders->addMultiCellWidget(m_metadata, 0, 0, 0, 1);

    QPushButton* addPropButton = new QPushButton(i18n("Add New Property"),
                                 frameOtherHeaders);
    layoutOtherHeaders->addWidget(addPropButton, 1, 0, Qt::AlignHCenter);

    QPushButton* deletePropButton = new QPushButton(i18n("Delete Property"),
                                    frameOtherHeaders);
    layoutOtherHeaders->addWidget(deletePropButton, 1, 1, Qt::AlignHCenter);

    connect(addPropButton, SIGNAL(clicked()),
            this, SLOT(slotAddNewProperty()));

    connect(deletePropButton, SIGNAL(clicked()),
            this, SLOT(slotDeleteProperty()));
}

void
HeadersConfigurationPage::slotAddNewProperty()
{
    QString propertyName;
    int i = 0;

    while (1) {
        propertyName =
            (i > 0 ? i18n("{new property %1}").arg(i) : i18n("{new property}"));
        if (!m_doc->getComposition().getMetadata().has(qstrtostr(propertyName)) &&
	    m_metadata->findItem(qstrtostr(propertyName),0) == 0)
            break;
        ++i;
    }

    new KListViewItem(m_metadata, propertyName, i18n("{undefined}"));
}

void
HeadersConfigurationPage::slotDeleteProperty()
{
    delete m_metadata->currentItem();
}

void HeadersConfigurationPage::apply()
{
    KConfig *config = kapp->config();
    config->setGroup(NotationViewConfigGroup);

    // If one of the items still has focus, it won't remember edits.
    // Switch between two fields in order to lose the current focus.
    m_editTitle->setFocus();
    m_metadata->setFocus();

    //
    // Update header fields
    //

    Configuration &metadata = (&m_doc->getComposition())->getMetadata();
    metadata.clear();

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

    for (QListViewItem *item = m_metadata->firstChild();
            item != 0; item = item->nextSibling()) {

        metadata.set<String>(qstrtostr(item->text(0).lower()),
                             qstrtostr(item->text(1)));
    }

    m_doc->slotDocumentModified();
}

}
#include "HeadersConfigurationPage.moc"
