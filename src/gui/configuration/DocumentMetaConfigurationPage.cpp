/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2008 the Rosegarden development team.
 
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
#include "base/BasicQuantizer.h"
#include "base/RealTime.h"
#include "base/Segment.h"
#include "ConfigurationPage.h"
#include "document/RosegardenGUIDoc.h"
#include "gui/editors/notation/NotationStrings.h"
#include "gui/configuration/HeadersConfigurationPage.h"
#include "gui/general/GUIPalette.h"
#include "TabbedConfigurationPage.h"
#include <kconfig.h>
#include <QListView>
#include <QFrame>
#include <QLabel>
#include <QListView>
#include <QPixmap>
#include <QPushButton>
#include <QString>
#include <qtable.h>
#include <QTabWidget>
#include <QWidget>
#include <QLayout>


namespace Rosegarden
{

static QString durationToString(Rosegarden::Composition &comp,
				Rosegarden::timeT absTime,
				Rosegarden::timeT duration,
				Rosegarden::RealTime rt)
{
    return i18n("%1 minutes %2.%3%4 seconds (%5 units, %6 measures)", // TODO - PLURAL
	 rt.sec / 60, rt.sec % 60,
	 rt.msec() / 100, (rt.msec() / 10) % 10,
	 duration, comp.getBarNumber(absTime + duration) -
			   comp.getBarNumber(absTime));
}

class SegmentDataItem : public QTableItem
{
public:
    SegmentDataItem(QTable *t, QString s) :
	QTableItem(t, QTableItem::Never, s) { }
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

DocumentMetaConfigurationPage::DocumentMetaConfigurationPage(RosegardenGUIDoc *doc,
        QWidget *parent,
        const char *name) :
        TabbedConfigurationPage(doc, parent, name)
{
    m_headersPage = new HeadersConfigurationPage(this, doc);
    addTab(m_headersPage, i18n("Headers"));

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
    QGridLayout *layout = new QGridLayout(frame,
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
    layout->addWidget(new QLabel(i18n("%1 used, %2 total",
                                  usedTracks.size(),
                                  comp.getNbTracks()),
                                 frame), 3, 1);

    layout->addWidget(new QLabel(i18n("Segments:"), frame), 4, 0);
    layout->addWidget(new QLabel(i18n("%1 MIDI, %2 audio, %3 total",
                                  internalSegments,
                                  audioSegments,
                                  internalSegments + audioSegments),
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
DocumentMetaConfigurationPage::apply()
{
    m_headersPage->apply();

    m_doc->slotDocumentModified();
}

/* hjj: WHAT TO DO WITH THIS ?
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
            m_fixed->setCurrentIndex(item);
            return ;
        }

        ++i;
    }

    for (QListViewItem *item = m_metadata->firstChild();
            item != 0; item = item->nextSibling()) {

        if (item->text(0).toLower() != name)
            continue;

        m_metadata->setSelected(item, true);
        m_metadata->setCurrentIndex(item);
        return ;
    }
}
*/

}
#include "DocumentMetaConfigurationPage.moc"
