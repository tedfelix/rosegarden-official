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

#ifndef _RG_CONTROLRULERWIDGET_H_
#define _RG_CONTROLRULERWIDGET_H_

#include <QStackedWidget>
#include "base/Event.h"
#include "base/ViewElement.h"
#include "base/MidiDevice.h"

namespace Rosegarden
{

class RosegardenDocument;
class Segment;
class ControlRuler;
class ControlParameter;
class RulerScale;
class PropertyName;
class MatrixScene;
class EventSelection;

class ControlRulerWidget : public QStackedWidget //, Observer
{
Q_OBJECT

public:
    ControlRulerWidget();
    virtual ~ControlRulerWidget();

    void setSegments(RosegardenDocument *document,
                     std::vector<Segment *> segments);

    void setScene(MatrixScene *);
    QString getCurrentToolName() { return m_currentToolName; }
    void removeRuler(std::list<ControlRuler*>::iterator);

public slots:
    void slotTogglePropertyRuler(const PropertyName &);
    void slotToggleControlRuler(std::string);
    void slotAddRuler();
    void slotAddControlRuler(const ControlParameter &);
    void slotAddPropertyRuler(const PropertyName &);
    void slotSetPannedRect(QRectF pr);
    void slotSelectionChanged(EventSelection *);
    void slotHoveredOverNoteChanged();
    void slotHoveredOverNoteChanged(int evPitch, bool haveEvent, timeT evTime);
    void slotSetToolName(const QString &);
    void slotDragScroll(timeT);

signals:
    void dragScroll(timeT);
    
protected:
    std::list<ControlRuler *> m_controlRulerList;
    const ControlList *m_controlList;

    RosegardenDocument *m_document;
    Segment *m_segment;
    MatrixScene *m_scene;
    RulerScale *m_scale;
    QString m_currentToolName;
    QRectF m_pannedRect;
    std::vector <ViewElement*> m_selectedElements;
};

}

#endif
