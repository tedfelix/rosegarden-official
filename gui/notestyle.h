// -*- c-basic-offset: 4 -*-

/*
    Rosegarden-4 v0.1
    A sequencer and musical notation editor.

    This program is Copyright 2000-2002
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

#ifndef _NOTE_STYLE_H_
#define _NOTE_STYLE_H_

#include "NotationTypes.h"
#include "notecharname.h"
#include <qxml.h>
#include <vector>
#include <map>

class NoteStyle;


typedef std::string NoteStyleName;

namespace StandardNoteStyleNames
{
    extern const NoteStyleName Classical;
    extern const NoteStyleName Cross;
    extern const NoteStyleName Triangle;
    extern const NoteStyleName Mensural;

    /**
     * Get the predefined note head styles (i.e. the ones listed
     * above) in their defined order.
     */
    extern std::vector<NoteStyleName> getStandardStyles();
}


class NoteStyleFactory
{
public:
    static NoteStyle *getStyle(NoteStyleName name);

    struct StyleUnavailable {
        StyleUnavailable(std::string r) : reason(r) { }
        std::string reason;
    };

private:
    typedef std::map<NoteStyleName, NoteStyle *> StyleMap;
    static StyleMap m_styles;
};


class NoteStyle
{
public:
    virtual ~NoteStyle();

    typedef std::string NoteHeadShape;

    static const NoteHeadShape AngledOval;
    static const NoteHeadShape LevelOval;
    static const NoteHeadShape Breve;
    static const NoteHeadShape Cross;
    static const NoteHeadShape TriangleUp;
    static const NoteHeadShape TriangleDown;
    static const NoteHeadShape Diamond;
    static const NoteHeadShape Rectangle;
    static const NoteHeadShape Number;

    NoteHeadShape getShape     (Rosegarden::Note::Type);
    bool          isFilled     (Rosegarden::Note::Type);
    bool          hasStem      (Rosegarden::Note::Type);
    int           getFlagCount (Rosegarden::Note::Type);
    
    CharName getNoteHeadCharName(Rosegarden::Note::Type);
    CharName getRestCharName(Rosegarden::Note::Type);
    CharName getFlagCharName(int flagCount);
    CharName getAccidentalCharName(const Rosegarden::Accidental &);
    CharName getMarkCharName(const Rosegarden::Mark &);
    CharName getClefCharName(const Rosegarden::Clef &);

protected:
    struct NoteDescription {
	NoteHeadShape shape;
	bool filled;
	bool stem;
	int flags;

	NoteDescription() :
	    shape(AngledOval), filled(true), stem(true), flags(0) { }

	NoteDescription(NoteHeadShape _shape,
			bool _filled, bool _stem, int _flags) :
	    shape(_shape), filled(_filled), stem(_stem), flags(_flags) { }
    };

    typedef std::map<Rosegarden::Note::Type,
		     NoteDescription> NoteDescriptionMap;

    NoteDescriptionMap m_notes;
    NoteStyle *m_baseStyle;

    NoteStyle() : m_baseStyle(0) { }
    virtual void initialiseNotes() = 0;
};


/*!!!
class ClassicalNoteStyle : public NoteStyle
{
protected:
    virtual void initialiseNotes();
};

class CrossNoteStyle : public NoteStyle
{
protected:
    virtual void initialiseNotes();
};


class TriangleNoteStyle : public NoteStyle
{
protected:
    virtual void initialiseNotes();
};


class MensuralNoteStyle : public NoteStyle
{
protected:
    virtual void initialiseNotes();
};
*/

class CustomNoteStyle : public NoteStyle, public QXmlDefaultHandler
{
public:
    CustomNoteStyle(NoteStyleName name);

    struct StyleFileReadFailed {
        StyleFileReadFailed(std::string r) : reason(r) { }
        std::string reason;
    };

    void setBaseStyle (NoteStyleName name);
    void setShape     (Rosegarden::Note::Type, NoteHeadShape);
    void setFilled    (Rosegarden::Note::Type, bool);
    void setStem      (Rosegarden::Note::Type, bool);
    void setFlagCount (Rosegarden::Note::Type, int);


    // Xml handler methods:

    virtual bool startElement
    (const QString& namespaceURI, const QString& localName,
     const QString& qName, const QXmlAttributes& atts);
    
protected:
    virtual void initialiseNotes();

private:
    QString m_errorString;
};


#endif
