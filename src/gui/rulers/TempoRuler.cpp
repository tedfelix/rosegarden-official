/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2016 the Rosegarden development team.

    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#define RG_MODULE_STRING "[TempoRuler]"

#include "TempoRuler.h"

#include "misc/Debug.h"
#include "base/Composition.h"
#include "base/NotationTypes.h"
#include "base/RealTime.h"
#include "base/RulerScale.h"
#include "base/SnapGrid.h"
#include "document/RosegardenDocument.h"
#include "document/CommandHistory.h"
#include "gui/dialogs/TempoDialog.h"
#include "gui/general/GUIPalette.h"
#include "gui/general/EditTempoController.h"
#include "gui/widgets/TextFloat.h"
#include "TempoColour.h"

#include <QColor>
#include <QCursor>
#include <QEvent>
#include <QFont>
#include <QFontMetrics>
#include <QIcon>
#include <QObject>
#include <QPainter>
#include <QPixmap>
#include <QPoint>
#include <QMenu>
#include <QRect>
#include <QSize>
#include <QString>
#include <QWidget>
#include <QAction>
#include <QEvent>
#include <QPaintEvent>
#include <QMouseEvent>



namespace Rosegarden
{

TempoRuler::TempoRuler(RulerScale *rulerScale,
                       RosegardenDocument *doc,
                       double xorigin,
                       int height,
                       bool small,
                       bool Thorn) :
        QWidget(0),
        m_xorigin(xorigin),
        m_height(height),
        m_currentXOffset(0),
        m_width( -1),
        m_small(small),
        m_illuminate( -1),
        m_illuminatePoint(false),
        m_illuminateTarget(false),
        m_refreshLinesOnly(false),
        m_dragVert(false),
        m_dragTarget(false),
        m_dragHoriz(false),
        m_dragStartY(0),
        m_dragStartX(0),
        m_dragFine(false),
        m_clickX(0),
        m_dragStartTempo( -1),
        m_dragStartTarget( -1),
        m_dragOriginalTempo( -1),
        m_dragOriginalTarget( -1),
        m_composition(&doc->getComposition()),
        m_rulerScale(rulerScale),
        m_menu(0),
        m_editTempoController(EditTempoController::self()),
        m_fontMetrics(m_boldFont),
        m_Thorn(Thorn)
{
    m_font.setPixelSize(m_height / 3);
    m_boldFont.setPixelSize(m_height * 2 / 5);
    m_boldFont.setBold(true);
    m_fontMetrics = QFontMetrics(m_boldFont);

    m_editTempoController->setDocument(doc); // in case self() just created it

    QObject::connect
    (CommandHistory::getInstance(), SIGNAL(commandExecuted()),
     this, SLOT(update()));

    createAction("insert_tempo_here", SLOT(slotInsertTempoHere()));
    createAction("insert_tempo_at_pointer", SLOT(slotInsertTempoAtPointer()));
    createAction("delete_tempo", SLOT(slotDeleteTempoChange()));
    createAction("ramp_to_next", SLOT(slotRampToNext()));
    createAction("unramp", SLOT(slotUnramp()));
    createAction("edit_tempo", SLOT(slotEditTempo()));
    createAction("edit_time_signature", SLOT(slotEditTimeSignature()));
    createAction("edit_tempos", SLOT(slotEditTempos()));

    setMouseTracking(false);
}

TempoRuler::~TempoRuler()
{
}

void
TempoRuler::slotScrollHoriz(int x)
{
    // int w = width();
    // int h = height();
    // int dx = x - ( -m_currentXOffset);
    m_currentXOffset = -x;

    update();
}

void
TempoRuler::mousePressEvent(QMouseEvent *e)
{
    if (e->button() == Qt::LeftButton) {

        if (e->type() == QEvent::MouseButtonDblClick) {
            timeT t = m_rulerScale->getTimeForX
                      (e->x() - m_currentXOffset - m_xorigin);
            m_editTempoController->emitEditTempos(t);
            return;
        }

        int x = e->x() + 1;
        int y = e->y();
        timeT t = m_rulerScale->getTimeForX(x - m_currentXOffset - m_xorigin);
        int tcn = m_composition->getTempoChangeNumberAt(t);

        if (tcn < 0 || tcn >= m_composition->getTempoChangeCount())
            return ;

        std::pair<timeT, tempoT> tc = m_composition->getTempoChange(tcn);
        std::pair<bool, tempoT> tr = m_composition->getTempoRamping(tcn, true);

        m_dragStartY = y;
        m_dragStartX = x;
        m_dragStartTime = tc.first;
        m_dragPreviousTime = m_dragStartTime;
        m_dragStartTempo = tc.second;
        m_dragStartTarget = tr.first ? tr.second : -1;
        m_dragOriginalTempo = m_dragStartTempo;
        m_dragOriginalTarget = m_dragStartTarget;
        m_dragFine = ((e->modifiers() & Qt::ShiftModifier) != 0);

        int px = m_rulerScale->getXForTime(tc.first) + m_currentXOffset + m_xorigin;
        if (x >= px && x < px + 5) {
            m_dragHoriz = true;
            m_dragVert = false;
            setCursor(Qt::SplitHCursor);
        } else {
            timeT nt = m_composition->getEndMarker();
            if (tcn < m_composition->getTempoChangeCount() - 1) {
                nt = m_composition->getTempoChange(tcn + 1).first;
            }
            int nx = m_rulerScale->getXForTime(nt) + m_currentXOffset + m_xorigin;
            if (x > px + 5 && x > nx - 5) {
                m_dragTarget = true;
                setCursor(Qt::SizeVerCursor);
            } else {
                m_dragTarget = false;
                setCursor(Qt::SplitVCursor);
            }
            m_dragVert = true;
            m_dragHoriz = false;
        }

    } else if (e->button() == Qt::RightButton) {

        m_clickX = e->x();
        if (!m_menu)
            createMenu();
        if (m_menu) {
            // enable 'delete' action only if cursor is actually over a tempo change
//             actionCollection()->action("delete_tempo")->setEnabled(m_illuminatePoint);
            findAction("delete_tempo")->setEnabled(m_illuminatePoint);

            m_menu->exec(QCursor::pos());
        }

    }
}

void
TempoRuler::mouseReleaseEvent(QMouseEvent *e)
{
    if (m_dragVert) {

        m_dragVert = false;
        unsetCursor();

        if (e->x() < 0 || e->x() >= width() ||
                e->y() < 0 || e->y() >= height()) {
            leaveEvent(0);
        }

        // First we make a note of the values that we just set and
        // restore the tempo to whatever it was previously, so that
        // the undo for any following command will work correctly.
        // Then we emit so that our user can issue the right command.

        int tcn = m_composition->getTempoChangeNumberAt(m_dragStartTime);
        std::pair<timeT, tempoT> tc = m_composition->getTempoChange(tcn);
        std::pair<bool, tempoT> tr = m_composition->getTempoRamping(tcn, true);

        if (tc.second != m_dragOriginalTempo) {
            m_composition->addTempoAtTime(m_dragStartTime,
                                          m_dragOriginalTempo,
                                          m_dragOriginalTarget);
            m_editTempoController->changeTempo(m_dragStartTime, tc.second,
                                               tr.first ? tr.second : -1,
                                               TempoDialog::AddTempo);
        }

    } else if (m_dragHoriz) {

        m_dragHoriz = false;
        unsetCursor();

        if (e->x() < 0 || e->x() >= width() ||
                e->y() < 0 || e->y() >= height()) {
            leaveEvent(0);
        }

        if (m_dragPreviousTime != m_dragStartTime) {

            // As above, restore the original tempo and then emit a
            // signal to ensure a proper command happens.

            int tcn = m_composition->getTempoChangeNumberAt(m_dragPreviousTime);
            m_composition->removeTempoChange(tcn);
            m_composition->addTempoAtTime(m_dragStartTime,
                                          m_dragStartTempo,
                                          m_dragStartTarget);

            m_editTempoController->moveTempo(m_dragStartTime, m_dragPreviousTime);
        }
    }
}

void
TempoRuler::mouseMoveEvent(QMouseEvent *e)
{
    bool shiftPressed = ((e->modifiers() & Qt::ShiftModifier) != 0);

    if (m_dragVert) {

        if (shiftPressed != m_dragFine) {

            m_dragFine = shiftPressed;
            m_dragStartY = e->y();

            // reset the start tempi to whatever we last updated them
            // to as we switch into or out of fine mode
            int tcn = m_composition->getTempoChangeNumberAt(m_dragStartTime);
            std::pair<timeT, tempoT> tc = m_composition->getTempoChange(tcn);
            std::pair<bool, tempoT> tr = m_composition->getTempoRamping(tcn, true);
            m_dragStartTempo = tc.second;
            m_dragStartTarget = tr.first ? tr.second : -1;
        }

        int diff = m_dragStartY - e->y(); // +ve for upwards drag
        tempoT newTempo = m_dragStartTempo;
        tempoT newTarget = m_dragStartTarget;

        if (diff != 0) {

            float qpm = m_composition->getTempoQpm(newTempo);

            if (m_dragTarget && newTarget > 0) {
                qpm = m_composition->getTempoQpm(newTarget);
            }

            float qdiff = (m_dragFine ? diff * 0.05 : diff * 0.5);
            qpm += qdiff;
            if (qpm < 1)
                qpm = 1;

            if (m_dragTarget) {

                newTarget = m_composition->getTempoForQpm(qpm);

            } else {

                newTempo = m_composition->getTempoForQpm(qpm);

                if (newTarget >= 0) {
                    qpm = m_composition->getTempoQpm(newTarget);
                    qpm += qdiff;
                    if (qpm < 1)
                        qpm = 1;
                    newTarget = m_composition->getTempoForQpm(qpm);
                }
            }
        }

        showTextFloat(newTempo, newTarget, m_dragStartTime);
        m_composition->addTempoAtTime(m_dragStartTime, newTempo, newTarget);
        update();

    } else if (m_dragHoriz) {

        int x = e->x();

        SnapGrid grid(m_rulerScale);
        if (shiftPressed) {
            grid.setSnapTime(SnapGrid::NoSnap);
        } else {
            grid.setSnapTime(SnapGrid::SnapToUnit);
        }
        timeT newTime = grid.snapX(x - m_currentXOffset - m_xorigin,
                                   SnapGrid::SnapEither);

        int tcn = m_composition->getTempoChangeNumberAt(m_dragPreviousTime);
        int ncn = m_composition->getTempoChangeNumberAt(newTime);
        if (ncn > tcn || ncn < tcn - 1)
            return ;
        if (ncn >= 0 && ncn == tcn - 1) {
            std::pair<timeT, tempoT> nc = m_composition->getTempoChange(ncn);
            if (nc.first == newTime)
                return ;
        }

        //    std::cerr << " -> " << newTime << std::endl;

        m_composition->removeTempoChange(tcn);
        m_composition->addTempoAtTime(newTime,
                                      m_dragStartTempo,
                                      m_dragStartTarget);
        showTextFloat(m_dragStartTempo, m_dragStartTarget, newTime, true);
        m_dragPreviousTime = newTime;
        update();

    } else {

        int x = e->x() + 1;
        timeT t = m_rulerScale->getTimeForX(x - m_currentXOffset - m_xorigin);
        int tcn = m_composition->getTempoChangeNumberAt(t);

        if (tcn < 0 || tcn >= m_composition->getTempoChangeCount())
            return ;

        std::pair<timeT, tempoT> tc = m_composition->getTempoChange(tcn);
        std::pair<bool, tempoT> tr = m_composition->getTempoRamping(tcn, true);

        int bar, beat, fraction, remainder;
        m_composition->getMusicalTimeForAbsoluteTime(tc.first, bar, beat,
                fraction, remainder);
        //RG_DEBUG << "Tempo change: tempo " << m_composition->getTempoQpm(tc.second) << " at " << bar << ":" << beat << ":" << fraction << ":" << remainder;

        m_illuminate = tcn;
        m_illuminatePoint = false;
        m_illuminateTarget = false;
        //!!!    m_refreshLinesOnly = true;

        //!!! merge this test with the one in mousePressEvent as
        //isCloseToStart or equiv, and likewise for close to end

        int px = m_rulerScale->getXForTime(tc.first) + m_currentXOffset + m_xorigin;
        if (x >= px && x < px + 5) {
            m_illuminatePoint = true;
        } else {
            timeT nt = m_composition->getEndMarker();
            if (tcn < m_composition->getTempoChangeCount() - 1) {
                nt = m_composition->getTempoChange(tcn + 1).first;
            }
            int nx = m_rulerScale->getXForTime(nt) + m_currentXOffset + m_xorigin;
            if (x > px + 5 && x > nx - 5) {
                m_illuminateTarget = true;
            }

            //        std::cerr << "nt = " << nt << ", nx = " << nx << ", x = " << x << ", m_illuminateTarget = " << m_illuminateTarget << std::endl;
        }

        showTextFloat(tc.second, tr.first ? tr.second : -1,
                      tc.first, m_illuminatePoint);

        update();
    }
}

void
TempoRuler::wheelEvent(QWheelEvent */* e */)
{}

void
TempoRuler::enterEvent(QEvent *)
{
    TextFloat::getTextFloat()->attach(this);
    setMouseTracking(true);
}

void
TempoRuler::leaveEvent(QEvent *)
{
    if (!m_dragVert && !m_dragHoriz) {
        setMouseTracking(false);
        m_illuminate = -1;
        m_illuminatePoint = false;
        //!!!    m_refreshLinesOnly = true;
        TextFloat::getTextFloat()->hide();

        update();
    }
}

void
TempoRuler::showTextFloat(tempoT tempo, tempoT target,
                          timeT time, bool showTime)
{
    float qpm = m_composition->getTempoQpm(tempo);
    int qi = int(qpm + 0.0001);
    int q0 = int(qpm * 10 + 0.0001) % 10;
    int q00 = int(qpm * 100 + 0.0001) % 10;

    bool haveSet = false;

    QString tempoText, timeText;

    if (time >= 0) {

        if (showTime) {
            int bar, beat, fraction, remainder;
            m_composition->getMusicalTimeForAbsoluteTime
            (time, bar, beat, fraction, remainder);
            RealTime rt = m_composition->getElapsedRealTime(time);
           
            // bars in user space start at 1, not 0
            bar++;

            // blargh -- duplicated with TempoView::makeTimeString
            timeText = QString("%1%2%3-%4%5-%6%7-%8%9")
                       .arg(bar / 100)
                       .arg((bar % 100) / 10)
                       .arg(bar % 10)
                       .arg(beat / 10)
                       .arg(beat % 10)
                       .arg(fraction / 10)
                       .arg(fraction % 10)
                       .arg(remainder / 10)
                       .arg(remainder % 10);

            timeText = QString("%1\n%2")
                       .arg(timeText)
                       //        .arg(rt.toString().c_str());
                       .arg(rt.toText(true).c_str());
        }

        TimeSignature sig =
            m_composition->getTimeSignatureAt(time);

        if (sig.getBeatDuration() !=
                Note(Note::Crotchet).getDuration()) {

            float bpm =
                (qpm *
                 Note(Note::Crotchet).getDuration())
                / sig.getBeatDuration();
            int bi = int(bpm + 0.0001);
            int b0 = int(bpm * 10 + 0.0001) % 10;
            int b00 = int(bpm * 100 + 0.0001) % 10;

            tempoText = tr("%1.%2%3 (%4.%5%6 bpm)")
                         .arg(qi).arg(q0).arg(q00)
                         .arg(bi).arg(b0).arg(b00);
            haveSet = true;
        }
    }

    if (!haveSet) {
        tempoText = tr("%1.%2%3 bpm").arg(qi).arg(q0).arg(q00);
    }

    if (target > 0 && target != tempo) {
        float tq = m_composition->getTempoQpm(target);
        int tqi = int(tq + 0.0001);
        int tq0 = int(tq * 10 + 0.0001) % 10;
        int tq00 = int(tq * 100 + 0.0001) % 10;
        tempoText = tr("%1 - %2.%3%4")
                     .arg(tempoText).arg(tqi).arg(tq0).arg(tq00);
    }

    TextFloat *textFloat = TextFloat::getTextFloat();

    if (showTime && time >= 0) {
        textFloat->setText(QString("%1\n%2").arg(timeText).arg(tempoText));
    } else {
        textFloat->setText(tempoText);
    }

    QPoint cp = mapFromGlobal(QPoint(QCursor::pos()));
      //  std::cerr << "cp = " << cp.x() << "," << cp.y() << ", tempo = " << qpm << std::endl;

    QPoint offset = cp + QPoint(10, 25 - cp.y() - textFloat->height());
    textFloat->display(offset);

}

QSize
TempoRuler::sizeHint() const
{
    double width =
        m_rulerScale->getBarPosition(m_rulerScale->getLastVisibleBar()) +
        m_rulerScale->getBarWidth(m_rulerScale->getLastVisibleBar()) +
        m_xorigin;

    QSize res(std::max(int(width), m_width), m_height);

    return res;
}

QSize
TempoRuler::minimumSizeHint() const
{
    double firstBarWidth = m_rulerScale->getBarWidth(0) + m_xorigin;
    QSize res = QSize(int(firstBarWidth), m_height);
    return res;
}

int
TempoRuler::getYForTempo(tempoT tempo)
{
    int drawh = height() - 4;
    int y = drawh / 2;

    tempoT minTempo = m_composition->getMinTempo();
    tempoT maxTempo = m_composition->getMaxTempo();

    if (maxTempo > minTempo) {
        y = drawh -
            int((double(tempo - minTempo) / double(maxTempo - minTempo))
                * drawh + 0.5);
    }

    return y;
}

tempoT
TempoRuler::getTempoForY(int y)
{
    int drawh = height() - 4;

    tempoT minTempo = m_composition->getMinTempo();
    tempoT maxTempo = m_composition->getMaxTempo();

    tempoT tempo = minTempo;

    if (maxTempo > minTempo) {
        tempo = (maxTempo - minTempo) *
                (double(drawh - y) / double(drawh)) + minTempo + 0.5;
    }

    return tempo;
}

void
TempoRuler::paintEvent(QPaintEvent* e)
{
    QRect clipRect = e->rect();

    if (m_buffer.width() < width() || m_buffer.height() < height()) {
        m_buffer = QPixmap(width(), height());
    }

    // NEW: don't set the background for "null space" to the TextRulerBackground
    // color, because this has always looked like crap.  If we're not using
    // Thorn, just leave the background at system default, because there's
    // nothing in this part of the ruler that means anything anyway.  If we're
    // using Thorn, use a nice dark gray that just contrasts with the black
    // horizontal line here
    QColor kuller(0x40, 0x40, 0x40);
    if (!m_Thorn) kuller = palette().background().color();
    m_buffer.fill(kuller);

    QPainter paint(&m_buffer);
    paint.setPen(GUIPalette::getColour
                 (GUIPalette::TextRulerForeground));

    paint.setClipRegion(e->region());
    paint.setClipRect(clipRect);

    if (m_xorigin > 0) {
        paint.fillRect(0, 0, m_xorigin, height(), kuller);
    }

    timeT from = m_rulerScale->getTimeForX
                 (clipRect.x() - m_currentXOffset - 100 - m_xorigin);
    timeT to = m_rulerScale->getTimeForX
               (clipRect.x() + clipRect.width() - m_currentXOffset + 100 - m_xorigin);

    QRect boundsForHeight = m_fontMetrics.boundingRect("019");
    int fontHeight = boundsForHeight.height();
    //int textY = fontHeight + 2;
    // bmp text aligns better in temporuler now - is this font dependent?
    int textY = fontHeight - 3;

    double prevEndX = -1000.0;
    double prevTempo = 0.0;
    long prevBpm = 0;

    typedef std::map<timeT, int> TimePoints;
    int tempoChangeHere = 1;
    int timeSigChangeHere = 2;
    TimePoints timePoints;

    for (int tempoNo = m_composition->getTempoChangeNumberAt(from);
            tempoNo <= m_composition->getTempoChangeNumberAt(to) + 1; ++tempoNo) {

        if (tempoNo >= 0 && tempoNo < m_composition->getTempoChangeCount()) {
            timePoints.insert
            (TimePoints::value_type
             (m_composition->getTempoChange(tempoNo).first,
              tempoChangeHere));
        }
    }

    for (int sigNo = m_composition->getTimeSignatureNumberAt(from);
            sigNo <= m_composition->getTimeSignatureNumberAt(to) + 1; ++sigNo) {

        if (sigNo >= 0 && sigNo < m_composition->getTimeSignatureCount()) {
            timeT time(m_composition->getTimeSignatureChange(sigNo).first);
            if (timePoints.find(time) != timePoints.end()) {
                timePoints[time] |= timeSigChangeHere;
            } else {
                timePoints.insert(TimePoints::value_type(time, timeSigChangeHere));
            }
        }
    }

    int lastx = 0, lasty = 0, lastx1 = 0;
    bool haveSome = false;
    //    tempoT minTempo = m_composition->getMinTempo();
    //    tempoT maxTempo = m_composition->getMaxTempo();
    bool illuminate = false;

    if (m_illuminate >= 0) {
        int tcn = m_composition->getTempoChangeNumberAt(from);
        illuminate = (m_illuminate == tcn);
    }

    for (TimePoints::iterator i = timePoints.begin(); ; ++i) {

        timeT t0, t1;

        if (i == timePoints.begin()) {
            t0 = from;
        } else {
            TimePoints::iterator j(i);
            --j;
            t0 = j->first;
        }

        if (i == timePoints.end()) {
            t1 = to;
        } else {
            t1 = i->first;
        }

        if (t1 <= t0)
            t1 = to;

        int tcn = m_composition->getTempoChangeNumberAt(t0);
        tempoT tempo = m_composition->getTempoAtTime(t0);

        std::pair<bool, tempoT> ramping(false, tempo);
        if (tcn > 0 && tcn < m_composition->getTempoChangeCount() + 1) {
            ramping = m_composition->getTempoRamping(tcn - 1, true);
        }

        double x0, x1;
        x0 = m_rulerScale->getXForTime(t0) + m_currentXOffset + m_xorigin;
        x1 = m_rulerScale->getXForTime(t1) + m_currentXOffset + m_xorigin;
        /*!!!
            if (x0 > e->rect().x()) {
                paint.fillRect(e->rect().x(), 0, x0 - e->rect().x(), height(),
                       paletteBackgroundColor());
            }
        */
        QColor colour = TempoColour::getColour(m_composition->getTempoQpm(tempo));
        paint.setPen(colour);
        paint.setBrush(colour);

        if (!m_refreshLinesOnly) {
            //         RG_DEBUG << "TempoRuler: draw rect from " << x0 << " to " << x1;
            paint.drawRect(int(x0), 0, int(x1 - x0) + 1, height());
        }

        int y = getYForTempo(tempo);
        /*!!!
            int drawh = height() - 4;
            int y = drawh / 2;
            if (maxTempo > minTempo) {
                y = drawh -
                int((double(tempo - minTempo) / double(maxTempo - minTempo))
                    * drawh + 0.5);
            }
        */
        y += 2;

        if (haveSome) {

            int x = int(x0) + 1;
            int ry = lasty;

            bool illuminateLine = (illuminate &&
                                   !m_illuminatePoint && !m_illuminateTarget);

            paint.setPen(illuminateLine ? QColor(Qt::white) : QColor(Qt::black));

            if (ramping.first) {
                ry = getYForTempo(ramping.second);
                ry += 2;
                /*!!!
                        ry = drawh -
                            int((double(ramping.second - minTempo) /
                             double(maxTempo - minTempo))
                            * drawh + 0.5);
                */
            }

            paint.drawLine(lastx + 1, lasty, x - 2, ry);

            if (!illuminateLine && illuminate && m_illuminateTarget) {
                if (x > lastx) {
                    paint.setPen(QColor(Qt::white));
                    paint.drawLine(x - 6, ry - ((ry - lasty) * 6) / (x - lastx),
                                   x - 2, ry);
                }
            }

            if (m_illuminate >= 0) {
                illuminate = (m_illuminate == tcn);
            }

            bool illuminatePoint = (illuminate && m_illuminatePoint);

            paint.setPen(illuminatePoint ? QColor(Qt::white) : QColor(Qt::black));
            paint.drawRect(x - 1, y - 1, 3, 3);

            paint.setPen(illuminatePoint ? QColor(Qt::black) : QColor(Qt::white));
            paint.drawPoint(x, y);
        }

        lastx = int(x0) + 1;
        lastx1 = int(x1) + 1;
        lasty = y;
        if (i == timePoints.end())
            break;
        haveSome = true;
    }

    if (lastx1 < e->rect().x() + e->rect().width()) {
        /*!!!
            paint.fillRect(lastx1, 0,
                       e->rect().x() + e->rect().width() - lastx1, height(),
                       paletteBackgroundColor());
        */
    }

    if (haveSome) {
        bool illuminateLine = (illuminate && !m_illuminatePoint);
        paint.setPen(illuminateLine ? QColor(Qt::white) : QColor(Qt::black));
        paint.drawLine(lastx + 1, lasty, width(), lasty);
    } else if (!m_refreshLinesOnly) {
        tempoT tempo = m_composition->getTempoAtTime(from);
        QColor colour = TempoColour::getColour(m_composition->getTempoQpm(tempo));
        paint.setPen(colour);
        paint.setBrush(colour);
        paint.drawRect(e->rect());
    }

    paint.setPen(QColor(Qt::black));
    paint.setBrush(QColor(Qt::black));
    paint.drawLine(0, 0, width(), 0);

    for (TimePoints::iterator i = timePoints.begin();
            i != timePoints.end(); ++i) {

        timeT time = i->first;
        double x = m_rulerScale->getXForTime(time) + m_currentXOffset
                   + m_xorigin;

        /*
            paint.drawLine(static_cast<int>(x),
                       height() - (height()/4),
                       static_cast<int>(x),
                       height());
        */

        if ((i->second & timeSigChangeHere) && !m_refreshLinesOnly) {

            TimeSignature sig =
                m_composition->getTimeSignatureAt(time);

            QString str = QString("%1/%2")
                          .arg(sig.getNumerator())
                          .arg(sig.getDenominator());

            paint.setFont(m_boldFont);
            paint.drawText(static_cast<int>(x) + 2, m_height - 2, str);
        }

        if ((i->second & tempoChangeHere) && !m_refreshLinesOnly) {

            double tempo = m_composition->getTempoQpm(m_composition->getTempoAtTime(time));
            long bpm = long(tempo);
            //        long frac = long(tempo * 100 + 0.001) - 100 * bpm;

            QString tempoString = QString("%1").arg(bpm);

            if (tempo == prevTempo) {
                if (m_small)
                    continue;
                tempoString = "=";
            } else if (bpm == prevBpm) {
                tempoString = (tempo > prevTempo ? "+" : "-");
            } else {
                if (m_small && (bpm != (bpm / 10 * 10))) {
                    if (bpm == prevBpm + 1)
                        tempoString = "+";
                    else if (bpm == prevBpm - 1)
                        tempoString = "-";
                }
            }
            prevTempo = tempo;
            prevBpm = bpm;

            QRect bounds = m_fontMetrics.boundingRect(tempoString);
            
            x += 3; // bmp text aligns better in temporuler now - is this font dependent?
            
            paint.setFont(m_font);
            if (time > 0)
                x -= bounds.width() / 2;
            //        if (x > bounds.width() / 2) x -= bounds.width() / 2;
            if (prevEndX >= x - 3)
                x = prevEndX + 3;
            paint.drawText(static_cast<int>(x), textY, tempoString);
            prevEndX = x + bounds.width();
        }
    }

    paint.end();

    QPainter dbpaint(this);
    //    dbpaint.drawPixmap(0, 0, m_buffer);
    dbpaint.drawPixmap(clipRect.x(), clipRect.y(),
                       m_buffer,
                       clipRect.x(), clipRect.y(),
                       clipRect.width(), clipRect.height());

    dbpaint.end();

    m_refreshLinesOnly = false;
}

void
TempoRuler::slotInsertTempoHere()
{
    SnapGrid grid(m_rulerScale);
    grid.setSnapTime(SnapGrid::SnapToUnit);
    timeT t = grid.snapX(m_clickX - m_currentXOffset - m_xorigin,
                         SnapGrid::SnapLeft);
    tempoT tempo = Composition::getTempoForQpm(120.0);

    int tcn = m_composition->getTempoChangeNumberAt(t);
    if (tcn >= 0 && tcn < m_composition->getTempoChangeCount()) {
        std::pair<timeT, tempoT> tc = m_composition->getTempoChange(tcn);
        if (tc.first == t)
            return ;
        tempo = tc.second;
    }

    m_editTempoController->changeTempo(t, tempo, -1, TempoDialog::AddTempo);
}

void
TempoRuler::slotInsertTempoAtPointer()
{
    timeT t = m_composition->getPosition();
    tempoT tempo = Composition::getTempoForQpm(120.0);

    int tcn = m_composition->getTempoChangeNumberAt(t);
    if (tcn >= 0 && tcn < m_composition->getTempoChangeCount()) {
        std::pair<timeT, tempoT> tc = m_composition->getTempoChange(tcn);
        if (tc.first == t)
            return ;
        tempo = tc.second;
    }

    m_editTempoController->changeTempo(t, tempo, -1, TempoDialog::AddTempo);
}

void
TempoRuler::slotDeleteTempoChange()
{
    timeT t = m_rulerScale->getTimeForX(m_clickX - m_currentXOffset - m_xorigin);
    m_editTempoController->deleteTempoChange(t);
}

void
TempoRuler::slotRampToNext()
{
    timeT t = m_rulerScale->getTimeForX(m_clickX - m_currentXOffset - m_xorigin);

    int tcn = m_composition->getTempoChangeNumberAt(t);
    if (tcn < 0 || tcn >= m_composition->getTempoChangeCount())
        return ;

    std::pair<timeT, tempoT> tc = m_composition->getTempoChange(tcn);

    m_editTempoController->changeTempo(tc.first, tc.second, 0, TempoDialog::AddTempo);
}

void
TempoRuler::slotUnramp()
{
    timeT t = m_rulerScale->getTimeForX(m_clickX - m_currentXOffset - m_xorigin);

    int tcn = m_composition->getTempoChangeNumberAt(t);
    if (tcn < 0 || tcn >= m_composition->getTempoChangeCount())
        return ;

    std::pair<timeT, tempoT> tc = m_composition->getTempoChange(tcn);

    m_editTempoController->changeTempo(tc.first, tc.second, -1, TempoDialog::AddTempo);
}

void
TempoRuler::slotEditTempo()
{
    const timeT atTime = m_rulerScale->getTimeForX(m_clickX - m_currentXOffset - m_xorigin);
    m_editTempoController->editTempo(this, atTime);
}

void
TempoRuler::slotEditTimeSignature()
{
    timeT t = m_rulerScale->getTimeForX(m_clickX - m_currentXOffset - m_xorigin);
    m_editTempoController->editTimeSignature(this, t);
}

void
TempoRuler::slotEditTempos()
{
    timeT t = m_rulerScale->getTimeForX(m_clickX - m_currentXOffset - m_xorigin);
    m_editTempoController->emitEditTempos(t);
}

void
TempoRuler::createMenu()
{
    createGUI("temporuler.rc");

    m_menu = findChild<QMenu *>("tempo_ruler_menu");

//    if (!tmp) {
//        RG_DEBUG << "MarkerRuler::createMenu() menu not found\n"
//                 << domDocument().toString(4) << endl;
//    }

    if (!m_menu) {
        RG_DEBUG << "TempoRuler::createMenu() failed\n";
    }
}


} // namespace
