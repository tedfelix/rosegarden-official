/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
 
    This program is Copyright 2000-2006
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


#include "DocumentMetaConfigurationPage.h"

#include "base/Event.h"
#include "base/BaseProperties.h"
#include "misc/Strings.h"
#include "base/Colour.h"
#include "base/Composition.h"
#include "base/Configuration.h"
#include "base/NotationTypes.h"
#include "base/PropertyName.h"
#include "base/RealTime.h"
#include "base/Segment.h"
#include "ConfigurationPage.h"
#include "document/RosegardenGUIDoc.h"
#include "gui/editors/notation/NotationStrings.h"
#include "gui/general/GUIPalette.h"
#include "TabbedConfigurationPage.h"
#include <kconfig.h>
#include <klistview.h>
#include <qframe.h>
#include <qlabel.h>
#include <qlistview.h>
#include <qpixmap.h>
#include <qpushbutton.h>
#include <qstring.h>
#include <qtable.h>
#include <qtabwidget.h>
#include <qwidget.h>


namespace Rosegarden
{

DocumentMetaConfigurationPage::DocumentMetaConfigurationPage(RosegardenGUIDoc *doc,
        QWidget *parent,
        const char *name) :
        TabbedConfigurationPage(doc, parent, name)
{
    QFrame *frame = new QFrame(m_tabWidget);
    QGridLayout *layout = new QGridLayout(frame, 3, 2, 10, 5);

    m_fixed = new KListView(frame);
    m_fixed->addColumn(i18n("Name"));
    m_fixed->addColumn(i18n("Value"));
    m_fixed->setFullWidth(true);
    m_fixed->setItemsRenameable(true);
    m_fixed->setRenameable(1);
    m_fixed->setItemMargin(5);
    m_fixed->setSorting( -1);
    m_fixed->setDefaultRenameAction(QListView::Accept);
    m_fixed->setShowSortIndicator(false);

    m_metadata = new KListView(frame);
    m_metadata->addColumn(i18n("Name"));
    m_metadata->addColumn(i18n("Value"));
    m_metadata->setFullWidth(true);
    m_metadata->setItemsRenameable(true);
    m_metadata->setRenameable(0);
    m_metadata->setRenameable(1);
    m_metadata->setItemMargin(5);
    m_metadata->setDefaultRenameAction(QListView::Accept);
    m_metadata->setShowSortIndicator(true);

    Configuration &metadata = doc->getComposition().getMetadata();

    std::set
        <std::string> shown;

    std::vector<PropertyName> fixedKeys =
        CompositionMetadataKeys::getFixedKeys();

    // do these in reverse order, as the list appears to insert at the start
    for (unsigned int i = fixedKeys.size(); i > 0; --i) {
        PropertyName pn = fixedKeys[i - 1];
        QString trName;
        if (pn == CompositionMetadataKeys::Copyright) {
            trName = i18n("Copyright");
        } else if (pn == CompositionMetadataKeys::Composer) {
            trName = i18n("Composer");
        } else if (pn == CompositionMetadataKeys::Title) {
            trName = i18n("Title");
        } else if (pn == CompositionMetadataKeys::Subtitle) {
            trName = i18n("Subtitle");
        } else if (pn == CompositionMetadataKeys::Arranger) {
            trName = i18n("Arranger");
        } else {
            trName = strtoqstr(pn.getName());
            trName = trName.left(1).upper() + trName.right(trName.length() - 1);
        }
        new KListViewItem(m_fixed, trName, strtoqstr(metadata.get<String>(pn, "")));
        shown.insert(pn.getName());
    }

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

    layout->addMultiCellWidget(m_fixed, 0, 0, 0, 1);
    layout->addMultiCellWidget(m_metadata, 1, 1, 0, 1);

    QPushButton* addPropButton = new QPushButton(i18n("Add New Property"),
                                 frame);
    layout->addWidget(addPropButton, 2, 0, Qt::AlignHCenter);

    QPushButton* deletePropButton = new QPushButton(i18n("Delete Property"),
                                    frame);
    layout->addWidget(deletePropButton, 2, 1, Qt::AlignHCenter);

    connect(addPropButton, SIGNAL(clicked()),
            this, SLOT(slotAddNewProperty()));

    connect(deletePropButton, SIGNAL(clicked()),
            this, SLOT(slotDeleteProperty()));

    addTab(frame, i18n("Description"));

    Composition &comp = doc->getComposition();
    std::set
        <TrackId> usedTracks;

    int audioSegments = 0, internalSegments = 0;
    for (Composition::iterator ci = comp.begin();
            ci != comp.end(); ++ci) {
        usedTracks.insert((*ci)->getTrack());
        if ((*ci)->getType() == Segment::Audio)
            ++audioSegments;
        else
            ++internalSegments;
    }

    frame = new QFrame(m_tabWidget);
    layout = new QGridLayout(frame,
                             6, 2,
                             10, 5);

    layout->addWidget(new QLabel(i18n("Filename:"), frame), 0, 0);
    layout->addWidget(new QLabel(doc->getTitle(), frame), 0, 1);

    layout->addWidget(new QLabel(i18n("Formal duration (to end marker):"), frame), 1, 0);
    timeT d = comp.getEndMarker();
    RealTime rtd = comp.getElapsedRealTime(d);
    layout->addWidget(new QLabel(durationToString(comp, 0, d, rtd), frame), 1, 1);

    layout->addWidget(new QLabel(i18n("Playing duration:"), frame), 2, 0);
    d = comp.getDuration();
    rtd = comp.getElapsedRealTime(d);
    layout->addWidget(new QLabel(durationToString(comp, 0, d, rtd), frame), 2, 1);

    layout->addWidget(new QLabel(i18n("Tracks:"), frame), 3, 0);
    layout->addWidget(new QLabel(i18n("%1 used, %2 total")
                                 .arg(usedTracks.size())
                                 .arg(comp.getNbTracks()),
                                 frame), 3, 1);

    layout->addWidget(new QLabel(i18n("Segments:"), frame), 4, 0);
    layout->addWidget(new QLabel(i18n("%1 MIDI, %2 audio, %3 total")
                                 .arg(internalSegments)
                                 .arg(audioSegments)
                                 .arg(internalSegments + audioSegments),
                                 frame), 4, 1);

    layout->setRowStretch(5, 2);

    addTab(frame, i18n("Statistics"));

    frame = new QFrame(m_tabWidget);
    layout = new QGridLayout(frame, 1, 1, 10, 5);

    QTable *table = new QTable(1, 11, frame, "Segment Table");
    table->setSelectionMode(QTable::NoSelection);
    table->setSorting(true);
    table->horizontalHeader()->setLabel(0, i18n("Type"));
    table->horizontalHeader()->setLabel(1, i18n("Track"));
    table->horizontalHeader()->setLabel(2, i18n("Label"));
    table->horizontalHeader()->setLabel(3, i18n("Time"));
    table->horizontalHeader()->setLabel(4, i18n("Duration"));
    table->horizontalHeader()->setLabel(5, i18n("Events"));
    table->horizontalHeader()->setLabel(6, i18n("Polyphony"));
    table->horizontalHeader()->setLabel(7, i18n("Repeat"));
    table->horizontalHeader()->setLabel(8, i18n("Quantize"));
    table->horizontalHeader()->setLabel(9, i18n("Transpose"));
    table->horizontalHeader()->setLabel(10, i18n("Delay"));
    table->setNumRows(audioSegments + internalSegments);

    table->setColumnWidth(0, 50);
    table->setColumnWidth(1, 50);
    table->setColumnWidth(2, 150);
    table->setColumnWidth(3, 80);
    table->setColumnWidth(4, 80);
    table->setColumnWidth(5, 80);
    table->setColumnWidth(6, 80);
    table->setColumnWidth(7, 80);
    table->setColumnWidth(8, 80);
    table->setColumnWidth(9, 80);
    table->setColumnWidth(10, 80);

    int i = 0;

    for (Composition::iterator ci = comp.begin();
            ci != comp.end(); ++ci) {

        Segment *s = *ci;

        table->setItem(i, 0, new SegmentDataItem
                       (table,
                        s->getType() == Segment::Audio ?
                        i18n("Audio") : i18n("MIDI")));

        table->setItem(i, 1, new SegmentDataItem
                       (table,
                        QString("%1").arg(s->getTrack() + 1)));

        QPixmap colourPixmap(16, 16);
        Colour colour =
            comp.getSegmentColourMap().getColourByIndex(s->getColourIndex());
        colourPixmap.fill(GUIPalette::convertColour(colour));

        table->setItem(i, 2,
                       new QTableItem(table, QTableItem::Never,
                                      strtoqstr(s->getLabel()),
                                      colourPixmap));

        table->setItem(i, 3, new SegmentDataItem
                       (table,
                        QString("%1").arg(s->getStartTime())));

        table->setItem(i, 4, new SegmentDataItem
                       (table,
                        QString("%1").arg(s->getEndMarkerTime() -
                                          s->getStartTime())));

        std::set
            <long> notesOn;
        std::multimap<timeT, long> noteOffs;
        int events = 0, notes = 0, poly = 0, maxPoly = 0;

        for (Segment::iterator si = s->begin();
                s->isBeforeEndMarker(si); ++si) {
            ++events;
            if ((*si)->isa(Note::EventType)) {
                ++notes;
                timeT startTime = (*si)->getAbsoluteTime();
                timeT endTime = startTime + (*si)->getDuration();
                if (endTime == startTime)
                    continue;
                while (!noteOffs.empty() &&
                        (startTime >= noteOffs.begin()->first)) {
                    notesOn.erase(noteOffs.begin()->second);
                    noteOffs.erase(noteOffs.begin());
                }
                long pitch = 0;
                (*si)->get
                <Int>(BaseProperties::PITCH, pitch);
                notesOn.insert(pitch);
                noteOffs.insert(std::multimap<timeT, long>::value_type(endTime, pitch));
                poly = notesOn.size();
                if (poly > maxPoly)
                    maxPoly = poly;
            }
        }

        table->setItem(i, 5, new SegmentDataItem
                       (table,
                        QString("%1").arg(events)));

        table->setItem(i, 6, new SegmentDataItem
                       (table,
                        QString("%1").arg(maxPoly)));

        table->setItem(i, 7, new SegmentDataItem
                       (table,
                        s->isRepeating() ? i18n("Yes") : i18n("No")));

        timeT discard;

        if (s->getQuantizer() && s->hasQuantization()) {
            timeT unit = s->getQuantizer()->getUnit();
            table->setItem(i, 8, new SegmentDataItem
                           (table,
                            NotationStrings::makeNoteMenuLabel
                            (unit, true, discard, false)));
        } else {
            table->setItem(i, 8, new SegmentDataItem
                           (table,
                            i18n("Off")));
        }

        table->setItem(i, 9, new SegmentDataItem
                       (table,
                        QString("%1").arg(s->getTranspose())));

        if (s->getDelay() != 0) {
            if (s->getRealTimeDelay() != RealTime::zeroTime) {
                table->setItem(i, 10, new SegmentDataItem
                               (table,
                                QString("%1 + %2 ms")
                                .arg(NotationStrings::makeNoteMenuLabel
                                     (s->getDelay(), true, discard, false))
                                .arg(s->getRealTimeDelay().sec * 1000 +
                                     s->getRealTimeDelay().msec())));
            } else {
                table->setItem(i, 10, new SegmentDataItem
                               (table,
                                NotationStrings::makeNoteMenuLabel
                                (s->getDelay(), true, discard, false)));
            }
        } else if (s->getRealTimeDelay() != RealTime::zeroTime) {
            table->setItem(i, 10, new SegmentDataItem
                           (table,
                            QString("%2 ms")
                            .arg(s->getRealTimeDelay().sec * 1000 +
                                 s->getRealTimeDelay().msec())));
        } else {
            table->setItem(i, 10, new SegmentDataItem
                           (table,
                            i18n("None")));
        }

        ++i;
    }

    layout->addWidget(table, 0, 0);

    addTab(frame, i18n("Segment Summary"));

}

void
DocumentMetaConfigurationPage::slotAddNewProperty()
{
    QString propertyName;
    int i = 0;

    while (1) {
        propertyName =
            (i > 0 ? i18n("{new property %1}").arg(i) : i18n("{new property}"));
        if (!m_doc->getComposition().getMetadata().has(qstrtostr(propertyName)))
            break;
        ++i;
    }

    new KListViewItem(m_metadata, propertyName, i18n("{undefined}"));
}

void
DocumentMetaConfigurationPage::slotDeleteProperty()
{
    delete m_metadata->currentItem();
}

void
DocumentMetaConfigurationPage::apply()
{
    Configuration &metadata = m_doc->getComposition().getMetadata();
    metadata.clear();

    // If one of the items still has focus, it won't remember edits
    m_fixed->setFocus();
    m_metadata->setFocus();

    std::vector<PropertyName> fixedKeys =
        CompositionMetadataKeys::getFixedKeys();
    std::vector<PropertyName>::iterator i = fixedKeys.begin();

    for (QListViewItem *item = m_fixed->firstChild();
            item != 0; item = item->nextSibling()) {

        if (i == fixedKeys.end())
            break;

        metadata.set<String>(*i, qstrtostr(item->text(1)));

        ++i;
    }

    for (QListViewItem *item = m_metadata->firstChild();
            item != 0; item = item->nextSibling()) {

        metadata.set<String>(qstrtostr(item->text(0).lower()),
                             qstrtostr(item->text(1)));
    }

    m_doc->slotDocumentModified();
}

void
DocumentMetaConfigurationPage::selectMetadata(QString name)
{
    std::vector<PropertyName> fixedKeys =
        CompositionMetadataKeys::getFixedKeys();
    std::vector<PropertyName>::iterator i = fixedKeys.begin();

    for (QListViewItem *item = m_fixed->firstChild();
            item != 0; item = item->nextSibling()) {

        if (i == fixedKeys.end())
            break;

        if (name == strtoqstr(i->getName())) {
            m_fixed->setSelected(item, true);
            m_fixed->setCurrentItem(item);
            return ;
        }

        ++i;
    }

    for (QListViewItem *item = m_metadata->firstChild();
            item != 0; item = item->nextSibling()) {

        if (item->text(0).lower() != name)
            continue;

        m_metadata->setSelected(item, true);
        m_metadata->setCurrentItem(item);
        return ;
    }
}

}
