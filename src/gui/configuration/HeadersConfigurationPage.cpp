
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

#define RG_MODULE_STRING "[HeadersConfigurationPage]"

#include "HeadersConfigurationPage.h"

#include "misc/ConfigGroups.h"
#include "document/RosegardenDocument.h"
#include "document/io/LilyPondExporter.h"
#include "gui/widgets/CollapsingFrame.h"
#include "misc/Strings.h"
#include "misc/Debug.h"
#include "gui/widgets/LineEdit.h"
#include "gui/configuration/CommentsConfigurationPage.h"
#include "gui/dialogs/ConfigureDialogBase.h"

#include <QApplication>
#include <QSettings>
#include <QListWidget>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QGroupBox>
#include <QLabel>
#include <QLayout>
#include <QPushButton>
#include <QString>
#include <QTabWidget>
#include <QToolTip>
#include <QWidget>
#include <QVBoxLayout>
#include <QFont>


namespace Rosegarden
{

HeadersConfigurationPage::HeadersConfigurationPage(
        QWidget *parent, RosegardenDocument *doc,
        ConfigureDialogBase *parentDialog) :
    QWidget(parent),
    m_doc(doc),
    m_parentDialog(parentDialog)
{
    QVBoxLayout *layout = new QVBoxLayout;

    //
    // LilyPond export: Printable headers
    //

    QGroupBox *headersBox = new QGroupBox(tr("Printable headers"), this);
    QHBoxLayout *headersBoxLayout = new QHBoxLayout;
    layout->addWidget(headersBox);
    QFrame *frameHeaders = new QFrame(headersBox);
    frameHeaders->setContentsMargins(10, 10, 10, 10);
    QGridLayout *layoutHeaders = new QGridLayout(frameHeaders);
    layoutHeaders->setSpacing(5);
    headersBoxLayout->addWidget(frameHeaders);
    headersBox->setLayout(headersBoxLayout);

    // grab user headers from metadata
    Configuration metadata = m_doc->getComposition().getMetadata();
    std::vector<std::string> propertyNames = metadata.getPropertyNames();
    std::vector<PropertyName> fixedKeys = CompositionMetadataKeys::getFixedKeys();

    // Fixed keys without value are not in propertyNames. Add them.
    for (unsigned int i = 0; i < fixedKeys.size(); i++) {
        std::string key = fixedKeys[i].getName();
        bool found = false;
        for (unsigned int j = 0; j < propertyNames.size(); ++j) {
            if (key == propertyNames[j]) {
                found = true;
                break;
            }
        }
        if (!found) {
            propertyNames.push_back(key);
        }
    }

    std::set<std::string> shown;

    for (unsigned int i = 0; i < propertyNames.size(); ++i) {
        std::string property = propertyNames[i];
        QString headerStr("");
        std::string key = "";
        std::string header = "";
        bool found = false;
        
        for (unsigned int index = 0; index < fixedKeys.size(); index++) {
            key = fixedKeys[index].getName();
            if (property == key) {
                // get the std::string from metadata
                std::string value = "";
                try { value = metadata.get<String>(property);
                } catch (Configuration::NoData) {
                    value = "";
                }
                header = value;
                //@@@ dtb: tr() only works with char* now, so I'm going to try
                // using header directly instead of a QString version of header.
                headerStr = QObject::trUtf8(header.c_str());
                found = true;
                break;
            }
        }

        if (!found) {
            if (strtoqstr(property).
                    startsWith(CommentsConfigurationPage::commentsKeyBase)) {
                // This metadata is a comment line
                shown.insert(property); // remember it
            }

            // Don't process here properties which are not a LilyPond header
            continue;
        }

        unsigned int row = 0, col = 0, width = 1;
        LineEdit *editHeader = new LineEdit(headerStr, frameHeaders);
        if (m_parentDialog) {
            connect(editHeader, SIGNAL(textEdited(QString)),
                    m_parentDialog, SLOT(slotActivateApply()));
        }
        QString trName;
        if (key == headerDedication) {  
            m_editDedication = editHeader;
            row = 0; col = 2; width = 2;
            trName = tr("Dedication");
        } else if (key == headerTitle) {       
            m_editTitle = editHeader;    
            row = 1; col = 1; width = 4;
            trName = tr("Title");
        } else if (key == headerSubtitle) {
            m_editSubtitle = editHeader;
            row = 2; col = 1; width = 4;
            trName = tr("Subtitle");
        } else if (key == headerSubsubtitle) { 
            m_editSubsubtitle = editHeader;
            row = 3; col = 2; width = 2;
            trName = tr("Subsubtitle");
        } else if (key == headerPoet) {        
            m_editPoet = editHeader;
            row = 4; col = 0; width = 2;
            trName = tr("Poet");
        } else if (key == headerInstrument) {  
            m_editInstrument = editHeader;
            row = 4; col = 2; width = 2;
            trName = tr("Instrument");
        } else if (key == headerComposer) {    
            m_editComposer = editHeader;
            row = 4; col = 4; width = 2; 
            trName = tr("Composer");
        } else if (key == headerMeter) {       
            m_editMeter = editHeader;
            row = 5; col = 0; width = 3; 
            trName = tr("Meter");
        } else if (key == headerArranger) {    
            m_editArranger = editHeader;
            row = 5; col = 3; width = 3; 
            trName = tr("Arranger");
        } else if (key == headerPiece) {       
            m_editPiece = editHeader;
            row = 6; col = 0; width = 3; 
            trName = tr("Piece");
        } else if (key == headerOpus) {        
            m_editOpus = editHeader;
            row = 6; col = 3; width = 3; 
            trName = tr("Opus");
        } else if (key == headerCopyright) {   
            m_editCopyright = editHeader;
            row = 8; col = 1; width = 4; 
            trName = tr("Copyright");
        } else if (key == headerTagline) {     
            m_editTagline = editHeader;
            row = 9; col = 1; width = 4; 
            trName = tr("Tagline");
        }

        // editHeader->setReadOnly(true);
        editHeader->setAlignment((col == 0 ? Qt::AlignLeft :
                            (col >= 3 ? Qt::AlignRight : Qt::AlignCenter)));

        layoutHeaders->addWidget(editHeader, row, col, 1, width);

        //
        // ToolTips
        //
        // (These particular LineEdit objects were still taking a mixture of styles
        // from the external sheet and the internal hard coded stylesheet in the
        // subclass LineEdit.  I guess I should have re-implemented QLineEdit
        // instead of subclassing it.
        QString localStyle("QToolTip {background-color: #FFFBD4; color: #000000;}"
                           " QLineEdit {background-color: #FFFFFF; color: #000000;}");
        editHeader->setStyleSheet(localStyle); 
        editHeader->setToolTip(trName);

        shown.insert(key);
    }
    QLabel *separator = new QLabel(tr("The composition comes here."), frameHeaders);
    separator->setAlignment(Qt::AlignCenter);
    layoutHeaders->addWidget(separator, 7, 1, 1, 4 - 1+1);

    frameHeaders->setLayout(layoutHeaders);


    //
    // LilyPond export: Non-printable headers
    //

    // set default expansion to false for this group -- what a faff
    // (see note in TrackParameterBox.cpp for an explanation of this baffling
    // but necessary code)
    QSettings settings;
    settings.beginGroup(CollapsingFrameConfigGroup);

    bool expanded = qStrToBool(settings.value("nonprintableheaders", "false")) ;
    settings.setValue("nonprintableheaders", expanded);
    settings.endGroup();

    CollapsingFrame *otherHeadersBox = new CollapsingFrame
        (tr("Additional headers"), this, "nonprintableheaders");
        
    layout->addWidget(otherHeadersBox);
    setLayout(layout);        
    
    QFrame *frameOtherHeaders = new QFrame(otherHeadersBox);
    otherHeadersBox->setWidgetFill(true);
    QFont font(otherHeadersBox->font());
    font.setBold(false);
    otherHeadersBox->setFont(font);
    otherHeadersBox->setWidget(frameOtherHeaders);

    frameOtherHeaders->setContentsMargins(10, 10, 10, 10);
    QGridLayout *layoutOtherHeaders = new QGridLayout(frameOtherHeaders);
    layoutOtherHeaders->setSpacing(5);

    m_metadata = new QTableWidget( 2, 2, frameOtherHeaders ); // rows, columns
    if (m_parentDialog) {
        connect(m_metadata, SIGNAL(cellChanged(int, int)),
                m_parentDialog, SLOT(slotActivateApply()));
    }
    m_metadata->setObjectName("StyledTable");
    m_metadata->setAlternatingRowColors(true);
    
    m_metadata->setHorizontalHeaderItem( 0, new QTableWidgetItem(tr("Name")) ); // column, item
    m_metadata->setHorizontalHeaderItem( 1, new QTableWidgetItem(tr("Value")) ); // column, item
    m_metadata->setMinimumSize(40, 40); // width, height

    std::vector<std::string> names(metadata.getPropertyNames());
    
    QTableWidgetItem* tabItem;
    int row = 0;
    
    for (unsigned int i = 0; i < names.size(); ++i) {

        if (shown.find(names[i]) != shown.end())
            continue;

        m_metadata->setRowCount(row + 1);

        QString name(strtoqstr(names[i]));

        // property names stored in lower case
        name = name.left(1).toUpper() + name.right(name.length() - 1);

        tabItem = new QTableWidgetItem(name);
        m_metadata->setItem(row, 0, tabItem);
        tabItem = new QTableWidgetItem(strtoqstr(metadata.get<String>(names[i])));
        m_metadata->setItem(row, 1, tabItem);

        shown.insert(names[i]);
        row++;
    }

    layoutOtherHeaders->addWidget(m_metadata, 0, 0, 1, 2);

    QPushButton* addPropButton = new QPushButton(tr("Add New Property"),
                                                 frameOtherHeaders);
    layoutOtherHeaders->addWidget(addPropButton, 1, 0, Qt::AlignHCenter);

    QPushButton* deletePropButton = new QPushButton(tr("Delete Property"),
                                                    frameOtherHeaders);
    layoutOtherHeaders->addWidget(deletePropButton, 1, 1, Qt::AlignHCenter);

    frameOtherHeaders->setLayout(layoutOtherHeaders);

    connect(addPropButton, SIGNAL(clicked()),
            this, SLOT(slotAddNewProperty()));

    connect(deletePropButton, SIGNAL(clicked()),
            this, SLOT(slotDeleteProperty()));
    
    if (m_parentDialog) {
        m_parentDialog->deactivateApply();
    }
}

void
HeadersConfigurationPage::slotAddNewProperty()
{
    QString propertyName;
    int i = 0;
    
    while (1) {
        propertyName =
            (i > 0 ? tr("{new property %1}").arg(i) : tr("{new property}"));
        QList<QTableWidgetItem*> foundItems = m_metadata->findItems(
                    propertyName, Qt::MatchContains | Qt::MatchCaseSensitive);
    
        if (!m_doc->getComposition().getMetadata().has(qstrtostr(propertyName)) &&
                     foundItems.isEmpty()){
            break;
        }
        ++i;
    }

    int rc = m_metadata->rowCount();
    m_metadata->setRowCount(rc + 1);
    m_metadata->setItem(rc, 0, new QTableWidgetItem(propertyName));
    m_metadata->setItem(rc, 1, new QTableWidgetItem()); // empty value

    if (m_parentDialog) {
        m_parentDialog->slotActivateApply();
    }
}

void
HeadersConfigurationPage::slotDeleteProperty()
{
    m_metadata->removeRow(m_metadata->currentRow());

    if (m_parentDialog) {
        m_parentDialog->slotActivateApply();
    }
}

static void setMetadataString(Configuration &metadata, const PropertyName &property, const QString &value)
{
    // Don't set empty values (this is to match the XML loading code)
    if (!value.isEmpty()) {
        metadata.set<String>(property, qstrtostr(value));
    }
}

void HeadersConfigurationPage::apply()
{
    // Should only be called from DocumentMetaConfigurationPage::apply()
    
    // If one of the items still has focus, it won't remember edits.
    // Switch between two fields in order to lose the current focus.
    m_editTitle->setFocus();
    m_metadata->setFocus();

    //
    // Update header fields
    //

    Configuration &metadata = m_doc->getComposition().getMetadata();
    const Configuration origmetadata = metadata;

    // If m_parentDialog is defined, HeaderConfigurationPage is owned by
    // DocumentMetaConfigurationPage along with CommentsConfigurationPage.
    // In this case HeaderConfigurationPage::apply() is called from
    // DocumentMetaConfigurationPage::apply() and must not clear the meta data
    // nor preserve the comments meta data.
    //
    // If m_parentDialog is null, HeaderConfigurationPage has been instantiated
    // alone, without DocumentMetaConfigurationPage and
    // CommentsConfigurationPage.
    // In this case HeaderConfigurationPage::apply() must clear the meta data
    // except the comments.
    if (!m_parentDialog) {

        // Clear the metadata
        metadata.clear();

        // Restore the comments from origmetadata
        for (Configuration::const_iterator
            	it = origmetadata.begin(); it != origmetadata.end(); ++it) {
            QString key = strtoqstr(it->first);
            if (key.startsWith(CommentsConfigurationPage::commentsKeyBase)) {
                metadata.set<String>(it->first,
                                     origmetadata.get<String>(it->first));
            }
        }
    }

    // Remember the current fixed keys metadata
    setMetadataString(metadata, CompositionMetadataKeys::Dedication, m_editDedication->text());
    setMetadataString(metadata, CompositionMetadataKeys::Title, m_editTitle->text());
    setMetadataString(metadata, CompositionMetadataKeys::Subtitle, m_editSubtitle->text());
    setMetadataString(metadata, CompositionMetadataKeys::Subsubtitle, m_editSubsubtitle->text());
    setMetadataString(metadata, CompositionMetadataKeys::Poet, m_editPoet->text());
    setMetadataString(metadata, CompositionMetadataKeys::Composer, m_editComposer->text());
    setMetadataString(metadata, CompositionMetadataKeys::Meter, m_editMeter->text());
    setMetadataString(metadata, CompositionMetadataKeys::Opus, m_editOpus->text());
    setMetadataString(metadata, CompositionMetadataKeys::Arranger, m_editArranger->text());
    setMetadataString(metadata, CompositionMetadataKeys::Instrument, m_editInstrument->text());
    setMetadataString(metadata, CompositionMetadataKeys::Piece, m_editPiece->text());
    setMetadataString(metadata, CompositionMetadataKeys::Copyright, m_editCopyright->text());
    setMetadataString(metadata, CompositionMetadataKeys::Tagline, m_editTagline->text());

    // Remember the other metadata
    for (int r=0; r < m_metadata->rowCount(); r++) {
        QTableWidgetItem* tabItem = m_metadata->item(r, 0);
        QTableWidgetItem* tabItem2 = m_metadata->item(r, 1);

        if ((!tabItem) || (!tabItem2)) {
            RG_DEBUG << "ERROR: Any TableWidgetItem is NULL in HeadersConfigurationPage::apply() " << endl;
            continue;
        }

        setMetadataString(metadata, qstrtostr(tabItem->text().toLower()), tabItem2->text());
    }

    // If m_parentDialog is defined, HeaderConfigurationPage is owned by
    // DocumentMetaConfigurationPage along with CommentsConfigurationPage.
    // In this case HeaderConfigurationPage::apply() can't know if 
    // m_doc->slotDocumentModified() has to be called because metadata has
    // just been cleared.
    // If needed DocumentMetaConfigurationPage::apply() will call
    // m_doc->slotDocumentModified().
    if (!m_parentDialog) {
        if (metadata != origmetadata) {
            m_doc->slotDocumentModified();
        }
    }

}

}
