
/*
    Rosegarden-4 v0.1
    A sequencer and musical notation editor.

    This program is Copyright 2000-2001
        Guillaume Laurent   <glaurent@telegraph-road.org>,
        Chris Cannam        <cannam@all-day-breakfast.com>,
        Richard Bown        <bownie@bownie.com>

    The moral right of the authors to claim authorship of this work
    has been asserted.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#include <cstdio>

#include <algorithm>

#include <kmessagebox.h>

#include "rosedebug.h"
#include "rosegardenguiview.h"
#include "notepixmapfactory.h"
#include "staff.h"
#include "NotationTypes.h"


NotePixmapOffsets::NotePixmapOffsets()
{
}

void
NotePixmapOffsets::offsetsFor(Note::Type note,
                              bool dotted,
                              Accidental accidental,
                              bool drawTail, bool stalkGoesUp)
{
    m_note = note;
    m_accidental = accidental;
    m_drawTail = drawTail;
    m_stalkGoesUp = stalkGoesUp;
    m_dotted = dotted;
    m_noteHasStalk = note < Note::WholeNote; //!!!

    m_bodyOffset.setX(0);     m_bodyOffset.setY(0);
    m_hotSpot.setX(0);        m_hotSpot.setY(0);
    m_accidentalOffset.setX(0); m_accidentalOffset.setY(0);
    
    if (note >= Note::HalfNote)
        m_bodySize = m_noteBodyEmptySize;
    else
        m_bodySize = m_noteBodyFilledSize;

    computeAccidentalAndStalkSize();
    computePixmapSize();
    computeBodyOffset();
}

void
NotePixmapOffsets::computeAccidentalAndStalkSize()
{
    unsigned int tailOffset = (m_note < Note::QuarterNote && m_stalkGoesUp) ? m_tailWidth : 0;
    unsigned int totalXOffset = tailOffset,
        totalYOffset = 0;
    
    if (m_accidental == Sharp) {
        totalXOffset += m_sharpWidth + 1;
        totalYOffset = 3;
    }
    else if (m_accidental == Flat) {
        totalXOffset += m_flatWidth + 1;
        if (!m_noteHasStalk || !m_stalkGoesUp) totalYOffset = 4;
    }
    else if (m_accidental == Natural) {
        totalXOffset += m_naturalWidth + 1;
        totalYOffset = 3;
    }

    m_accidentalStalkSize.setWidth(totalXOffset);
    m_accidentalStalkSize.setHeight(totalYOffset);
}



void
NotePixmapOffsets::computePixmapSize()
{
    m_pixmapSize.setWidth(m_bodySize.width() + m_accidentalStalkSize.width() +
                          (m_dotted ? m_dotSize.width() : 0));

    if (m_noteHasStalk) {

        m_pixmapSize.setHeight(m_bodySize.height() / 2 +
                               Staff::stalkLen +
                               m_accidentalStalkSize.height());

        if (m_note < Note::QuarterNote) {

            // readjust pixmap height according to its duration - the stalk
            // is longer for 8th, 16th, etc.. because the tail is higher
            //
            if (m_note == Note::EighthNote)
                m_pixmapSize.rheight() += 1;
            else if (m_note == Note::SixteenthNote)
                m_pixmapSize.rheight() += 4;
            else if (m_note == Note::ThirtySecondNote)
                m_pixmapSize.rheight() += 9;
            else if (m_note == Note::SixtyFourthNote)
                m_pixmapSize.rheight() += 14;
        }
        
    }
    else {
        m_pixmapSize.setHeight(m_bodySize.height() + m_accidentalStalkSize.height());
    }


    switch (m_accidental) {
        
    case NoAccidental:
        break;
        
    case Sharp:
    case Natural:

        m_pixmapSize.rheight() += 3;
        break;
        
    case Flat:

        if (!m_stalkGoesUp) {
            m_pixmapSize.rheight() += 5;
        }

        break;
    }
}

void
NotePixmapOffsets::computeBodyOffset()
{
    // Simple case : no accidental - Y coord is valid for all cases
    //
    if (m_stalkGoesUp) {

        m_bodyOffset.setY(m_pixmapSize.height() - m_bodySize.height());
        m_hotSpot.setY(m_pixmapSize.height() - m_bodySize.height() / 2);

        m_stalkPoints.first.setY(m_pixmapSize.height() - m_bodySize.height() / 2 - 1);

        m_stalkPoints.second.setY(0);

    } else {

        m_hotSpot.setY(m_bodySize.height() / 2);

        m_stalkPoints.first.setY(m_bodySize.height() / 2);

        m_stalkPoints.second.setX(m_stalkPoints.first.x());
        m_stalkPoints.second.setY(m_pixmapSize.height());
    }   

    switch (m_accidental) {
        
    case NoAccidental:
        break;
        
    case Sharp:

        m_bodyOffset.setX(m_sharpWidth + 1);

        if (m_stalkGoesUp) {
            m_bodyOffset.ry() -= 3;
            m_hotSpot.ry() -= 3;
            m_stalkPoints.first.ry() -= 3;
        } else {
            m_bodyOffset.ry() += 3;
            m_hotSpot.ry() += 3;
            m_stalkPoints.first.ry() += 3;
        }

        m_accidentalOffset.setY(m_bodyOffset.y() - 3);
        break;
        
    case Flat:

        m_bodyOffset.setX(m_flatWidth + 1);

        if (!m_stalkGoesUp) {
            m_bodyOffset.ry() += 4;
            m_hotSpot.ry() += 4;
            m_stalkPoints.first.ry() += 4;
        }

        m_accidentalOffset.setY(m_bodyOffset.y() - 5);
        break;
        
    case Natural:

        m_bodyOffset.setX(m_naturalWidth + 1);

        if (m_stalkGoesUp) {
            m_bodyOffset.ry() -= 3;
            m_hotSpot.ry() -= 3;
            m_stalkPoints.first.ry() -= 3;
        } else {
            m_bodyOffset.ry() += 3;
            m_hotSpot.ry() += 3;
            m_stalkPoints.first.ry() += 3;
        }

        m_accidentalOffset.setY(m_bodyOffset.y() - 3);
        break;

    }

    if (m_accidental != NoAccidental)
        m_hotSpot.setX(m_bodyOffset.x());
    

    if (m_stalkGoesUp)
        m_stalkPoints.first.setX(m_bodyOffset.x() + m_bodySize.width() - 2);
    else
        m_stalkPoints.first.setX(m_bodyOffset.x());

    m_stalkPoints.second.setX(m_stalkPoints.first.x());

}


void
NotePixmapOffsets::setNoteBodySizes(QSize empty, QSize filled)
{
    m_noteBodyEmptySize = empty;
    m_noteBodyFilledSize = filled;
}

void
NotePixmapOffsets::setTailWidth(unsigned int s)
{
    m_tailWidth = s;
}

void
NotePixmapOffsets::setAccidentalsWidth(unsigned int sharp,
                                     unsigned int flat,
                                     unsigned int natural)
{
    m_sharpWidth = sharp;
    m_flatWidth = flat;
    m_naturalWidth = natural;
}

void NotePixmapOffsets::setDotSize(QSize size)
{
    m_dotSize = size;
}



NotePixmapFactory::NotePixmapFactory()
    : m_generatedPixmapHeight(0),
      m_noteBodyHeight(0),
      m_tailWidth(0),
      m_noteBodyFilled("pixmaps/note-bodyfilled.xpm"),
      m_noteBodyEmpty("pixmaps/note-bodyempty.xpm"),
      m_accidentalSharp("pixmaps/notemod-sharp.xpm"),
      m_accidentalFlat("pixmaps/notemod-flat.xpm"),
      m_accidentalNatural("pixmaps/notemod-natural.xpm"),
      m_dot("pixmaps/dot.xpm")
{
    // Yes, this is not a mistake. Don't ask me why - Chris named those
    QString pixmapTailUpFileName("pixmaps/tail-up-%1.xpm"),
        pixmapTailDownFileName("pixmaps/tail-down-%1.xpm");

    for (unsigned int i = 0; i < 4; ++i) {
        
        m_tailsUp.push_back(new QPixmap(pixmapTailDownFileName.arg(i+1)));
        m_tailsDown.push_back(new QPixmap(pixmapTailUpFileName.arg(i+1)));
    }

    //////////////////////////////////////////////////////
    m_generatedPixmapHeight = m_noteBodyEmpty.height() / 2 + Staff::stalkLen;
    m_noteBodyHeight        = m_noteBodyEmpty.height();
    m_noteBodyWidth         = m_noteBodyEmpty.width();
    //////////////////////////////////////////////////////

    // Load rests
    m_rests.push_back(new QPixmap("pixmaps/rest-hemidemisemi.xpm"));
    m_rests.push_back(new QPixmap("pixmaps/rest-demisemi.xpm"));
    m_rests.push_back(new QPixmap("pixmaps/rest-semiquaver.xpm"));
    m_rests.push_back(new QPixmap("pixmaps/rest-quaver.xpm"));
    m_rests.push_back(new QPixmap("pixmaps/rest-crotchet.xpm"));
    m_rests.push_back(new QPixmap("pixmaps/rest-minim.xpm"));
    m_rests.push_back(new QPixmap("pixmaps/rest-semibreve.xpm"));

    // Init offsets
    m_offsets.setNoteBodySizes(m_noteBodyEmpty.size(),
                               m_noteBodyFilled.size());

    m_offsets.setTailWidth(m_tailsUp[0]->width());
    m_offsets.setAccidentalsWidth(m_accidentalSharp.width(),
                                m_accidentalFlat.width(),
                                m_accidentalNatural.width());
    
    m_offsets.setDotSize(m_dot.size());
}

NotePixmapFactory::~NotePixmapFactory()
{
    for (unsigned int i = 0; i < m_tailsUp.size(); ++i) {
        delete m_tailsUp[i];
        delete m_tailsDown[i];
    }

    for (unsigned int i = 0; i < m_rests.size(); ++i) {
        delete m_rests[i];
    }
}



QCanvasPixmap
NotePixmapFactory::makeNotePixmap(Note::Type note,
                                  bool dotted,
                                  Accidental accidental,
                                  bool drawTail,
                                  bool stalkGoesUp)
{

    kdDebug(KDEBUG_AREA) << "NotePixmapFactory::makeNotePixmap: note is "
                         << note << ", dotted is " << dotted << endl;

    m_offsets.offsetsFor(note, dotted, accidental, drawTail, stalkGoesUp);

    if (note > Note::Longest) {
        kdDebug(KDEBUG_AREA) << "NotePixmapFactory::makeNotePixmap : note > LastNote ("
                             << note << ")\n";
        throw -1;
    }

    bool noteHasStalk = note < Note::WholeNote;
    m_generatedPixmapHeight = m_offsets.getPixmapSize().height();
    
    createPixmapAndMask();

    // paint note body
    //
    QPixmap *body = (note >= Note::HalfNote) ? &m_noteBodyEmpty : &m_noteBodyFilled;

    m_p.drawPixmap (m_offsets.getBodyOffset(), *body);
    m_pm.drawPixmap(m_offsets.getBodyOffset(), *(body->mask()));
    
    if (dotted)
        drawDot();

    // paint stalk (if needed)
    //
    if (noteHasStalk)
        drawStalk(note, drawTail, stalkGoesUp);

    // paint accidental (if needed)
    //
    if (accidental != NoAccidental)
        drawAccidental(accidental, stalkGoesUp);
    
    //#define ROSE_DEBUG_NOTE_PIXMAP_FACTORY
#ifdef ROSE_DEBUG_NOTE_PIXMAP_FACTORY
    // add red dots at each corner of the pixmap
    m_p.setPen(Qt::red); m_p.setBrush(Qt::red);
    m_p.drawPoint(0,0);
    m_p.drawPoint(0,m_offsets.getHotSpot().y());
    m_p.drawPoint(0,m_generatedPixmap->height() - 1);
    m_p.drawPoint(m_generatedPixmap->width() - 1,0);
    m_p.drawPoint(m_generatedPixmap->width() - 1,m_generatedPixmap->height() - 1);

    m_pm.drawPoint(0,0);
    m_pm.drawPoint(0,m_offsets.getHotSpot().y());
    m_pm.drawPoint(0,m_generatedPixmap->height() -1);
    m_pm.drawPoint(m_generatedPixmap->width() -1,0);
    m_pm.drawPoint(m_generatedPixmap->width() -1,m_generatedPixmap->height()-1);
#endif

    // We're done - generate the returned pixmap with the right offset
    //
    m_p.end();
    m_pm.end();

    QCanvasPixmap notePixmap(*m_generatedPixmap, m_offsets.getHotSpot());
    QBitmap mask(*m_generatedMask);
    notePixmap.setMask(mask);

    delete m_generatedPixmap;
    delete m_generatedMask;

    return notePixmap;
}


QCanvasPixmap
NotePixmapFactory::makeRestPixmap(Note::Type note, bool dotted)
{
    switch (note) {
    case Note::SixtyFourthNote:
        return QCanvasPixmap(*m_rests[0], m_pointZero);
    case Note::ThirtySecondNote:
        return QCanvasPixmap(*m_rests[1], m_pointZero);
    case Note::SixteenthNote:
        return QCanvasPixmap(*m_rests[2], m_pointZero);
    case Note::EighthNote:
        return QCanvasPixmap(*m_rests[3], m_pointZero);
    case Note::QuarterNote:
        return QCanvasPixmap(*m_rests[4], m_pointZero);
    case Note::HalfNote:
        return QCanvasPixmap(*m_rests[5], m_pointZero); // QPoint(0, 19)
    case Note::WholeNote:
        return QCanvasPixmap(*m_rests[6], m_pointZero); // QPoint(0, 9)
        //!!! ... and breve
    default:
        kdDebug(KDEBUG_AREA) << "NotePixmapFactory::makeRestPixmap() for note "
                             << note << " not yet implemented or note out of range\n";
        return QCanvasPixmap(*m_rests[0], m_pointZero);
    }
}

QCanvasPixmap
NotePixmapFactory::makeClefPixmap(string type)
{
    static string defaultClefFile = "pixmaps/clef-treble.xpm";

    try {
	Clef clef(type);
	string filename = "pixmaps/clef-" + clef.getName() + ".xpm";
	return QCanvasPixmap(filename.c_str());
    } catch (Clef::BadClefName) {
	kdDebug(KDEBUG_AREA) << "Bad clef name \"" << type << "\"" << endl;
	return QCanvasPixmap(defaultClefFile.c_str());
    }
}

QCanvasPixmap
NotePixmapFactory::makeKeyPixmap(string type, string cleftype)
{
    try {
        // Key is a Qt type as well, so we have to specify ::Key

        ::Key key(type);
        Clef clef(cleftype);
        vector<int> ah = key.getAccidentalHeights(clef);

        QPixmap &accidentalPixmap
            (key.isSharp() ? m_accidentalSharp : m_accidentalFlat);

        createPixmapAndMask((Staff::accidentWidth - 1) * ah.size(),
                            Staff::lineWidth * 6 + 1);

        int x = 0;

        for (unsigned int i = 0; i < ah.size(); ++i) {

            int h = ah[i];
            int y = ((8 - h) * Staff::lineWidth) / 2 + ((h % 2 == 1) ? 1 : 0);

            m_p.drawPixmap(x, y, accidentalPixmap);
            m_pm.drawPixmap(x, y, *(accidentalPixmap.mask()));

            x += Staff::accidentWidth - 1;
        }

        m_p.end();
        m_pm.end();

        QCanvasPixmap p(*m_generatedPixmap, m_pointZero);
        QBitmap m(*m_generatedMask);
        p.setMask(m);

        delete m_generatedPixmap;
        delete m_generatedMask;

        return p;

    } catch (Key::BadKeyName) {
        return QCanvasPixmap("pixmaps/dot.xpm"); // why not?
    } catch (Clef::BadClefName) {
        return QCanvasPixmap("pixmaps/dot.xpm"); // why not?
    }
}
        

void
NotePixmapFactory::createPixmapAndMask(int width, int height)
{
    if (width < 0)  width  = m_offsets.getPixmapSize().width();
    if (height < 0) height = m_offsets.getPixmapSize().height();

    m_generatedPixmap = new QPixmap(width, height);
    m_generatedMask =
        new QBitmap(m_generatedPixmap->width(), m_generatedPixmap->height());

    // clear up pixmap and mask
    m_generatedPixmap->fill();
    m_generatedMask->fill(Qt::color0);

    // initiate painting
    m_p.begin(m_generatedPixmap);
    m_pm.begin(m_generatedMask);

    m_p.setPen(Qt::black); m_p.setBrush(Qt::black);
    m_pm.setPen(Qt::white); m_pm.setBrush(Qt::white);
}

const QPixmap*
NotePixmapFactory::tailUp(Note::Type note) const
{
    if (note >= Note::QuarterNote) {
        kdDebug(KDEBUG_AREA) << "NotePixmapFactory::tailUp : note("
                             << note << ") > EighthDotted" << endl;
        throw -1;
        return 0;
    }
    
    if (note == Note::EighthNote)
        return m_tailsUp[0];
    if (note == Note::SixteenthNote)
        return m_tailsUp[1];
    if (note == Note::ThirtySecondNote)
        return m_tailsUp[2];
    if (note == Note::SixtyFourthNote)
        return m_tailsUp[3];
    else {
        kdDebug(KDEBUG_AREA) << "NotePixmapFactory::tailUp : unknown note"
                             << endl;
        throw -1;
    }
    
}

const QPixmap*
NotePixmapFactory::tailDown(Note::Type note) const
{
    if (note >= Note::QuarterNote) {
        kdDebug(KDEBUG_AREA) << "NotePixmapFactory::tailDown : note("
                             << note << ") > EighthDotted" << endl;
        throw -1;
        return 0;
    }
    
    if (note == Note::EighthNote)
        return m_tailsDown[0];
    if (note == Note::SixteenthNote)
        return m_tailsDown[1];
    if (note == Note::ThirtySecondNote)
        return m_tailsDown[2];
    if (note == Note::SixtyFourthNote)
        return m_tailsDown[3];
    else {
        kdDebug(KDEBUG_AREA) << "NotePixmapFactory::tailUp : unknown note"
                             << endl;
        throw -1;
    }
}



void
NotePixmapFactory::drawStalk(Note::Type note,
                             bool drawTail, bool stalkGoesUp)
{
    QPoint lineOrig, lineDest;

    lineOrig = m_offsets.getStalkPoints().first;
    lineDest = m_offsets.getStalkPoints().second;

    m_p.drawLine(lineOrig, lineDest);
    m_pm.drawLine(lineOrig, lineDest);

    if (drawTail && note < Note::QuarterNote) {
        // need to add a tail pixmap
        //
        const QPixmap *tailPixmap = 0;

        if (stalkGoesUp) {
            tailPixmap = tailUp(note);

            m_p.drawPixmap (m_offsets.getStalkPoints().first.x() + 1 , 0, *tailPixmap);
            m_pm.drawPixmap(m_offsets.getStalkPoints().first.x() + 1 , 0, *(tailPixmap->mask()));

        } else {

            tailPixmap = tailDown(note);

            m_p.drawPixmap (m_offsets.getStalkPoints().first.x() + 1,
                            m_generatedPixmapHeight - tailPixmap->height(),
                            *tailPixmap);

            m_pm.drawPixmap(m_offsets.getStalkPoints().first.x() + 1,
                            m_generatedPixmapHeight - tailPixmap->height(),
                            *(tailPixmap->mask()));

//             m_p.drawPixmap (1,
//                             m_generatedPixmapHeight - tailPixmap->height(),
//                             *tailPixmap);

//             m_pm.drawPixmap(1,
//                             m_generatedPixmapHeight - tailPixmap->height(),
//                             *(tailPixmap->mask()));
        }

    }
}

void
NotePixmapFactory::drawDot()
{
    int x = m_offsets.getBodyOffset().x() + m_noteBodyFilled.size().width();
    int y = m_offsets.getBodyOffset().y();

    kdDebug(KDEBUG_AREA) << "NotePixmapFactory::drawDot(): placing dot at "
                         << x << "," << y << endl;

    m_p.drawPixmap(x, y, m_dot);
    m_pm.drawPixmap(x, y, *(m_dot.mask()));
}

void
NotePixmapFactory::drawAccidental(Accidental accidental, bool /*stalkGoesUp*/)
{
    const QPixmap *accidentalPixmap = 0;

    switch (accidental) {

    case NoAccidental:
        kdDebug(KDEBUG_AREA) << "NotePixmapFactory::drawAccidental() called with NoAccidental"
                             << endl;
        KMessageBox::error(0, "NotePixmapFactory::drawAccidental() called with NoAccidental");
        return;
        break;
        
    case Sharp:
        accidentalPixmap = &m_accidentalSharp;
        break;

    case Flat:
        accidentalPixmap = &m_accidentalFlat;
        break;

    case Natural:
        accidentalPixmap = &m_accidentalNatural;
        break;
 
        //!!! double sharp, double flat
    }

    m_p.drawPixmap(m_offsets.getAccidentalOffset().x(),
                   m_offsets.getAccidentalOffset().y(),
                   *accidentalPixmap);

    m_pm.drawPixmap(m_offsets.getAccidentalOffset().x(),
                    m_offsets.getAccidentalOffset().y(),
                    *(accidentalPixmap->mask()));
    
}


QPoint
NotePixmapFactory::m_pointZero;


//////////////////////////////////////////////////////////////////////


ChordPixmapFactory::ChordPixmapFactory(const Staff &s)
    : m_referenceStaff(s)
{
}


QCanvasPixmap
ChordPixmapFactory::makeChordPixmap(const ChordPitches &pitches,
                                    const Accidentals &accidentals,
                                    Note::Type note, bool dotted,
                                    bool drawTail, bool stalkGoesUp)
{
    // The heights we calculate are in the default clef, but they're
    // just used to calculate relative distances &c so that should be
    // okay.  What _won't_ be okay is using the default key, because
    // the relative height of an item can depend on the key (e.g. is
    // this a C-sharp or a D-flat?)

    int topHeight =
        NotationDisplayPitch(pitches[pitches.size()-1],
                             Clef::DefaultClef, Key::DefaultKey).getHeightOnStaff(); //!!! no -- default key may produce incorrect result
    int topY = m_referenceStaff.yCoordOfHeight(topHeight);

    int bottomHeight =
        NotationDisplayPitch(pitches[0],
                             Clef::DefaultClef, Key::DefaultKey).getHeightOnStaff();  //!!! no -- default key may produce incorrect result
    int bottomY = m_referenceStaff.yCoordOfHeight(bottomHeight);

    bool noteHasStalk = note < Note::WholeNote;

    m_generatedPixmapHeight = topY - bottomY + m_noteBodyHeight;

    if (noteHasStalk)
        m_generatedPixmapHeight += Staff::stalkLen;

    kdDebug(KDEBUG_AREA) << "m_generatedPixmapHeight : " << m_generatedPixmapHeight << endl
                         << "topHeight : " << topHeight << " - bottomHeight : " << bottomHeight << endl;
    

    //readjustGeneratedPixmapHeight(note);

    // X-offset at which the tail should be drawn
    //unsigned int tailOffset = (note < Quarter && stalkGoesUp) ? m_tailWidth : 0;

    createPixmapAndMask(/*tailOffset*/);

    // paint note bodies

    // set mask painter RasterOp to Or
    m_pm.setRasterOp(Qt::OrROP);

    QPixmap *body = (note >= Note::HalfNote) ? &m_noteBodyEmpty : &m_noteBodyFilled;

    if (stalkGoesUp) {
        int offset = m_generatedPixmap->height() - body->height() - topY;
        
        for (int i = pitches.size() - 1; i >= 0; --i) {
            int y = m_referenceStaff.yCoordOfHeight
                (NotationDisplayPitch(pitches[i], Clef::DefaultClef,
                                      Key::DefaultKey).getHeightOnStaff());
            m_p.drawPixmap (0, y + offset, *body);
            m_pm.drawPixmap(0, y + offset, *(body->mask()));
        }
        
    } else {
        int offset = bottomY;
        for (unsigned int i = 0; i < pitches.size(); ++i) {
            int y = m_referenceStaff.yCoordOfHeight
                (NotationDisplayPitch(pitches[i], Clef::DefaultClef,
                                      Key::DefaultKey).getHeightOnStaff());
            m_p.drawPixmap (0, y - offset, *body);
            m_pm.drawPixmap(0, y - offset, *(body->mask()));
        }
    }

    // restore mask painter RasterOp to Copy
    m_pm.setRasterOp(Qt::CopyROP);

    if (noteHasStalk)
        drawStalk(note, drawTail, stalkGoesUp);

    m_p.end();
    m_pm.end();

    QCanvasPixmap notePixmap(*m_generatedPixmap, m_pointZero);
    QBitmap mask(*m_generatedMask);
    notePixmap.setMask(mask);

    delete m_generatedPixmap;
    delete m_generatedMask;

    return notePixmap;

}


/*!
QCanvasPixmap
ChordPixmapFactory::makeChordPixmap(const chordpitches &pitches,
                                    Note::Type note, bool drawTail,
                                    bool stalkGoesUp)
{
    //!!! oh jeez

    int highestNote = m_referenceStaff.pitchYOffset(pitches[pitches.size() - 1]),
        lowestNote = m_referenceStaff.pitchYOffset(pitches[0]);


    bool noteHasStalk = note < Note::WholeNote;

    m_generatedPixmapHeight = highestNote - lowestNote + m_noteBodyHeight;

    if (noteHasStalk)
        m_generatedPixmapHeight += Staff::stalkLen;

    kdDebug(KDEBUG_AREA) << "m_generatedPixmapHeight : " << m_generatedPixmapHeight << endl
                         << "highestNote : " << highestNote << " - lowestNote : " << lowestNote << endl;
    

    //readjustGeneratedPixmapHeight(note);

    // X-offset at which the tail should be drawn
    //unsigned int tailOffset = (note < Quarter && stalkGoesUp) ? m_tailWidth : 0;

    createPixmapAndMask();

    // paint note bodies

    // set mask painter RasterOp to Or
    m_pm.setRasterOp(Qt::OrROP);

    QPixmap *body = (note >= Note::HalfNote) ? &m_noteBodyEmpty : &m_noteBodyFilled;

    if (stalkGoesUp) {
        int offset = m_generatedPixmap->height() - body->height() - highestNote;
        
        for (int i = pitches.size() - 1; i >= 0; --i) {
            m_p.drawPixmap (0, m_referenceStaff.pitchYOffset(pitches[i]) + offset, *body);
            m_pm.drawPixmap(0, m_referenceStaff.pitchYOffset(pitches[i]) + offset, *(body->mask()));
        }
        
    } else {
        int offset = lowestNote;
        for (unsigned int i = 0; i < pitches.size(); ++i) {
            m_p.drawPixmap (0, m_referenceStaff.pitchYOffset(pitches[i]) - offset, *body);
            m_pm.drawPixmap(0, m_referenceStaff.pitchYOffset(pitches[i]) - offset, *(body->mask()));
        }
    }

    // restore mask painter RasterOp to Copy
    m_pm.setRasterOp(Qt::CopyROP);

    if (noteHasStalk)
        drawStalk(note, drawTail, stalkGoesUp);

    m_p.end();
    m_pm.end();

    QCanvasPixmap notePixmap(*m_generatedPixmap, m_pointZero);
    QBitmap mask(*m_generatedMask);
    notePixmap.setMask(mask);

    delete m_generatedPixmap;
    delete m_generatedMask;

    return notePixmap;

}

*/

