/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2022 the Rosegarden development team.

    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#define RG_MODULE_STRING "[MatrixElement]"
#define RG_NO_DEBUG_PRINT 1

#include "MatrixElement.h"
#include "MatrixScene.h"
#include "misc/Debug.h"
#include "base/RulerScale.h"
#include "misc/ConfigGroups.h"

#include <QGraphicsRectItem>
#include <QGraphicsPolygonItem>
#include <QBrush>
#include <QColor>
#include <QSettings>

#include "base/Event.h"
#include "base/NotationTypes.h"
#include "base/BaseProperties.h"
#include "gui/general/GUIPalette.h"
#include "gui/rulers/DefaultVelocityColour.h"
#include "gui/general/MidiPitchLabel.h"


namespace Rosegarden
{

static const int MatrixElementData = 2;

MatrixElement::MatrixElement(MatrixScene *scene, Event *event,
                             bool drum, long pitchOffset) :
    ViewElement(event),
    m_scene(scene),
    m_drum(drum),
    m_current(true),
    m_item(nullptr),
    m_textItem(nullptr),
    m_pitchOffset(pitchOffset)
{
    reconfigure();
}

MatrixElement::~MatrixElement()
{
    RG_DEBUG << "deleting item:" << m_item << this;
    delete m_item;
    if (m_textItem) {
        RG_DEBUG << "deleting text item:" << m_textItem << this;
        delete m_textItem;
    }
}

void
MatrixElement::reconfigure()
{
    timeT time = event()->getAbsoluteTime();
    timeT duration = event()->getDuration();
    reconfigure(time, duration);
}

void
MatrixElement::reconfigure(int velocity)
{
    timeT time = event()->getAbsoluteTime();
    timeT duration = event()->getDuration();
    long pitch = 60;
    event()->get<Int>(BaseProperties::PITCH, pitch);
    reconfigure(time, duration, pitch, velocity);
}

void
MatrixElement::reconfigure(timeT time, timeT duration)
{
    long pitch = 60;
    event()->get<Int>(BaseProperties::PITCH, pitch);

    reconfigure(time, duration, pitch);
}

void
MatrixElement::reconfigure(timeT time, timeT duration, int pitch)
{
    long velocity = 100;
    event()->get<Int>(BaseProperties::VELOCITY, velocity);

    reconfigure(time, duration, pitch, velocity);
}

void
MatrixElement::reconfigure(timeT time, timeT duration, int pitch, int velocity)
{
    RG_DEBUG << "reconfigure" << time << duration <<
        pitch << velocity << m_current;
    const RulerScale *scale = m_scene->getRulerScale();
    int resolution = m_scene->getYResolution();

    double x0 = scale->getXForTime(time);
    double x1 = scale->getXForTime(time + duration);
    m_width = x1 - x0;

    m_velocity = velocity;

    // if the note has TIED_FORWARD or TIED_BACK properties, draw it with a
    // different fill pattern
    bool tiedNote = (event()->has(BaseProperties::TIED_FORWARD) ||
                     event()->has(BaseProperties::TIED_BACKWARD));
    Qt::BrushStyle brushPattern = (tiedNote ? Qt::Dense2Pattern : Qt::SolidPattern);

    QColor colour;
    if (event()->has(BaseProperties::TRIGGER_SEGMENT_ID)) {
        //!!! Using gray for trigger events and events from other, non-active
        // segments won't work.  This should be handled some other way, with a
        // color outside the range of possible velocity choices, which probably
        // leaves some kind of curious light blue or something
        colour = Qt::cyan;
    } else {
        colour = DefaultVelocityColour::getInstance()->getColour(velocity);
    }
    if (!m_current) {
        colour = QColor(200, 200, 200);
    }
    colour.setAlpha(160);

    double fres(resolution);

    if (m_drum) {
        fres = resolution + 1;
        QGraphicsPolygonItem *item = dynamic_cast<QGraphicsPolygonItem *>(m_item);
        if (!item) {
            RG_DEBUG << "reconfigure drum deleting item:" << m_item << this;
            delete m_item;
            item = new QGraphicsPolygonItem;
            RG_DEBUG << "reconfigure drum created item:" << m_item << this;
            m_item = item;
            m_scene->addItem(m_item);
        }
        QPolygonF polygon;
        polygon << QPointF(0, 0)
                << QPointF(fres/2, fres/2)
                << QPointF(0, fres)
                << QPointF(-fres/2, fres/2)
                << QPointF(0, 0);
        item->setPolygon(polygon);
        item->setPen
            (QPen(GUIPalette::getColour(GUIPalette::MatrixElementBorder), 0));
        item->setBrush(QBrush(colour, brushPattern));
    } else {
        QGraphicsRectItem *item = dynamic_cast<QGraphicsRectItem *>(m_item);
        if (!item) {
            RG_DEBUG << "reconfigure deleting item:" << m_item << this;
            delete m_item;
            item = new QGraphicsRectItem;
            m_item = item;
            RG_DEBUG << "reconfigure created item:" << m_item << this;
            m_scene->addItem(m_item);
        }
        float width = m_width;
        if (width < 1) {
            x0 = std::max(0.0, x1 - 1);
            width = 1;
        }
        QRectF rect(0, 0, width, fres + 1);
        item->setRect(rect);
        item->setPen
            (QPen(GUIPalette::getColour(GUIPalette::MatrixElementBorder), 0));
        item->setBrush(QBrush(colour, brushPattern));

        QSettings settings;
        settings.beginGroup(MatrixViewConfigGroup);
        bool showName = settings.value("show_note_names", false).toBool();
        settings.endGroup();

        if (m_textItem) {
            if (! showName) {
                RG_DEBUG << "reconfigure deleting text item:" << m_textItem << this;
                delete m_textItem;
                m_textItem = nullptr;
            }
        } else {
            if (showName) {
                m_textItem = new QGraphicsSimpleTextItem;
                RG_DEBUG << "reconfigure created text item:" << m_textItem << this;
                m_scene->addItem(m_textItem);
            }
        }

        if (m_textItem) {
            m_textItem->setZValue(1.0); // keep text above notes
            m_textItem->setBrush(GUIPalette::getColour(GUIPalette::MatrixElementBorder));
            QString noteName = MidiPitchLabel(pitch).getQString();
            m_textItem->setText(noteName);
            QFont font;
            font.setPixelSize(8);
            m_textItem->setFont(font);
            m_textItem->setData(MatrixElementData,
                                QVariant::fromValue((void *)this));
        }
    }

    setLayoutX(x0);

    m_item->setData(MatrixElementData, QVariant::fromValue((void *)this));

    // set the Y position taking m_pitchOffset into account, subtracting the
    // opposite of whatever the originating segment transpose was

//    std::cout << "TRANSPOSITION TEST: event pitch: "
//              << (pitch ) << " m_pitchOffset: " << m_pitchOffset
//              << std::endl;

    double pitchy = (127 - pitch - m_pitchOffset) * (resolution + 1);
    m_item->setPos(x0, pitchy);

    if (m_textItem) {
            m_textItem->setPos(x0 + 1, pitchy - 1);
        }

    // set a tooltip explaining why this event is drawn in a different pattern
    if (tiedNote) m_item->setToolTip(QObject::tr("This event is tied to another event."));
}

bool
MatrixElement::isNote() const
{
    return event()->isa(Note::EventType);
}

void
MatrixElement::setSelected(bool selected)
{
    RG_DEBUG << "setSelected" << event()->getAbsoluteTime() << selected;
    QAbstractGraphicsShapeItem *item =
        dynamic_cast<QAbstractGraphicsShapeItem *>(m_item);
    if (!item) return;

    if (selected) {
        QPen pen(GUIPalette::getColour(GUIPalette::SelectedElement), 2,
                 Qt::SolidLine, Qt::SquareCap, Qt::MiterJoin);
        pen.setCosmetic(!m_drum);
        item->setPen(pen);
    } else {
        item->setPen
            (QPen(GUIPalette::getColour(GUIPalette::MatrixElementBorder), 0));
    }
}

void
MatrixElement::setCurrent(bool current)
{
    RG_DEBUG << "setCurrent" << event()->getAbsoluteTime() <<
        current << m_current;
    if (m_current == current) return;

    QAbstractGraphicsShapeItem *item =
        dynamic_cast<QAbstractGraphicsShapeItem *>(m_item);
    if (!item) return;

    QColor colour;

    if (!current) {
        colour = QColor(200, 200, 200);
    } else {
        if (event()->has(BaseProperties::TRIGGER_SEGMENT_ID)) {
            colour = Qt::gray;
        } else {
            long velocity = 100;
            event()->get<Int>(BaseProperties::VELOCITY, velocity);
            colour = DefaultVelocityColour::getInstance()->getColour(velocity);
        }
    }

    item->setBrush(colour);
    item->setZValue(current ? 1 : 0);

    if (current) {
        item->setPen
            (QPen(GUIPalette::getColour(GUIPalette::MatrixElementBorder), 0));
    } else {
        item->setPen
            (QPen(GUIPalette::getColour(GUIPalette::MatrixElementLightBorder), 0));
    }

    m_current = current;
}

MatrixElement *
MatrixElement::getMatrixElement(QGraphicsItem *item)
{
    QVariant v = item->data(MatrixElementData);
    if (v.isNull()) return nullptr;
    return (MatrixElement *)v.value<void *>();
}


}
