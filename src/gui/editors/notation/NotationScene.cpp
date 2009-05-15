/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2009 the Rosegarden development team.

    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#include "NotationScene.h"

#include "base/Segment.h"
#include "base/BaseProperties.h"

#include "NotationStaff.h"
#include "NotationHLayout.h"
#include "NotationVLayout.h"
#include "NotePixmapFactory.h"
#include "NotationProperties.h"
#include "NotationWidget.h"
#include "NotationMouseEvent.h"
#include "NoteFontFactory.h"

#include "misc/Debug.h"
#include "misc/Strings.h"

#include "misc/ConfigGroups.h"
#include "document/CommandHistory.h"
#include "document/RosegardenDocument.h"

#include "gui/studio/StudioControl.h"
#include "sound/MappedEvent.h"

#include <QSettings>
#include <QGraphicsSceneMouseEvent>

using std::vector;

namespace Rosegarden
{

static int instanceCount = 0;

NotationScene::NotationScene() :
    m_widget(0),
    m_document(0),
    m_properties(0),
    m_notePixmapFactory(0),
    m_notePixmapFactorySmall(0),
    m_selection(0),
    m_hlayout(0),
    m_vlayout(0),
    m_title(0),
    m_subtitle(0),
    m_composer(0),
    m_copyright(0),
//    m_pageMode(StaffLayout::MultiPageMode),
    m_pageMode(StaffLayout::LinearMode),
    m_printSize(5),
    m_leftGutter(20),
    m_currentStaff(0),
    m_compositionRefreshStatusId(0),
    m_timeSignatureChanged(false)
{
    QString prefix(QString("NotationScene%1::").arg(instanceCount++));
    m_properties = new NotationProperties(qstrtostr(prefix));

//    qRegisterMetaType<NotationMouseEvent>("Rosegarden::NotationMouseEvent");

    setNotePixmapFactories();

    connect(CommandHistory::getInstance(), SIGNAL(commandExecuted()),
            this, SLOT(slotCommandExecuted()));
}

NotationScene::~NotationScene()
{
    if (m_document) {
        if (!isCompositionDeleted()) { // implemented in CompositionObserver
            m_document->getComposition().removeObserver(this);
        }
    }
    delete m_hlayout;
    delete m_vlayout;
    for (unsigned int i = 0; i < m_staffs.size(); ++i) delete m_staffs[i];
}

void
NotationScene::setNotePixmapFactories(QString fontName, int size)
{
    delete m_notePixmapFactory;
    delete m_notePixmapFactorySmall;
    
    m_notePixmapFactory = new NotePixmapFactory(fontName, size);

    fontName = m_notePixmapFactory->getFontName();
    size = m_notePixmapFactory->getSize();

    std::vector<int> sizes = NoteFontFactory::getScreenSizes(fontName);
    int small = size;
    for (unsigned int i = 0; i < sizes.size(); ++i) {
        if (sizes[i] == size || sizes[i] > size*3 / 4) break;
        small = sizes[i];
    }

    m_notePixmapFactorySmall = new NotePixmapFactory(fontName, small);

    if (m_hlayout) m_hlayout->setNotePixmapFactory(m_notePixmapFactory);
    if (m_vlayout) m_vlayout->setNotePixmapFactory(m_notePixmapFactory);
}

void
NotationScene::setNotationWidget(NotationWidget *w)
{
    m_widget = w;
}

const RulerScale *
NotationScene::getRulerScale() const
{
    return m_hlayout;
}

void
NotationScene::setStaffs(RosegardenDocument *document,
                          vector<Segment *> segments)
{
    if (m_document && document != m_document) {
        m_document->getComposition().removeObserver(this);
    }

    m_document = document;
    m_segments = segments;

    m_document->getComposition().addObserver(this);

    m_compositionRefreshStatusId =
        document->getComposition().getNewRefreshStatusId();

    delete m_hlayout;
    delete m_vlayout;

    m_hlayout = new NotationHLayout(&m_document->getComposition(),
                                    m_notePixmapFactory,
                                    *m_properties,
                                    this,
                                    "NotationHLayout");

    m_vlayout = new NotationVLayout(&m_document->getComposition(),
                                    m_notePixmapFactory,
                                    *m_properties,
                                    this,
                                    "NotationVLayout");

    for (unsigned int i = 0; i < m_staffs.size(); ++i) {
        delete m_staffs[i];
    }
    m_staffs.clear();

    for (unsigned int i = 0; i < m_segments.size(); ++i) {
        NotationStaff *staff = new NotationStaff
            (this,
             m_segments[i],
             0, // no snap grid for notation
             i,
             m_notePixmapFactory,
             m_notePixmapFactorySmall);
        m_staffs.push_back(staff);
    }

    positionStaffs();
    layoutAll();
}

NotationStaff *
NotationScene::getStaffForSceneCoords(double x, int y) const
{
    // (i)  Do not change staff, if mouse was clicked within the current staff.

    StaffLayout *s = 0;

    if (m_currentStaff < m_staffs.size()) {
        s = m_staffs[m_currentStaff];
    }

    if (s && s->containsSceneCoords(x, y)) {

        StaffLayout::StaffLayoutCoords coords =
            s->getLayoutCoordsForSceneCoords(x, y);

        timeT t = m_hlayout->getTimeForX(coords.first);

	// In order to find the correct starting and ending bar of the
	// segment, make infinitesimal shifts (+1 and -1) towards its
	// center.

	timeT t0 = m_document->getComposition().getBarStartForTime
            (m_staffs[m_currentStaff]->getSegment().getStartTime() + 1);

	timeT t1 = m_document->getComposition().getBarEndForTime
            (m_staffs[m_currentStaff]->getSegment().getEndTime() - 1);

        if (t >= t0 && t < t1) {
            return m_staffs[m_currentStaff];
        }
    }

    // (ii) Find staff under cursor, if clicked outside the current staff.

    for (unsigned int i = 0; i < m_staffs.size(); ++i) {

        StaffLayout *s = m_staffs[i];

        if (s->containsSceneCoords(x, y)) {

            StaffLayout::StaffLayoutCoords coords =
                s->getLayoutCoordsForSceneCoords(x, y);

	    timeT t = m_hlayout->getTimeForX(coords.first);

	    // In order to find the correct starting and ending bar of
	    // the segment, make infinitesimal shifts (+1 and -1)
	    // towards its center.

	    timeT t0 = m_document->getComposition().getBarStartForTime
                (m_staffs[i]->getSegment().getStartTime() + 1);
	    timeT t1 = m_document->getComposition().getBarEndForTime
                (m_staffs[i]->getSegment().getEndTime() - 1);

	    if (t >= t0 && t < t1) {
                return m_staffs[i];
            }
        }
    }

    return 0;
}

Segment *
NotationScene::getCurrentSegment()
{
    NotationStaff *s = 0;

    if (m_currentStaff < m_staffs.size()) {
        s = m_staffs[m_currentStaff];
    }

    if (s) return &s->getSegment();
    return 0;
}

void
NotationScene::setupMouseEvent(QGraphicsSceneMouseEvent *e,
                               NotationMouseEvent &nme) const
{
    double sx = e->scenePos().x();
    int sy = lrint(e->scenePos().y());

    nme.sceneX = sx;
    nme.sceneY = sy;

    nme.modifiers = e->modifiers();
    nme.buttons = e->buttons();

    nme.element = 0;
    nme.staff = getStaffForSceneCoords(sx, sy);
    
    bool haveClickHeight = false;

    if (nme.staff) {

        Event *clefEvent = 0, *keyEvent = 0;
        NotationElementList::iterator i =
            nme.staff->getElementUnderSceneCoords(sx, sy, clefEvent, keyEvent);

        if (i != nme.staff->getViewElementList()->end()) {
            nme.element = dynamic_cast<NotationElement *>(*i);
        }
        if (clefEvent) nme.clef = Clef(*clefEvent);

        std::cerr << "clef = " << nme.clef.getClefType() << " (have = " << (clefEvent != 0) << ")" << std::endl;

        if (keyEvent) nme.key = ::Rosegarden::Key(*keyEvent);

        std::cerr << "key = " << nme.key.getName() << " (have = " << (keyEvent != 0) << ")" << std::endl;

        nme.time = nme.staff->getTimeAtSceneCoords(sx, sy);
        nme.height = nme.staff->getHeightAtSceneCoords(sx, sy);
        haveClickHeight = true;

    } else {
        nme.element = 0;
        nme.time = 0;
        nme.height = 0;
    }

    // we've discovered what the context is -- now check whether we're
    // clicking on something specific

    QList<QGraphicsItem *> collisions = items(e->scenePos());

    NotationElement *clickedNote = 0;
    NotationElement *clickedVagueNote = 0;
    NotationElement *clickedNonNote = 0;

    for (QList<QGraphicsItem *>::iterator i = collisions.begin();
         i != collisions.end(); ++i) {

        NotationElement *element = NotationElement::getNotationElement(*i);
        if (!element) continue;
        
        // #957364 (Notation: Hard to select upper note in chords of
        // seconds) -- adjust x-coord for shifted note head

        double cx = element->getSceneX();
        int nbw = 10;

        nbw = m_notePixmapFactory->getNoteBodyWidth();
        bool shifted = false;

        if (element->event()->get<Bool>
            (m_properties->NOTE_HEAD_SHIFTED, shifted) && shifted) {
            cx += nbw;
        }

        if (element->isNote() && haveClickHeight) {

            long eventHeight = 0;

            if (element->event()->get<Int>
                (NotationProperties::HEIGHT_ON_STAFF, eventHeight)) {

                if (eventHeight == nme.height) {

                    if (!clickedNote &&
                        nme.sceneX >= cx &&
                        nme.sceneX <= cx + nbw) {
                        clickedNote = element;
                    } else if (!clickedVagueNote &&
                               nme.sceneX >= cx - 2 &&
                               nme.sceneX <= cx + nbw + 2) {
                        clickedVagueNote = element;
                    }

                } else if (eventHeight - 1 == nme.height ||
                           eventHeight + 1 == nme.height) {
                    if (!clickedVagueNote) {
                        clickedVagueNote = element;
                    }
                }
            }
        } else if (!element->isNote()) {
            if (!clickedNonNote) {
                clickedNonNote = element;
            }
        }
    }

    nme.exact = false;
    
    if (clickedNote) {
        nme.element = clickedNote;
        nme.exact = true;
    } else if (clickedNonNote) {
        nme.element = clickedNonNote;
        nme.exact = true;
    } else if (clickedVagueNote) {
        nme.element = clickedVagueNote;
        nme.exact = true;
    }
    

    NOTATION_DEBUG << "NotationScene::setupMouseEvent: sx = " << sx
                   << ", sy = " << sy
                   << ", modifiers = " << nme.modifiers
                   << ", buttons = " << nme.buttons
                   << ", element = " << nme.element
                   << ", staff = " << nme.staff
                   << " (id = " << (nme.staff ? nme.staff->getId() : -1)
                   << ", clef = " << nme.clef.getClefType()
                   << ", key = " << nme.key.getName()
                   << ", time = " << nme.time
                   << ", height = " << nme.height
                   << endl;
}

void
NotationScene::mousePressEvent(QGraphicsSceneMouseEvent *e)
{
    NotationMouseEvent nme;
    setupMouseEvent(e, nme);
    emit mousePressed(&nme);
}

void
NotationScene::mouseMoveEvent(QGraphicsSceneMouseEvent *e)
{
    NotationMouseEvent nme;
    setupMouseEvent(e, nme);
    emit mouseMoved(&nme);
}

void
NotationScene::mouseReleaseEvent(QGraphicsSceneMouseEvent *e)
{
    NotationMouseEvent nme;
    setupMouseEvent(e, nme);
    emit mouseReleased(&nme);
}

void
NotationScene::mouseDoubleClickEvent(QGraphicsSceneMouseEvent *e)
{
    NotationMouseEvent nme;
    setupMouseEvent(e, nme);
    emit mouseDoubleClicked(&nme);
}

int
NotationScene::getPageWidth()
{
    if (m_pageMode != StaffLayout::MultiPageMode) {
        
        if (isInPrintMode()) {
            return sceneRect().width();
        }

        return m_widget->width() -
//!!!            m_widget->verticalScrollBar()->width() -
            m_leftGutter - 10;

    } else {

        //!!! For the moment we use A4 for this calculation

        double printSizeMm = 25.4 * ((double)m_printSize / 72.0);
        double mmPerPixel = printSizeMm / (double)m_notePixmapFactory->getSize();
        return (int)(210.0 / mmPerPixel);
    }
}

int
NotationScene::getPageHeight()
{
    if (m_pageMode != StaffLayout::MultiPageMode) {

        if (isInPrintMode()) {
            return sceneRect().height();
        }

        return m_widget->height();

    } else {

        //!!! For the moment we use A4 for this calculation

        double printSizeMm = 25.4 * ((double)m_printSize / 72.0);
        double mmPerPixel = printSizeMm / (double)m_notePixmapFactory->getSize();
        return (int)(297.0 / mmPerPixel);
    }
}

void
NotationScene::getPageMargins(int &left, int &top)
{
    if (m_pageMode != StaffLayout::MultiPageMode) {

        left = 0;
        top = 0;

    } else {

        //!!! For the moment we use A4 for this calculation

        double printSizeMm = 25.4 * ((double)m_printSize / 72.0);
        double mmPerPixel = printSizeMm / (double)m_notePixmapFactory->getSize();
        left = (int)(20.0 / mmPerPixel);
        top = (int)(15.0 / mmPerPixel);
    }
}

void
NotationScene::slotCommandExecuted()
{
    checkUpdate();
}

void
NotationScene::timeSignatureChanged(const Composition *c)
{
    if (!m_document || !c || (c != &m_document->getComposition())) return;
    m_timeSignatureChanged = true;
}

void
NotationScene::checkUpdate()
{
    bool need = false;
    bool all = false;
    timeT start = 0, end = 0;
    int count = 0;
    NotationStaff *single = 0;

    bool compositionModified = m_document &&
        m_document->getComposition().getRefreshStatus
        (m_compositionRefreshStatusId).needsRefresh();

    for (unsigned int i = 0; i < m_staffs.size(); ++i) {

        SegmentRefreshStatus &rs = m_staffs[i]->getRefreshStatus();
        
        if (m_timeSignatureChanged ||
            (rs.needsRefresh() && compositionModified)) {

            need = true;
            all = true;

            // don't break, because we want to reset refresh statuses
            // on other segments as well
            
        } else if (rs.needsRefresh()) {
            
            if (!need || rs.from() < start) start = rs.from();
            if (!need || rs.to() < end) end = rs.to();

            need = true;

            single = m_staffs[i];
            ++count;
        }
            
        rs.setNeedsRefresh(false);
    }

    m_timeSignatureChanged = false;
    m_document->getComposition().getRefreshStatus
        (m_compositionRefreshStatusId).setNeedsRefresh(false);

    if (need) {
        if (all) layoutAll();
        else layout(single, start, end);
    }
}

void
NotationScene::segmentRemoved(const Composition *c, Segment *s)
{
    NOTATION_DEBUG << "NotationScene::segmentRemoved(" << c << "," << s << ")" << endl;

    if (!m_document || !c || (c != &m_document->getComposition())) return;

    bool found = false;

    for (std::vector<NotationStaff *>::iterator i = m_staffs.begin();
         i != m_staffs.end(); ++i) {
        if (s == &(*i)->getSegment()) {
            found = true;
            delete *i;
            m_staffs.erase(i);
            break;
        }
    }

    if (found) {
        if (m_staffs.empty()) {
            NOTATION_DEBUG << "(Scene is now empty)" << endl;
            emit sceneDeleted();
        } else {
            positionStaffs();
            checkUpdate();
        }
    }
}

void
NotationScene::positionStaffs()
{
    NOTATION_DEBUG << "NotationView::positionStaffs" << endl;

    QSettings settings;
    settings.beginGroup(NotationViewConfigGroup);

    m_printSize = settings.value("printingnotesize", 5).toUInt() ;

    int minTrack = 0, maxTrack = 0;
    bool haveMinTrack = false;
    typedef std::map<int, int> TrackIntMap;
    TrackIntMap trackHeights;
    TrackIntMap trackCoords;

    int pageWidth, pageHeight, leftMargin, topMargin;
    pageWidth = getPageWidth();
    pageHeight = getPageHeight();
    leftMargin = 0, topMargin = 0;
    getPageMargins(leftMargin, topMargin);

    int accumulatedHeight;
    int rowsPerPage = 1;
    int legerLines = 8;
    if (m_pageMode != StaffLayout::LinearMode) legerLines = 7;
    int rowGapPercent = (m_staffs.size() > 1 ? 40 : 10);
    int aimFor = -1;

    bool done = false;

    int titleHeight = 0;

    delete m_title;
    delete m_subtitle;
    delete m_composer;
    delete m_copyright;
    m_title = m_subtitle = m_composer = m_copyright = 0;

    if (m_pageMode == StaffLayout::MultiPageMode) {

        const Configuration &metadata =
            m_document->getComposition().getMetadata();

        QFont defaultFont(NotePixmapFactory::defaultSerifFontFamily);

        QVariant fv = settings.value("textfont", defaultFont);
        QFont font(defaultFont);
        if (fv.canConvert(QVariant::Font)) font = fv.value<QFont>();

        font.setPixelSize(m_notePixmapFactory->getSize() * 5);
        QFontMetrics metrics(font);

        if (metadata.has(CompositionMetadataKeys::Title)) {
            QString title(strtoqstr(metadata.get<String>
                                    (CompositionMetadataKeys::Title)));
            m_title = new QGraphicsTextItem(title);
            addItem(m_title);
            m_title->setFont(font);
            m_title->setPos(m_leftGutter + pageWidth / 2 - metrics.width(title) / 2,
                            20 + topMargin / 4 + metrics.ascent());
            m_title->show();
            titleHeight += metrics.height() * 3 / 2 + topMargin / 4;
        }

        font.setPixelSize(m_notePixmapFactory->getSize() * 3);
        metrics = QFontMetrics(font);

        if (metadata.has(CompositionMetadataKeys::Subtitle)) {
            QString subtitle(strtoqstr(metadata.get<String>
                                       (CompositionMetadataKeys::Subtitle)));
            m_subtitle = new QGraphicsTextItem(subtitle);
            addItem(m_subtitle);
            m_subtitle->setFont(font);
            m_subtitle->setPos(m_leftGutter + pageWidth / 2 - metrics.width(subtitle) / 2,
                               20 + titleHeight + metrics.ascent());
            m_subtitle->show();
            titleHeight += metrics.height() * 3 / 2;
        }

        if (metadata.has(CompositionMetadataKeys::Composer)) {
            QString composer(strtoqstr(metadata.get<String>
                                       (CompositionMetadataKeys::Composer)));
            m_composer = new QGraphicsTextItem(composer);
            addItem(m_composer);
            m_composer->setFont(font);
            m_composer->setPos(m_leftGutter + pageWidth - metrics.width(composer) - leftMargin,
                               20 + titleHeight + metrics.ascent());
            m_composer->show();
            titleHeight += metrics.height() * 3 / 2;
        }

        font.setPixelSize(m_notePixmapFactory->getSize() * 2);
        metrics = QFontMetrics(font);

        if (metadata.has(CompositionMetadataKeys::Copyright)) {
            QString copyright(strtoqstr(metadata.get<String>
                                        (CompositionMetadataKeys::Copyright)));
            m_copyright = new QGraphicsTextItem(copyright);
            addItem(m_copyright);
            m_copyright->setFont(font);
            m_copyright->setPos(m_leftGutter + leftMargin,
                                20 + pageHeight - topMargin - metrics.descent());
            m_copyright->show();
        }
    }
    settings.endGroup();

    while (1) {

        accumulatedHeight = 0;
        int maxTrackHeight = 0;

        trackHeights.clear();

        for (unsigned int i = 0; i < m_staffs.size(); ++i) {

            m_staffs[i]->setLegerLineCount(legerLines);

            int height = m_staffs[i]->getHeightOfRow();
            TrackId trackId = m_staffs[i]->getSegment().getTrack();
            Track *track =
                m_staffs[i]->getSegment().getComposition()->
                getTrackById(trackId);

            if (!track)
                continue; // This Should Not Happen, My Friend

            int trackPosition = track->getPosition();

            TrackIntMap::iterator hi = trackHeights.find(trackPosition);
            if (hi == trackHeights.end()) {
                trackHeights.insert(TrackIntMap::value_type
                                    (trackPosition, height));
            } else if (height > hi->second) {
                hi->second = height;
            }

            if (height > maxTrackHeight)
                maxTrackHeight = height;

            if (trackPosition < minTrack || !haveMinTrack) {
                minTrack = trackPosition;
                haveMinTrack = true;
            }
            if (trackPosition > maxTrack) {
                maxTrack = trackPosition;
            }
        }

        for (int i = minTrack; i <= maxTrack; ++i) {
            TrackIntMap::iterator hi = trackHeights.find(i);
            if (hi != trackHeights.end()) {
                trackCoords[i] = accumulatedHeight;
                accumulatedHeight += hi->second;
            }
        }

        accumulatedHeight += maxTrackHeight * rowGapPercent / 100;

        if (done)
            break;

        if (m_pageMode != StaffLayout::MultiPageMode) {

            rowsPerPage = 0;
            done = true;
            break;

        } else {

            // Check how well all this stuff actually fits on the
            // page.  If things don't fit as well as we'd like, modify
            // at most one parameter so as to save some space, then
            // loop around again and see if it worked.  This iterative
            // approach is inefficient but the time spent here is
            // neglible in context, and it's a simple way to code it.

            int staffPageHeight = pageHeight - topMargin * 2 - titleHeight;
            rowsPerPage = staffPageHeight / accumulatedHeight;

            if (rowsPerPage < 1) {

                if (legerLines > 5)
                    --legerLines;
                else if (rowGapPercent > 20)
                    rowGapPercent -= 10;
                else if (legerLines > 4)
                    --legerLines;
                else if (rowGapPercent > 0)
                    rowGapPercent -= 10;
                else if (legerLines > 3)
                    --legerLines;
                else if (m_printSize > 3)
                    --m_printSize;
                else { // just accept that we'll have to overflow
                    rowsPerPage = 1;
                    done = true;
                }

            } else {

                if (aimFor == rowsPerPage) {

                    titleHeight +=
                        (staffPageHeight - (rowsPerPage * accumulatedHeight)) / 2;

                    done = true;

                } else {

                    if (aimFor == -1)
                        aimFor = rowsPerPage + 1;

                    // we can perhaps accommodate another row, with care
                    if (legerLines > 5)
                        --legerLines;
                    else if (rowGapPercent > 20)
                        rowGapPercent -= 10;
                    else if (legerLines > 3)
                        --legerLines;
                    else if (rowGapPercent > 0)
                        rowGapPercent -= 10;
                    else { // no, we can't
                        rowGapPercent = 0;
                        legerLines = 8;
                        done = true;
                    }
                }
            }
        }
    }

    m_hlayout->setPageWidth(pageWidth - leftMargin * 2);

    int topGutter = 0;

    if (m_pageMode == StaffLayout::MultiPageMode) {

        topGutter = 20;

    } else if (m_pageMode == StaffLayout::ContinuousPageMode) {

        // fewer leger lines above staff than in linear mode --
        // compensate for this on the top staff
        topGutter = m_notePixmapFactory->getLineSpacing() * 2;
    }

    for (unsigned int i = 0; i < m_staffs.size(); ++i) {

        TrackId trackId = m_staffs[i]->getSegment().getTrack();
        Track *track =
            m_staffs[i]->getSegment().getComposition()->
            getTrackById(trackId);

        if (!track)
            continue; // Once Again, My Friend, You Should Never See Me Here

        int trackPosition = track->getPosition();

        m_staffs[i]->setTitleHeight(titleHeight);
        m_staffs[i]->setRowSpacing(accumulatedHeight);

        if (trackPosition < maxTrack) {
            m_staffs[i]->setConnectingLineLength(trackHeights[trackPosition]);
        }

        if (trackPosition == minTrack &&
            m_pageMode != StaffLayout::LinearMode) {
            m_staffs[i]->setBarNumbersEvery(5);
        } else {
            m_staffs[i]->setBarNumbersEvery(0);
        }

        m_staffs[i]->setX(m_leftGutter);
        m_staffs[i]->setY(topGutter + trackCoords[trackPosition] + topMargin);
        m_staffs[i]->setPageWidth(pageWidth - leftMargin * 2);
        m_staffs[i]->setRowsPerPage(rowsPerPage);
        m_staffs[i]->setPageMode(m_pageMode);
        m_staffs[i]->setMargin(leftMargin);

        NOTATION_DEBUG << "NotationScene::positionStaffs: set staff's page width to "
                       << (pageWidth - leftMargin * 2) << endl;

    }

/*!!!
    if (!m_printMode && m_headersGroupView) {
        // Destroy then recreate all track headers
        hideHeadersGroup();
        m_headersGroup->removeAllHeaders();
        if (m_pageMode == StaffLayout::LinearMode) {
            for (int i = minTrack; i <= maxTrack; ++i) {
                TrackIntMap::iterator hi = trackHeights.find(i);
                if (hi != trackHeights.end()) {
                    TrackId trackId = m_document->getComposition()
                                            .getTrackByPosition(i)->getId();
                    m_headersGroup->addHeader(trackId, trackHeights[i],
                                              trackCoords[i], getCanvasLeftX());
                }
            }

            m_headersGroup->completeToHeight(canvas()->height());

 			m_headersGroupView->addChild(m_headersGroup);
//			m_headersGroupView->layout()->addWidget(m_headersGroup);

            getCanvasView()->updateLeftWidgetGeometry();

            if (    (m_showHeadersGroup == HeadersGroup::ShowAlways)
                || (    (m_showHeadersGroup == HeadersGroup::ShowWhenNeeded)
                      && (m_headersGroup->getUsedHeight()
                              > getCanvasView()->visibleHeight()))) {
                m_headersGroup->slotUpdateAllHeaders(getCanvasLeftX(), 0, true);
                showHeadersGroup();

                // Disable menu entry when headers are shown
                findAction("show_track_headers")->setEnabled(false);
            } else {
                // Enable menu entry when headers are hidden
                findAction("show_track_headers")->setEnabled(true);
            }
        } else {
            // Disable menu entry when not in linear mode
            findAction("show_track_headers")->setEnabled(false);
        }
    }
*/
}

void
NotationScene::layoutAll()
{
    layout(0, 0, 0);
}

void
NotationScene::layout(NotationStaff *singleStaff,
                      timeT startTime, timeT endTime)
{
    m_hlayout->setViewSegmentCount(m_staffs.size());


    if (startTime == endTime) {

        bool first = true;

        for (unsigned int i = 0; i < m_segments.size(); ++i) {
            
            if (singleStaff && m_segments[i] != &singleStaff->getSegment()) {
                continue;
            }

            timeT thisStart = m_segments[i]->getStartTime();
            timeT thisEnd = m_segments[i]->getEndMarkerTime();

            if (first || thisStart < startTime) startTime = thisStart;
            if (first || thisEnd > endTime) endTime = thisEnd;
            
            first = false;
        }
    }

    for (unsigned int i = 0; i < m_staffs.size(); ++i) {
        
        NotationStaff *staff = m_staffs[i];

        if (singleStaff && staff != singleStaff) continue;

        m_hlayout->resetViewSegment(*staff, startTime, endTime);
        m_vlayout->resetViewSegment(*staff, startTime, endTime);

        m_hlayout->scanViewSegment(*staff, startTime, endTime);
        m_vlayout->scanViewSegment(*staff, startTime, endTime);
    }

    m_hlayout->finishLayout(startTime, endTime);
    m_vlayout->finishLayout(startTime, endTime);

    for (unsigned int i = 0; i < m_staffs.size(); ++i) {

        StaffLayout &staff = *m_staffs[i];
        staff.sizeStaff(*m_hlayout);
    }

    for (unsigned int i = 0; i < m_staffs.size(); ++i) {

        NotationStaff *staff = m_staffs[i];

        staff->regenerate(startTime, endTime,
                          !singleStaff || (singleStaff == staff));

        //!!! bogus
        m_staffs[i]->setPointerPosition(*m_hlayout, 0);
        m_staffs[i]->setInsertCursorPosition(*m_hlayout, 0);
    }
}

void
NotationScene::handleEventRemoved(Event *e)
{
    if (m_selection && m_selection->contains(e)) m_selection->removeEvent(e);
    emit eventRemoved(e);
}

void
NotationScene::setSelection(EventSelection *s,
                            bool preview)
{
    if (m_selection) {
        setSelectionElementStatus(m_selection, false);
    }

    if (s != m_selection) {
        delete m_selection;
        m_selection = s;
    }
    
    if (m_selection) {
        setSelectionElementStatus(m_selection, true, preview);
    }
}

void
NotationScene::setSingleSelectedEvent(NotationStaff *staff,
                                      NotationElement *e,
                                      bool preview)
{
    if (!staff || !e) return;
    EventSelection *s = new EventSelection(staff->getSegment());
    s->addEvent(e->event());
    setSelection(s, preview);
}

void
NotationScene::setSingleSelectedEvent(Segment *seg,
                                      Event *e,
                                      bool preview)
{
    if (!seg || !e) return;
    EventSelection *s = new EventSelection(*seg);
    s->addEvent(e);
    setSelection(s, preview);
}

void
NotationScene::setSelectionElementStatus(EventSelection *s, 
                                         bool set,
                                         bool preview)
{
    if (!s) return;

    NotationStaff *staff = 0;

    for (std::vector<NotationStaff *>::iterator i = m_staffs.begin();
         i != m_staffs.end(); ++i) {

        if (&(*i)->getSegment() == &s->getSegment()) {
            staff = *i;
            break;
        }
    }

    if (!staff) return;

    for (EventSelection::eventcontainer::iterator i = s->getSegmentEvents().begin();
         i != s->getSegmentEvents().end(); ++i) {

        Event *e = *i;
        
        ViewElementList::iterator staffi = staff->findEvent(e);
        if (staffi == staff->getViewElementList()->end()) continue;
        
        NotationElement *el = static_cast<NotationElement *>(*staffi);
        
        el->setSelected(set);

        if (set && preview) {
            long pitch;
            if (e->get<Int>(BaseProperties::PITCH, pitch)) {
                long velocity = -1;
                (void)(e->get<Int>(BaseProperties::VELOCITY, velocity));
                if (!(e->has(BaseProperties::TIED_BACKWARD) &&
                      e->get<Bool>(BaseProperties::TIED_BACKWARD))) {
                    playNote(s->getSegment(), pitch, velocity);
                }
            }
        }
    }
}

void
NotationScene::showPreviewNote(NotationStaff *staff, double layoutX,
                               int pitch, int height,
                               const Note &note,
                               bool grace,
                               int velocity)
{
    if (staff) {
        staff->showPreviewNote(layoutX, height, note, grace);
        playNote(staff->getSegment(), pitch, velocity);
    }
}

void
NotationScene::clearPreviewNote(NotationStaff *staff)
{
    if (staff) {
        staff->clearPreviewNote();
    }
}

static bool
canPreviewAnotherNote()
{
    static time_t lastCutOff = 0;
    static int sinceLastCutOff = 0;

    time_t now = time(0);
    ++sinceLastCutOff;

    if ((now - lastCutOff) > 0) {
        sinceLastCutOff = 0;
        lastCutOff = now;
    } else {
        if (sinceLastCutOff >= 20) {
            // don't permit more than 20 notes per second or so, to
            // avoid gungeing up the sound drivers
            return false;
        }
    }

    return true;
}

void
NotationScene::playNote(Segment &segment, int pitch, int velocity)
{
    if (!m_document) return;

    Instrument *instrument = m_document->getStudio().getInstrumentFor(&segment);
    if (!instrument) return;

    if (!canPreviewAnotherNote()) return;

    if (velocity < 0) velocity = 100;

    MappedEvent mE(instrument->getId(),
                   MappedEvent::MidiNoteOneShot,
                   pitch + segment.getTranspose(),
                   velocity,
                   RealTime::zeroTime,
                   RealTime(0, 250000000),
                   RealTime::zeroTime);

    StudioControl::sendMappedEvent(mE);
}    
    
bool
NotationScene::constrainToSegmentArea(QPointF &scenePos)
{
    //!!!
#ifdef NOT_JUST_YET
    bool ok = true;

    int pitch = 127 - (lrint(scenePos.y()) / (m_resolution + 1));
    if (pitch < 0) {
        ok = false;
        scenePos.setY(127 * (m_resolution + 1));
    } else if (pitch > 127) {
        ok = false;
        scenePos.setY(0);
    }

    timeT t = m_scale->getTimeForX(scenePos.x());
    timeT start = 0, end = 0;
    for (size_t i = 0; i < m_segments.size(); ++i) {
        timeT t0 = m_segments[i]->getStartTime();
        timeT t1 = m_segments[i]->getEndMarkerTime();
        if (i == 0 || t0 < start) start = t0;
        if (i == 0 || t1 > end) end = t1; 
    }
    if (t < start) {
        ok = false;
        scenePos.setX(m_scale->getXForTime(start));
    } else if (t > end) {
        ok = false;
        scenePos.setX(m_scale->getXForTime(end));
    }
    return ok;
#endif
    return true;
}

}

#include "NotationScene.moc"

