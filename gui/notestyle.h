// -*- c-basic-offset: 4 -*-

/*
    Rosegarden-4
    A sequencer and musical notation editor.

    This program is Copyright 2000-2003
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

#include <vector>

#include "StringHash.h"
#include "NotationTypes.h"
#include "Exception.h"

#include "notecharname.h"

class NoteStyle;

// It would be nice to make this a typedef for PropertyName, but we
// can't safely store PropertyNames in properties (at least, not in
// their internal-integer form) because their values are allocated
// on demand within each run of the program, they don't persist.

typedef std::string NoteStyleName;

class NoteStyleFactory
{
public:
    static std::vector<NoteStyleName> getAvailableStyleNames();
    static const NoteStyleName DefaultStyle;

    static NoteStyle *getStyle(NoteStyleName name);
    static NoteStyle *getStyleForEvent(Rosegarden::Event *event);

    typedef Rosegarden::Exception StyleUnavailable;

private:
    typedef Rosegarden::hash_string<NoteStyle*> StyleMap;
    static StyleMap m_styles;
};


class NoteStyle
{
public:
    NoteStyle() : m_baseStyle(0) { }
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
    static const NoteHeadShape CustomCharName;
    static const NoteHeadShape Number;

    enum HFixPoint { Normal, Central, Reversed };
    enum VFixPoint { Near, Middle, Far };

    NoteHeadShape getShape     (Rosegarden::Note::Type);
    bool          isFilled     (Rosegarden::Note::Type);
    bool          hasStem      (Rosegarden::Note::Type);
    int           getFlagCount (Rosegarden::Note::Type);
    int           getSlashCount(Rosegarden::Note::Type);

    typedef std::pair<CharName, bool> CharNameRec; // bool is "inverted"
    CharNameRec getNoteHeadCharName(Rosegarden::Note::Type);

    CharName getRestCharName(Rosegarden::Note::Type);
    CharName getFlagCharName(int flagCount);
    CharName getAccidentalCharName(const Rosegarden::Accidental &);
    CharName getMarkCharName(const Rosegarden::Mark &);
    CharName getClefCharName(const Rosegarden::Clef &);

    void setBaseStyle (NoteStyleName name);
    void setShape     (Rosegarden::Note::Type, NoteHeadShape);
    void setCharName  (Rosegarden::Note::Type, CharName);
    void setFilled    (Rosegarden::Note::Type, bool);
    void setStem      (Rosegarden::Note::Type, bool);
    void setFlagCount (Rosegarden::Note::Type, int);
    void setSlashCount(Rosegarden::Note::Type, int);

    void getStemFixPoints(Rosegarden::Note::Type, HFixPoint &, VFixPoint &);
    void setStemFixPoints(Rosegarden::Note::Type, HFixPoint, VFixPoint);

protected:
    struct NoteDescription {
	NoteHeadShape shape; // if CustomCharName, use charName
	CharName charName; // only used if shape == CustomCharName
	bool filled;
	bool stem;
	int flags;
	int slashes;
	HFixPoint hfix;
	VFixPoint vfix;

	NoteDescription() :
	    shape(AngledOval), charName(NoteCharacterNames::UNKNOWN),
	    filled(true), stem(true), flags(0), slashes(0),
	    hfix(Normal), vfix(Middle) { }

	NoteDescription(NoteHeadShape _shape, CharName _charName,
			bool _filled, bool _stem, int _flags, int _slashes,
			HFixPoint _hfix, VFixPoint _vfix) :
	    shape(_shape), charName(_charName),
	    filled(_filled), stem(_stem), flags(_flags), slashes(_slashes),
	    hfix(_hfix), vfix(_vfix) { }
    };

    typedef __HASH_NS::hash_map<Rosegarden::Note::Type,
				NoteDescription> NoteDescriptionMap;

    NoteDescriptionMap m_notes;
    NoteStyle *m_baseStyle;

    void checkDescription(Rosegarden::Note::Type type);
};



#endif
