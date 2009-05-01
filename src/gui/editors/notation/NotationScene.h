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

#ifndef _NOTATION_SCENE_H_
#define _NOTATION_SCENE_H_

#include <QGraphicsScene>

#include "base/NotationTypes.h"
#include "base/Composition.h"
#include "gui/general/SelectionManager.h"
#include "StaffLayout.h"

class QGraphicsItem;
class QGraphicsTextItem;

namespace Rosegarden
{

class NotationStaff;
class NotationHLayout;
class NotationVLayout;
class NotePixmapFactory;
class NotationProperties;
class NotationMouseEvent;
class EventSelection;
class Event;
class NotationScene;
class NotationElement;
class NotationWidget;
class RosegardenDocument;
class Segment;
class RulerScale;

class NotationScene : public QGraphicsScene,
                      public CompositionObserver,
                      public SelectionManager
{
    Q_OBJECT

public:
    NotationScene();
    ~NotationScene();

    void setNotationWidget(NotationWidget *w);
    void setStaffs(RosegardenDocument *document, std::vector<Segment *> segments);

    Segment *getCurrentSegment();

    //!!! to keep current staff implementation happy:
    bool isInPrintMode() const { return false; }
    NotationHLayout *getHLayout() { return m_hlayout; }
    NotationVLayout *getVLayout() { return m_vlayout; }
    NotationProperties &getProperties() { return *m_properties; }
    RosegardenDocument *getDocument() { return m_document; }

    virtual EventSelection *getSelection() const { return 0; } //!!!
    virtual void setSelection(EventSelection* s, bool preview) { } //!!!

    const RulerScale *getRulerScale() const;

    /**
     * Show and sound the given note.  The height is used for display,
     * the pitch for performance, so the two need not correspond (e.g.
     * under ottava there may be octave differences).
     */
    void showPreviewNote(NotationStaff *staff, double layoutX,
                         int pitch, int height,
                         const Note &note,
                         bool grace,
                         int velocity = -1);

    /// Remove any visible preview note
    void clearPreviewNote(NotationStaff *);

    void playNote(Segment &segment, int pitch, int velocity = -1);

    void slotSetInsertCursorPosition(timeT position,
                                     bool scroll, bool updateNow) { }


    // more dubious:
    void handleEventRemoved(Event *) { }//!!! tell tools, in case they have m_someEvent stored
    bool areAnnotationsVisible() { return true; }
    bool areLilyPondDirectivesVisible() { return true; }

signals:
    void mousePressed(const NotationMouseEvent *e);
    void mouseMoved(const NotationMouseEvent *e);
    void mouseReleased(const NotationMouseEvent *e);
    void mouseDoubleClicked(const NotationMouseEvent *e);

    void sceneDeleted(); // all segments have been removed
    
    /**
     * Emitted when the mouse cursor moves to a different height
     * on the staff
     *
     * \a noteName contains the MIDI name of the corresponding note
     */
//!!!    void hoveredOverNoteChanged(const QString &noteName);

    /**
     * Emitted when the mouse cursor moves to a note which is at a
     * different time
     *
     * \a time is set to the absolute time of the note the cursor is
     * hovering on
     */
//!!!    void hoveredOverAbsoluteTimeChanged(unsigned int time);

protected slots:    
    void slotCommandExecuted();

protected:
    void mousePressEvent(QGraphicsSceneMouseEvent *);
    void mouseMoveEvent(QGraphicsSceneMouseEvent *);
    void mouseReleaseEvent(QGraphicsSceneMouseEvent *);
    void mouseDoubleClickEvent(QGraphicsSceneMouseEvent *);

    void segmentRemoved(const Composition *, Segment *); // CompositionObserver
    void timeSignatureChanged(const Composition *); // CompositionObserver

private:
    void setNotePixmapFactories(QString fontName = "", int size = -1);

    NotationWidget *m_widget; // I do not own this

    RosegardenDocument *m_document; // I do not own this
    NotationProperties *m_properties; // I own this

    NotePixmapFactory *m_notePixmapFactory; // I own this
    NotePixmapFactory *m_notePixmapFactorySmall; // I own this

    std::vector<Segment *> m_segments; // I do not own these
    std::vector<NotationStaff *> m_staffs; // I own these

    NotationHLayout *m_hlayout; // I own this
    NotationVLayout *m_vlayout; // I own this

    QGraphicsTextItem *m_title;
    QGraphicsTextItem *m_subtitle;
    QGraphicsTextItem *m_composer;
    QGraphicsTextItem *m_copyright;

    std::vector<QGraphicsItem *> m_pages;
    std::vector<QGraphicsItem *> m_pageNumbers;
    
    StaffLayout::PageMode m_pageMode;
    int m_printSize;
    int m_leftGutter;

    int m_currentStaff;

    unsigned int m_compositionRefreshStatusId;
    bool m_timeSignatureChanged;

    /// Returns the page width according to the layout mode (page/linear)
    int getPageWidth();

    /// Returns the page height according to the layout mode (page/linear)
    int getPageHeight();

    /// Returns the margins within the page (zero if not in MultiPageMode)
    void getPageMargins(int &left, int &top);

    NotationStaff *getStaffForSceneCoords(double x, int y) const;

    void setupMouseEvent(QGraphicsSceneMouseEvent *, NotationMouseEvent &) const;

    void checkUpdate();
    void positionStaffs();
    void layoutAll();
    void layout(NotationStaff *singleStaff, timeT start, timeT end);
    
};

}

#endif
