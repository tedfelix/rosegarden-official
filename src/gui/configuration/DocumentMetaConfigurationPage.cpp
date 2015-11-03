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


#include "DocumentMetaConfigurationPage.h"
#include "TabbedConfigurationPage.h"

#include "base/Event.h"
#include "base/BaseProperties.h"
#include "misc/Strings.h"
#include "base/Colour.h"
#include "base/Composition.h"
#include "base/Configuration.h"
#include "base/NotationTypes.h"
#include "base/PropertyName.h"
#include "base/BasicQuantizer.h"
#include "base/RealTime.h"
#include "base/Segment.h"
#include "ConfigurationPage.h"
#include "document/RosegardenDocument.h"
#include "gui/editors/notation/NotationStrings.h"
#include "gui/configuration/HeadersConfigurationPage.h"
#include "gui/general/GUIPalette.h"

#include <QSettings>
#include <QFrame>
#include <QLabel>
#include <QPixmap>
#include <QPushButton>
#include <QString>
#include <QTableWidget>
#include <QTabWidget>
#include <QWidget>
#include <QLayout>


namespace Rosegarden
{

class SegmentDataItem : public QTableWidgetItem
{
public:
    SegmentDataItem(QTableWidget */* t */, QString s) : QTableWidgetItem(s) { }    //### removed t from p-class
    
    virtual int alignment() const { return Qt::AlignCenter; }

    virtual QString key() const {

    // It doesn't seem to be possible to specify a comparator so
    // as to get the right sorting for numeric items (what am I
    // missing here?), only to override this function to return a
    // string for comparison.  So for integer items we'll return a
    // string that starts with a single digit corresponding to the
    // number of digits in the integer, which should ensure that
    // dictionary sorting works correctly.
    // 
    // This relies on the assumption that any item whose text
    // starts with a digit will contain nothing other than a
    // single non-negative integer of no more than 9 digits.  That
    // assumption should hold for all current uses of this class,
    // but may need checking for future uses...

    QString s(text());
    if (s[0].digitValue() >= 0) {
        return QString("%1%2").arg(s.length()).arg(s);
    } else {
        return s;
    }
    }
};

DocumentMetaConfigurationPage::DocumentMetaConfigurationPage(RosegardenDocument *doc,
        QWidget *parent) :
        TabbedConfigurationPage(doc, parent)
{
    m_headersPage = new HeadersConfigurationPage(this, doc);
    addTab(m_headersPage, tr("Headers"));

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

    QFrame *frame = new QFrame(m_tabWidget);
    frame->setContentsMargins(10, 10, 10, 10);
    QGridLayout *layout = new QGridLayout(frame);
    layout->setSpacing(5);

    layout->addWidget(new QLabel(tr("Filename:"), frame), 0, 0);
    layout->addWidget(new QLabel(doc->getTitle(), frame), 0, 1);

    layout->addWidget(new QLabel(tr("Formal duration (to end marker):"), frame), 1, 0);
    timeT d = comp.getEndMarker();
    RealTime rtd = comp.getElapsedRealTime(d);
    layout->addWidget(new QLabel(durationToString(comp, 0, d, rtd), frame), 1, 1);

    layout->addWidget(new QLabel(tr("Playing duration:"), frame), 2, 0);
    d = comp.getDuration();
    rtd = comp.getElapsedRealTime(d);
    layout->addWidget(new QLabel(durationToString(comp, 0, d, rtd), frame), 2, 1);

    layout->addWidget(new QLabel(tr("Tracks:"), frame), 3, 0);
    layout->addWidget(new QLabel(tr("%1 used, %2 total")
                                  .arg(usedTracks.size())
                                  .arg(comp.getNbTracks()),
                                 frame), 3, 1);

    layout->addWidget(new QLabel(tr("Segments:"), frame), 4, 0);
    layout->addWidget(new QLabel(tr("%1 MIDI, %2 audio, %3 total")
                                  .arg(internalSegments)
                                  .arg(audioSegments)
                                  .arg(internalSegments + audioSegments),
                                 frame), 4, 1);

    layout->setRowStretch(5, 2);

    addTab(frame, tr("Statistics"));

    frame = new QFrame(m_tabWidget);
    frame->setContentsMargins(10, 10, 10, 10);
    layout = new QGridLayout(frame);
    layout->setSpacing(5);

    QTableWidget *table = new QTableWidget(1, 11, frame); // , "Segment Table"
    table->setObjectName("StyledTable");
    table->setAlternatingRowColors(true);
    //table->setSelectionMode(QTableWidget::NoSelection);
    table->setSelectionBehavior( QAbstractItemView::SelectRows );
    table->setSelectionMode( QAbstractItemView::SingleSelection );
    
    // Sorting must be disabled while filling the table
    table->setSortingEnabled(false);
    
    table->setHorizontalHeaderItem( 0, new QTableWidgetItem( tr("Type")));    // p1=column
    table->setHorizontalHeaderItem( 1, new QTableWidgetItem( tr("Track")));
    table->setHorizontalHeaderItem( 2, new QTableWidgetItem( tr("Label")));
    table->setHorizontalHeaderItem( 3, new QTableWidgetItem( tr("Time")));
    table->setHorizontalHeaderItem( 4, new QTableWidgetItem( tr("Duration")));
    table->setHorizontalHeaderItem( 5, new QTableWidgetItem( tr("Events")));
    table->setHorizontalHeaderItem( 6, new QTableWidgetItem( tr("Polyphony")));
    table->setHorizontalHeaderItem( 7, new QTableWidgetItem( tr("Repeat")));
    table->setHorizontalHeaderItem( 8, new QTableWidgetItem( tr("Quantize")));
    table->setHorizontalHeaderItem( 9, new QTableWidgetItem( tr("Transpose")));
    table->setHorizontalHeaderItem( 10, new QTableWidgetItem( tr("Delay")));
    
    //table->setNumRows(audioSegments + internalSegments);
    table->setRowCount(audioSegments + internalSegments);

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
                        tr("Audio") : tr("MIDI")));

        table->setItem(i, 1, new SegmentDataItem
                       (table,
                        QString("%1").arg(s->getTrack() + 1)));

        QPixmap colourPixmap(16, 16);
        Colour colour =
            comp.getSegmentColourMap().getColourByIndex(s->getColourIndex());
        colourPixmap.fill(GUIPalette::convertColour(colour));

        table->setItem(i, 2,
                       new QTableWidgetItem( colourPixmap, strtoqstr(s->getLabel())) );
//        new QTableWidgetItem(table, QTableWidgetItem::Never,
    //                         strtoqstr(s->getLabel()),
        //                               colourPixmap));

        table->setItem(i, 3, new SegmentDataItem
                       (table,
                        QString("%1").arg(s->getStartTime())));

        table->setItem(i, 4, new SegmentDataItem
                       (table,
                        QString("%1").arg(s->getEndMarkerTime() -
                                          s->getStartTime())));

        std::set<long> notesOn;
        std::multimap<timeT, long> noteOffs;
        int events = 0, notes = 0, poly = 0, maxPoly = 0;

        for (Segment::iterator si = s->begin();
                s->isBeforeEndMarker(si); ++si) {
            ++events;
            if ((*si)->isa(Note::EventType)) {
                ++notes;
                timeT startTime = (*si)->getAbsoluteTime();
                timeT endTime = startTime + (*si)->getDuration();
                if (endTime == startTime) continue;
                while (!noteOffs.empty() &&
                        (startTime >= noteOffs.begin()->first)) {
                    notesOn.erase(noteOffs.begin()->second);
                    noteOffs.erase(noteOffs.begin());
                }
                long pitch = 0;
                (*si)->get<Int>(BaseProperties::PITCH, pitch);
                notesOn.insert(pitch);
                noteOffs.insert(std::multimap<timeT, long>::value_type(endTime, pitch));
                poly = notesOn.size();
                if (poly > maxPoly) maxPoly = poly;
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
                        s->isRepeating() ? tr("Yes") : tr("No")));

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
                            tr("Off")));
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
                            tr("None")));
        }

        ++i;
    }

    // Sorting may be enabled now
    table->setSortingEnabled(true);

    layout->addWidget(table, 0, 0);

    addTab(frame, tr("Segment Summary"));

}

void
DocumentMetaConfigurationPage::apply()
{
    m_headersPage->apply();

    m_doc->slotDocumentModified();
}

}
#include "DocumentMetaConfigurationPage.moc"
